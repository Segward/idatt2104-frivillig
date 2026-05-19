#include <text_RGA.hpp>

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <utility>

text_RGA::text_RGA(std::string client_id)
    : client_id(std::move(client_id)) {
    if (this->client_id.empty()) {
        throw std::invalid_argument("Client ID cannot be empty");
    }

    // Empty string represents beginning of the text.
    children[""] = {};
}

//Generate new ID, unique identifier
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

//Checks if node exists
bool text_RGA::node_exists(const std::string& node_id) const {
    return nodes.find(node_id) != nodes.end();
}

//Checks if pending changes have a given operation
bool text_RGA::pending_contains(const std::string& operation_id) const {
    return std::any_of(
        pending_changes.begin(),
        pending_changes.end(),
        [&operation_id](const text_change& change) {
            return change.operation_id == operation_id;
        }
    );
}

//Insert text at beginning if string; empty string
text_change text_RGA::insert_at_beginning(char value) {
    return insert_after("", value);
}

//Insert text after a given character ID
text_change text_RGA::insert_after(
    const std::string& previous_id,
    char value
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
    change.value = value;

    apply(change);

    return change;
}

//Erase element
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
    change.value = '\0';

    apply(change);

    return change;
}

//Apply a given text change
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

//Apply change if state allows it
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

//Retry applying pending changes
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

//Create text from given element
void text_RGA::render_from(
    const std::string& previous_id,
    std::string& output
) const {
    auto iterator = children.find(previous_id);

    if (iterator == children.end()) {
        return;
    }

    for (const std::string& child_id : iterator->second) {
        const text_character& child = nodes.at(child_id);

        if (!child.deleted) {
            output.push_back(child.value);
        }

        render_from(child_id, output);
    }
}

//Return text
std::string text_RGA::value() const {
    std::string output;

    render_from("", output);

    return output;
}

//Check if operation applied, to avoid repetition
bool text_RGA::has_applied(const std::string& operation_id) const {
    return (applied_operations.find(operation_id) != applied_operations.end());
}