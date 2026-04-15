#include <gtest/gtest.h>

#include <cstdio>
#include <string>

#include "file_queue.h"
#include "queue_logic.h"
#include "receiver.h"
#include "sender.h"
#include "sync_objects.h"

namespace {

    std::string make_test_filename(const std::string& test_name) {
        return "test_ipc_" + test_name + ".bin";
    }

    std::string make_test_queue_name(const std::string& test_name) {
        return "test_ipc_queue_" + test_name;
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

    class SyncCleanupGuard {
    public:
        explicit SyncCleanupGuard(SyncHandles& handles)
            : handles_(handles) {
        }

        ~SyncCleanupGuard() {
            close_sync_handles(handles_);
        }

    private:
        SyncHandles& handles_;
    };

} // namespace

TEST(IpcTest, SendOneMessageWritesMessageToQueue) {
    const std::string filename = make_test_filename("send_one_message");
    const std::string queue_name = make_test_queue_name("send_one_message");

    FileCleanupGuard file_cleanup(filename);

    create_queue_file(filename, 3);

    SyncNames names = make_sync_names(queue_name);
    SyncHandles handles = create_sync_objects(names, 3);
    SyncCleanupGuard sync_cleanup(handles);

    send_one_message(filename, handles, "hello");

    QueueHeader header = read_header(filename);
    EXPECT_EQ(header.count, 1u);
    EXPECT_EQ(header.write_pos, 1u);
    EXPECT_EQ(header.read_pos, 0u);

    MessageRecord record = read_record(filename, 0);
    EXPECT_EQ(std::string(record.data), "hello");
}

TEST(IpcTest, SendAndReceiveOneMessageReturnsSameText) {
    const std::string filename = make_test_filename("send_receive_one");
    const std::string queue_name = make_test_queue_name("send_receive_one");

    FileCleanupGuard file_cleanup(filename);

    create_queue_file(filename, 3);

    SyncNames names = make_sync_names(queue_name);
    SyncHandles handles = create_sync_objects(names, 3);
    SyncCleanupGuard sync_cleanup(handles);

    send_one_message(filename, handles, "message");
    std::string received = receive_one_message(filename, handles);

    EXPECT_EQ(received, "message");

    QueueHeader header = read_header(filename);
    EXPECT_EQ(header.count, 0u);
    EXPECT_EQ(header.read_pos, 1u);
    EXPECT_EQ(header.write_pos, 1u);
}

TEST(IpcTest, MultipleMessagesAreReceivedInFifoOrder) {
    const std::string filename = make_test_filename("fifo_order");
    const std::string queue_name = make_test_queue_name("fifo_order");

    FileCleanupGuard file_cleanup(filename);

    create_queue_file(filename, 5);

    SyncNames names = make_sync_names(queue_name);
    SyncHandles handles = create_sync_objects(names, 5);
    SyncCleanupGuard sync_cleanup(handles);

    send_one_message(filename, handles, "one");
    send_one_message(filename, handles, "two");
    send_one_message(filename, handles, "three");

    EXPECT_EQ(receive_one_message(filename, handles), "one");
    EXPECT_EQ(receive_one_message(filename, handles), "two");
    EXPECT_EQ(receive_one_message(filename, handles), "three");

    QueueHeader header = read_header(filename);
    EXPECT_EQ(header.count, 0u);
    EXPECT_TRUE(is_queue_empty(header));
}

TEST(IpcTest, LongMessageIsTruncatedDuringRealSend) {
    const std::string filename = make_test_filename("long_message");
    const std::string queue_name = make_test_queue_name("long_message");

    FileCleanupGuard file_cleanup(filename);

    create_queue_file(filename, 3);

    SyncNames names = make_sync_names(queue_name);
    SyncHandles handles = create_sync_objects(names, 3);
    SyncCleanupGuard sync_cleanup(handles);

    std::string long_message(MAX_MESSAGE_LENGTH + 10, 'x');

    send_one_message(filename, handles, long_message);
    std::string received = receive_one_message(filename, handles);

    EXPECT_EQ(received.size(), MAX_MESSAGE_LENGTH);
    EXPECT_EQ(received, std::string(MAX_MESSAGE_LENGTH, 'x'));
}

TEST(IpcTest, QueueHeaderChangesCorrectlyAfterSeveralOperations) {
    const std::string filename = make_test_filename("header_changes");
    const std::string queue_name = make_test_queue_name("header_changes");

    FileCleanupGuard file_cleanup(filename);

    create_queue_file(filename, 3);

    SyncNames names = make_sync_names(queue_name);
    SyncHandles handles = create_sync_objects(names, 3);
    SyncCleanupGuard sync_cleanup(handles);

    send_one_message(filename, handles, "A");
    send_one_message(filename, handles, "B");

    QueueHeader header = read_header(filename);
    EXPECT_EQ(header.count, 2u);
    EXPECT_EQ(header.read_pos, 0u);
    EXPECT_EQ(header.write_pos, 2u);

    EXPECT_EQ(receive_one_message(filename, handles), "A");

    header = read_header(filename);
    EXPECT_EQ(header.count, 1u);
    EXPECT_EQ(header.read_pos, 1u);
    EXPECT_EQ(header.write_pos, 2u);

    send_one_message(filename, handles, "C");

    header = read_header(filename);
    EXPECT_EQ(header.count, 2u);
}

TEST(IpcTest, RingBehaviorWorksWithRealSendAndReceiveFunctions) {
    const std::string filename = make_test_filename("ring_behavior");
    const std::string queue_name = make_test_queue_name("ring_behavior");

    FileCleanupGuard file_cleanup(filename);

    create_queue_file(filename, 3);

    SyncNames names = make_sync_names(queue_name);
    SyncHandles handles = create_sync_objects(names, 3);
    SyncCleanupGuard sync_cleanup(handles);

    send_one_message(filename, handles, "A");
    send_one_message(filename, handles, "B");
    send_one_message(filename, handles, "C");

    EXPECT_EQ(receive_one_message(filename, handles), "A");
    EXPECT_EQ(receive_one_message(filename, handles), "B");

    send_one_message(filename, handles, "D");
    send_one_message(filename, handles, "E");

    EXPECT_EQ(receive_one_message(filename, handles), "C");
    EXPECT_EQ(receive_one_message(filename, handles), "D");
    EXPECT_EQ(receive_one_message(filename, handles), "E");

    QueueHeader header = read_header(filename);
    EXPECT_TRUE(is_queue_empty(header));
}