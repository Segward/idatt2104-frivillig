find_package(GTest CONFIG REQUIRED)
include(GoogleTest)

file(GLOB_RECURSE TEST_SOURCES "*.cpp")

add_executable(tests ${TEST_SOURCES})

target_link_libraries(tests
    PRIVATE
        GTest::gtest_main
        shared
)

gtest_discover_tests(tests)
