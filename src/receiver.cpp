#include "receiver.h"

#include "file_queue.h"
#include "queue_logic.h"
#include "queue_types.h"
#include "winapi_error.h"

#include <iostream>
#include <string>
#include <windows.h>


    std::string message_record_to_string(const MessageRecord& record) {
        return std::string(record.data);
    }

std::string receive_one_message(const std::string& filename,
    const SyncHandles& handles) {
    if (handles.empty_semaphore == NULL ||
        handles.full_semaphore == NULL ||
        handles.mutex_handle == NULL) {
        throw std::runtime_error("Invalid synchronization handles");
    }

    while (true) {
        DWORD wait_result = WaitForSingleObject(handles.full_semaphore, 100);

        if (wait_result == WAIT_OBJECT_0) {
            break;
        }

        if (wait_result == WAIT_FAILED) {
            throw WinApiError::from_last_error("WaitForSingleObject failed for full semaphore");
        }

        if (wait_result != WAIT_TIMEOUT) {
            throw std::runtime_error("Unexpected wait result for full semaphore");
        }

        if (WaitForSingleObject(handles.mutex_handle, INFINITE) != WAIT_OBJECT_0) {
            throw WinApiError::from_last_error("WaitForSingleObject failed for mutex");
        }

        QueueHeader header = read_header(filename);
        bool queue_empty = is_queue_empty(header);
        bool no_active_senders = (header.active_senders == 0);

        if (!ReleaseMutex(handles.mutex_handle)) {
            throw WinApiError::from_last_error("ReleaseMutex failed");
        }

        if (queue_empty && no_active_senders) {
            throw std::runtime_error("Queue is empty and all sender processes have finished");
        }
    }

    bool mutex_locked = false;

    try {
        if (WaitForSingleObject(handles.mutex_handle, INFINITE) != WAIT_OBJECT_0) {
            throw WinApiError::from_last_error("WaitForSingleObject failed for mutex");
        }
        mutex_locked = true;

        QueueHeader header = read_header(filename);

        if (is_queue_empty(header)) {
            throw std::runtime_error("Queue is empty");
        }

        MessageRecord record = read_record(filename, header.read_pos);
        std::string message = message_record_to_string(record);

        on_dequeue(header);
        write_header(filename, header);

        if (!ReleaseMutex(handles.mutex_handle)) {
            throw WinApiError::from_last_error("ReleaseMutex failed");
        }
        mutex_locked = false;

        if (!ReleaseSemaphore(handles.empty_semaphore, 1, NULL)) {
            throw WinApiError::from_last_error("ReleaseSemaphore failed for empty semaphore");
        }

        return message;
    }
    catch (...) {
        if (mutex_locked) {
            ReleaseMutex(handles.mutex_handle);
        }
        throw;
    }
}

int run_receiver(const std::string& filename,
    const SyncNames& names,
    std::size_t sender_count) {
    SyncHandles handles{};

    try {
        handles = open_sync_objects(names);

        std::cout << "Waiting for sender processes..." << std::endl;

        for (std::size_t i = 0; i < sender_count; ++i) {
            if (WaitForSingleObject(handles.ready_semaphore, INFINITE) != WAIT_OBJECT_0) {
                throw WinApiError::from_last_error("WaitForSingleObject failed for ready semaphore");
            }
        }

        std::cout << "All senders are ready." << std::endl;

        while (true) {
            std::string command;

            std::cout << "\nReceiver command (read/exit): ";
            std::getline(std::cin, command);

            if (command == "exit") {
                break;
            }

            if (command == "read") {
                try {
                    std::string message = receive_one_message(filename, handles);
                    std::cout << "Received: " << message << std::endl;
                }
                catch (const std::exception& ex) {
                    std::cout << ex.what() << std::endl;
                }
            }
            else {
                std::cout << "Unknown command." << std::endl;
            }
        }

        if (WaitForSingleObject(handles.mutex_handle, INFINITE) != WAIT_OBJECT_0) {
            throw WinApiError::from_last_error("WaitForSingleObject failed for mutex");
        }

        QueueHeader header = read_header(filename);
        header.receiver_active = 0;
        write_header(filename, header);

        if (!ReleaseMutex(handles.mutex_handle)) {
            throw WinApiError::from_last_error("ReleaseMutex failed");
        }

        close_sync_handles(handles);
        return 0;
    }
    catch (const WinApiError& ex) {
        std::cerr << "Receiver WinAPI error: " << ex.what() << std::endl;
        close_sync_handles(handles);
        return static_cast<int>(ex.code());
    }
    catch (const std::exception& ex) {
        std::cerr << "Receiver error: " << ex.what() << std::endl;
        close_sync_handles(handles);
        return 1;
    }
}