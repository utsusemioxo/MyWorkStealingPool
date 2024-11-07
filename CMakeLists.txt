cmake_minimum_required(VERSION 3.20)

project(ThreadPool VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

# Add thread_pool library
add_library(thread_pool STATIC src/thread_pool.cpp)
target_include_directories(thread_pool PUBLIC inc)

# Ensure RTTI is enabled for thread_pool
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(thread_pool PRIVATE -frtti)
elseif (MSVC)
    target_compile_options(thread_pool PRIVATE /GR)
endif()

# Add test_thread_pool executable
add_executable(test_thread_pool test/main.cpp)

# Link thread_pool to the executable
target_link_libraries(test_thread_pool PRIVATE thread_pool)
