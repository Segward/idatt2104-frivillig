#include <text_rga.hpp>

#include <gtest/gtest.h>

TEST(TextRGATest, StartsEmpty) {
    text_RGA text("A");

    EXPECT_EQ(text.value(), "");
}

TEST(TextRGATest, InsertAtBeginningAddsCharacter) {
    text_RGA text("A");

    text.insert_at_beginning('H');

    EXPECT_EQ(text.value(), "H");
}

TEST(TextRGATest, InsertAfterAddsCharacterAfterExistingCharacter) {
    text_RGA text("A");

    text_change h = text.insert_at_beginning('H');
    text.insert_after(h.element_id, 'i');

    EXPECT_EQ(text.value(), "Hi");
}

TEST(TextRGATest, MultipleSequentialInsertsBuildText) {
    text_RGA text("A");

    text_change h = text.insert_at_beginning('H');
    text_change e = text.insert_after(h.element_id, 'e');
    text_change l1 = text.insert_after(e.element_id, 'l');
    text_change l2 = text.insert_after(l1.element_id, 'l');
    text.insert_after(l2.element_id, 'o');

    EXPECT_EQ(text.value(), "Hello");
}

TEST(TextRGATest, EraseRemovesCharacterFromVisibleText) {
    text_RGA text("A");

    text_change h = text.insert_at_beginning('H');
    text_change i = text.insert_after(h.element_id, 'i');

    text.erase(h.element_id);

    EXPECT_EQ(text.value(), "i");
}

TEST(TextRGATest, ReplicatesInsertOperations) {
    text_RGA text_A("A");
    text_RGA text_B("B");

    text_change h = text_A.insert_at_beginning('H');
    text_change i = text_A.insert_after(h.element_id, 'i');

    text_B.apply(h);
    text_B.apply(i);

    EXPECT_EQ(text_A.value(), "Hi");
    EXPECT_EQ(text_B.value(), "Hi");
}

TEST(TextRGATest, ReplicatesDeleteOperations) {
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
TEST(TextRGATest, MergeIntoEmptyCopiesAllNodes) {
    text_RGA source("A");
    text_change h = source.insert_at_beginning('H');
    source.insert_after(h.element_id, 'i');

    text_RGA target("B");
    target.merge(source.state());

    EXPECT_EQ(target.value(), source.value());
}

TEST(TextRGATest, MergeIsIdempotent) {
    text_RGA source("A");
    text_change h = source.insert_at_beginning('H');
    source.insert_after(h.element_id, 'i');

    text_RGA target("B");
    target.merge(source.state());
    target.merge(source.state());
    target.merge(source.state());

    EXPECT_EQ(target.value(), source.value());
}

TEST(TextRGATest, MergeTombstoneWinsWhenRemoteDeleted) {
    text_RGA source("A");
    text_change h = source.insert_at_beginning('H');
    source.insert_after(h.element_id, 'i');

    text_RGA target("B");
    target.merge(source.state());

    source.erase(h.element_id);
    target.merge(source.state());

    EXPECT_EQ(target.value(), "i");
}

TEST(TextRGATest, MergeTombstoneWinsWhenLocalDeleted) {
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

TEST(TextRGATest, MergeCommutativeAcrossReplicas) {
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
