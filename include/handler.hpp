#ifndef HANDLER_HPP
#define HANDLER_HPP

#include <counter_pn.hpp>
#include <list.hpp>
#include <text.hpp>

class Handler {
  public:
    explicit Handler(counter_pn& target, std::string filter_id = {});
    ~Handler() = default;

    Handler(const Handler&) = delete;
    Handler& operator=(const Handler&) = delete;
    Handler(Handler&&) = delete;
    Handler& operator=(Handler&&) = delete;

    // Returns true if the message was a recognized counter update that merged.
    bool apply(const std::string& json_text);

    static std::string encode_auth(const std::string& id);
    static std::string encode_counter(const counter_pn_state& state);
    static std::optional<counter_pn_state> decode_counter(const std::string& json_text);

    static std::string encode_list_op(const list_change& change);
    static std::optional<list_change> decode_list_op(const std::string& json_text);
    static std::string encode_list_init(const std::vector<list_change>& ops);

    static std::string encode_text_op(const text_change& change);
    static std::optional<text_change> decode_text_op(const std::string& json_text);
    static std::string encode_text_init(const std::vector<text_change>& ops);

  private:
    counter_pn& _target;
    std::string _filter_id;
};

#endif
