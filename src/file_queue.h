#pragma once

#include "queue_types.h"
#include <string>

void create_queue_file(const std::string& filename, std::uint32_t capacity);

QueueHeader read_header(const std::string& filename);
void write_header(const std::string& filename, const QueueHeader& header);

void write_record(const std::string& filename,
    std::uint32_t index,
    const MessageRecord& record);

MessageRecord read_record(const std::string& filename,
    std::uint32_t index);