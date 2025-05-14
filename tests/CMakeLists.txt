include(FetchContent)

FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/release-1.12.1.zip
        DOWNLOAD_EXTRACT_TIMESTAMP ON
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

enable_testing()

include(GoogleTest)

add_executable(web_server_integration_tests web_server_integration_tests.cpp)
target_link_libraries(web_server_integration_tests PRIVATE GTest::gtest_main lws)
gtest_discover_tests(web_server_integration_tests)

add_executable(web_server_unit_tests web_server_unit_tests.cpp)
target_link_libraries(web_server_unit_tests PRIVATE GTest::gtest_main lws)
gtest_discover_tests(web_server_unit_tests)

add_executable(web_server_parser_tests web_server_parser_tests.cpp)
target_link_libraries(web_server_parser_tests PRIVATE GTest::gtest_main lws)
gtest_discover_tests(web_server_parser_tests)

