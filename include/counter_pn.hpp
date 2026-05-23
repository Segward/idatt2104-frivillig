#ifndef COUNTER_PN_HPP
#define COUNTER_PN_HPP

#include <cstdint>
#include <string>
#include <unordered_map>

// State for a PN-Counter, storing increments and decrements per client.
struct counter_pn_state {
  std::unordered_map<std::string, std::uint64_t> increments;
  std::unordered_map<std::string, std::uint64_t> decrements;
};

// PN-Counter CRDT supporting both increment and decrement operations.
class counter_pn {
  public:
    // Creates a counter for a specific client/replica.
    explicit counter_pn(std::string client_id);

    // Uses the default destructor.
    ~counter_pn() = default;

    // Prevents copying to avoid duplicating a replica identity.
    counter_pn(const counter_pn&) = delete;

    // Prevents copy assignment to avoid replacing replica identity/state.
    counter_pn& operator=(const counter_pn&) = delete;

    // Prevents moving to keep replica ownership stable.
    counter_pn(counter_pn&&) = delete;

    // Prevents move assignment to keep replica ownership stable.
    counter_pn& operator=(counter_pn&&) = delete;

    // Increments this client's local positive counter.
    void increment(std::uint64_t amount = 1);

    // Increments this client's local negative counter.
    void decrement(std::uint64_t amount = 1);

    // Merges another PN-Counter into this counter.
    void merge(const counter_pn& other);

    // Merges raw PN-Counter state into this counter.
    void merge(const counter_pn_state& other);

    // Returns the visible value: sum(increments) - sum(decrements).
    std::int64_t value() const;

    // Returns the internal CRDT state.
    const counter_pn_state& state() const { return _state; }

  private:
    // Unique identifier for this replica/client.
    std::string _client_id;

    // Internal PN-Counter state.
    counter_pn_state _state;
};

#endif