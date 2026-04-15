#include "sync_objects.h"
#include "winapi_error.h"

    void close_handle_if_not_null(HANDLE& handle) {
        if (handle != NULL) {
            CloseHandle(handle);
            handle = NULL;
        }
    }


SyncNames make_sync_names(const std::string& queue_name) {
    SyncNames names;
    names.mutex_name = queue_name + "_mutex";
    names.empty_semaphore_name = queue_name + "_empty";
    names.full_semaphore_name = queue_name + "_full";
    names.ready_semaphore_name = queue_name + "_ready";
    return names;
}

SyncHandles create_sync_objects(const SyncNames& names, long capacity) {
    SyncHandles handles{};
    handles.mutex_handle = NULL;
    handles.empty_semaphore = NULL;
    handles.full_semaphore = NULL;
    handles.ready_semaphore = NULL;

    handles.mutex_handle = CreateMutexA(NULL, FALSE, names.mutex_name.c_str());
    if (handles.mutex_handle == NULL) {
        throw WinApiError::from_last_error("CreateMutexA failed");
    }

    handles.empty_semaphore = CreateSemaphoreA(
        NULL,
        capacity,
        capacity,
        names.empty_semaphore_name.c_str()
    );
    if (handles.empty_semaphore == NULL) {
        close_sync_handles(handles);
        throw WinApiError::from_last_error("CreateSemaphoreA failed for empty semaphore");
    }

    handles.full_semaphore = CreateSemaphoreA(
        NULL,
        0,
        capacity,
        names.full_semaphore_name.c_str()
    );
    if (handles.full_semaphore == NULL) {
        close_sync_handles(handles);
        throw WinApiError::from_last_error("CreateSemaphoreA failed for full semaphore");
    }

    handles.ready_semaphore = CreateSemaphoreA(
        NULL,
        0,
        capacity,
        names.ready_semaphore_name.c_str()
    );
    if (handles.ready_semaphore == NULL) {
        close_sync_handles(handles);
        throw WinApiError::from_last_error("CreateSemaphoreA failed for ready semaphore");
    }

    return handles;
}

SyncHandles open_sync_objects(const SyncNames& names) {
    SyncHandles handles{};
    handles.mutex_handle = NULL;
    handles.empty_semaphore = NULL;
    handles.full_semaphore = NULL;
    handles.ready_semaphore = NULL;

    handles.mutex_handle = OpenMutexA(
        SYNCHRONIZE | MUTEX_MODIFY_STATE,
        FALSE,
        names.mutex_name.c_str()
    );
    if (handles.mutex_handle == NULL) {
        throw WinApiError::from_last_error("OpenMutexA failed");
    }

    handles.empty_semaphore = OpenSemaphoreA(
        SYNCHRONIZE | SEMAPHORE_MODIFY_STATE,
        FALSE,
        names.empty_semaphore_name.c_str()
    );
    if (handles.empty_semaphore == NULL) {
        close_sync_handles(handles);
        throw WinApiError::from_last_error("OpenSemaphoreA failed for empty semaphore");
    }

    handles.full_semaphore = OpenSemaphoreA(
        SYNCHRONIZE | SEMAPHORE_MODIFY_STATE,
        FALSE,
        names.full_semaphore_name.c_str()
    );
    if (handles.full_semaphore == NULL) {
        close_sync_handles(handles);
        throw WinApiError::from_last_error("OpenSemaphoreA failed for full semaphore");
    }

    handles.ready_semaphore = OpenSemaphoreA(
        SYNCHRONIZE | SEMAPHORE_MODIFY_STATE,
        FALSE,
        names.ready_semaphore_name.c_str()
    );
    if (handles.ready_semaphore == NULL) {
        close_sync_handles(handles);
        throw WinApiError::from_last_error("OpenSemaphoreA failed for ready semaphore");
    }

    return handles;
}

void close_sync_handles(SyncHandles& handles) {
    close_handle_if_not_null(handles.mutex_handle);
    close_handle_if_not_null(handles.empty_semaphore);
    close_handle_if_not_null(handles.full_semaphore);
    close_handle_if_not_null(handles.ready_semaphore);
}