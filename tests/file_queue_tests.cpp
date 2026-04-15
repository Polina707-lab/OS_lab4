#include <gtest/gtest.h>

#include <array>
#include <cstdio>
#include <cstring>
#include <string>

#include "file_queue.h"
#include "queue_logic.h"
#include "queue_types.h"

namespace {

    std::string make_test_filename(const std::string& test_name) {
        return "test_queue_" + test_name + ".bin";
    }

    void remove_file_if_exists(const std::string& filename) {
        std::remove(filename.c_str());
    }

    class FileCleanupGuard {
    public:
        explicit FileCleanupGuard(const std::string& filename)
            : filename_(filename) {
        }

        ~FileCleanupGuard() {
            remove_file_if_exists(filename_);
        }

    private:
        std::string filename_;
    };

    MessageRecord make_record_from_text(const std::string& text) {
        MessageRecord record{};
        std::string normalized = normalize_message(text);

        std::memcpy(record.data, normalized.c_str(), normalized.size());
        record.data[normalized.size()] = '\0';

        return record;
    }

    std::string record_to_string(const MessageRecord& record) {
        return std::string(record.data);
    }

} // namespace

TEST(FileQueueTest, CreateQueueFileInitializesHeaderCorrectly) {
    const std::string filename = make_test_filename("header_init");
    FileCleanupGuard cleanup(filename);

    create_queue_file(filename, 5);

    QueueHeader header = read_header(filename);

    EXPECT_EQ(header.capacity, 5u);
    EXPECT_EQ(header.read_pos, 0u);
    EXPECT_EQ(header.write_pos, 0u);
    EXPECT_EQ(header.count, 0u);
}

TEST(FileQueueTest, CreateQueueFileThrowsForZeroCapacity) {
    const std::string filename = make_test_filename("zero_capacity");
    FileCleanupGuard cleanup(filename);

    EXPECT_THROW(create_queue_file(filename, 0), std::invalid_argument);
}

TEST(FileQueueTest, ReadHeaderThrowsForMissingFile) {
    const std::string filename = make_test_filename("missing_header");
    remove_file_if_exists(filename);

    EXPECT_THROW(read_header(filename), std::runtime_error);
}

TEST(FileQueueTest, WriteHeaderAndReadHeaderPreserveAllFields) {
    const std::string filename = make_test_filename("write_read_header");
    FileCleanupGuard cleanup(filename);

    create_queue_file(filename, 5);

    QueueHeader header{};
    header.capacity = 5;
    header.read_pos = 2;
    header.write_pos = 4;
    header.count = 3;

    write_header(filename, header);

    QueueHeader loaded = read_header(filename);

    EXPECT_EQ(loaded.capacity, header.capacity);
    EXPECT_EQ(loaded.read_pos, header.read_pos);
    EXPECT_EQ(loaded.write_pos, header.write_pos);
    EXPECT_EQ(loaded.count, header.count);
}

TEST(FileQueueTest, WriteRecordAndReadRecordPreserveMessageContent) {
    const std::string filename = make_test_filename("single_record");
    FileCleanupGuard cleanup(filename);

    create_queue_file(filename, 5);

    MessageRecord written = make_record_from_text("hello world");
    write_record(filename, 0, written);

    MessageRecord loaded = read_record(filename, 0);

    EXPECT_EQ(record_to_string(loaded), "hello world");
}

TEST(FileQueueTest, ReadAndWriteRecordThrowForIndexOutOfRange) {
    const std::string filename = make_test_filename("index_out_of_range");
    FileCleanupGuard cleanup(filename);

    create_queue_file(filename, 2);

    MessageRecord record = make_record_from_text("test");

    EXPECT_THROW(write_record(filename, 2, record), std::out_of_range);
    EXPECT_THROW(read_record(filename, 2), std::out_of_range);
}

TEST(FileQueueTest, RingBehaviorPreservesFifoOrder) {
    const std::string filename = make_test_filename("ring_fifo");
    FileCleanupGuard cleanup(filename);

    create_queue_file(filename, 3);

    QueueHeader header = read_header(filename);

    write_record(filename, header.write_pos, make_record_from_text("A"));
    on_enqueue(header);

    write_record(filename, header.write_pos, make_record_from_text("B"));
    on_enqueue(header);

    write_record(filename, header.write_pos, make_record_from_text("C"));
    on_enqueue(header);

    write_header(filename, header);

    header = read_header(filename);
    EXPECT_TRUE(is_queue_full(header));

    EXPECT_EQ(record_to_string(read_record(filename, header.read_pos)), "A");
    on_dequeue(header);

    EXPECT_EQ(record_to_string(read_record(filename, header.read_pos)), "B");
    on_dequeue(header);

    write_record(filename, header.write_pos, make_record_from_text("D"));
    on_enqueue(header);
    write_header(filename, header);

    header = read_header(filename);

    EXPECT_EQ(record_to_string(read_record(filename, header.read_pos)), "C");
    on_dequeue(header);

    EXPECT_EQ(record_to_string(read_record(filename, header.read_pos)), "D");
    on_dequeue(header);

    EXPECT_TRUE(is_queue_empty(header));
}