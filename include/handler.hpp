#ifndef HANDLER_HPP
#define HANDLER_HPP

#include <counter_pn.hpp>
#include <list.hpp>
#include <text.hpp>

class Handler {
  public:
    static std::string encode_auth(const std::string& id);

    static std::string encode_counter(const CounterPNState& state);
    static std::optional<CounterPNState> decode_counter(const std::string& json_text);

    static std::string encode_list_state(const ListRGAState& state);
    static std::optional<ListRGAState> decode_list_state(const std::string& json_text);

    static std::string encode_text_state(const TextRGAState& state);
    static std::optional<TextRGAState> decode_text_state(const std::string& json_text);
};

#endif
