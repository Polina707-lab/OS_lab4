#include <gtest/gtest.h>

#include <stdexcept>
#include <string>

#include "queue_logic.h"
#include "queue_types.h"

TEST(QueueLogicTest, EmptyAndFullStateAreDetectedCorrectly) {
    QueueHeader header{};
    header.capacity = 3;
    header.read_pos = 0;
    header.write_pos = 0;
    header.count = 0;

    EXPECT_TRUE(is_queue_empty(header));
    EXPECT_FALSE(is_queue_full(header));

    header.count = 3;

    EXPECT_FALSE(is_queue_empty(header));
    EXPECT_TRUE(is_queue_full(header));
}

TEST(QueueLogicTest, NextIndexAdvancesAndWrapsAround) {
    EXPECT_EQ(next_index(0, 5), 1u);
    EXPECT_EQ(next_index(3, 5), 4u);
    EXPECT_EQ(next_index(4, 5), 0u);
}

TEST(QueueLogicTest, NextIndexThrowsForZeroCapacity) {
    EXPECT_THROW(next_index(0, 0), std::invalid_argument);
}

TEST(QueueLogicTest, OnEnqueueUpdatesStateAndThrowsForFullQueue) {
    QueueHeader header{};
    header.capacity = 3;
    header.read_pos = 0;
    header.write_pos = 0;
    header.count = 0;

    on_enqueue(header);
    EXPECT_EQ(header.write_pos, 1u);
    EXPECT_EQ(header.count, 1u);

    on_enqueue(header);
    EXPECT_EQ(header.write_pos, 2u);
    EXPECT_EQ(header.count, 2u);

    on_enqueue(header);
    EXPECT_EQ(header.write_pos, 0u);
    EXPECT_EQ(header.count, 3u);
    EXPECT_TRUE(is_queue_full(header));

    EXPECT_THROW(on_enqueue(header), std::logic_error);
}

TEST(QueueLogicTest, OnDequeueUpdatesStateAndThrowsForEmptyQueue) {
    QueueHeader header{};
    header.capacity = 3;
    header.read_pos = 0;
    header.write_pos = 0;
    header.count = 3;

    on_dequeue(header);
    EXPECT_EQ(header.read_pos, 1u);
    EXPECT_EQ(header.count, 2u);

    on_dequeue(header);
    EXPECT_EQ(header.read_pos, 2u);
    EXPECT_EQ(header.count, 1u);

    on_dequeue(header);
    EXPECT_EQ(header.read_pos, 0u);
    EXPECT_EQ(header.count, 0u);
    EXPECT_TRUE(is_queue_empty(header));

    EXPECT_THROW(on_dequeue(header), std::logic_error);
}

TEST(QueueLogicTest, RecordOffsetAndQueueConfigAreCorrect) {
    EXPECT_EQ(record_offset(0), sizeof(QueueHeader));
    EXPECT_EQ(record_offset(1), sizeof(QueueHeader) + sizeof(MessageRecord));
    EXPECT_EQ(record_offset(3), sizeof(QueueHeader) + 3 * sizeof(MessageRecord));

    EXPECT_THROW(validate_queue_config(0), std::invalid_argument);
    EXPECT_NO_THROW(validate_queue_config(1));
    EXPECT_NO_THROW(validate_queue_config(5));
}

TEST(QueueLogicTest, NormalizeMessageKeepsShortAndTruncatesLongMessages) {
    EXPECT_EQ(normalize_message("hello"), "hello");

    const std::string exact_message(MAX_MESSAGE_LENGTH, 'a');
    EXPECT_EQ(normalize_message(exact_message), exact_message);

    const std::string long_message(MAX_MESSAGE_LENGTH + 5, 'x');
    const std::string normalized = normalize_message(long_message);

    EXPECT_EQ(normalized.size(), MAX_MESSAGE_LENGTH);
    EXPECT_EQ(normalized, std::string(MAX_MESSAGE_LENGTH, 'x'));
}