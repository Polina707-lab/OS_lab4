#pragma once

#include <windows.h>
#include <stdexcept>
#include <string>

class WinApiError : public std::runtime_error {
public:
    WinApiError(const std::string& where, DWORD code);

    DWORD code() const noexcept;

    static std::string format_message(DWORD code);
    static WinApiError from_last_error(const std::string& where);

private:
    static std::string build_message(const std::string& where, DWORD code);

    DWORD error_code_;
};