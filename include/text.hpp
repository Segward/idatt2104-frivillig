#ifndef TEXT_HPP
#define TEXT_HPP

#include <string>

enum class text_operation_type {
    Insert,
    Delete
};

struct text_change {
    text_operation_type type;

    // Unique ID for this operation.
    std::string operation_id;

    // For Insert: ID of the new character.
    // For Delete: ID of the character to delete.
    std::string element_id;

    // For Insert: insert after this element.
    // Empty string means ROOT / beginning.
    // For Delete: unused.
    std::string previous_id;

    // For Insert: the character to insert.
    // For Delete: unused.
    char value{};
};

struct text_character {
    std::string id;
    std::string previous_id;
    char value;
    bool deleted = false;
};

#endif
