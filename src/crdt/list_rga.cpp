// ListRGA implementation. See include/crdt/list_rga.hpp for the API.
//
// Notes:
//  - Operation IDs are "<client_id>:<20-digit zero-padded seq>". The padding
//    makes lex comparison agree with numeric ordering; the client prefix
//    gives global uniqueness without coordination and a deterministic
//    sibling tiebreaker for concurrent inserts under the same parent.
//  - apply() has three paths: already-applied -> no-op; applicable now ->
//    apply and drain pending; predecessor missing -> buffer for retry.
//  - merge() is a fixed-point loop that copies in any incoming node whose
//    predecessor is already present and OR-merges tombstones.

#include <crdt/list_rga.hpp>

ListRGA::ListRGA(std::string client_id)
    : _client_id(std::move(client_id)) {
    if (_client_id.empty()) {
        throw std::invalid_argument("Client ID cannot be empty");
    }

    // Empty string represents the list head.
    _children[""] = {};
}

std::string ListRGA::next_id() {
    _local_sequence++;

    std::ostringstream stream;

    // Width-20 zero pad so a lexicographic comparison on the id matches a
    // numeric comparison on the sequence number.
    stream << _client_id
           << ":"
           << std::setw(20)
           << std::setfill('0')
           << _local_sequence;

    return stream.str();
}

ListChange ListRGA::insert_at_beginning(const std::string& value) {
    return insert_after("", value);
}

ListChange ListRGA::insert_after(
    const std::string& previous_id,
    const std::string& value
) {
    if (value.empty()) {
        throw std::invalid_argument("List item value cannot be empty");
    }

    if (!previous_id.empty() && _nodes.find(previous_id) == _nodes.end()) {
        throw std::invalid_argument("Cannot insert after unknown element");
    }

    std::string new_element_id = next_id();

    ListChange change;
    change.type = ListOperationType::insert_op;
    change.operation_id = new_element_id;
    change.element_id = new_element_id;
    change.previous_id = previous_id;
    change.value = value;

    apply(change);

    return change;
}

ListChange ListRGA::erase(const std::string& element_id) {
    if (_nodes.find(element_id) == _nodes.end()) {
        throw std::invalid_argument("Cannot erase unknown element");
    }

    std::string operation_id = next_id();

    ListChange change;
    change.type = ListOperationType::delete_op;
    change.operation_id = operation_id;
    change.element_id = element_id;
    change.previous_id = "";
    change.value = "";

    apply(change);

    return change;
}

bool ListRGA::pending_contains(const std::string& operation_id) const {
    return std::any_of(
        _pending_changes.begin(),
        _pending_changes.end(),
        [&operation_id](const ListChange& change) {
            return change.operation_id == operation_id;
        }
    );
}

bool ListRGA::try_apply_change(const ListChange& change) {
    if (change.type == ListOperationType::insert_op) {
        if (change.element_id.empty()) {
            throw std::invalid_argument("Insert operation requires an element ID");
        }

        if (change.value.empty()) {
            throw std::invalid_argument("Insert operation requires a non-empty value");
        }

        if (!change.previous_id.empty() &&
            _nodes.find(change.previous_id) == _nodes.end()) {
            return false;
        }

        // Idempotent: if the node is already present, treat the replayed
        // change as successfully applied without touching state.
        if (_nodes.find(change.element_id) != _nodes.end()) {
            return true;
        }

        ListItem item;
        item.id = change.element_id;
        item.previous_id = change.previous_id;
        item.value = change.value;
        item.deleted = false;

        _nodes.emplace(change.element_id, item);

        auto& sibling_list = _children[change.previous_id];

        if (std::find(
                sibling_list.begin(),
                sibling_list.end(),
                change.element_id
            ) == sibling_list.end()) {
            sibling_list.push_back(change.element_id);
        }
        std::sort(sibling_list.begin(), sibling_list.end());

        _children.try_emplace(change.element_id, std::vector<std::string>{});

        return true;
    }

    if (change.type == ListOperationType::delete_op) {
        if (change.element_id.empty()) {
            throw std::invalid_argument("Delete operation requires an element ID");
        }

        // Delete arrived before the target insert; defer and let the caller
        // buffer the op until the insert lands.
        if (_nodes.find(change.element_id) == _nodes.end()) {
            return false;
        }

        _nodes.at(change.element_id).deleted = true;
        return true;
    }

    throw std::invalid_argument("Unknown list operation type");
}

