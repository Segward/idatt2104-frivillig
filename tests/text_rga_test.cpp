#include <crdt/text_rga.hpp>

#include <gtest/gtest.h>

TEST(text_rga_test, starts_empty) {
    TextRGA text("A");

    EXPECT_EQ(text.value(), "");
}

TEST(text_rga_test, insert_at_beginning_adds_character) {
    TextRGA text("A");

    text.insert_at_beginning("H");

    EXPECT_EQ(text.value(), "H");
}

TEST(text_rga_test, insert_after_adds_character_after_existing_character) {
    TextRGA text("A");

    TextChange h = text.insert_at_beginning("H");
    text.insert_after(h.element_id, "i");

    EXPECT_EQ(text.value(), "Hi");
}

TEST(text_rga_test, multiple_sequential_inserts_build_text) {
    TextRGA text("A");

    TextChange h = text.insert_at_beginning("H");
    TextChange e = text.insert_after(h.element_id, "e");
    TextChange l1 = text.insert_after(e.element_id, "l");
    TextChange l2 = text.insert_after(l1.element_id, "l");
    text.insert_after(l2.element_id, "o");

    EXPECT_EQ(text.value(), "Hello");
}

TEST(text_rga_test, erase_removes_character_from_visible_text) {
    TextRGA text("A");

    TextChange h = text.insert_at_beginning("H");
    TextChange i = text.insert_after(h.element_id, "i");

    text.erase(h.element_id);

    EXPECT_EQ(text.value(), "i");
}

TEST(text_rga_test, replicates_insert_operations) {
    TextRGA text_a("A");
    TextRGA text_b("B");

    TextChange h = text_a.insert_at_beginning("H");
    TextChange i = text_a.insert_after(h.element_id, "i");

    text_b.apply(h);
    text_b.apply(i);

    EXPECT_EQ(text_a.value(), "Hi");
    EXPECT_EQ(text_b.value(), "Hi");
}

TEST(text_rga_test, replicates_delete_operations) {
    TextRGA text_a("A");
    TextRGA text_b("B");

    TextChange h = text_a.insert_at_beginning("H");
    TextChange i = text_a.insert_after(h.element_id, "i");
    TextChange delete_h = text_a.erase(h.element_id);

    text_b.apply(h);
    text_b.apply(i);
    text_b.apply(delete_h);

    EXPECT_EQ(text_a.value(), "i");
    EXPECT_EQ(text_b.value(), "i");
}

TEST(text_rga_test, merge_into_empty_copies_all_nodes) {
    TextRGA source("A");
    TextChange h = source.insert_at_beginning("H");
    source.insert_after(h.element_id, "i");

    TextRGA target("B");
    target.merge(source.state());

    EXPECT_EQ(target.value(), source.value());
}

TEST(text_rga_test, merge_is_idempotent) {
    TextRGA source("A");
    TextChange h = source.insert_at_beginning("H");
    source.insert_after(h.element_id, "i");

    TextRGA target("B");
    target.merge(source.state());
    target.merge(source.state());
    target.merge(source.state());

    EXPECT_EQ(target.value(), source.value());
}

TEST(text_rga_test, merge_tombstone_wins_when_remote_deleted) {
    TextRGA source("A");
    TextChange h = source.insert_at_beginning("H");
    source.insert_after(h.element_id, "i");

    TextRGA target("B");
    target.merge(source.state());

    source.erase(h.element_id);
    target.merge(source.state());

    EXPECT_EQ(target.value(), "i");
}

TEST(text_rga_test, merge_tombstone_wins_when_local_deleted) {
    TextRGA source("A");
    TextChange h = source.insert_at_beginning("H");
    source.insert_after(h.element_id, "i");

    TextRGA target("B");
    target.merge(source.state());

    TextRGAState pre_delete = source.state();
    TextChange delete_h = source.erase(h.element_id);
    target.apply(delete_h);

    target.merge(pre_delete);

    EXPECT_EQ(target.value(), "i");
}

