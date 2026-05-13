#include <counter.hpp>

#include <algorithm>
#include <utility>

//Counter featuring both increment and decrement functionality
Counter::Counter(std::string client_id)
    : client_id(std::move(client_id)) 
{
    state.increments[this->client_id] = 0;
    state.decrements[this->client_id] = 0;
}

//Increment counter by arbitrary amount
void Counter::increment(std::uint64_t amount) 
{
    if (amount < 0) throw std::exception("Increment amount cannot be negative");
    state.increments[client_id] += amount;
}

//Decrement the counter by arbitrary amount
void Counter::decrement(std::uint64_t amount) 
{
    if (amount < 0) throw std::exception("Decrement amount cannot be negative");
    state.decrements[client_id] += amount;
}

// Merges counter with an incoming counter, combining results
void Counter::merge(const Counter& incoming_counter) 
{
    for (const auto& [id, incoming_value] : incoming_counter.state.increments) {
        std::uint64_t current_value = 0;

        auto incoming_increments = state.increments.find(id);
        if (incoming_increments != state.increments.end()) {
            current_value = incoming_increments->second;
        }

        state.increments[id] = std::max(current_value, incoming_value);
    }

    for (const auto& [id, incoming_value] : incoming_counter.state.decrements) {
        std::uint64_t current_value = 0;

        auto incoming_decrements = state.decrements.find(id);
        if (incoming_decrements != state.decrements.end()) {
            current_value = incoming_decrements->second;
        }

        state.decrements[id] = std::max(current_value, incoming_value);
    }
}

//Calculates current value based on increment and decrement history
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