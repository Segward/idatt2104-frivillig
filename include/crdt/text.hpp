#ifndef TEXT_HPP
#define TEXT_HPP

enum class TextOperationType {
    insert_op,
    delete_op
};

// A single character-level mutation packaged for replication. Carries
// everything a peer needs to apply the change, or to buffer it until its
// causal predecessor arrives. previous_id is empty for the document head
// and unused for deletes. value is one UTF-8 codepoint on inserts.
struct TextChange {
    TextOperationType type;
    std::string operation_id;
    std::string element_id;
    std::string previous_id;
    std::string value;
};

// One character in the text. `deleted` is a tombstone flag rather than a
// physical removal so concurrent deletes converge without orphaning inserts
// that reference this character as previous_id.
struct TextCharacter {
    std::string id;
    std::string previous_id;
    std::string value;
    bool deleted = false;
};

// Flat snapshot of every character for a state transfer between replicas.
struct TextRGAState {
    std::vector<TextCharacter> nodes;
};

#endif
