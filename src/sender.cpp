#include "sender.h"

#include "file_queue.h"
#include "queue_logic.h"
#include "queue_types.h"
#include "winapi_error.h"

#include <cstring>
#include <iostream>
#include <string>
#include <windows.h>


    MessageRecord make_message_record(const std::string& message) {
        MessageRecord record{};
        std::string text = normalize_message(message);

        std::memcpy(record.data, text.c_str(), text.size());
        record.data[text.size()] = '\0';

        return record;
    }



void send_one_message(const std::string& filename,
    const SyncHandles& handles,
    const std::string& message) {
    if (handles.empty_semaphore == NULL ||
        handles.full_semaphore == NULL ||
        handles.mutex_handle == NULL) {
        throw std::runtime_error("Invalid synchronization handles");
    }

    if (WaitForSingleObject(handles.empty_semaphore, INFINITE) != WAIT_OBJECT_0) {
        throw WinApiError::from_last_error("WaitForSingleObject failed for empty semaphore");
    }

    bool mutex_locked = false;

    try {
        if (WaitForSingleObject(handles.mutex_handle, INFINITE) != WAIT_OBJECT_0) {
            throw WinApiError::from_last_error("WaitForSingleObject failed for mutex");
        }
        mutex_locked = true;

        QueueHeader header = read_header(filename);

        if (header.receiver_active == 0) {
            if (!ReleaseMutex(handles.mutex_handle)) {
                throw WinApiError::from_last_error("ReleaseMutex failed");
            }
            mutex_locked = false;

            if (!ReleaseSemaphore(handles.empty_semaphore, 1, NULL)) {
                throw WinApiError::from_last_error("ReleaseSemaphore failed for empty semaphore");
            }

            throw std::runtime_error("Receiver process has already finished");
        }

        if (is_queue_full(header)) {
            throw std::runtime_error("Queue is full");
        }

        MessageRecord record = make_message_record(message);

        write_record(filename, header.write_pos, record);
        on_enqueue(header);
        write_header(filename, header);

        if (!ReleaseMutex(handles.mutex_handle)) {
            throw WinApiError::from_last_error("ReleaseMutex failed");
        }
        mutex_locked = false;

        if (!ReleaseSemaphore(handles.full_semaphore, 1, NULL)) {
            throw WinApiError::from_last_error("ReleaseSemaphore failed for full semaphore");
        }
    }
    catch (...) {
        if (mutex_locked) {
            ReleaseMutex(handles.mutex_handle);
        }
        throw;
    }
}

int run_sender(const std::string& filename,
    const SyncNames& names) {
    SyncHandles handles{};

    try {
        handles = open_sync_objects(names);

        if (!ReleaseSemaphore(handles.ready_semaphore, 1, NULL)) {
            throw WinApiError::from_last_error("ReleaseSemaphore failed for ready semaphore");
        }

        while (true) {
            std::string command;

            std::cout << "\nSender command (send/exit): ";
            std::getline(std::cin, command);

            if (command == "exit") {
                break;
            }

            if (command == "send") {
                try {
                    std::string message;

                    std::cout << "Enter message: ";
                    std::getline(std::cin, message);

                    send_one_message(filename, handles, message);
                    std::cout << "Message sent." << std::endl;
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
        if (header.active_senders > 0) {
            header.active_senders--;
            write_header(filename, header);
        }

        if (!ReleaseMutex(handles.mutex_handle)) {
            throw WinApiError::from_last_error("ReleaseMutex failed");
        }

        close_sync_handles(handles);
        return 0;
    }
    catch (const WinApiError& ex) {
        std::cerr << "Sender WinAPI error: " << ex.what() << std::endl;
        close_sync_handles(handles);
        return static_cast<int>(ex.code());
    }
    catch (const std::exception& ex) {
        std::cerr << "Sender error: " << ex.what() << std::endl;
        close_sync_handles(handles);
        return 1;
    }
}