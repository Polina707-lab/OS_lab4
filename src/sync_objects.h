#pragma once

#include <string>
#include <windows.h>

struct SyncNames {
    std::string mutex_name;
    std::string empty_semaphore_name;
    std::string full_semaphore_name;
    std::string ready_semaphore_name;
};

struct SyncHandles {
    HANDLE mutex_handle;
    HANDLE empty_semaphore;
    HANDLE full_semaphore;
    HANDLE ready_semaphore;
};

SyncNames make_sync_names(const std::string& queue_name);

SyncHandles create_sync_objects(const SyncNames& names, long capacity);
SyncHandles open_sync_objects(const SyncNames& names);

void close_sync_handles(SyncHandles& handles);