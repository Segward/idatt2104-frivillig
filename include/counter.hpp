#ifndef COUNTER_HPP
#define COUNTER_HPP

struct CounterState {
  std::unordered_map<std::string, std::uint64_t> increments;
  std::unordered_map<std::string, std::uint64_t> decrements;
};

class Counter {
  public:
    explicit Counter(std::string client_id);
    ~Counter() = default;

    Counter(const Counter&) = delete;
    Counter& operator=(const Counter&) = delete;
    Counter(Counter&&) = delete;
    Counter& operator=(Counter&&) = delete;

    void increment(std::uint64_t amount = 1);
    void decrement(std::uint64_t amount = 1);

    void merge(const Counter& other);
    void merge(const CounterState& other);

    std::int64_t value() const;
    const CounterState& state() const { return _state; }

  private:
    std::string _client_id;
    CounterState _state;
};

#endif
