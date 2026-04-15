#include "file_queue.h"
#include "receiver.h"
#include "sender.h"
#include "sync_objects.h"
#include "winapi_error.h"

#include <iostream>
#include <string>
#include <vector>
#include <windows.h>

const std::uint32_t MAX_SENDER_COUNT = 10;

void close_process_info(PROCESS_INFORMATION& pi) {
    if (pi.hThread != NULL) {
        CloseHandle(pi.hThread);
        pi.hThread = NULL;
    }

    if (pi.hProcess != NULL) {
        CloseHandle(pi.hProcess);
        pi.hProcess = NULL;
    }
}

std::string quote(const std::string& text) {
    return "\"" + text + "\"";
}

std::string build_sender_command_line(const std::string& exe_path,
    const std::string& filename) {
    return quote(exe_path) + " sender " + quote(filename);
}

PROCESS_INFORMATION start_sender_process(const std::string& command_line) {
    STARTUPINFOA si{};
    PROCESS_INFORMATION pi{};
    si.cb = sizeof(si);

    std::vector<char> cmd_line(command_line.begin(), command_line.end());
    cmd_line.push_back('\0');

    if (!CreateProcessA(
        NULL,
        cmd_line.data(),
        NULL,
        NULL,
        FALSE,
        CREATE_NEW_CONSOLE,
        NULL,
        NULL,
        &si,
        &pi)) {
        throw WinApiError::from_last_error("CreateProcessA failed");
    }

    return pi;
}

void print_usage(const std::string& exe_name) {
    std::cout << "Usage:" << std::endl;
    std::cout << "  " << exe_name << std::endl;
    std::cout << "  " << exe_name << " sender <file>" << std::endl;
}

std::string input_file_name() {
    std::string filename;

    while (true) {
        std::cout << "Enter binary file name: ";
        std::getline(std::cin, filename);

        if (!filename.empty()) {
            return filename;
        }

        std::cout << "File name cannot be empty." << std::endl;
    }
}

std::uint32_t input_positive_number(const std::string& prompt) {
    std::uint32_t value = 0;

    while (true) {
        std::cout << prompt;

        if ((std::cin >> value) && value > 0) {
            std::cin.ignore(10000, '\n');
            return value;
        }

        std::cout << "Invalid input. Enter a positive number." << std::endl;
        std::cin.clear();
        std::cin.ignore(10000, '\n');
    }
}

std::uint32_t input_sender_count() {
    std::uint32_t value = 0;

    while (true) {
        std::cout << "Enter number of sender processes (1-"
            << MAX_SENDER_COUNT << "): ";

        if ((std::cin >> value) && value > 0 && value <= MAX_SENDER_COUNT) {
            std::cin.ignore(10000, '\n');
            return value;
        }

        std::cout << "Invalid input. Enter a number from 1 to "
            << MAX_SENDER_COUNT << "." << std::endl;

        std::cin.clear();
        std::cin.ignore(10000, '\n');
    }
}

int run_main_receiver(const std::string& exe_path) {
    std::string filename = input_file_name();
    std::uint32_t capacity = input_positive_number("Enter number of records: ");
    std::uint32_t sender_count = input_sender_count();

    create_queue_file(filename, capacity);

    QueueHeader header = read_header(filename);
    header.active_senders = sender_count;
    header.receiver_active = 1;
    write_header(filename, header);

    SyncNames sync_names = make_sync_names(filename);
    SyncHandles handles = create_sync_objects(sync_names, static_cast<long>(capacity));

    std::vector<PROCESS_INFORMATION> sender_processes;

    try {
        for (std::uint32_t i = 0; i < sender_count; ++i) {
            std::string command_line =
                build_sender_command_line(exe_path, filename);

            PROCESS_INFORMATION pi = start_sender_process(command_line);
            sender_processes.push_back(pi);
        }

        int result = run_receiver(filename, sync_names, sender_count);

        for (std::size_t i = 0; i < sender_processes.size(); ++i) {
            close_process_info(sender_processes[i]);
        }

        close_sync_handles(handles);
        return result;
    }
    catch (...) {
        for (std::size_t i = 0; i < sender_processes.size(); ++i) {
            close_process_info(sender_processes[i]);
        }

        close_sync_handles(handles);
        throw;
    }
}

int main(int argc, char* argv[]) {
    try {
        if (argc == 1) {
            return run_main_receiver(argv[0]);
        }

        std::string mode = argv[1];

        if (mode == "sender") {
            if (argc != 3) {
                print_usage(argv[0]);
                return 1;
            }

            std::string filename = argv[2];

            return run_sender(filename, make_sync_names(filename));
        }

        print_usage(argv[0]);
        return 1;
    }
    catch (const WinApiError& ex) {
        std::cerr << "WinAPI error: " << ex.what() << std::endl;
        return static_cast<int>(ex.code());
    }
    catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        return 1;
    }
}