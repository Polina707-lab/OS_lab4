#pragma once

#include <cstdint>

const std::uint32_t MAX_MESSAGE_LENGTH = 20;
const std::uint32_t MESSAGE_BUFFER_SIZE = MAX_MESSAGE_LENGTH + 1;

struct QueueHeader {
    std::uint32_t capacity;
    std::uint32_t read_pos;
    std::uint32_t write_pos;
    std::uint32_t count;

    std::uint32_t active_senders;
    std::uint32_t receiver_active;
};

struct MessageRecord {
    char data[MESSAGE_BUFFER_SIZE];
};