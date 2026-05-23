#include <text_rga.hpp>

// Initializes a text CRDT for this client
text_RGA::text_RGA(std::string client_id)
    : client_id(std::move(client_id)) {
    if (this->client_id.empty()) {
        throw std::invalid_argument("Client ID cannot be empty");
    }

    // Empty string represents beginning of the text.
    children[""] = {};
}

// Generates a unique ID for local operations and elements
std::string text_RGA::next_id() {
    local_sequence++;

    std::ostringstream stream;

    stream << client_id
           << ":"
           << std::setw(20)
           << std::setfill('0')
           << local_sequence;

    return stream.str();
}

// Checks whether a character node exists
bool text_RGA::node_exists(const std::string& node_id) const {
    return nodes.find(node_id) != nodes.end();
}

// Checks whether an operation is already waiting in pending changes
bool text_RGA::pending_contains(const std::string& operation_id) const {
    return std::any_of(
        pending_changes.begin(),
        pending_changes.end(),
        [&operation_id](const text_change& change) {
            return change.operation_id == operation_id;
        }
    );
}

// Inserts text at the beginning of the document
text_change text_RGA::insert_at_beginning(std::string value) {
    return insert_after("", std::move(value));
}

// Inserts a character after a given character ID
text_change text_RGA::insert_after(
    const std::string& previous_id,
    std::string value
) {
    if (!previous_id.empty() && !node_exists(previous_id)) {
        throw std::invalid_argument("Cannot insert after unknown element");
    }

    std::string new_element_id = next_id();

    text_change change;
    change.type = text_operation_type::Insert;
    change.operation_id = new_element_id;
    change.element_id = new_element_id;
    change.previous_id = previous_id;
    change.value = std::move(value);

    apply(change);

    return change;
}

// Creates and applies a delete operation for a character
text_change text_RGA::erase(const std::string& element_id) {
    if (!node_exists(element_id)) {
        throw std::invalid_argument("Cannot erase unknown element");
    }

    std::string operation_id = next_id();

    text_change change;
    change.type = text_operation_type::Delete;
    change.operation_id = operation_id;
    change.element_id = element_id;
    change.previous_id = "";
    change.value.clear();

    apply(change);

    return change;
}

// Applies a local or remote text operation
void text_RGA::apply(const text_change& change) {
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

// Attempts to apply a text operation if its dependencies exist
bool text_RGA::try_apply_change(const text_change& change) {
    if (change.type == text_operation_type::Insert) {
        if (change.element_id.empty()) {
            throw std::invalid_argument("Insert operation requires an element ID");
        }

        if (!change.previous_id.empty() && !node_exists(change.previous_id)) {
            return false;
        }

        if (node_exists(change.element_id)) {
            return true;
        }

        text_character character;
        character.id = change.element_id;
        character.previous_id = change.previous_id;
        character.value = change.value;
        character.deleted = false;

        nodes.emplace(change.element_id, character);

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

    if (change.type == text_operation_type::Delete) {
        if (change.element_id.empty()) {
            throw std::invalid_argument("Delete operation requires an element ID");
        }

        if (!node_exists(change.element_id)) {
            return false;
        }

        nodes.at(change.element_id).deleted = true;

        return true;
    }

    throw std::invalid_argument("Unknown text operation type");
}

// Retries operations that previously arrived before their dependencies
void text_RGA::retry_pending_changes() {
    bool made_progress = true;

    while (made_progress) {
        made_progress = false;

        auto iterator = pending_changes.begin();

        while (iterator != pending_changes.end()) {
            const text_change& change = *iterator;

            if ((applied_operations.find(change.operation_id) != applied_operations.end())) {
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

// Creates a serializable snapshot of the text state
text_RGA_state text_RGA::state() const {
    text_RGA_state snapshot;
    snapshot.nodes.reserve(nodes.size());
    for (const auto& [_, character] : nodes) {
        snapshot.nodes.push_back(character);
    }
    return snapshot;
}

// Merges incoming text state into this replica
void text_RGA::merge(const text_RGA_state& other) {
    std::vector<text_character> pending(other.nodes.begin(), other.nodes.end());

    bool made_progress = true;
    while (made_progress) {
        made_progress = false;

        auto iterator = pending.begin();
        while (iterator != pending.end()) {
            const text_character& incoming = *iterator;

            auto existing = nodes.find(incoming.id);
            if (existing != nodes.end()) {
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

            text_character character = incoming;
            nodes.emplace(character.id, character);

            auto& sibling_list = children[character.previous_id];
            if (std::find(sibling_list.begin(), sibling_list.end(), character.id)
                == sibling_list.end()) {
                sibling_list.push_back(character.id);
            }
            std::sort(sibling_list.begin(), sibling_list.end());

            children.try_emplace(character.id, std::vector<std::string>{});

            applied_operations.insert(character.id);

            iterator = pending.erase(iterator);
            made_progress = true;
        }
    }
}

// Builds the visible text recursively from a given character
void text_RGA::render_from(
    const std::string& previous_id,
    std::string& output
) const {
    // Iterative pre-order walk; the recursive form overflowed the worker-thread
    // stack on docs of a few thousand characters.
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
        const text_character& child = nodes.at(child_id);
        if (!child.deleted) {
            output.append(child.value);
        }
        auto kids = children.find(child_id);
        if (kids != children.end() && !kids->second.empty()) {
            stack.push_back({&kids->second, 0});
        }
    }
}

// Returns the visible text value
std::string text_RGA::value() const {
    std::string output;

    render_from("", output);

    return output;
}

// Checks whether an operation has already been applied
bool text_RGA::has_applied(const std::string& operation_id) const {
    return (applied_operations.find(operation_id) != applied_operations.end());
}