#ifndef COUNTER_PN_HPP
#define COUNTER_PN_HPP

#include <cstdint>
#include <string>
#include <unordered_map>

struct counter_pn_state {
  std::unordered_map<std::string, std::uint64_t> increments;
  std::unordered_map<std::string, std::uint64_t> decrements;
};

class counter_pn {
  public:
    explicit counter_pn(std::string client_id);
    ~counter_pn() = default;

    counter_pn(const counter_pn&) = delete;
    counter_pn& operator=(const counter_pn&) = delete;
    counter_pn(counter_pn&&) = delete;
    counter_pn& operator=(counter_pn&&) = delete;

    void increment(std::uint64_t amount = 1);
    void decrement(std::uint64_t amount = 1);

    void merge(const counter_pn& other);
    void merge(const counter_pn_state& other);

    std::int64_t value() const;
    const counter_pn_state& state() const { return _state; }

  private:
    std::string _client_id;
    counter_pn_state _state;
};

#endif
