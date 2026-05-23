#ifndef TEXT_HPP
#define TEXT_HPP

#include <string>
#include <vector>

enum class TextOperationType {
    insert_op,
    delete_op
};

struct TextChange {
    TextOperationType type;

    // Unique ID for this operation.
    std::string operation_id;

    // For Insert: ID of the new character.
    // For Delete: ID of the character to delete.
    std::string element_id;

    // For Insert: insert after this element.
    // Empty string means ROOT / beginning.
    // For Delete: unused.
    std::string previous_id;

    // For Insert: the character to insert (one UTF-8 codepoint).
    // For Delete: unused.
    std::string value;
};

struct TextCharacter {
    std::string id;
    std::string previous_id;
    std::string value;
    bool deleted = false;
};

struct TextRGAState {
    std::vector<TextCharacter> nodes;
};

#endif
