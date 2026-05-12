find_package(GTest CONFIG REQUIRED)
include(GoogleTest)

file(GLOB_RECURSE TEST_SOURCES "*.cpp")
add_executable(tests ${TEST_SOURCES} ${CMAKE_SOURCE_DIR}/src/client.cpp ${CMAKE_SOURCE_DIR}/src/server.cpp)

target_include_directories(tests PRIVATE
  ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(tests PRIVATE GTest::gtest_main Sockpp::sockpp-static)

gtest_discover_tests(tests)
