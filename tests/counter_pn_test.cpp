#include <crdt/counter_pn.hpp>

#include <gtest/gtest.h>

TEST(counter_test, starts_at_zero) {
  CounterPN counter("A");

  EXPECT_EQ(counter.value(), 0);
}

TEST(counter_test, increment_increases_value) {
  CounterPN counter("A");

  counter.increment();

  EXPECT_EQ(counter.value(), 1);
}

TEST(counter_test, increment_by_amount_increases_value) {
  CounterPN counter("A");

  counter.increment(5);

  EXPECT_EQ(counter.value(), 5);
}

TEST(counter_test, decrement_decreases_value) {
  CounterPN counter("A");

  counter.increment(5);
  counter.decrement();

  EXPECT_EQ(counter.value(), 4);
}

TEST(counter_test, decrement_by_amount_decreases_value) {
  CounterPN counter("A");

  counter.increment(5);
  counter.decrement(4);

  EXPECT_EQ(counter.value(), 1);
}

TEST(counter_test, merge_combines_value) {
  CounterPN counter_a("A");
  CounterPN counter_b("B");

  counter_a.increment(5);
  counter_b.decrement();

  counter_a.merge(counter_b);

  EXPECT_EQ(counter_a.value(), 4);
}

TEST(counter_test, commutative_merge) {
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

TEST(counter_test, idempotent_merge) {
  CounterPN counter_a("A");

  counter_a.increment(5);

  counter_a.merge(counter_a);

  EXPECT_EQ(counter_a.value(), 5);
}
