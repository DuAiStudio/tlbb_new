cmake_minimum_required(VERSION 3.20)

# Single source of truth for semver; used by CMake project() and documented in README (A2).
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION")
    file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" _sharemem_ver_raw)
    string(STRIP "${_sharemem_ver_raw}" SHAREMEMORY64_VERSION)
else()
    set(SHAREMEMORY64_VERSION "0.0.0")
endif()

project(ShareMemory64 VERSION ${SHAREMEMORY64_VERSION} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(sharememory64
    src/main.cpp
    src/config.cpp
    src/db_access.cpp
    src/logger.cpp
    src/odbc_interface.cpp
    src/share_mem_linux.cpp
    src/share_memory.cpp
)

target_include_directories(sharememory64 PRIVATE include)

# Foreground log line: `(###Hurricane###) Server Dev:… ClientVer:…` (override: -DSHAREMEMORY_SERVER_DEV=…)
set(SHAREMEMORY_SERVER_DEV "1271" CACHE STRING "Logged as Server Dev in Hurricane line (legacy default)")
set(SHAREMEMORY_CLIENT_VER "533" CACHE STRING "Logged as ClientVer in Hurricane line (legacy default)")

target_compile_definitions(
    sharememory64
    PRIVATE
    SHAREMEMORY64_VERSION="${PROJECT_VERSION}"
    SHAREMEMORY_SERVER_DEV=${SHAREMEMORY_SERVER_DEV}
    SHAREMEMORY_CLIENT_VER=${SHAREMEMORY_CLIENT_VER}
)

find_package(ODBC REQUIRED)
target_link_libraries(sharememory64 PRIVATE ODBC::ODBC pthread)