TEST(text_rga_test, merge_commutative_across_replicas) {
    TextRGA a("A");
    TextRGA b("B");

    a.insert_at_beginning("H");
    b.insert_at_beginning("B");

    TextRGA replica_1("X");
    replica_1.merge(a.state());
    replica_1.merge(b.state());

    TextRGA replica_2("Y");
    replica_2.merge(b.state());
    replica_2.merge(a.state());

    EXPECT_EQ(replica_1.value(), replica_2.value());
}

TEST(text_rga_test, duplicate_insert_operation_does_not_duplicate_character) {
    TextRGA text_a("A");
    TextRGA text_b("B");

    TextChange h = text_a.insert_at_beginning("H");

    text_b.apply(h);
    text_b.apply(h);
    text_b.apply(h);

    EXPECT_EQ(text_b.value(), "H");
}

TEST(text_rga_test, duplicate_delete_operation_does_not_break_state) {
    TextRGA text_a("A");
    TextRGA text_b("B");

    TextChange h = text_a.insert_at_beginning("H");
    TextChange delete_h = text_a.erase(h.element_id);

    text_b.apply(h);
    text_b.apply(delete_h);
    text_b.apply(delete_h);
    text_b.apply(delete_h);

    EXPECT_EQ(text_b.value(), "");
}

TEST(text_rga_test, concurrent_inserts_after_same_parent_converge) {
    TextRGA text_a("A");
    TextRGA text_b("B");

    TextChange x = text_a.insert_at_beginning("X");
    TextChange y = text_b.insert_at_beginning("Y");

    text_a.apply(y);
    text_b.apply(x);

    EXPECT_EQ(text_a.value(), text_b.value());
}

TEST(text_rga_test, applying_independent_operations_in_different_orders_converges) {
    TextRGA text_a("A");
    TextRGA text_b("B");

    TextChange x = text_a.insert_at_beginning("X");
    TextChange y = text_b.insert_at_beginning("Y");

    TextRGA result_1("C");
    result_1.apply(x);
    result_1.apply(y);

    TextRGA result_2("D");
    result_2.apply(y);
    result_2.apply(x);

    EXPECT_EQ(result_1.value(), result_2.value());
}

TEST(text_rga_test, insert_after_unknown_element_throws_invalid_argument) {
    TextRGA text("A");

    EXPECT_THROW(
        text.insert_after("unknown-id", "x"),
        std::invalid_argument
    );
}

TEST(text_rga_test, erase_unknown_element_throws_invalid_argument) {
    TextRGA text("A");

    EXPECT_THROW(
        text.erase("unknown-id"),
        std::invalid_argument
    );
}

TEST(text_rga_test, apply_buffers_child_insert_until_parent_arrives) {
    TextRGA source("A");
    TextChange h = source.insert_at_beginning("H");
    TextChange i = source.insert_after(h.element_id, "i");

    TextRGA target("B");

    target.apply(i);
    EXPECT_EQ(target.value(), "");

    target.apply(h);

    EXPECT_EQ(target.value(), "Hi");
}

TEST(text_rga_test, apply_buffers_delete_until_target_arrives) {
    TextRGA source("A");
    TextChange h = source.insert_at_beginning("H");
    TextChange delete_h = source.erase(h.element_id);

    TextRGA target("B");

    target.apply(delete_h);
    EXPECT_EQ(target.value(), "");

    target.apply(h);

    EXPECT_EQ(target.value(), "");
}

TEST(text_rga_test, apply_drains_chain_after_root_arrives) {
    TextRGA source("A");
    TextChange h = source.insert_at_beginning("H");
    TextChange e = source.insert_after(h.element_id, "e");
    TextChange y = source.insert_after(e.element_id, "y");

    TextRGA target("B");

    target.apply(y);
    target.apply(e);
    EXPECT_EQ(target.value(), "");

    target.apply(h);

    EXPECT_EQ(target.value(), "Hey");
}
