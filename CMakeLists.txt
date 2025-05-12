cmake_minimum_required(VERSION 3.22)
project(web_server)

set(CMAKE_CXX_STANDARD 20)

include(FetchContent)

FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/release-1.12.1.zip
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)


add_executable(web_server main.cpp src/WebServer.cpp src/HttpRequest.cpp
        include/HttpResponse.hpp include/HttpRequest.hpp include/WebServer.hpp include/sha1.hpp include/encryption.hpp
        src/HttpResponse.cpp
        src/HttpRequestReader.cpp
        include/HttpRequestReader.hpp)

target_include_directories(web_server PRIVATE include)
target_link_libraries(web_server pthread)

add_executable(web_server_integration_tests tests/main.cpp src/WebServer.cpp src/HttpRequest.cpp src/HttpResponse.cpp
        src/HttpRequestReader.cpp)
target_include_directories(web_server_integration_tests PRIVATE include)
target_link_libraries(web_server_integration_tests PRIVATE GTest::gtest GTest::gmock pthread)

add_executable(web_server_unit_tests tests/unit_tests.cpp src/WebServer.cpp src/HttpRequest.cpp src/HttpResponse.cpp
        src/HttpRequestReader.cpp
)
target_include_directories(web_server_unit_tests PRIVATE include)
target_link_libraries(web_server_unit_tests PRIVATE GTest::gtest GTest::gmock pthread)

add_executable(web_server_benchmark_tests tests/benchmark_tests.cpp src/WebServer.cpp src/HttpRequest.cpp src/HttpResponse.cpp
        src/HttpRequestReader.cpp
)
target_include_directories(web_server_benchmark_tests PRIVATE include)
target_link_libraries(web_server_benchmark_tests PRIVATE GTest::gtest GTest::gmock pthread)