// TextRGA implementation. See include/crdt/text_rga.hpp for the API. Mirrors
// ListRGA almost exactly; render_from concatenates instead of pushing.
//
// Notes:
//  - Operation IDs are "<client_id>:<20-digit zero-padded seq>". The padding
//    makes lex comparison agree with numeric ordering; the client prefix
//    gives global uniqueness without coordination and a deterministic
//    sibling tiebreaker for concurrent inserts under the same parent.
//  - apply() has three paths: already-applied -> no-op; applicable now ->
//    apply and drain pending; predecessor missing -> buffer for retry.
//  - merge() is a fixed-point loop that copies in any incoming character
//    whose predecessor is already present and OR-merges tombstones.

#include <crdt/text_rga.hpp>

TextRGA::TextRGA(std::string client_id)
    : _client_id(std::move(client_id)) {
    if (_client_id.empty()) {
        throw std::invalid_argument("Client ID cannot be empty");
    }

    // Empty string represents the document head.
    _children[""] = {};
}

std::string TextRGA::next_id() {
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

bool TextRGA::node_exists(const std::string& node_id) const {
    return _nodes.find(node_id) != _nodes.end();
}

bool TextRGA::pending_contains(const std::string& operation_id) const {
    return std::any_of(
        _pending_changes.begin(),
        _pending_changes.end(),
        [&operation_id](const TextChange& change) {
            return change.operation_id == operation_id;
        }
    );
}

TextChange TextRGA::insert_at_beginning(std::string value) {
    return insert_after("", std::move(value));
}

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

void TextRGA::apply(const TextChange& change) {
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

bool TextRGA::try_apply_change(const TextChange& change) {
    if (change.type == TextOperationType::insert_op) {
        if (change.element_id.empty()) {
            throw std::invalid_argument("Insert operation requires an element ID");
        }

        if (!change.previous_id.empty() && !node_exists(change.previous_id)) {
            return false;
        }

        // Idempotent: if the character is already present, treat the
        // replayed change as successfully applied without touching state.
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

        // Delete arrived before the target insert; defer and let the caller
        // buffer the op until the insert lands.
        if (!node_exists(change.element_id)) {
            return false;
        }

        _nodes.at(change.element_id).deleted = true;

        return true;
    }

    throw std::invalid_argument("Unknown text operation type");
}

void TextRGA::retry_pending_changes() {
    // Outer loop: each successful apply may unblock another pending op
    // (e.g. an insert that depended on this one). Keep sweeping until no
    // pass makes progress.
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

TextRGAState TextRGA::state() const {
    TextRGAState snapshot;
    snapshot.nodes.reserve(_nodes.size());
    for (const auto& [_, character] : _nodes) {
        snapshot.nodes.push_back(character);
    }
    return snapshot;
}

void TextRGA::merge(const TextRGAState& other) {
    // Multi-pass: a node can only be inserted once its previous_id is present.
    std::vector<TextCharacter> pending(other.nodes.begin(), other.nodes.end());

    bool made_progress = true;
    while (made_progress) {
        made_progress = false;

        auto iterator = pending.begin();
        while (iterator != pending.end()) {
            const TextCharacter& incoming = *iterator;

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

            TextCharacter character = incoming;
            _nodes.emplace(character.id, character);

            auto& sibling_list = _children[character.previous_id];
            if (std::find(sibling_list.begin(), sibling_list.end(), character.id)
                == sibling_list.end()) {
                sibling_list.push_back(character.id);
            }
            std::sort(sibling_list.begin(), sibling_list.end());

            _children.try_emplace(character.id, std::vector<std::string>{});

            // Mark the merged-in character's id as applied. Future
            // deliveries of the same op (via apply()) short-circuit as
            // already-seen.
            _applied_operations.insert(character.id);

            iterator = pending.erase(iterator);
            made_progress = true;
        }
    }
}

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

std::string TextRGA::value() const {
    std::string output;

    render_from("", output);

    return output;
}

bool TextRGA::has_applied(const std::string& operation_id) const {
    return (_applied_operations.find(operation_id) != _applied_operations.end());
}
