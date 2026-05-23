#ifndef TEXT_RGA_HPP
#define TEXT_RGA_HPP

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

#include <text.hpp>

// RGA-style text CRDT for ordered character sequences.
class TextRGA {
private:
    // Unique identifier for this replica/client.
    std::string _client_id;

    // Local counter used to generate unique operation and element IDs.
    std::uint64_t _local_sequence = 0;

    // Stores all text characters by their unique element ID.
    std::unordered_map<std::string, TextCharacter> _nodes;

    // Stores ordering: previous_id -> characters inserted after previous_id.
    std::unordered_map<std::string, std::vector<std::string>> _children;

    // Tracks applied operations to prevent duplicate application.
    std::unordered_set<std::string> _applied_operations;

    // Stores operations that arrived before their dependencies.
    std::vector<TextChange> _pending_changes;

    // Generates a unique ID for local operations and elements.
    std::string next_id();

    // Checks whether a character node exists.
    bool node_exists(const std::string& node_id) const;

    // Checks whether an operation is already waiting in the pending buffer.
    bool pending_contains(const std::string& operation_id) const;

    // Attempts to apply an operation if its dependencies are available.
    bool try_apply_change(const TextChange& change);

    // Retries pending operations after new dependencies are added.
    void retry_pending_changes();

    // Recursively builds the visible text from a given previous element.
    void render_from(
        const std::string& previous_id,
        std::string& output
    ) const;

public:
    // Creates a text CRDT for a specific client/replica.
    explicit TextRGA(std::string client_id);

    // Inserts a character at the beginning of the text.
    TextChange insert_at_beginning(std::string value);

    // Inserts a character after an existing character ID.
    TextChange insert_after(
        const std::string& previous_id,
        std::string value
    );

    // Marks a character as deleted without physically removing it.
    TextChange erase(const std::string& element_id);

    // Applies a local or remote text operation.
    void apply(const TextChange& change);

    // Returns a serializable snapshot of the current text state.
    TextRGAState state() const;

    // Merges incoming text state into this replica.
    void merge(const TextRGAState& other);

    // Returns the visible, non-deleted text.
    std::string value() const;

    // Checks whether an operation has already been applied.
    bool has_applied(const std::string& operation_id) const;
};

#endif
