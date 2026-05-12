#include <gtest/gtest.h>
#include <string_view>

#include "test.hpp"

TEST(TestHeader, TestDefMatchesHelloWorld) {
  EXPECT_EQ(std::string_view{TEST_DEF}, std::string_view{"Hello, World\n"});
}
