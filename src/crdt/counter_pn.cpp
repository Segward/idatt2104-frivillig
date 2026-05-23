// CounterPN implementation. See include/crdt/counter_pn.hpp for the API.

#include <crdt/counter_pn.hpp>

namespace {
    // Sums the per-replica counts, clamping at INT64_MAX so a hostile peer
    // shipping UINT64_MAX can't wrap value() to a negative result.
    template<typename Bucket>
    std::uint64_t saturating_sum(const Bucket& bucket) {
        constexpr auto cap = static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max());
        std::uint64_t total = 0;
        for (const auto& [id, amount] : bucket) {
            if (amount >= cap - total) return cap;
            total += amount;
        }
        return total;
    }
}

// Prime both maps so this replica always appears in a state() snapshot,
// distinguishing "present with zero count" from "absent".
CounterPN::CounterPN(std::string client_id)
  : _client_id(std::move(client_id)) {
  _state.increments[_client_id] = 0;
  _state.decrements[_client_id] = 0;
}

void CounterPN::increment(std::uint64_t amount) {
  _state.increments[_client_id] += amount;
}

void CounterPN::decrement(std::uint64_t amount) {
  _state.decrements[_client_id] += amount;
}

void CounterPN::merge(const CounterPN& other) {
  merge(other._state);
}

void CounterPN::merge(const CounterPNState& other) {
  for (const auto& [id, incoming] : other.increments) {
    auto& current = _state.increments[id];
    current = std::max(current, incoming);
  }
  for (const auto& [id, incoming] : other.decrements) {
    auto& current = _state.decrements[id];
    current = std::max(current, incoming);
  }
}

std::int64_t CounterPN::value() const {
  const std::uint64_t inc = saturating_sum(_state.increments);
  const std::uint64_t dec = saturating_sum(_state.decrements);
  return static_cast<std::int64_t>(inc) - static_cast<std::int64_t>(dec);
}
