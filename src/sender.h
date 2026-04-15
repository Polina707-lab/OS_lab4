#pragma once

#include "sync_objects.h"
#include <string>

void send_one_message(const std::string& filename,
    const SyncHandles& handles,
    const std::string& message);

int run_sender(const std::string& filename,
    const SyncNames& names);