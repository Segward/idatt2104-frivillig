#include <list_rga.hpp>

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <utility>

list_RGA::list_RGA(std::string client_id)
    : client_id(std::move(client_id)) {
    if (this->client_id.empty()) {
        throw std::invalid_argument("Client ID cannot be empty");
    }

    // Empty string represents beginning of the list.
    children[""] = {};
}

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

list_change list_RGA::insert_at_beginning(const std::string& value) {
    return insert_after("", value);
}

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

void list_RGA::apply(const list_change& change) {
    if (change.operation_id.empty()) {
        throw std::invalid_argument("Operation ID cannot be empty");
    }

    if (applied_operations.find(change.operation_id) != applied_operations.end()) {
        return;
    }

    if (change.type == list_operation_type::Insert) {
        if (change.element_id.empty()) {
            throw std::invalid_argument("Insert operation requires an element ID");
        }

        if (change.value.empty()) {
            throw std::invalid_argument("Insert operation requires a non-empty value");
        }

        if (!change.previous_id.empty() &&
            nodes.find(change.previous_id) == nodes.end()) {
            throw std::runtime_error(
                "Cannot apply insert because previous element is missing"
            );
        }

        if (nodes.find(change.element_id) == nodes.end()) {
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

            children[change.element_id] = {};
        }
    }
    else if (change.type == list_operation_type::Delete) {
        if (change.element_id.empty()) {
            throw std::invalid_argument("Delete operation requires an element ID");
        }

        if (nodes.find(change.element_id) == nodes.end()) {
            throw std::runtime_error(
                "Cannot apply delete because element is missing"
            );
        }

        nodes.at(change.element_id).deleted = true;
    }
    else {
        throw std::invalid_argument("Unknown list operation type");
    }

    applied_operations.insert(change.operation_id);
}

list_RGA_state list_RGA::state() const {
    list_RGA_state snapshot;
    snapshot.nodes.reserve(nodes.size());
    for (const auto& [_, item] : nodes) {
        snapshot.nodes.push_back(item);
    }
    return snapshot;
}

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

void list_RGA::render_from(
    const std::string& previous_id,
    std::vector<std::string>& output
) const {
    auto iterator = children.find(previous_id);

    if (iterator == children.end()) {
        return;
    }

    for (const std::string& child_id : iterator->second) {
        const list_item& child = nodes.at(child_id);

        if (!child.deleted) {
            output.push_back(child.value);
        }

        render_from(child_id, output);
    }
}

std::vector<std::string> list_RGA::value() const {
    std::vector<std::string> output;

    render_from("", output);

    return output;
}

std::string list_RGA::to_string() const {
    std::vector<std::string> items = value();

    std::ostringstream stream;

    for (const std::string& item : items) {
        stream << "- " << item << '\n';
    }

    return stream.str();
}

bool list_RGA::has_applied(const std::string& operation_id) const {
    return applied_operations.find(operation_id) != applied_operations.end();
}