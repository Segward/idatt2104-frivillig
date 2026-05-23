#include <crdt/counter_pn.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <limits>
#include <type_traits>

static_assert(!std::is_copy_constructible_v<CounterPN>, "CounterPN must not be copyable");
static_assert(!std::is_move_constructible_v<CounterPN>, "CounterPN must not be movable");

TEST(counter_pn_test, value_saturates_on_uint64_max_increment) {
  // A hostile or buggy peer can ship UINT64_MAX in its state — merge accepts
  // it (per-replica max is monotonic). value() must not wrap to a negative
  // number; it should saturate at INT64_MAX instead.
  CounterPN counter("A");
  CounterPNState hostile;
  hostile.increments["evil"] = std::numeric_limits<std::uint64_t>::max();
  counter.merge(hostile);
  EXPECT_EQ(counter.value(), std::numeric_limits<std::int64_t>::max());
}

TEST(counter_pn_test, starts_at_zero) {
  CounterPN counter("A");
  EXPECT_EQ(counter.value(), 0);
}

TEST(counter_pn_test, increment_increases_value) {
  CounterPN counter("A");
  counter.increment();
  EXPECT_EQ(counter.value(), 1);
}

TEST(counter_pn_test, increment_by_amount_increases_value) {
  CounterPN counter("A");
  counter.increment(5);
  EXPECT_EQ(counter.value(), 5);
}

TEST(counter_pn_test, decrement_decreases_value) {
  CounterPN counter("A");
  counter.increment(5);
  counter.decrement();
  EXPECT_EQ(counter.value(), 4);
}

TEST(counter_pn_test, decrement_by_amount_decreases_value) {
  CounterPN counter("A");
  counter.increment(5);
  counter.decrement(4);
  EXPECT_EQ(counter.value(), 1);
}

TEST(counter_pn_test, merge_combines_value) {
  CounterPN counter_a("A");
  CounterPN counter_b("B");
  counter_a.increment(5);
  counter_b.decrement();
  counter_a.merge(counter_b);
  EXPECT_EQ(counter_a.value(), 4);
}

TEST(counter_pn_test, commutative_merge) {
  CounterPN counter_a("A");
  CounterPN counter_b("B");
  counter_a.increment(10);
  counter_a.decrement(2);
  counter_b.increment(2);
  counter_b.decrement(5);
  counter_a.merge(counter_b);
  counter_b.merge(counter_a);
  EXPECT_EQ(counter_a.value(), 5);
  EXPECT_EQ(counter_b.value(), 5);
}

TEST(counter_pn_test, idempotent_merge) {
  CounterPN counter_a("A");
  counter_a.increment(5);
  counter_a.merge(counter_a);
  EXPECT_EQ(counter_a.value(), 5);
}

TEST(counter_pn_test, decrement_below_zero_yields_negative_value) {
  CounterPN counter("A");
  counter.decrement(3);
  EXPECT_EQ(counter.value(), -3);
}

TEST(counter_pn_test, associative_merge) {
  CounterPN counter_a("A");
  CounterPN counter_b("B");
  CounterPN counter_c("C");
  counter_a.increment(10);
  counter_b.increment(3);
  counter_b.decrement(1);
  counter_c.decrement(2);
  CounterPN left_first("L");
  left_first.merge(counter_a);
  left_first.merge(counter_b);
  left_first.merge(counter_c);
  CounterPN right_first("R");
  right_first.merge(counter_b);
  right_first.merge(counter_c);
  right_first.merge(counter_a);
  EXPECT_EQ(left_first.value(), right_first.value());
  EXPECT_EQ(left_first.value(), 10);
}
