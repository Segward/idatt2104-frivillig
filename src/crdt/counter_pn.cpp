// CounterPN implementation. See include/crdt/counter_pn.hpp for the API.
//
// Notes:
//  - State is two unordered_maps keyed by client ID; merge takes the
//    per-client max in each direction (commutative, associative, idempotent).
//  - value() casts uint64 totals to int64 before subtracting. Lossy near
//    2^63; expected counter magnitudes are nowhere close, so unguarded.

#include <crdt/counter_pn.hpp>

CounterPN::CounterPN(std::string client_id)
  : _client_id(std::move(client_id)) {
  // Prime both maps so this replica always appears in a state() snapshot,
  // distinguishing "present with zero count" from "absent".
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
  std::int64_t total_increments = 0;
  std::int64_t total_decrements = 0;

  for (const auto& [id, amount] : _state.increments) {
    total_increments += static_cast<std::int64_t>(amount);
  }
  for (const auto& [id, amount] : _state.decrements) {
    total_decrements += static_cast<std::int64_t>(amount);
  }

  return total_increments - total_decrements;
}
