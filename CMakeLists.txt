cmake_minimum_required(VERSION 3.25)
project(idatt2104 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(sockpp CONFIG REQUIRED)

add_library(crdt STATIC
  src/shared/crdt/counter.cpp
  src/shared/crdt/packet.cpp
)
target_include_directories(crdt PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_precompile_headers(crdt PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include/common.hpp
)

add_executable(client
  src/client/main.cpp
  src/client/client.cpp
)
target_include_directories(client PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(client PRIVATE crdt Sockpp::sockpp-static)

add_executable(server
  src/server/main.cpp
  src/server/server.cpp
)
target_include_directories(server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(server PRIVATE crdt Sockpp::sockpp-static)

enable_testing()
add_subdirectory(tests)
