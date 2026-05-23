#include <text_rga.hpp>

// Initializes a text CRDT for this client
TextRGA::TextRGA(std::string client_id)
    : _client_id(std::move(client_id)) {
    if (_client_id.empty()) {
        throw std::invalid_argument("Client ID cannot be empty");
    }

    // Empty string represents beginning of the text.
    _children[""] = {};
}

// Generates a unique ID for local operations and elements
std::string TextRGA::next_id() {
    _local_sequence++;

    std::ostringstream stream;

    stream << _client_id
           << ":"
           << std::setw(20)
           << std::setfill('0')
           << _local_sequence;

    return stream.str();
}

// Checks whether a character node exists
bool TextRGA::node_exists(const std::string& node_id) const {
    return _nodes.find(node_id) != _nodes.end();
}

// Checks whether an operation is already waiting in pending changes
bool TextRGA::pending_contains(const std::string& operation_id) const {
    return std::any_of(
        _pending_changes.begin(),
        _pending_changes.end(),
        [&operation_id](const TextChange& change) {
            return change.operation_id == operation_id;
        }
    );
}

// Inserts text at the beginning of the document
TextChange TextRGA::insert_at_beginning(std::string value) {
    return insert_after("", std::move(value));
}

// Inserts a character after a given character ID
TextChange TextRGA::insert_after(
    const std::string& previous_id,
    std::string value
) {
    if (!previous_id.empty() && !node_exists(previous_id)) {
        throw std::invalid_argument("Cannot insert after unknown element");
    }

    std::string new_element_id = next_id();

    TextChange change;
    change.type = TextOperationType::insert_op;
    change.operation_id = new_element_id;
    change.element_id = new_element_id;
    change.previous_id = previous_id;
    change.value = std::move(value);

    apply(change);

    return change;
}

// Creates and applies a delete operation for a character
TextChange TextRGA::erase(const std::string& element_id) {
    if (!node_exists(element_id)) {
        throw std::invalid_argument("Cannot erase unknown element");
    }

    std::string operation_id = next_id();

    TextChange change;
    change.type = TextOperationType::delete_op;
    change.operation_id = operation_id;
    change.element_id = element_id;
    change.previous_id = "";
    change.value.clear();

    apply(change);

    return change;
}

// Applies a local or remote text operation
void TextRGA::apply(const TextChange& change) {
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

// Attempts to apply a text operation if its dependencies exist
bool TextRGA::try_apply_change(const TextChange& change) {
    if (change.type == TextOperationType::insert_op) {
        if (change.element_id.empty()) {
            throw std::invalid_argument("Insert operation requires an element ID");
        }

        if (!change.previous_id.empty() && !node_exists(change.previous_id)) {
            return false;
        }

        if (node_exists(change.element_id)) {
            return true;
        }

        TextCharacter character;
        character.id = change.element_id;
        character.previous_id = change.previous_id;
        character.value = change.value;
        character.deleted = false;

        _nodes.emplace(change.element_id, character);

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

    if (change.type == TextOperationType::delete_op) {
        if (change.element_id.empty()) {
            throw std::invalid_argument("Delete operation requires an element ID");
        }

        if (!node_exists(change.element_id)) {
            return false;
        }

        _nodes.at(change.element_id).deleted = true;

        return true;
    }

    throw std::invalid_argument("Unknown text operation type");
}

// Retries operations that previously arrived before their dependencies
void TextRGA::retry_pending_changes() {
    bool made_progress = true;

    while (made_progress) {
        made_progress = false;

        auto iterator = _pending_changes.begin();

        while (iterator != _pending_changes.end()) {
            const TextChange& change = *iterator;

            if ((_applied_operations.find(change.operation_id) != _applied_operations.end())) {
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

// Creates a serializable snapshot of the text state
TextRGAState TextRGA::state() const {
    TextRGAState snapshot;
    snapshot.nodes.reserve(_nodes.size());
    for (const auto& [_, character] : _nodes) {
        snapshot.nodes.push_back(character);
    }
    return snapshot;
}

// Merges incoming text state into this replica
void TextRGA::merge(const TextRGAState& other) {
    std::vector<TextCharacter> pending(other.nodes.begin(), other.nodes.end());

    bool made_progress = true;
    while (made_progress) {
        made_progress = false;

        auto iterator = pending.begin();
        while (iterator != pending.end()) {
            const TextCharacter& incoming = *iterator;

            auto existing = _nodes.find(incoming.id);
            if (existing != _nodes.end()) {
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

            TextCharacter character = incoming;
            _nodes.emplace(character.id, character);

            auto& sibling_list = _children[character.previous_id];
            if (std::find(sibling_list.begin(), sibling_list.end(), character.id)
                == sibling_list.end()) {
                sibling_list.push_back(character.id);
            }
            std::sort(sibling_list.begin(), sibling_list.end());

            _children.try_emplace(character.id, std::vector<std::string>{});

            _applied_operations.insert(character.id);

            iterator = pending.erase(iterator);
            made_progress = true;
        }
    }
}

// Builds the visible text recursively from a given character
void TextRGA::render_from(
    const std::string& previous_id,
    std::string& output
) const {
    // Iterative pre-order walk; the recursive form overflowed the worker-thread
    // stack on docs of a few thousand characters.
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
        const TextCharacter& child = _nodes.at(child_id);
        if (!child.deleted) {
            output.append(child.value);
        }
        auto kids = _children.find(child_id);
        if (kids != _children.end() && !kids->second.empty()) {
            stack.push_back({&kids->second, 0});
        }
    }
}

// Returns the visible text value
std::string TextRGA::value() const {
    std::string output;

    render_from("", output);

    return output;
}

// Checks whether an operation has already been applied
bool TextRGA::has_applied(const std::string& operation_id) const {
    return (_applied_operations.find(operation_id) != _applied_operations.end());
}
