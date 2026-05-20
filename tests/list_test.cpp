#include <list_rga.hpp>

#include <gtest/gtest.h>
#include <string>
#include <vector>

TEST(list_test, starts_empty) {
    list_RGA list("A");

    EXPECT_TRUE(list.value().empty());
}

TEST(list_test, insert_at_beginning_adds_item) {
    list_RGA list("A");

    list.insert_at_beginning("Milk");

    std::vector<std::string> expected = {"Milk"};

    EXPECT_EQ(list.value(), expected);
}

TEST(list_test, insert_after_adds_item_after_existing_item) {
    list_RGA list("A");

    list_change milk = list.insert_at_beginning("Milk");
    list.insert_after(milk.element_id, "Bread");

    std::vector<std::string> expected = {"Milk", "Bread"};

    EXPECT_EQ(list.value(), expected);
}

TEST(list_test, multiple_sequential_inserts_build_list) {
    list_RGA list("A");

    list_change milk = list.insert_at_beginning("Milk");
    list_change bread = list.insert_after(milk.element_id, "Bread");
    list_change eggs = list.insert_after(bread.element_id, "Eggs");
    list.insert_after(eggs.element_id, "Butter");

    std::vector<std::string> expected = {
        "Milk",
        "Bread",
        "Eggs",
        "Butter"
    };

    EXPECT_EQ(list.value(), expected);
}

TEST(list_test, erase_removes_item_from_visible_list) {
    list_RGA list("A");

    list_change milk = list.insert_at_beginning("Milk");
    list.insert_after(milk.element_id, "Bread");

    list.erase(milk.element_id);

    std::vector<std::string> expected = {"Bread"};

    EXPECT_EQ(list.value(), expected);
}

TEST(list_test, replicates_insert_operations) {
    list_RGA list_a("A");
    list_RGA list_b("B");

    list_change milk = list_a.insert_at_beginning("Milk");
    list_change bread = list_a.insert_after(milk.element_id, "Bread");

    list_b.apply(milk);
    list_b.apply(bread);

    std::vector<std::string> expected = {"Milk", "Bread"};

    EXPECT_EQ(list_a.value(), expected);
    EXPECT_EQ(list_b.value(), expected);
}

TEST(list_test, replicates_delete_operations) {
    list_RGA list_a("A");
    list_RGA list_b("B");

    list_change milk = list_a.insert_at_beginning("Milk");
    list_change bread = list_a.insert_after(milk.element_id, "Bread");
    list_change delete_milk = list_a.erase(milk.element_id);

    list_b.apply(milk);
    list_b.apply(bread);
    list_b.apply(delete_milk);

    std::vector<std::string> expected = {"Bread"};

    EXPECT_EQ(list_a.value(), expected);
    EXPECT_EQ(list_b.value(), expected);
}

TEST(list_test, duplicate_insert_operation_does_not_duplicate_item) {
    list_RGA list_a("A");
    list_RGA list_b("B");

    list_change milk = list_a.insert_at_beginning("Milk");

    list_b.apply(milk);
    list_b.apply(milk);
    list_b.apply(milk);

    std::vector<std::string> expected = {"Milk"};

    EXPECT_EQ(list_b.value(), expected);
}

TEST(list_test, duplicate_delete_operation_does_not_break_state) {
    list_RGA list_a("A");
    list_RGA list_b("B");

    list_change milk = list_a.insert_at_beginning("Milk");
    list_change delete_milk = list_a.erase(milk.element_id);

    list_b.apply(milk);
    list_b.apply(delete_milk);
    list_b.apply(delete_milk);
    list_b.apply(delete_milk);

    EXPECT_TRUE(list_b.value().empty());
}

TEST(list_test, concurrent_inserts_after_same_parent_converge) {
    list_RGA list_a("A");
    list_RGA list_b("B");

    list_change milk = list_a.insert_at_beginning("Milk");
    list_change bread = list_b.insert_at_beginning("Bread");

    list_a.apply(bread);
    list_b.apply(milk);

    EXPECT_EQ(list_a.value(), list_b.value());
}

TEST(list_test, applying_independent_operations_in_different_orders_converges) {
    list_RGA list_a("A");
    list_RGA list_b("B");

    list_change milk = list_a.insert_at_beginning("Milk");
    list_change bread = list_b.insert_at_beginning("Bread");

    list_RGA result_1("C");
    result_1.apply(milk);
    result_1.apply(bread);

    list_RGA result_2("D");
    result_2.apply(bread);
    result_2.apply(milk);

    EXPECT_EQ(result_1.value(), result_2.value());
}

TEST(list_test, to_string_formats_list_items) {
    list_RGA list("A");

    list_change milk = list.insert_at_beginning("Milk");
    list.insert_after(milk.element_id, "Bread");

    std::string expected =
        "- Milk\n"
        "- Bread\n";

    EXPECT_EQ(list.to_string(), expected);
}

TEST(list_test, empty_item_throws_invalid_argument) {
    list_RGA list("A");

    EXPECT_THROW(list.insert_at_beginning(""), std::invalid_argument);
}

TEST(list_test, insert_after_unknown_element_throws_invalid_argument) {
    list_RGA list("A");

    EXPECT_THROW(
        list.insert_after("unknown-id", "Milk"),
        std::invalid_argument
    );
}

TEST(list_test, erase_unknown_element_throws_invalid_argument) {
    list_RGA list("A");

    EXPECT_THROW(
        list.erase("unknown-id"),
        std::invalid_argument
    );
}