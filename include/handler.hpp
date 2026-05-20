#ifndef HANDLER_HPP
#define HANDLER_HPP

#include <counter_pn.hpp>
#include <list.hpp>
#include <text.hpp>

class Handler {
  public:
    static std::string encode_auth(const std::string& id);

    static std::string encode_counter(const counter_pn_state& state);
    static std::optional<counter_pn_state> decode_counter(const std::string& json_text);

    static std::string encode_list_state(const list_RGA_state& state);
    static std::optional<list_RGA_state> decode_list_state(const std::string& json_text);

    static std::string encode_text_state(const text_RGA_state& state);
    static std::optional<text_RGA_state> decode_text_state(const std::string& json_text);
};

#endif
