cmake_minimum_required(VERSION 3.25)
project(idatt2104 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(sockpp CONFIG REQUIRED)

# ------------------------------------------------------------
# Core library: all reusable project code, such as Counter.cpp.
# Do NOT include main.cpp here.
# ------------------------------------------------------------

file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

# Remove main.cpp from the library if you have one.
# A library should not contain main().
list(FILTER CORE_SOURCES EXCLUDE REGEX ".*/main\\.cpp$")

add_library(crdt_core ${CORE_SOURCES})

target_include_directories(crdt_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(crdt_core
    PUBLIC
        Sockpp::sockpp-static
)

# ------------------------------------------------------------
# Main executable
# ------------------------------------------------------------

add_executable(${PROJECT_NAME}
    src/main.cpp
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        crdt_core
)

# ------------------------------------------------------------
# Tests
# ------------------------------------------------------------

enable_testing()
add_subdirectory(tests)