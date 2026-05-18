#include <counter.hpp>

Counter::Counter(std::string client_id)
  : _client_id(std::move(client_id)) {
  _state.increments[_client_id] = 0;
  _state.decrements[_client_id] = 0;
}

void Counter::increment(std::uint64_t amount) {
  _state.increments[_client_id] += amount;
}

void Counter::decrement(std::uint64_t amount) {
  _state.decrements[_client_id] += amount;
}

void Counter::merge(const Counter& other) {
  merge(other._state);
}

void Counter::merge(const CounterState& other) {
  for (const auto& [id, incoming] : other.increments) {
    auto& current = _state.increments[id];
    current = std::max(current, incoming);
  }
  for (const auto& [id, incoming] : other.decrements) {
    auto& current = _state.decrements[id];
    current = std::max(current, incoming);
  }
}

std::int64_t Counter::value() const {
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
