cmake_minimum_required(VERSION 3.14)

project(OS_Lab)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_library(os_lab_core
    src/winapi_error.cpp
    src/queue_logic.cpp
    src/file_queue.cpp
    src/sync_objects.cpp
    src/sender.cpp
    src/receiver.cpp
)

target_include_directories(os_lab_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

add_executable(OS_Lab
    src/main.cpp
)

target_link_libraries(OS_Lab PRIVATE
    os_lab_core
)

add_executable(OS_Lab_Tests
    tests/queue_logic_tests.cpp
    tests/file_queue_tests.cpp
    tests/ipc_tests.cpp
)

target_link_libraries(OS_Lab_Tests PRIVATE
    os_lab_core
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(OS_Lab_Tests)