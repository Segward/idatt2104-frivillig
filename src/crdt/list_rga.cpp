// ListRGA implementation. See include/crdt/list_rga.hpp for the API.

#include <crdt/list_rga.hpp>

// Empty string represents the list head.
ListRGA::ListRGA(std::string client_id)
    : _client_id(std::move(client_id)) {
    if (_client_id.empty()) {
        throw std::invalid_argument("Client ID cannot be empty");
    }
    _children[""] = {};
}

// Operation IDs are "<client_id>:<20-digit zero-padded seq>". The width-20
// zero pad makes lexicographic comparison agree with numeric ordering; the
// client prefix gives global uniqueness without coordination and a
// deterministic sibling tiebreaker for concurrent inserts under the same
// parent.
std::string ListRGA::next_id() {
    _local_sequence++;
    std::ostringstream stream;
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
    if (!previous_id.empty() && !_nodes.contains(previous_id)) {
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
    if (!_nodes.contains(element_id)) {
        throw std::invalid_argument("Cannot erase unknown element");
    }
    ListChange change;
    change.type = ListOperationType::delete_op;
    change.operation_id = next_id();
    change.element_id = element_id;
    apply(change);
    return change;
}

// Returns false (without throwing) if the change's causal predecessor hasn't
// been seen yet, so the caller can buffer it. An insert for an
// already-present element is treated as a successful no-op (idempotent
// redelivery); a delete for a missing target defers the same way an insert
// with a missing predecessor does.
bool ListRGA::try_apply_change(const ListChange& change) {
    if (change.type == ListOperationType::insert_op) {
        if (change.element_id.empty()) {
            throw std::invalid_argument("Insert operation requires an element ID");
        }
        if (change.value.empty()) {
            throw std::invalid_argument("Insert operation requires a non-empty value");
        }
        if (!change.previous_id.empty() && !_nodes.contains(change.previous_id)) {
            return false;
        }
        if (_nodes.contains(change.element_id)) {
            return true;
        }
        _nodes.emplace(change.element_id, ListItem{
            .id = change.element_id,
            .previous_id = change.previous_id,
            .value = change.value,
            .deleted = false,
        });
        auto& sibling_list = _children[change.previous_id];
        if (std::ranges::find(sibling_list, change.element_id) == sibling_list.end()) {
            sibling_list.push_back(change.element_id);
        }
        std::ranges::sort(sibling_list);
        _children.try_emplace(change.element_id);
        return true;
    }
    if (change.type == ListOperationType::delete_op) {
        if (change.element_id.empty()) {
            throw std::invalid_argument("Delete operation requires an element ID");
        }
        auto it = _nodes.find(change.element_id);
        if (it == _nodes.end()) {
            return false;
        }
        it->second.deleted = true;
        return true;
    }
    throw std::invalid_argument("Unknown list operation type");
}

// Each successful apply may unblock another pending op (e.g. an insert that
// depended on this one), so keep sweeping until a full pass makes no
// progress.
void ListRGA::retry_pending_changes() {
    bool made_progress = true;
    while (made_progress) {
        made_progress = false;
        auto iterator = _pending_changes.begin();
        while (iterator != _pending_changes.end()) {
            const ListChange& change = *iterator;
            const bool already_applied = _applied_operations.contains(change.operation_id);
            if (already_applied || try_apply_change(change)) {
                if (!already_applied) {
                    _applied_operations.insert(change.operation_id);
                }
                _pending_operations.erase(change.operation_id);
                iterator = _pending_changes.erase(iterator);
                made_progress = true;
                continue;
            }
            ++iterator;
        }
    }
}

// Three paths: already applied -> no-op (so redelivery is harmless);
// dependencies present -> apply, mark, and drain anything in the buffer that
// this op may have unblocked; predecessor not yet seen -> buffer for later
// retry (deduped so the same op isn't queued twice).
void ListRGA::apply(const ListChange& change) {
    if (change.operation_id.empty()) {
        throw std::invalid_argument("Operation ID cannot be empty");
    }
    if (_applied_operations.contains(change.operation_id)) {
        return;
    }
    if (try_apply_change(change)) {
        _applied_operations.insert(change.operation_id);
        retry_pending_changes();
        return;
    }
    if (_pending_operations.insert(change.operation_id).second) {
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

// Multi-pass fixed-point loop: a node can only be inserted once its
// previous_id is present. Tombstones OR-merge, so a delete on either side
// always sticks; merged-in ids are marked applied so a later delivery of the
// same op via apply() short-circuits.
void ListRGA::merge(const ListRGAState& other) {
    std::vector<ListItem> pending(other.nodes.begin(), other.nodes.end());
    bool made_progress = true;
    while (made_progress) {
        made_progress = false;
        auto iterator = pending.begin();
        while (iterator != pending.end()) {
            const ListItem& incoming = *iterator;
            auto existing = _nodes.find(incoming.id);
            if (existing != _nodes.end()) {
                existing->second.deleted = existing->second.deleted || incoming.deleted;
                iterator = pending.erase(iterator);
                made_progress = true;
                continue;
            }
            if (!incoming.previous_id.empty() && !_nodes.contains(incoming.previous_id)) {
                ++iterator;
                continue;
            }
            _nodes.emplace(incoming.id, incoming);
            auto& sibling_list = _children[incoming.previous_id];
            if (std::ranges::find(sibling_list, incoming.id) == sibling_list.end()) {
                sibling_list.push_back(incoming.id);
            }
            std::ranges::sort(sibling_list);
            _children.try_emplace(incoming.id);
            _applied_operations.insert(incoming.id);
            iterator = pending.erase(iterator);
            made_progress = true;
        }
    }
}

// Iterative pre-order walk; the recursive form overflowed the worker-thread
// stack on docs of a few thousand nodes.
void ListRGA::render_from(
    const std::string& previous_id,
    std::vector<std::string>& output
) const {
    auto root = _children.find(previous_id);
    if (root == _children.end()) return;
    struct Frame {
        const std::vector<std::string>* siblings;
        std::size_t index;
    };
    std::vector<Frame> stack;
    stack.push_back({&root->second, 0});
    while (!stack.empty()) {
        Frame& top = stack.back();
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
    return _applied_operations.contains(operation_id);
}
