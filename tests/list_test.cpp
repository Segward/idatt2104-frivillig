#include <list_RGA.hpp>

#include <gtest/gtest.h>
#include <vector>
#include <string>

TEST(ListRGATest, StartsEmpty) {
    list_RGA list("A");

    EXPECT_TRUE(list.value().empty());
}

TEST(ListRGATest, InsertAtBeginningAddsItem) {
    list_RGA list("A");

    list.insert_at_beginning("Milk");

    std::vector<std::string> expected = {"Milk"};

    EXPECT_EQ(list.value(), expected);
}

TEST(ListRGATest, InsertAfterAddsItemAfterExistingItem) {
    list_RGA list("A");

    list_change milk = list.insert_at_beginning("Milk");
    list.insert_after(milk.element_id, "Bread");

    std::vector<std::string> expected = {"Milk", "Bread"};

    EXPECT_EQ(list.value(), expected);
}

TEST(ListRGATest, MultipleSequentialInsertsBuildList) {
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

TEST(ListRGATest, EraseRemovesItemFromVisibleList) {
    list_RGA list("A");

    list_change milk = list.insert_at_beginning("Milk");
    list.insert_after(milk.element_id, "Bread");

    list.erase(milk.element_id);

    std::vector<std::string> expected = {"Bread"};

    EXPECT_EQ(list.value(), expected);
}

TEST(ListRGATest, ReplicatesInsertOperations) {
    list_RGA list_A("A");
    list_RGA list_B("B");

    list_change milk = list_A.insert_at_beginning("Milk");
    list_change bread = list_A.insert_after(milk.element_id, "Bread");

    list_B.apply(milk);
    list_B.apply(bread);

    std::vector<std::string> expected = {"Milk", "Bread"};

    EXPECT_EQ(list_A.value(), expected);
    EXPECT_EQ(list_B.value(), expected);
}

TEST(ListRGATest, ReplicatesDeleteOperations) {
    list_RGA list_A("A");
    list_RGA list_B("B");

    list_change milk = list_A.insert_at_beginning("Milk");
    list_change bread = list_A.insert_after(milk.element_id, "Bread");
    list_change delete_milk = list_A.erase(milk.element_id);

    list_B.apply(milk);
    list_B.apply(bread);
    list_B.apply(delete_milk);

    std::vector<std::string> expected = {"Bread"};

    EXPECT_EQ(list_A.value(), expected);
    EXPECT_EQ(list_B.value(), expected);
}

TEST(ListRGATest, DuplicateInsertOperationDoesNotDuplicateItem) {
    list_RGA list_A("A");
    list_RGA list_B("B");

    list_change milk = list_A.insert_at_beginning("Milk");

    list_B.apply(milk);
    list_B.apply(milk);
    list_B.apply(milk);

    std::vector<std::string> expected = {"Milk"};

    EXPECT_EQ(list_B.value(), expected);
}

TEST(ListRGATest, DuplicateDeleteOperationDoesNotBreakState) {
    list_RGA list_A("A");
    list_RGA list_B("B");

    list_change milk = list_A.insert_at_beginning("Milk");
    list_change delete_milk = list_A.erase(milk.element_id);

    list_B.apply(milk);
    list_B.apply(delete_milk);
    list_B.apply(delete_milk);
    list_B.apply(delete_milk);

    EXPECT_TRUE(list_B.value().empty());
}

TEST(ListRGATest, ConcurrentInsertsAfterSameParentConverge) {
    list_RGA list_A("A");
    list_RGA list_B("B");

    list_change milk = list_A.insert_at_beginning("Milk");
    list_change bread = list_B.insert_at_beginning("Bread");

    list_A.apply(bread);
    list_B.apply(milk);

    EXPECT_EQ(list_A.value(), list_B.value());
}

TEST(ListRGATest, ApplyingIndependentOperationsInDifferentOrdersConverges) {
    list_RGA list_A("A");
    list_RGA list_B("B");

    list_change milk = list_A.insert_at_beginning("Milk");
    list_change bread = list_B.insert_at_beginning("Bread");

    list_RGA result_1("C");
    result_1.apply(milk);
    result_1.apply(bread);

    list_RGA result_2("D");
    result_2.apply(bread);
    result_2.apply(milk);

    EXPECT_EQ(result_1.value(), result_2.value());
}

TEST(ListRGATest, ToStringFormatsListItems) {
    list_RGA list("A");

    list_change milk = list.insert_at_beginning("Milk");
    list.insert_after(milk.element_id, "Bread");

    std::string expected =
        "- Milk\n"
        "- Bread\n";

    EXPECT_EQ(list.to_string(), expected);
}

TEST(ListRGATest, EmptyItemThrowsInvalidArgument) {
    list_RGA list("A");

    EXPECT_THROW(list.insert_at_beginning(""), std::invalid_argument);
}

TEST(ListRGATest, InsertAfterUnknownElementThrowsInvalidArgument) {
    list_RGA list("A");

    EXPECT_THROW(
        list.insert_after("unknown-id", "Milk"),
        std::invalid_argument
    );
}

TEST(ListRGATest, EraseUnknownElementThrowsInvalidArgument) {
    list_RGA list("A");

    EXPECT_THROW(
        list.erase("unknown-id"),
        std::invalid_argument
    );
}