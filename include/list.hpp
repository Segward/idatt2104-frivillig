#ifndef LIST_HPP
#define LIST_HPP

#include <string>
#include <vector>

enum class ListOperationType {
    insert_op,
    delete_op
};

struct ListChange {
    ListOperationType type;

    std::string operation_id;

    std::string element_id;

    std::string previous_id;

    std::string value;
};

struct ListItem {
    std::string id;
    std::string previous_id;
    std::string value;
    bool deleted = false;
};

struct ListRGAState {
    std::vector<ListItem> nodes;
};

#endif
