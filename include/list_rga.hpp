#ifndef LIST_RGA_HPP
#define LIST_RGA_HPP

#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <list.hpp>

class list_RGA {
private:
    std::string client_id;
    std::uint64_t local_sequence = 0;

    std::unordered_map<std::string, list_item> nodes;

    std::unordered_map<std::string, std::vector<std::string>> children;

    std::unordered_set<std::string> applied_operations;

    std::vector<list_change> pending_changes;

    std::string next_id();

    bool pending_contains(const std::string& operation_id) const;

    bool try_apply_change(const list_change& change);

    void retry_pending_changes();

    void render_from(
        const std::string& previous_id,
        std::vector<std::string>& output
    ) const;

public:
    explicit list_RGA(std::string client_id);

    list_change insert_at_beginning(const std::string& value);

    list_change insert_after(
        const std::string& previous_id,
        const std::string& value
    );

    list_change erase(const std::string& element_id);

    void apply(const list_change& change);

    list_RGA_state state() const;

    void merge(const list_RGA_state& other);

    std::vector<std::string> value() const;

    std::string to_string() const;

    bool has_applied(const std::string& operation_id) const;
};

#endif
