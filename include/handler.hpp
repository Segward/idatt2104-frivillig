#ifndef HANDLER_HPP
#define HANDLER_HPP

#include <crdt/counter_pn.hpp>
#include <crdt/list.hpp>
#include <crdt/text.hpp>

// Translates between in-memory CRDT state and the JSON envelopes exchanged
// with browser clients. Centralising the wire format here keeps the server,
// the tests, and the JS client agreeing on a single schema; decoders return
// std::nullopt rather than throwing so callers can try each envelope in
// turn without exception-driven control flow.
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
