#include <list_rga.hpp>

// Initializes a list CRDT for this client
list_RGA::list_RGA(std::string client_id)
    : client_id(std::move(client_id)) {
    if (this->client_id.empty()) {
        throw std::invalid_argument("Client ID cannot be empty");
    }

    // Empty string represents beginning of the list.
    children[""] = {};
}

// Generates a unique ID for local operations and elements
std::string list_RGA::next_id() {
    local_sequence++;

    std::ostringstream stream;

    stream << client_id
           << ":"
           << std::setw(20)
           << std::setfill('0')
           << local_sequence;

    return stream.str();
}

// Inserts an item at the beginning of the list
list_change list_RGA::insert_at_beginning(const std::string& value) {
    return insert_after("", value);
}

// Inserts an item after a given list item ID
list_change list_RGA::insert_after(
    const std::string& previous_id,
    const std::string& value
) {
    if (value.empty()) {
        throw std::invalid_argument("List item value cannot be empty");
    }

    if (!previous_id.empty() && nodes.find(previous_id) == nodes.end()) {
        throw std::invalid_argument("Cannot insert after unknown element");
    }

    std::string new_element_id = next_id();

    list_change change;
    change.type = list_operation_type::Insert;
    change.operation_id = new_element_id;
    change.element_id = new_element_id;
    change.previous_id = previous_id;
    change.value = value;

    apply(change);

    return change;
}

// Creates and applies a delete operation for a list item
list_change list_RGA::erase(const std::string& element_id) {
    if (nodes.find(element_id) == nodes.end()) {
        throw std::invalid_argument("Cannot erase unknown element");
    }

    std::string operation_id = next_id();

    list_change change;
    change.type = list_operation_type::Delete;
    change.operation_id = operation_id;
    change.element_id = element_id;
    change.previous_id = "";
    change.value = "";

    apply(change);

    return change;
}

// Checks whether a change is already queued in the pending buffer
bool list_RGA::pending_contains(const std::string& operation_id) const {
    return std::any_of(
        pending_changes.begin(),
        pending_changes.end(),
        [&operation_id](const list_change& change) {
            return change.operation_id == operation_id;
        }
    );
}

// Attempts to apply a single change; returns false if dependencies are missing
bool list_RGA::try_apply_change(const list_change& change) {
    if (change.type == list_operation_type::Insert) {
        if (change.element_id.empty()) {
            throw std::invalid_argument("Insert operation requires an element ID");
        }

        if (change.value.empty()) {
            throw std::invalid_argument("Insert operation requires a non-empty value");
        }

        if (!change.previous_id.empty() &&
            nodes.find(change.previous_id) == nodes.end()) {
            return false;
        }

        if (nodes.find(change.element_id) != nodes.end()) {
            return true;
        }

        list_item item;
        item.id = change.element_id;
        item.previous_id = change.previous_id;
        item.value = change.value;
        item.deleted = false;

        nodes.emplace(change.element_id, item);

        auto& sibling_list = children[change.previous_id];

        if (std::find(
                sibling_list.begin(),
                sibling_list.end(),
                change.element_id
            ) == sibling_list.end()) {
            sibling_list.push_back(change.element_id);
        }

        std::sort(sibling_list.begin(), sibling_list.end());

        children.try_emplace(change.element_id, std::vector<std::string>{});

        return true;
    }

    if (change.type == list_operation_type::Delete) {
        if (change.element_id.empty()) {
            throw std::invalid_argument("Delete operation requires an element ID");
        }

        if (nodes.find(change.element_id) == nodes.end()) {
            return false;
        }

        nodes.at(change.element_id).deleted = true;
        return true;
    }

    throw std::invalid_argument("Unknown list operation type");
}

