#pragma once

#include "queue_types.h"
#include <cstddef>
#include <cstdint>
#include <string>

bool is_queue_empty(const QueueHeader& header);
bool is_queue_full(const QueueHeader& header);

std::uint32_t next_index(std::uint32_t index, std::uint32_t capacity);

void on_enqueue(QueueHeader& header);
void on_dequeue(QueueHeader& header);

std::size_t record_offset(std::uint32_t index);

void validate_queue_config(std::uint32_t capacity);
std::string normalize_message(const std::string& message);