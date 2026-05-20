#ifndef LIST_HPP
#define LIST_HPP

#include <string>

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

#endif
