#ifndef COUNTER_HPP
#define COUNTER_HPP

#include <cstdint>
#include <string>
#include <unordered_map>

struct counter_state {
  std::unordered_map<std::string, std::uint64_t> increments;
  std::unordered_map<std::string, std::uint64_t> decrements;
};

class counter {
private:
  std::string client_id;
  counter_state state;

public:
  explicit counter(std::string client_id);

  void increment(std::uint64_t amount = 1);
  void decrement(std::uint64_t amount = 1);

  void merge(const counter& incoming_counter);
  void merge(const counter_state& incoming_state);

  std::int64_t value() const;

  const counter_state& get_state() const;
};

#endif
