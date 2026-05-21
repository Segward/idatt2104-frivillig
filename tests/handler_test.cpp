#include <handler.hpp>

#include <gtest/gtest.h>

TEST(handler_counter_test, round_trip_empty_state) {
  counter_pn_state state;

  auto text = Handler::encode_counter(state);
  auto out = Handler::decode_counter(text);

  ASSERT_TRUE(out.has_value());
  EXPECT_TRUE(out->increments.empty());
  EXPECT_TRUE(out->decrements.empty());
}

TEST(handler_counter_test, round_trip_single_entry) {
  counter_pn_state state;
  state.increments["A"] = 7;
  state.decrements["A"] = 3;

  auto text = Handler::encode_counter(state);
  auto out = Handler::decode_counter(text);

  ASSERT_TRUE(out.has_value());
  EXPECT_EQ(out->increments["A"], 7u);
  EXPECT_EQ(out->decrements["A"], 3u);
}

TEST(handler_counter_test, round_trip_multiple_entries) {
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

TEST(handler_counter_test, encode_omits_zero_entries) {
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

TEST(handler_counter_test, decode_rejects_non_json) {
  EXPECT_FALSE(Handler::decode_counter("not json at all").has_value());
}

TEST(handler_counter_test, decode_rejects_non_object) {
  EXPECT_FALSE(Handler::decode_counter("[1,2,3]").has_value());
}

TEST(handler_counter_test, decode_rejects_missing_type) {
  EXPECT_FALSE(Handler::decode_counter(R"({"increments":{},"decrements":{}})").has_value());
}

TEST(handler_counter_test, decode_rejects_wrong_type) {
  EXPECT_FALSE(Handler::decode_counter(
    R"({"type":"auth","increments":{},"decrements":{}})").has_value());
}

TEST(handler_counter_test, decode_rejects_missing_maps) {
  EXPECT_FALSE(Handler::decode_counter(R"({"type":"counter_state","increments":{}})").has_value());
  EXPECT_FALSE(Handler::decode_counter(R"({"type":"counter_state","decrements":{}})").has_value());
}

TEST(handler_counter_test, decode_rejects_non_integer_values) {
  EXPECT_FALSE(Handler::decode_counter(
    R"({"type":"counter_state","increments":{"a":"1"},"decrements":{}})").has_value());
}

TEST(handler_counter_test, decode_rejects_negative_values) {
  EXPECT_FALSE(Handler::decode_counter(
    R"({"type":"counter_state","increments":{"a":-1},"decrements":{}})").has_value());
}

TEST(handler_counter_test, encode_auth_shape) {
  auto text = Handler::encode_auth("client-7");
  EXPECT_NE(text.find(R"("type":"auth")"), std::string::npos);
  EXPECT_NE(text.find(R"("id":"client-7")"), std::string::npos);
}

TEST(handler_list_state_test, round_trip_empty) {
  list_RGA_state state;
  auto text = Handler::encode_list_state(state);
  auto out = Handler::decode_list_state(text);
  ASSERT_TRUE(out.has_value());
  EXPECT_TRUE(out->nodes.empty());
}

TEST(handler_list_state_test, round_trip_nodes) {
  list_RGA_state state;
  state.nodes.push_back({"A:1", "", "Milk", false});
  state.nodes.push_back({"A:2", "A:1", "Bread", true});

  auto text = Handler::encode_list_state(state);
  auto out = Handler::decode_list_state(text);
  ASSERT_TRUE(out.has_value());
  ASSERT_EQ(out->nodes.size(), 2u);
  EXPECT_EQ(out->nodes[0].id, "A:1");
  EXPECT_EQ(out->nodes[0].value, "Milk");
  EXPECT_FALSE(out->nodes[0].deleted);
  EXPECT_EQ(out->nodes[1].id, "A:2");
  EXPECT_EQ(out->nodes[1].previous_id, "A:1");
  EXPECT_TRUE(out->nodes[1].deleted);
}

TEST(handler_list_state_test, decode_rejects_wrong_type) {
  EXPECT_FALSE(Handler::decode_list_state(R"({"type":"text_state","nodes":[]})").has_value());
}

TEST(handler_text_state_test, round_trip_empty) {
  text_RGA_state state;
  auto text = Handler::encode_text_state(state);
  auto out = Handler::decode_text_state(text);
  ASSERT_TRUE(out.has_value());
  EXPECT_TRUE(out->nodes.empty());
}

TEST(handler_text_state_test, round_trip_nodes) {
  text_RGA_state state;
  state.nodes.push_back({"A:t:1", "", "H", false});
  state.nodes.push_back({"A:t:2", "A:t:1", "i", true});

  auto text = Handler::encode_text_state(state);
  auto out = Handler::decode_text_state(text);
  ASSERT_TRUE(out.has_value());
  ASSERT_EQ(out->nodes.size(), 2u);
  EXPECT_EQ(out->nodes[0].id, "A:t:1");
  EXPECT_EQ(out->nodes[0].value, "H");
  EXPECT_FALSE(out->nodes[0].deleted);
  EXPECT_EQ(out->nodes[1].id, "A:t:2");
  EXPECT_EQ(out->nodes[1].previous_id, "A:t:1");
  EXPECT_TRUE(out->nodes[1].deleted);
}

TEST(handler_text_state_test, decode_rejects_wrong_type) {
  EXPECT_FALSE(Handler::decode_text_state(R"({"type":"list_state","nodes":[]})").has_value());
}
