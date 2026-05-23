#include <server/messages.hpp>
#include <server/handler.hpp>

// Try each envelope in turn; whichever decoder accepts the payload owns the
// dispatch and the others return nullopt. The whole block is try/catch'd
// because any unexpected exception from the CRDT merges should kill the
// message, not the server.
std::optional<std::string> process_message(
    CounterPN& counter,
    ListRGA& list,
    TextRGA& text,
    const std::string& client_id,
    std::string_view payload
) {
    const std::string str(payload);
    try {
        if (auto incoming = Handler::decode_counter(str)) {
            counter.merge(*incoming);
            printf("[server] counter merged from %s; value=%lld\n",
                   client_id.c_str(), static_cast<long long>(counter.value()));
            return Handler::encode_counter(counter.state());
        }
        if (auto incoming = Handler::decode_list_state(str)) {
            list.merge(*incoming);
            printf("[server] list merged from %s; size=%zu\n",
                   client_id.c_str(), list.value().size());
            return Handler::encode_list_state(list.state());
        }
        if (auto incoming = Handler::decode_text_state(str)) {
            text.merge(*incoming);
            printf("[server] text merged from %s; len=%zu\n",
                   client_id.c_str(), text.value().size());
            return Handler::encode_text_state(text.state());
        }
    } catch (const std::exception& e) {
        fprintf(stderr, "[server] message handler error from %s: %s\n",
                client_id.c_str(), e.what());
    }
    return std::nullopt;
}
