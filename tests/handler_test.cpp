#include <handler.hpp>

#include <gtest/gtest.h>

TEST(handler_test, round_trip_empty_state) {
  counter_pn_state state;

  auto text = Handler::encode_counter(state);
  auto out = Handler::decode_counter(text);

  ASSERT_TRUE(out.has_value());
  EXPECT_TRUE(out->increments.empty());
  EXPECT_TRUE(out->decrements.empty());
}

TEST(handler_test, round_trip_single_entry) {
  counter_pn_state state;
  state.increments["A"] = 7;
  state.decrements["A"] = 3;

  auto text = Handler::encode_counter(state);
  auto out = Handler::decode_counter(text);

  ASSERT_TRUE(out.has_value());
  EXPECT_EQ(out->increments["A"], 7u);
  EXPECT_EQ(out->decrements["A"], 3u);
}

TEST(handler_test, round_trip_multiple_entries) {
  counter_pn_state state;
  state.increments["x"] = 1;
  state.increments["client-123"] = 42;
  state.decrements["client-123"] = 5;
  std::string long_id(64, 'q');
  state.increments[long_id] = 1000;
  state.decrements[long_id] = 999;

  auto text = Handler::encode_counter(state);
  auto out = Handler::decode_counter(text);

  ASSERT_TRUE(out.has_value());
  EXPECT_EQ(out->increments["x"], 1u);
  EXPECT_EQ(out->increments["client-123"], 42u);
  EXPECT_EQ(out->decrements["client-123"], 5u);
  EXPECT_EQ(out->increments[long_id], 1000u);
  EXPECT_EQ(out->decrements[long_id], 999u);
}

TEST(handler_test, encode_omits_zero_entries) {
  counter_pn_state state;
  state.increments["only-inc"] = 9;
  state.decrements["only-inc"] = 0;
  state.decrements["only-dec"] = 4;

  auto text = Handler::encode_counter(state);
  auto out = Handler::decode_counter(text);

  ASSERT_TRUE(out.has_value());
  EXPECT_EQ(out->increments.size(), 1u);
  EXPECT_EQ(out->increments["only-inc"], 9u);
  EXPECT_EQ(out->decrements.size(), 1u);
  EXPECT_EQ(out->decrements["only-dec"], 4u);
}

TEST(handler_test, decode_rejects_non_json) {
  EXPECT_FALSE(Handler::decode_counter("not json at all").has_value());
}

TEST(handler_test, decode_rejects_non_object) {
  EXPECT_FALSE(Handler::decode_counter("[1,2,3]").has_value());
}

TEST(handler_test, decode_rejects_missing_type) {
  EXPECT_FALSE(Handler::decode_counter(R"({"increments":{},"decrements":{}})").has_value());
}

TEST(handler_test, decode_rejects_wrong_type) {
  EXPECT_FALSE(Handler::decode_counter(
    R"({"type":"auth","increments":{},"decrements":{}})").has_value());
}

TEST(handler_test, decode_rejects_missing_maps) {
  EXPECT_FALSE(Handler::decode_counter(R"({"type":"counter","increments":{}})").has_value());
  EXPECT_FALSE(Handler::decode_counter(R"({"type":"counter","decrements":{}})").has_value());
}

TEST(handler_test, decode_rejects_non_integer_values) {
  EXPECT_FALSE(Handler::decode_counter(
    R"({"type":"counter","increments":{"a":"1"},"decrements":{}})").has_value());
}

TEST(handler_test, decode_rejects_negative_values) {
  EXPECT_FALSE(Handler::decode_counter(
    R"({"type":"counter","increments":{"a":-1},"decrements":{}})").has_value());
}

TEST(handler_test, apply_with_filter_drops_other_ids) {
  counter_pn_state state;
  state.increments["legit"] = 5;
  state.decrements["legit"] = 1;
  state.increments["spoof"] = 999;
  auto text = Handler::encode_counter(state);

  counter_pn target("server");
  Handler handler(target, "legit");
  ASSERT_TRUE(handler.apply(text));

  EXPECT_EQ(target.state().increments.at("legit"), 5u);
  EXPECT_EQ(target.state().decrements.at("legit"), 1u);
  EXPECT_EQ(target.state().increments.count("spoof"), 0u);
  EXPECT_EQ(target.state().decrements.count("spoof"), 0u);
}

TEST(handler_test, encode_auth_shape) {
  auto text = Handler::encode_auth("client-7");
  // Quick structural check without depending on nlohmann here.
  EXPECT_NE(text.find(R"("type":"auth")"), std::string::npos);
  EXPECT_NE(text.find(R"("id":"client-7")"), std::string::npos);
}
