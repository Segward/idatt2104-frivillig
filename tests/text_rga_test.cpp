#include <text_rga.hpp>

#include <gtest/gtest.h>

TEST(TextRGATest, starts_empty) {
    text_RGA text("A");

    EXPECT_EQ(text.value(), "");
}

TEST(TextRGATest, insert_at_beginning_adds_character) {
    text_RGA text("A");

    text.insert_at_beginning('H');

    EXPECT_EQ(text.value(), "H");
}

TEST(TextRGATest, insert_after_adds_character_after_existing_character) {
    text_RGA text("A");

    text_change h = text.insert_at_beginning('H');
    text.insert_after(h.element_id, 'i');

    EXPECT_EQ(text.value(), "Hi");
}

TEST(TextRGATest, multiple_sequential_inserts_build_text) {
    text_RGA text("A");

    text_change h = text.insert_at_beginning('H');
    text_change e = text.insert_after(h.element_id, 'e');
    text_change l1 = text.insert_after(e.element_id, 'l');
    text_change l2 = text.insert_after(l1.element_id, 'l');
    text.insert_after(l2.element_id, 'o');

    EXPECT_EQ(text.value(), "Hello");
}

TEST(TextRGATest, erase_removes_character_from_visible_text) {
    text_RGA text("A");

    text_change h = text.insert_at_beginning('H');
    text_change i = text.insert_after(h.element_id, 'i');

    text.erase(h.element_id);

    EXPECT_EQ(text.value(), "i");
}

TEST(TextRGATest, replicates_insert_operations) {
    text_RGA text_A("A");
    text_RGA text_B("B");

    text_change h = text_A.insert_at_beginning('H');
    text_change i = text_A.insert_after(h.element_id, 'i');

    text_B.apply(h);
    text_B.apply(i);

    EXPECT_EQ(text_A.value(), "Hi");
    EXPECT_EQ(text_B.value(), "Hi");
}

TEST(TextRGATest, replicates_delete_operations) {
    text_RGA text_A("A");
    text_RGA text_B("B");

    text_change h = text_A.insert_at_beginning('H');
    text_change i = text_A.insert_after(h.element_id, 'i');
    text_change delete_h = text_A.erase(h.element_id);

    text_B.apply(h);
    text_B.apply(i);
    text_B.apply(delete_h);

    EXPECT_EQ(text_A.value(), "i");
    EXPECT_EQ(text_B.value(), "i");
}
TEST(TextRGATest, merge_into_empty_copies_all_nodes) {
    text_RGA source("A");
    text_change h = source.insert_at_beginning('H');
    source.insert_after(h.element_id, 'i');

    text_RGA target("B");
    target.merge(source.state());

    EXPECT_EQ(target.value(), source.value());
}

TEST(TextRGATest, merge_is_idempotent) {
    text_RGA source("A");
    text_change h = source.insert_at_beginning('H');
    source.insert_after(h.element_id, 'i');

    text_RGA target("B");
    target.merge(source.state());
    target.merge(source.state());
    target.merge(source.state());

    EXPECT_EQ(target.value(), source.value());
}

TEST(TextRGATest, merge_tombstone_wins_when_remote_deleted) {
    text_RGA source("A");
    text_change h = source.insert_at_beginning('H');
    source.insert_after(h.element_id, 'i');

    text_RGA target("B");
    target.merge(source.state());

    source.erase(h.element_id);
    target.merge(source.state());

    EXPECT_EQ(target.value(), "i");
}

TEST(TextRGATest, merge_tombstone_wins_when_local_deleted) {
    text_RGA source("A");
    text_change h = source.insert_at_beginning('H');
    source.insert_after(h.element_id, 'i');

    text_RGA target("B");
    target.merge(source.state());

    text_RGA_state pre_delete = source.state();
    text_change delete_h = source.erase(h.element_id);
    target.apply(delete_h);

    target.merge(pre_delete);

    EXPECT_EQ(target.value(), "i");
}

TEST(TextRGATest, merge_commutative_across_replicas) {
    text_RGA a("A");
    text_RGA b("B");

    a.insert_at_beginning('H');
    b.insert_at_beginning('B');

    text_RGA replica_1("X");
    replica_1.merge(a.state());
    replica_1.merge(b.state());

    text_RGA replica_2("Y");
    replica_2.merge(b.state());
    replica_2.merge(a.state());

    EXPECT_EQ(replica_1.value(), replica_2.value());
}
