#pragma once

#include "sync_objects.h"
#include <string>

std::string receive_one_message(const std::string& filename,
    const SyncHandles& handles);

int run_receiver(const std::string& filename,
    const SyncNames& names,
    std::size_t sender_count);