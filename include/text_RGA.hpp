#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <text.hpp>

class text_RGA {
private:
    std::string client_id;
    std::uint64_t local_sequence = 0;

    // element_id -> text_character
    std::unordered_map<std::string, text_character> nodes;

    // previous_id -> list of elements inserted after previous_id
    std::unordered_map<std::string, std::vector<std::string>> children;

    // Used to avoid applying the same operation multiple times.
    std::unordered_set<std::string> applied_operations;

    // Operations that arrived before their dependencies.
    std::vector<text_change> pending_changes;

    std::string next_id();

    bool node_exists(const std::string& node_id) const;

    bool pending_contains(const std::string& operation_id) const;

    bool try_apply_change(const text_change& change);

    void retry_pending_changes();

    void render_from(
        const std::string& previous_id,
        std::string& output
    ) const;

public:
    explicit text_RGA(std::string client_id);

    text_change insert_at_beginning(char value);

    text_change insert_after(
        const std::string& previous_id,
        char value
    );

    text_change erase(const std::string& element_id);

    void apply(const text_change& change);

    std::string value() const;

    bool has_applied(const std::string& operation_id) const;
};