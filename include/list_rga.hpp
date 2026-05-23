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
class ListRGA {
private:
    // Unique identifier for this replica/client.
    std::string _client_id;

    // Local counter used to generate unique operation and element IDs.
    std::uint64_t _local_sequence = 0;

    // Stores all list items by their unique element ID.
    std::unordered_map<std::string, ListItem> _nodes;

    // Stores ordering: previous_id -> items inserted after previous_id.
    std::unordered_map<std::string, std::vector<std::string>> _children;

    // Tracks applied operations to prevent duplicate application.
    std::unordered_set<std::string> _applied_operations;

    // Buffers operations whose causal dependencies haven't arrived yet.
    std::vector<ListChange> _pending_changes;

    // Generates a unique ID for local operations and elements.
    std::string next_id();

    // Checks if an operation is already buffered as pending.
    bool pending_contains(const std::string& operation_id) const;

    // Attempts to apply a change; returns false if dependencies are missing.
    bool try_apply_change(const ListChange& change);

    // Re-attempts buffered pending changes after new state arrives.
    void retry_pending_changes();

    // Recursively builds the visible list from a given previous element.
    void render_from(
        const std::string& previous_id,
        std::vector<std::string>& output
    ) const;

public:
    // Creates a list CRDT for a specific client/replica.
    explicit ListRGA(std::string client_id);

    // Inserts a new item at the beginning of the list.
    ListChange insert_at_beginning(const std::string& value);

    // Inserts a new item after an existing item ID.
    ListChange insert_after(
        const std::string& previous_id,
        const std::string& value
    );

    // Marks an item as deleted without physically removing it.
    ListChange erase(const std::string& element_id);

    // Applies a local or remote list operation.
    void apply(const ListChange& change);

    // Returns a serializable snapshot of the current list state.
    ListRGAState state() const;

    // Merges incoming list state into this replica.
    void merge(const ListRGAState& other);

    // Returns the visible, non-deleted list items.
    std::vector<std::string> value() const;

    // Formats the visible list as a printable string.
    std::string to_string() const;

    // Checks whether an operation has already been applied.
    bool has_applied(const std::string& operation_id) const;
};

#endif
