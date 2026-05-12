#include "counter.hpp"

#include <algorithm>
#include <utility>

Counter::Counter(std::string client_id)
    : client_id(std::move(client_id)) 
{
    state.increments[this->client_id] = 0;
    state.decrements[this->client_id] = 0;
}

void Counter::increment(std::uint64_t amount) 
{
    state.increments[client_id] += amount;
}

void Counter::decrement(std::uint64_t amount) 
{
    state.decrements[client_id] += amount;
}

void Counter::merge(const Counter& incoming_counter) 
{
    for (const auto& [id, incoming_value] : incoming_counter.state.increments) {
        std::uint64_t current_value = 0;

        auto it = state.increments.find(id);
        if (it != state.increments.end()) {
            current_value = it->second;
        }

        state.increments[id] = std::max(current_value, incoming_value);
    }

    for (const auto& [id, incoming_value] : incoming_counter.state.decrements) {
        std::uint64_t current_value = 0;

        auto it = state.decrements.find(id);
        if (it != state.decrements.end()) {
            current_value = it->second;
        }

        state.decrements[id] = std::max(current_value, incoming_value);
    }
}

std::int64_t Counter::value() const 
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

const counter_state& Counter::get_state() const 
{
    return state;
}