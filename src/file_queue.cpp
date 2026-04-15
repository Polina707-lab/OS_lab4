#include "file_queue.h"
#include "queue_logic.h"

#include <fstream>
#include <stdexcept>

void create_queue_file(const std::string& filename, std::uint32_t capacity) {
    validate_queue_config(capacity);

    std::ofstream file(filename, std::ios::binary | std::ios::trunc);
    if (!file) {
        throw std::runtime_error("Cannot create queue file");
    }

    QueueHeader header;
    header.capacity = capacity;
    header.read_pos = 0;
    header.write_pos = 0;
    header.count = 0;
    header.active_senders = 0;
    header.receiver_active = 1;

    file.write(reinterpret_cast<const char*>(&header), sizeof(QueueHeader));
    if (!file) {
        throw std::runtime_error("Cannot write queue header");
    }

    MessageRecord record{};
    for (std::uint32_t i = 0; i < capacity; ++i) {
        file.write(reinterpret_cast<const char*>(&record), sizeof(MessageRecord));
        if (!file) {
            throw std::runtime_error("Cannot initialize queue records");
        }
    }
}

QueueHeader read_header(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Cannot open file to read header");
    }

    QueueHeader header{};
    file.read(reinterpret_cast<char*>(&header), sizeof(QueueHeader));
    if (!file) {
        throw std::runtime_error("Cannot read queue header");
    }

    return header;
}

void write_header(const std::string& filename, const QueueHeader& header) {
    std::fstream file(filename, std::ios::binary | std::ios::in | std::ios::out);
    if (!file) {
        throw std::runtime_error("Cannot open file to write header");
    }

    file.seekp(0, std::ios::beg);
    if (!file) {
        throw std::runtime_error("Cannot seek to header");
    }

    file.write(reinterpret_cast<const char*>(&header), sizeof(QueueHeader));
    if (!file) {
        throw std::runtime_error("Cannot write queue header");
    }
}

void write_record(const std::string& filename,
    std::uint32_t index,
    const MessageRecord& record) {
    QueueHeader header = read_header(filename);

    if (index >= header.capacity) {
        throw std::out_of_range("Record index out of range");
    }

    std::fstream file(filename, std::ios::binary | std::ios::in | std::ios::out);
    if (!file) {
        throw std::runtime_error("Cannot open file to write record");
    }

    file.seekp(static_cast<std::streamoff>(record_offset(index)), std::ios::beg);
    if (!file) {
        throw std::runtime_error("Cannot seek to record");
    }

    file.write(reinterpret_cast<const char*>(&record), sizeof(MessageRecord));
    if (!file) {
        throw std::runtime_error("Cannot write record");
    }
}

MessageRecord read_record(const std::string& filename, std::uint32_t index) {
    QueueHeader header = read_header(filename);

    if (index >= header.capacity) {
        throw std::out_of_range("Record index out of range");
    }

    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Cannot open file to read record");
    }

    file.seekg(static_cast<std::streamoff>(record_offset(index)), std::ios::beg);
    if (!file) {
        throw std::runtime_error("Cannot seek to record");
    }

    MessageRecord record{};
    file.read(reinterpret_cast<char*>(&record), sizeof(MessageRecord));
    if (!file) {
        throw std::runtime_error("Cannot read record");
    }

    return record;
}