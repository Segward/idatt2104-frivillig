#ifndef COUNTER_PN_HPP
#define COUNTER_PN_HPP

#include <cstdint>
#include <string>
#include <unordered_map>

// State for a PN-Counter, storing increments and decrements per client.
struct CounterPNState {
  std::unordered_map<std::string, std::uint64_t> increments;
  std::unordered_map<std::string, std::uint64_t> decrements;
};

// PN-Counter CRDT supporting both increment and decrement operations.
class CounterPN {
  public:
    // Creates a counter for a specific client/replica.
    explicit CounterPN(std::string client_id);

    // Uses the default destructor.
    ~CounterPN() = default;

    // Prevents copying to avoid duplicating a replica identity.
    CounterPN(const CounterPN&) = delete;

    // Prevents copy assignment to avoid replacing replica identity/state.
    CounterPN& operator=(const CounterPN&) = delete;

    // Prevents moving to keep replica ownership stable.
    CounterPN(CounterPN&&) = delete;

    // Prevents move assignment to keep replica ownership stable.
    CounterPN& operator=(CounterPN&&) = delete;

    // Increments this client's local positive counter.
    void increment(std::uint64_t amount = 1);

    // Increments this client's local negative counter.
    void decrement(std::uint64_t amount = 1);

    // Merges another PN-Counter into this counter.
    void merge(const CounterPN& other);

    // Merges raw PN-Counter state into this counter.
    void merge(const CounterPNState& other);

    // Returns the visible value: sum(increments) - sum(decrements).
    std::int64_t value() const;

    // Returns the internal CRDT state.
    const CounterPNState& state() const { return _state; }

  private:
    // Unique identifier for this replica/client.
    std::string _client_id;

    // Internal PN-Counter state.
    CounterPNState _state;
};

#endif