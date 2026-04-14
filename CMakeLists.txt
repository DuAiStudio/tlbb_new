cmake_minimum_required(VERSION 3.20)
project(ShareMemory64 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(sharememory64
    src/main.cpp
    src/config.cpp
    src/db_access.cpp
    src/logger.cpp
    src/odbc_interface.cpp
    src/share_memory.cpp
)

target_include_directories(sharememory64 PRIVATE include)

find_package(ODBC REQUIRED)
target_link_libraries(sharememory64 PRIVATE ODBC::ODBC pthread)
