#include "winapi_error.h"

WinApiError::WinApiError(const std::string& where, DWORD code)
    : std::runtime_error(build_message(where, code)),
    error_code_(code) {
}

DWORD WinApiError::code() const noexcept {
    return error_code_;
}

std::string WinApiError::format_message(DWORD code) {
    LPSTR buffer = nullptr;

    const DWORD flags =
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS;

    const DWORD size = FormatMessageA(
        flags,
        nullptr,
        code,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<LPSTR>(&buffer),
        0,
        nullptr
    );

    std::string result;

    if (size != 0 && buffer != nullptr) {
        result.assign(buffer, buffer + size);
    }
    else {
        result = "Unknown WinAPI error";
    }

    if (buffer != nullptr) {
        LocalFree(buffer);
    }

    while (!result.empty() &&
        (result.back() == '\n' || result.back() == '\r' || result.back() == ' ')) {
        result.pop_back();
    }

    return result;
}

std::string WinApiError::build_message(const std::string& where, DWORD code) {
    return where + ": (" + std::to_string(code) + ") " + format_message(code);
}

WinApiError WinApiError::from_last_error(const std::string& where) {
    return WinApiError(where, GetLastError());
}