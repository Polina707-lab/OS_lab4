#include "queue_logic.h"

#include <stdexcept>

bool is_queue_empty(const QueueHeader& header) {
    return header.count == 0;
}

bool is_queue_full(const QueueHeader& header) {
    return header.count == header.capacity;
}

std::uint32_t next_index(std::uint32_t index, std::uint32_t capacity) {
    if (capacity == 0) {
        throw std::invalid_argument("Queue capacity must be greater than zero");
    }

    return (index + 1) % capacity;
}

void on_enqueue(QueueHeader& header) {
    if (is_queue_full(header)) {
        throw std::logic_error("Queue is full");
    }

    header.write_pos = next_index(header.write_pos, header.capacity);
    ++header.count;
}

void on_dequeue(QueueHeader& header) {
    if (is_queue_empty(header)) {
        throw std::logic_error("Queue is empty");
    }

    header.read_pos = next_index(header.read_pos, header.capacity);
    --header.count;
}

std::size_t record_offset(std::uint32_t index) {
    return sizeof(QueueHeader) + index * sizeof(MessageRecord);
}

void validate_queue_config(std::uint32_t capacity) {
    if (capacity == 0) {
        throw std::invalid_argument("Queue capacity must be greater than zero");
    }
}

std::string normalize_message(const std::string& message) {
    if (message.size() > MAX_MESSAGE_LENGTH) {
        return message.substr(0, MAX_MESSAGE_LENGTH);
    }

    return message;
}