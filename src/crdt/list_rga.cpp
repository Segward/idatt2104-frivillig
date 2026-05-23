#include <list_rga.hpp>

// Initializes a list CRDT for this client
ListRGA::ListRGA(std::string client_id)
    : _client_id(std::move(client_id)) {
    if (_client_id.empty()) {
        throw std::invalid_argument("Client ID cannot be empty");
    }

    // Empty string represents beginning of the list.
    _children[""] = {};
}

// Generates a unique ID for local operations and elements
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

// Inserts an item at the beginning of the list
ListChange ListRGA::insert_at_beginning(const std::string& value) {
    return insert_after("", value);
}

// Inserts an item after a given list item ID
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

// Creates and applies a delete operation for a list item
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

// Checks whether a change is already queued in the pending buffer
bool ListRGA::pending_contains(const std::string& operation_id) const {
    return std::any_of(
        _pending_changes.begin(),
        _pending_changes.end(),
        [&operation_id](const ListChange& change) {
            return change.operation_id == operation_id;
        }
    );
}

// Attempts to apply a single change; returns false if dependencies are missing
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

        if (_nodes.find(change.element_id) == _nodes.end()) {
            return false;
        }

        _nodes.at(change.element_id).deleted = true;
        return true;
    }

    throw std::invalid_argument("Unknown list operation type");
}

// Re-attempts buffered pending changes until no further progress is possible
void ListRGA::retry_pending_changes() {
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

// Applies a local or remote list operation, buffering it if dependencies are missing
void ListRGA::apply(const ListChange& change) {
    if (change.operation_id.empty()) {
        throw std::invalid_argument("Operation ID cannot be empty");
    }

    if (_applied_operations.find(change.operation_id) != _applied_operations.end()) {
        return;
    }

    if (try_apply_change(change)) {
        _applied_operations.insert(change.operation_id);
        retry_pending_changes();
        return;
    }

    if (!pending_contains(change.operation_id)) {
        _pending_changes.push_back(change);
    }
}

// Creates a serializable snapshot of the list state
ListRGAState ListRGA::state() const {
    ListRGAState snapshot;
    snapshot.nodes.reserve(_nodes.size());
    for (const auto& [_, item] : _nodes) {
        snapshot.nodes.push_back(item);
    }
    return snapshot;
}

// Merges incoming list state into this replica
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
                // Tombstone join: deleted is a monotonic OR.
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

            _applied_operations.insert(item.id);

            iterator = pending.erase(iterator);
            made_progress = true;
        }
    }
}

// Builds the visible list recursively from a given item
void ListRGA::render_from(
    const std::string& previous_id,
    std::vector<std::string>& output
) const {
    // Iterative pre-order walk; see TextRGA::render_from.
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

// Returns the visible list items
std::vector<std::string> ListRGA::value() const {
    std::vector<std::string> output;

    render_from("", output);

    return output;
}

// Formats the visible list as a printable string
std::string ListRGA::to_string() const {
    std::vector<std::string> items = value();

    std::ostringstream stream;

    for (const std::string& item : items) {
        stream << "- " << item << '\n';
    }

    return stream.str();
}

// Checks whether an operation has already been applied
bool ListRGA::has_applied(const std::string& operation_id) const {
    return _applied_operations.find(operation_id) != _applied_operations.end();
}
