cmake_minimum_required(VERSION 3.25)
project(idatt2104 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(IDATT_SANITIZE "Build with address and undefined-behavior sanitizers" OFF)

if(MSVC)
  add_compile_options(/W4)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

if(IDATT_SANITIZE AND NOT MSVC)
  add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
  add_link_options(-fsanitize=address,undefined)
endif()

find_package(unofficial-uwebsockets CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Threads REQUIRED)

add_library(crdt STATIC
  src/crdt/counter_pn.cpp
  src/crdt/list_rga.cpp
  src/crdt/text_rga.cpp
  src/server/handler.cpp
  src/server/messages.cpp
)
target_include_directories(crdt PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(crdt PUBLIC
  nlohmann_json::nlohmann_json
  Threads::Threads
)
target_precompile_headers(crdt PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include/common.hpp
)

add_executable(server
  src/main.cpp
  src/server/server.cpp
  src/server/assets.cpp
  src/util/io.cpp
)
set_target_properties(server PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/release"
)
target_link_libraries(server PRIVATE
  crdt
  unofficial::uwebsockets::uwebsockets
)

add_custom_command(TARGET server POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/website"
    "$<TARGET_FILE_DIR:server>/website"
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/certs"
    "$<TARGET_FILE_DIR:server>/certs"
)
target_compile_definitions(server PRIVATE
  "WEBSITE_INDEX=\"$<TARGET_FILE_DIR:server>/website/index.html\""
  "CERT_PATH=\"$<TARGET_FILE_DIR:server>/certs/fullchain.pem\""
  "KEY_PATH=\"$<TARGET_FILE_DIR:server>/certs/privkey.pem\"")

enable_testing()
add_subdirectory(tests)
