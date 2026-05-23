#ifndef COUNTER_PN_HPP
#define COUNTER_PN_HPP

// Wire form of a PN-counter: per-replica increment and decrement totals
// shipped to a peer for a state sync.
struct CounterPNState {
  std::unordered_map<std::string, std::uint64_t> increments;
  std::unordered_map<std::string, std::uint64_t> decrements;
};

// PN-Counter CRDT: a counter every replica can update independently while
// still converging to the same value after merges. A plain shared counter
// loses concurrent updates; the PN-Counter avoids that by tracking each
// replica's increments and decrements separately and merging by taking the
// per-replica max.
class CounterPN {
  public:
    explicit CounterPN(std::string client_id);
    ~CounterPN() = default;

    // A replica's identity is its client ID; copying or moving the counter
    // would silently fork or relocate that identity and break convergence.
    CounterPN(const CounterPN&) = delete;
    CounterPN& operator=(const CounterPN&) = delete;
    CounterPN(CounterPN&&) = delete;
    CounterPN& operator=(CounterPN&&) = delete;

    void increment(std::uint64_t amount = 1);
    void decrement(std::uint64_t amount = 1);

    void merge(const CounterPN& other);

    // Per-client max is monotonic, so repeated or out-of-order merges all
    // reach the same value.
    void merge(const CounterPNState& other);

    std::int64_t value() const;
    const CounterPNState& state() const { return _state; }

  private:
    std::string _client_id;
    CounterPNState _state;
};

#endif
