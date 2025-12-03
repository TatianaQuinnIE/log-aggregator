cmake_minimum_required(VERSION 3.15)
project(LogAggregator LANGUAGES C CXX)

# Set C and C++ standards
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler flags
set(CMAKE_C_FLAGS_DEBUG "-g -Wall -Wextra -Wpedantic")
set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "-g -Wall -Wextra -Wpedantic")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")

# Include directories
include_directories(include)

# Source files
set(SOURCES
    src/main.c
    src/log_entry.c
    src/queue.c
    src/config.c
    src/log_source.c
    src/processor.c
    src/alerter.c
)

# Create executable
add_executable(log_aggregator ${SOURCES})

# Link pthread library
target_link_libraries(log_aggregator pthread)

# Enable testing
enable_testing()

# Test executable
add_executable(test_log_aggregator
    tests/test_main.c
    tests/test_log_entry.c
    tests/test_queue.c
    tests/test_config.c
    src/log_entry.c
    src/queue.c
    src/config.c
)

target_link_libraries(test_log_aggregator pthread)

# Add C test
add_test(NAME LogAggregatorTests COMMAND test_log_aggregator)

# Google Test (optional - only build if gtest is found)
find_package(GTest QUIET)
if(GTest_FOUND)
    # Create gtest executable
    add_executable(test_queue_gtest
        tests/test_queue_gtest.cpp
        src/queue.c
        src/log_entry.c
    )
    
    target_include_directories(test_queue_gtest PRIVATE include)
    target_link_libraries(test_queue_gtest GTest::GTest GTest::Main pthread)
    
    # Add gtest to CTest
    add_test(NAME QueueGTest COMMAND test_queue_gtest)
else()
    message(STATUS "Google Test not found - skipping gtest build")
    message(STATUS "To enable gtest, install it and reconfigure CMake")
endif()