// Re-attempts buffered pending changes until no further progress is possible
void list_RGA::retry_pending_changes() {
    bool made_progress = true;

    while (made_progress) {
        made_progress = false;

        auto iterator = pending_changes.begin();

        while (iterator != pending_changes.end()) {
            const list_change& change = *iterator;

            if (applied_operations.find(change.operation_id) != applied_operations.end()) {
                iterator = pending_changes.erase(iterator);
                made_progress = true;
                continue;
            }

            if (try_apply_change(change)) {
                applied_operations.insert(change.operation_id);
                iterator = pending_changes.erase(iterator);
                made_progress = true;
                continue;
            }

            ++iterator;
        }
    }
}

// Applies a local or remote list operation, buffering it if dependencies are missing
void list_RGA::apply(const list_change& change) {
    if (change.operation_id.empty()) {
        throw std::invalid_argument("Operation ID cannot be empty");
    }

    if (applied_operations.find(change.operation_id) != applied_operations.end()) {
        return;
    }

    if (try_apply_change(change)) {
        applied_operations.insert(change.operation_id);
        retry_pending_changes();
        return;
    }

    if (!pending_contains(change.operation_id)) {
        pending_changes.push_back(change);
    }
}

// Creates a serializable snapshot of the list state
list_RGA_state list_RGA::state() const {
    list_RGA_state snapshot;
    snapshot.nodes.reserve(nodes.size());
    for (const auto& [_, item] : nodes) {
        snapshot.nodes.push_back(item);
    }
    return snapshot;
}

// Merges incoming list state into this replica
void list_RGA::merge(const list_RGA_state& other) {
    // Multi-pass: a node can only be inserted once its previous_id is present.
    std::vector<list_item> pending(other.nodes.begin(), other.nodes.end());

    bool made_progress = true;
    while (made_progress) {
        made_progress = false;

        auto iterator = pending.begin();
        while (iterator != pending.end()) {
            const list_item& incoming = *iterator;

            auto existing = nodes.find(incoming.id);
            if (existing != nodes.end()) {
                // Tombstone join: deleted is a monotonic OR.
                existing->second.deleted = existing->second.deleted || incoming.deleted;
                iterator = pending.erase(iterator);
                made_progress = true;
                continue;
            }

            if (!incoming.previous_id.empty() &&
                nodes.find(incoming.previous_id) == nodes.end()) {
                ++iterator;
                continue;
            }

            list_item item = incoming;
            nodes.emplace(item.id, item);

            auto& sibling_list = children[item.previous_id];
            if (std::find(sibling_list.begin(), sibling_list.end(), item.id)
                == sibling_list.end()) {
                sibling_list.push_back(item.id);
            }
            std::sort(sibling_list.begin(), sibling_list.end());

            children.try_emplace(item.id, std::vector<std::string>{});

            applied_operations.insert(item.id);

            iterator = pending.erase(iterator);
            made_progress = true;
        }
    }
}

// Builds the visible list recursively from a given item
void list_RGA::render_from(
    const std::string& previous_id,
    std::vector<std::string>& output
) const {
    // Iterative pre-order walk; see text_RGA::render_from.
    auto root = children.find(previous_id);
    if (root == children.end()) return;

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
        const list_item& child = nodes.at(child_id);
        if (!child.deleted) {
            output.push_back(child.value);
        }
        auto kids = children.find(child_id);
        if (kids != children.end() && !kids->second.empty()) {
            stack.push_back({&kids->second, 0});
        }
    }
}

// Returns the visible list items
std::vector<std::string> list_RGA::value() const {
    std::vector<std::string> output;

    render_from("", output);

    return output;
}

// Formats the visible list as a printable string
std::string list_RGA::to_string() const {
    std::vector<std::string> items = value();

    std::ostringstream stream;

    for (const std::string& item : items) {
        stream << "- " << item << '\n';
    }

    return stream.str();
}

// Checks whether an operation has already been applied
bool list_RGA::has_applied(const std::string& operation_id) const {
    return applied_operations.find(operation_id) != applied_operations.end();
}