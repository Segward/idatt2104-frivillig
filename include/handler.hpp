#ifndef HANDLER_HPP
#define HANDLER_HPP

#include <counter.hpp>
#include <packet.hpp>

class Handler {
  public:
    explicit Handler(Counter& target, std::string filter_id = {});
    ~Handler() = default;

    Handler(const Handler&) = delete;
    Handler& operator=(const Handler&) = delete;
    Handler(Handler&&) = delete;
    Handler& operator=(Handler&&) = delete;

    bool apply(Packet::Type type, const std::vector<std::uint8_t>& payload);

    static std::vector<std::uint8_t> encode_counter(const CounterState& state);
    static std::optional<CounterState> decode_counter(const std::uint8_t* data, std::size_t len);

  private:
    Counter& _target;
    std::string _filter_id;
};

#endif
