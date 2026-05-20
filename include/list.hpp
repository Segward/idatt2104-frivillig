#ifndef LIST_HPP
#define LIST_HPP

#include <string>
#include <vector>

enum class list_operation_type {
    Insert,
    Delete
};

struct list_change {
    list_operation_type type;

    std::string operation_id;

    std::string element_id;

    std::string previous_id;

    std::string value;
};

struct list_item {
    std::string id;
    std::string previous_id;
    std::string value;
    bool deleted = false;
};

struct list_RGA_state {
    std::vector<list_item> nodes;
};

#endif
