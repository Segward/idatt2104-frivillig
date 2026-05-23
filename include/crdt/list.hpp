#ifndef LIST_HPP
#define LIST_HPP

enum class ListOperationType {
    insert_op,
    delete_op
};

// A single list mutation packaged for replication. Carries everything a peer
// needs to apply the change, or to buffer it until its causal predecessor
// arrives. previous_id is empty for the list head and unused for deletes.
struct ListChange {
    ListOperationType type;
    std::string operation_id;
    std::string element_id;
    std::string previous_id;
    std::string value;
};

// One node in the list. `deleted` is a tombstone flag rather than a physical
// removal so concurrent deletes converge without orphaning inserts that
// reference this node as previous_id.
struct ListItem {
    std::string id;
    std::string previous_id;
    std::string value;
    bool deleted = false;
};

// Flat snapshot of every node for a state transfer between replicas.
struct ListRGAState {
    std::vector<ListItem> nodes;
};

#endif
