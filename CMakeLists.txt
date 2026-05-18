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

find_package(sockpp CONFIG REQUIRED)

add_library(shared STATIC
  src/shared/counter.cpp
  src/shared/packet.cpp
  src/shared/handler.cpp
  src/shared/text_RGA.cpp
)
target_include_directories(shared PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(shared PUBLIC Sockpp::sockpp-static)
target_precompile_headers(shared PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include/common.hpp
)

add_executable(client
  src/client/main.cpp
  src/client/client.cpp
)
target_link_libraries(client PRIVATE shared)

add_executable(server
  src/server/main.cpp
  src/server/server.cpp
)
target_link_libraries(server PRIVATE shared)

enable_testing()
add_subdirectory(tests)