void ListRGA::retry_pending_changes() {
    // Outer loop: each successful apply may unblock another pending op
    // (e.g. an insert that depended on this one). Keep sweeping until no
    // pass makes progress.
    bool made_progress = true;

    while (made_progress) {
        made_progress = false;

        auto iterator = _pending_changes.begin();

        while (iterator != _pending_changes.end()) {
            const ListChange& change = *iterator;

            if (_applied_operations.find(change.operation_id) != _applied_operations.end()) {
                iterator = _pending_changes.erase(iterator);
                made_progress = true;
                continue;
            }

            if (try_apply_change(change)) {
                _applied_operations.insert(change.operation_id);
                iterator = _pending_changes.erase(iterator);
                made_progress = true;
                continue;
            }

            ++iterator;
        }
    }
}

void ListRGA::apply(const ListChange& change) {
    if (change.operation_id.empty()) {
        throw std::invalid_argument("Operation ID cannot be empty");
    }

    // Path 1: already applied — treat as a no-op so redelivery is harmless.
    if (_applied_operations.find(change.operation_id) != _applied_operations.end()) {
        return;
    }

    // Path 2: dependencies present — apply, mark, and drain anything in the
    // buffer that this op may have unblocked.
    if (try_apply_change(change)) {
        _applied_operations.insert(change.operation_id);
        retry_pending_changes();
        return;
    }

    // Path 3: predecessor not yet seen — buffer for later retry. Dedupe so
    // the same op doesn't get queued twice if it arrives multiple times
    // before its predecessor.
    if (!pending_contains(change.operation_id)) {
        _pending_changes.push_back(change);
    }
}

ListRGAState ListRGA::state() const {
    ListRGAState snapshot;
    snapshot.nodes.reserve(_nodes.size());
    for (const auto& [_, item] : _nodes) {
        snapshot.nodes.push_back(item);
    }
    return snapshot;
}

void ListRGA::merge(const ListRGAState& other) {
    // Multi-pass: a node can only be inserted once its previous_id is present.
    std::vector<ListItem> pending(other.nodes.begin(), other.nodes.end());

    bool made_progress = true;
    while (made_progress) {
        made_progress = false;

        auto iterator = pending.begin();
        while (iterator != pending.end()) {
            const ListItem& incoming = *iterator;

            auto existing = _nodes.find(incoming.id);
            if (existing != _nodes.end()) {
                // Tombstone join: deleted is a monotonic OR, so a delete on
                // either side always sticks.
                existing->second.deleted = existing->second.deleted || incoming.deleted;
                iterator = pending.erase(iterator);
                made_progress = true;
                continue;
            }

            if (!incoming.previous_id.empty() &&
                _nodes.find(incoming.previous_id) == _nodes.end()) {
                ++iterator;
                continue;
            }

            ListItem item = incoming;
            _nodes.emplace(item.id, item);

            auto& sibling_list = _children[item.previous_id];
            if (std::find(sibling_list.begin(), sibling_list.end(), item.id)
                == sibling_list.end()) {
                sibling_list.push_back(item.id);
            }
            std::sort(sibling_list.begin(), sibling_list.end());

            _children.try_emplace(item.id, std::vector<std::string>{});

            // Mark the merged-in node's id as applied. Future deliveries of
            // the same op (via apply()) will short-circuit as already-seen.
            _applied_operations.insert(item.id);

            iterator = pending.erase(iterator);
            made_progress = true;
        }
    }
}

void ListRGA::render_from(
    const std::string& previous_id,
    std::vector<std::string>& output
) const {
    // Iterative pre-order walk; see TextRGA::render_from for the rationale.
    auto root = _children.find(previous_id);
    if (root == _children.end()) return;

    struct frame {
        const std::vector<std::string>* siblings;
        std::size_t index;
    };
    std::vector<frame> stack;
    stack.push_back({&root->second, 0});

    while (!stack.empty()) {
        frame& top = stack.back();
        if (top.index >= top.siblings->size()) {
            stack.pop_back();
            continue;
        }
        const std::string& child_id = (*top.siblings)[top.index++];
        const ListItem& child = _nodes.at(child_id);
        if (!child.deleted) {
            output.push_back(child.value);
        }
        auto kids = _children.find(child_id);
        if (kids != _children.end() && !kids->second.empty()) {
            stack.push_back({&kids->second, 0});
        }
    }
}

std::vector<std::string> ListRGA::value() const {
    std::vector<std::string> output;

    render_from("", output);

    return output;
}

std::string ListRGA::to_string() const {
    std::vector<std::string> items = value();

    std::ostringstream stream;

    for (const std::string& item : items) {
        stream << "- " << item << '\n';
    }

    return stream.str();
}

bool ListRGA::has_applied(const std::string& operation_id) const {
    return _applied_operations.find(operation_id) != _applied_operations.end();
}
