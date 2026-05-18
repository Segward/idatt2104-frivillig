#include <counter.hpp>

#include <algorithm>
#include <utility>

counter::counter(std::string client_id)
  : client_id(std::move(client_id))
{
  state.increments[this->client_id] = 0;
  state.decrements[this->client_id] = 0;
}

void counter::increment(std::uint64_t amount)
{
  state.increments[client_id] += amount;
}

void counter::decrement(std::uint64_t amount)
{
  state.decrements[client_id] += amount;
}

void counter::merge(const counter& incoming_counter)
{
  merge(incoming_counter.state);
}

void counter::merge(const counter_state& incoming_state)
{
  for (const auto& [id, incoming_value] : incoming_state.increments) {
    auto& current_value = state.increments[id];
    current_value = std::max(current_value, incoming_value);
  }
  for (const auto& [id, incoming_value] : incoming_state.decrements) {
    auto& current_value = state.decrements[id];
    current_value = std::max(current_value, incoming_value);
  }
}

std::int64_t counter::value() const
{
  std::int64_t total_increments = 0;
  std::int64_t total_decrements = 0;

  for (const auto& [id, amount] : state.increments) {
    total_increments += static_cast<std::int64_t>(amount);
  }
  for (const auto& [id, amount] : state.decrements) {
    total_decrements += static_cast<std::int64_t>(amount);
  }

  return total_increments - total_decrements;
}

const counter_state& counter::get_state() const
{
  return state;
}
