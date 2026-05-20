#ifndef TEXT_RGA_HPP
#define TEXT_RGA_HPP

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

    std::unordered_map<std::string, text_character> nodes;

    std::unordered_map<std::string, std::vector<std::string>> children;

    std::unordered_set<std::string> applied_operations;

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

    text_RGA_state state() const;

    void merge(const text_RGA_state& other);

    std::string value() const;

    bool has_applied(const std::string& operation_id) const;
};

#endif
