#include <counter.hpp>

#include <gtest/gtest.h>

TEST(CounterTest, StartsAtZero) {
    Counter counter("A");

    EXPECT_EQ(counter.value(), 0);
}

TEST(CounterTest, IncrementIncreasesValue) {
    Counter counter("A");

    counter.increment();

    EXPECT_EQ(counter.value(), 1);
}

TEST(CounterTest, IncrementByAmountIncreasesValue) {
    Counter counter("A");

    counter.increment(5);

    EXPECT_EQ(counter.value(), 5);
}

TEST(CounterTest, DecrementDecreasesValue) {
    Counter counter("A");

    counter.increment(5);
    counter.decrement();

    EXPECT_EQ(counter.value(), 4);
}

TEST(CounterTest, DecrementByAmountDecreasesValue) {
    Counter counter("A");

    counter.increment(5);
    counter.decrement(4);

    EXPECT_EQ(counter.value(), 1);
}

TEST(CounterTest, MergeCombinesValue) {
    Counter counter_A("A");
    Counter counter_B("B");

    counter_A.increment(5);
    counter_B.decrement();

    counter_A.merge(counter_B);

    EXPECT_EQ(counter_A.value(), 4);
}

TEST(CounterTest, CommutativeMerge) {
    Counter counter_A("A");
    Counter counter_B("B");

    counter_A.increment(10);
    counter_A.decrement(2);

    counter_B.increment(2);
    counter_B.decrement(5);

    counter_A.merge(counter_B);
    counter_B.merge(counter_A);

    EXPECT_EQ(counter_A.value(), 5);
    EXPECT_EQ(counter_B.value(), 5);
}

TEST(CounterTest, IdempotentMerge) {
    Counter counter_A("A");

    counter_A.increment(5);

    counter_A.merge(counter_A);

    EXPECT_EQ(counter_A.value(), 5);
}
