#ifndef MESSAGES_HPP
#define MESSAGES_HPP

#include <crdt/counter_pn.hpp>
#include <crdt/list_rga.hpp>
#include <crdt/text_rga.hpp>

// Fallback listen address used by main.cpp when no override is supplied.
// Centralised so config, scripts, and tests stay in sync.
inline constexpr const char* default_host = "0.0.0.0";
inline constexpr unsigned default_port = 12345;

// Try-each-envelope dispatch. Feeds `payload` to each Handler::decode_* in
// turn; the first decoder that accepts owns the merge and returns the echo
// to send back. Returns nullopt if nothing matched or a decoder/merge threw
// (the throw is logged to stderr but swallowed so one bad payload can't kill
// the server).
std::optional<std::string> process_message(
    CounterPN& counter,
    ListRGA& list,
    TextRGA& text,
    const std::string& client_id,
    std::string_view payload
);

#endif
