#include <handler.hpp>

#include <gtest/gtest.h>

TEST(handler_test, round_trip_empty_state) {
  CounterState state;

  auto bytes = Handler::encode_counter(state);
  auto out = Handler::decode_counter(bytes.data(), bytes.size());

  ASSERT_TRUE(out.has_value());
  EXPECT_TRUE(out->increments.empty());
  EXPECT_TRUE(out->decrements.empty());
}

TEST(handler_test, round_trip_single_entry) {
  CounterState state;
  state.increments["A"] = 7;
  state.decrements["A"] = 3;

  auto bytes = Handler::encode_counter(state);
  auto out = Handler::decode_counter(bytes.data(), bytes.size());

  ASSERT_TRUE(out.has_value());
  EXPECT_EQ(out->increments["A"], 7u);
  EXPECT_EQ(out->decrements["A"], 3u);
}

TEST(handler_test, round_trip_multiple_entries_varied_id_lengths) {
  CounterState state;
  state.increments["x"] = 1;
  state.decrements["x"] = 0;
  state.increments["client-123"] = 42;
  state.decrements["client-123"] = 5;
  std::string long_id(64, 'q');
  state.increments[long_id] = 1000;
  state.decrements[long_id] = 999;

  auto bytes = Handler::encode_counter(state);
  auto out = Handler::decode_counter(bytes.data(), bytes.size());

  ASSERT_TRUE(out.has_value());
  EXPECT_EQ(out->increments.size(), 3u);
  EXPECT_EQ(out->decrements.size(), 3u);
  EXPECT_EQ(out->increments["x"], 1u);
  EXPECT_EQ(out->decrements["x"], 0u);
  EXPECT_EQ(out->increments["client-123"], 42u);
  EXPECT_EQ(out->decrements["client-123"], 5u);
  EXPECT_EQ(out->increments[long_id], 1000u);
  EXPECT_EQ(out->decrements[long_id], 999u);
}

TEST(handler_test, round_trip_inc_only_entry) {
  CounterState state;
  state.increments["only-inc"] = 9;

  auto bytes = Handler::encode_counter(state);
  auto out = Handler::decode_counter(bytes.data(), bytes.size());

  ASSERT_TRUE(out.has_value());
  EXPECT_EQ(out->increments["only-inc"], 9u);
  EXPECT_EQ(out->decrements["only-inc"], 0u);
}

TEST(handler_test, round_trip_dec_only_entry) {
  CounterState state;
  state.decrements["only-dec"] = 4;

  auto bytes = Handler::encode_counter(state);
  auto out = Handler::decode_counter(bytes.data(), bytes.size());

  ASSERT_TRUE(out.has_value());
  EXPECT_EQ(out->decrements["only-dec"], 4u);
  EXPECT_EQ(out->increments["only-dec"], 0u);
}

TEST(handler_test, decode_rejects_truncated_payload) {
  CounterState state;
  state.increments["abc"] = 1;
  state.decrements["abc"] = 1;

  auto bytes = Handler::encode_counter(state);
  ASSERT_GT(bytes.size(), 2u);
  auto out = Handler::decode_counter(bytes.data(), bytes.size() - 2);

  EXPECT_FALSE(out.has_value());
}

TEST(handler_test, decode_rejects_empty_buffer) {
  std::uint8_t empty = 0;
  auto out = Handler::decode_counter(&empty, 0);
  EXPECT_FALSE(out.has_value());
}

TEST(handler_test, decode_rejects_count_without_entries) {
  // Header claims 1 entry but no payload follows.
  std::vector<std::uint8_t> bytes = {0, 0, 0, 1};
  auto out = Handler::decode_counter(bytes.data(), bytes.size());
  EXPECT_FALSE(out.has_value());
}

TEST(handler_test, decode_rejects_oversized_count) {
  // Claims 0xFFFFFFFF entries; loop must bail at the first bounds check
  // without iterating ~4 billion times.
  std::vector<std::uint8_t> bytes = {0xFF, 0xFF, 0xFF, 0xFF};
  auto out = Handler::decode_counter(bytes.data(), bytes.size());
  EXPECT_FALSE(out.has_value());
}

TEST(handler_test, decode_rejects_trailing_bytes) {
  CounterState state;
  state.increments["A"] = 1;
  state.decrements["A"] = 0;
  auto bytes = Handler::encode_counter(state);
  bytes.push_back(0xAB);
  auto out = Handler::decode_counter(bytes.data(), bytes.size());
  EXPECT_FALSE(out.has_value());
}

TEST(handler_test, decode_rejects_duplicate_ids) {
  // Manually craft a frame with two entries both naming "A".
  std::vector<std::uint8_t> bytes;
  // count = 2
  bytes.insert(bytes.end(), {0, 0, 0, 2});
  // entry 1: id_len=1, "A", inc=1, dec=0
  bytes.insert(bytes.end(), {0, 1, 'A'});
  bytes.insert(bytes.end(), 8, 0); bytes.back() = 1;
  bytes.insert(bytes.end(), 8, 0);
  // entry 2: id_len=1, "A", inc=2, dec=0
  bytes.insert(bytes.end(), {0, 1, 'A'});
  bytes.insert(bytes.end(), 8, 0); bytes.back() = 2;
  bytes.insert(bytes.end(), 8, 0);

  auto out = Handler::decode_counter(bytes.data(), bytes.size());
  EXPECT_FALSE(out.has_value());
}

TEST(handler_test, apply_with_filter_drops_other_ids) {
  CounterState state;
  state.increments["legit"] = 5;
  state.decrements["legit"] = 1;
  state.increments["spoof"] = 999;
  state.decrements["spoof"] = 0;
  auto bytes = Handler::encode_counter(state);

  Counter target("server");
  Handler handler(target, "legit");
  ASSERT_TRUE(handler.apply(Packet::Type::Counter, bytes));

  EXPECT_EQ(target.state().increments.at("legit"), 5u);
  EXPECT_EQ(target.state().decrements.at("legit"), 1u);
  EXPECT_EQ(target.state().increments.count("spoof"), 0u);
  EXPECT_EQ(target.state().decrements.count("spoof"), 0u);
}
