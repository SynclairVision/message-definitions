cmake_minimum_required(VERSION 3.10)
project(MessageDefinitions)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

add_executable(msg_defs_test_program tests/msg_defs_test.cpp)
target_link_libraries(msg_defs_test_program ${GTEST_LIBRARIES} pthread)

enable_testing()
add_test(NAME MsgDefsTest COMMAND msg_defs_test_program)
