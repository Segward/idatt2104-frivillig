#include <text_rga.hpp>

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
    TextRGA text_A("A");
    TextRGA text_B("B");

    TextChange h = text_A.insert_at_beginning("H");
    TextChange i = text_A.insert_after(h.element_id, "i");

    text_B.apply(h);
    text_B.apply(i);

    EXPECT_EQ(text_A.value(), "Hi");
    EXPECT_EQ(text_B.value(), "Hi");
}

TEST(text_rga_test, replicates_delete_operations) {
    TextRGA text_A("A");
    TextRGA text_B("B");

    TextChange h = text_A.insert_at_beginning("H");
    TextChange i = text_A.insert_after(h.element_id, "i");
    TextChange delete_h = text_A.erase(h.element_id);

    text_B.apply(h);
    text_B.apply(i);
    text_B.apply(delete_h);

    EXPECT_EQ(text_A.value(), "i");
    EXPECT_EQ(text_B.value(), "i");
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
