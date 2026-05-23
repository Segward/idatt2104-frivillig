#include <list_rga.hpp>

#include <gtest/gtest.h>
#include <string>
#include <vector>

TEST(list_test, starts_empty) {
    ListRGA list("A");

    EXPECT_TRUE(list.value().empty());
}

TEST(list_test, insert_at_beginning_adds_item) {
    ListRGA list("A");

    list.insert_at_beginning("Milk");

    std::vector<std::string> expected = {"Milk"};

    EXPECT_EQ(list.value(), expected);
}

TEST(list_test, insert_after_adds_item_after_existing_item) {
    ListRGA list("A");

    ListChange milk = list.insert_at_beginning("Milk");
    list.insert_after(milk.element_id, "Bread");

    std::vector<std::string> expected = {"Milk", "Bread"};

    EXPECT_EQ(list.value(), expected);
}

TEST(list_test, multiple_sequential_inserts_build_list) {
    ListRGA list("A");

    ListChange milk = list.insert_at_beginning("Milk");
    ListChange bread = list.insert_after(milk.element_id, "Bread");
    ListChange eggs = list.insert_after(bread.element_id, "Eggs");
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
    ListRGA list("A");

    ListChange milk = list.insert_at_beginning("Milk");
    list.insert_after(milk.element_id, "Bread");

    list.erase(milk.element_id);

    std::vector<std::string> expected = {"Bread"};

    EXPECT_EQ(list.value(), expected);
}

TEST(list_test, replicates_insert_operations) {
    ListRGA list_a("A");
    ListRGA list_b("B");

    ListChange milk = list_a.insert_at_beginning("Milk");
    ListChange bread = list_a.insert_after(milk.element_id, "Bread");

    list_b.apply(milk);
    list_b.apply(bread);

    std::vector<std::string> expected = {"Milk", "Bread"};

    EXPECT_EQ(list_a.value(), expected);
    EXPECT_EQ(list_b.value(), expected);
}

TEST(list_test, replicates_delete_operations) {
    ListRGA list_a("A");
    ListRGA list_b("B");

    ListChange milk = list_a.insert_at_beginning("Milk");
    ListChange bread = list_a.insert_after(milk.element_id, "Bread");
    ListChange delete_milk = list_a.erase(milk.element_id);

    list_b.apply(milk);
    list_b.apply(bread);
    list_b.apply(delete_milk);

    std::vector<std::string> expected = {"Bread"};

    EXPECT_EQ(list_a.value(), expected);
    EXPECT_EQ(list_b.value(), expected);
}

TEST(list_test, duplicate_insert_operation_does_not_duplicate_item) {
    ListRGA list_a("A");
    ListRGA list_b("B");

    ListChange milk = list_a.insert_at_beginning("Milk");

    list_b.apply(milk);
    list_b.apply(milk);
    list_b.apply(milk);

    std::vector<std::string> expected = {"Milk"};

    EXPECT_EQ(list_b.value(), expected);
}

TEST(list_test, duplicate_delete_operation_does_not_break_state) {
    ListRGA list_a("A");
    ListRGA list_b("B");

    ListChange milk = list_a.insert_at_beginning("Milk");
    ListChange delete_milk = list_a.erase(milk.element_id);

    list_b.apply(milk);
    list_b.apply(delete_milk);
    list_b.apply(delete_milk);
    list_b.apply(delete_milk);

    EXPECT_TRUE(list_b.value().empty());
}

TEST(list_test, concurrent_inserts_after_same_parent_converge) {
    ListRGA list_a("A");
    ListRGA list_b("B");

    ListChange milk = list_a.insert_at_beginning("Milk");
    ListChange bread = list_b.insert_at_beginning("Bread");

    list_a.apply(bread);
    list_b.apply(milk);

    EXPECT_EQ(list_a.value(), list_b.value());
}

TEST(list_test, applying_independent_operations_in_different_orders_converges) {
    ListRGA list_a("A");
    ListRGA list_b("B");

    ListChange milk = list_a.insert_at_beginning("Milk");
    ListChange bread = list_b.insert_at_beginning("Bread");

    ListRGA result_1("C");
    result_1.apply(milk);
    result_1.apply(bread);

    ListRGA result_2("D");
    result_2.apply(bread);
    result_2.apply(milk);

    EXPECT_EQ(result_1.value(), result_2.value());
}

TEST(list_test, to_string_formats_list_items) {
    ListRGA list("A");

    ListChange milk = list.insert_at_beginning("Milk");
    list.insert_after(milk.element_id, "Bread");

    std::string expected =
        "- Milk\n"
        "- Bread\n";

    EXPECT_EQ(list.to_string(), expected);
}

TEST(list_test, empty_item_throws_invalid_argument) {
    ListRGA list("A");

    EXPECT_THROW(list.insert_at_beginning(""), std::invalid_argument);
}

TEST(list_test, insert_after_unknown_element_throws_invalid_argument) {
    ListRGA list("A");

    EXPECT_THROW(
        list.insert_after("unknown-id", "Milk"),
        std::invalid_argument
    );
}

TEST(list_test, erase_unknown_element_throws_invalid_argument) {
    ListRGA list("A");

    EXPECT_THROW(
        list.erase("unknown-id"),
        std::invalid_argument
    );
}

TEST(list_test, merge_into_empty_copies_all_nodes) {
    ListRGA source("A");
    ListChange milk = source.insert_at_beginning("Milk");
    source.insert_after(milk.element_id, "Bread");

    ListRGA target("B");
    target.merge(source.state());

    EXPECT_EQ(target.value(), source.value());
}

TEST(list_test, merge_is_idempotent) {
    ListRGA source("A");
    source.insert_at_beginning("Milk");

    ListRGA target("B");
    target.merge(source.state());
    target.merge(source.state());
    target.merge(source.state());

    EXPECT_EQ(target.value(), source.value());
}

TEST(list_test, merge_tombstone_wins_when_remote_deleted) {
    ListRGA source("A");
    ListChange milk = source.insert_at_beginning("Milk");
    source.insert_after(milk.element_id, "Bread");

    ListRGA target("B");
    target.merge(source.state());

    // Delete on source side, then merge back into target.
    source.erase(milk.element_id);
    target.merge(source.state());

    std::vector<std::string> expected = {"Bread"};
    EXPECT_EQ(target.value(), expected);
}

TEST(list_test, merge_tombstone_wins_when_local_deleted) {
    ListRGA source("A");
    ListChange milk = source.insert_at_beginning("Milk");
    source.insert_after(milk.element_id, "Bread");

    ListRGA target("B");
    target.merge(source.state());

    // Capture pre-delete remote state; delete locally; merge older state in.
    ListRGAState pre_delete = source.state();
    ListChange delete_milk = source.erase(milk.element_id);
    target.apply(delete_milk);

    target.merge(pre_delete);

    std::vector<std::string> expected = {"Bread"};
    EXPECT_EQ(target.value(), expected);
}

TEST(list_test, merge_commutative_across_replicas) {
    ListRGA a("A");
    ListRGA b("B");

    a.insert_at_beginning("Milk");
    b.insert_at_beginning("Bread");

    ListRGA replica_1("X");
    replica_1.merge(a.state());
    replica_1.merge(b.state());

    ListRGA replica_2("Y");
    replica_2.merge(b.state());
    replica_2.merge(a.state());

    EXPECT_EQ(replica_1.value(), replica_2.value());
}