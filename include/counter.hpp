#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

struct counter_state {
    std::unordered_map<std::string, std::uint64_t> increments;
    std::unordered_map<std::string, std::uint64_t> decrements;
};

class Counter {
private:
    std::string client_id;
    counter_state state;

public:
    explicit Counter(std::string client_id);

    void increment(std::uint64_t amount = 1);
    void decrement(std::uint64_t amount = 1);

    void merge(const Counter& incoming_counter);

    std::int64_t value() const;

    const counter_state& get_state() const;
};