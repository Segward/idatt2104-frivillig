#ifndef LIST_RGA_HPP
#define LIST_RGA_HPP

#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <utility>

#include <list.hpp>

// RGA-style list CRDT for ordered text items.
class list_RGA {
private:
    // Unique identifier for this replica/client.
    std::string client_id;

    // Local counter used to generate unique operation and element IDs.
    std::uint64_t local_sequence = 0;

    // Stores all list items by their unique element ID.
    std::unordered_map<std::string, list_item> nodes;

    // Stores ordering: previous_id -> items inserted after previous_id.
    std::unordered_map<std::string, std::vector<std::string>> children;

    // Tracks applied operations to prevent duplicate application.
    std::unordered_set<std::string> applied_operations;

    // Generates a unique ID for local operations and elements.
    std::string next_id();

    // Recursively builds the visible list from a given previous element.
    void render_from(
        const std::string& previous_id,
        std::vector<std::string>& output
    ) const;

public:
    // Creates a list CRDT for a specific client/replica.
    explicit list_RGA(std::string client_id);

    // Inserts a new item at the beginning of the list.
    list_change insert_at_beginning(const std::string& value);

    // Inserts a new item after an existing item ID.
    list_change insert_after(
        const std::string& previous_id,
        const std::string& value
    );

    // Marks an item as deleted without physically removing it.
    list_change erase(const std::string& element_id);

    // Applies a local or remote list operation.
    void apply(const list_change& change);

    // Returns a serializable snapshot of the current list state.
    list_RGA_state state() const;

    // Merges incoming list state into this replica.
    void merge(const list_RGA_state& other);

    // Returns the visible, non-deleted list items.
    std::vector<std::string> value() const;

    // Formats the visible list as a printable string.
    std::string to_string() const;

    // Checks whether an operation has already been applied.
    bool has_applied(const std::string& operation_id) const;
};

#endif