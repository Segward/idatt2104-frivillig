find_package(GTest CONFIG REQUIRED)
include(GoogleTest)

file(GLOB_RECURSE TEST_SOURCES "*.cpp")
add_executable(tests ${TEST_SOURCES})

target_include_directories(tests PRIVATE
  ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(tests 
  PRIVATE 
    crdt_core 
    GTest::gtest_main
)

gtest_discover_tests(tests)
