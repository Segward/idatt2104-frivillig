#include <packet.hpp>

#include <gtest/gtest.h>

TEST(packet_test, round_trip_empty_state) {
  counter_state s;

  auto bytes = serialize_counter(s);
  counter_state out = parse_counter(bytes.data(), bytes.size());

  EXPECT_TRUE(out.increments.empty());
  EXPECT_TRUE(out.decrements.empty());
}

TEST(packet_test, round_trip_single_entry) {
  counter_state s;
  s.increments["A"] = 7;
  s.decrements["A"] = 3;

  auto bytes = serialize_counter(s);
  counter_state out = parse_counter(bytes.data(), bytes.size());

  EXPECT_EQ(out.increments["A"], 7u);
  EXPECT_EQ(out.decrements["A"], 3u);
}

TEST(packet_test, round_trip_multiple_entries_varied_id_lengths) {
  counter_state s;
  s.increments["x"] = 1;
  s.decrements["x"] = 0;
  s.increments["client-123"] = 42;
  s.decrements["client-123"] = 5;
  std::string long_id(64, 'q');
  s.increments[long_id] = 1000;
  s.decrements[long_id] = 999;

  auto bytes = serialize_counter(s);
  counter_state out = parse_counter(bytes.data(), bytes.size());

  EXPECT_EQ(out.increments.size(), 3u);
  EXPECT_EQ(out.decrements.size(), 3u);
  EXPECT_EQ(out.increments["x"], 1u);
  EXPECT_EQ(out.decrements["x"], 0u);
  EXPECT_EQ(out.increments["client-123"], 42u);
  EXPECT_EQ(out.decrements["client-123"], 5u);
  EXPECT_EQ(out.increments[long_id], 1000u);
  EXPECT_EQ(out.decrements[long_id], 999u);
}

TEST(packet_test, round_trip_inc_only_entry) {
  counter_state s;
  s.increments["only-inc"] = 9;

  auto bytes = serialize_counter(s);
  counter_state out = parse_counter(bytes.data(), bytes.size());

  EXPECT_EQ(out.increments["only-inc"], 9u);
  EXPECT_EQ(out.decrements["only-inc"], 0u);
}

TEST(packet_test, round_trip_dec_only_entry) {
  counter_state s;
  s.decrements["only-dec"] = 4;

  auto bytes = serialize_counter(s);
  counter_state out = parse_counter(bytes.data(), bytes.size());

  EXPECT_EQ(out.decrements["only-dec"], 4u);
  EXPECT_EQ(out.increments["only-dec"], 0u);
}

TEST(packet_test, parse_rejects_truncated_payload) {
  counter_state s;
  s.increments["abc"] = 1;
  s.decrements["abc"] = 1;

  auto bytes = serialize_counter(s);
  ASSERT_GT(bytes.size(), 2u);
  counter_state out = parse_counter(bytes.data(), bytes.size() - 2);

  EXPECT_TRUE(out.increments.empty());
  EXPECT_TRUE(out.decrements.empty());
}
