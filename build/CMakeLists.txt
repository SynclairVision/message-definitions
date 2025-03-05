cmake_minimum_required(VERSION 3.10)
project(MessageDefinitions)

# Sätt C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Lägg till källfiler
set(SOURCE_FILES
    message.hpp
    msg_defs.hpp
    test_msg_defs.cpp
)

# Skapa körbar fil
add_executable(message_definitions ${SOURCE_FILES})

# Inkludera kataloger
include_directories(${CMAKE_SOURCE_DIR})

# Lägg till nödvändiga bibliotek
# target_link_libraries(message_definitions <library_name>)

