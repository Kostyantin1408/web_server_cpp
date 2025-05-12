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

# find openssl
find_package(OpenSSL REQUIRED)
if (NOT OpenSSL_FOUND)
    message(FATAL_ERROR "OpenSSL not found")
endif ()

add_library(server-core
        src/WebServer.cpp src/HttpRequest.cpp
        include/server/HttpResponse.hpp
        src/HttpResponse.cpp
        src/WebSocket.cpp
        include/server/WebSocket.hpp
        include/server/WSApplication.hpp
        src/WSApplication.cpp
        include/server/WebSocketParser.hpp
        src/WebSocketParser.cpp
        include/server/SocketListener.hpp
        src/SocketListener.cpp)

target_include_directories(server-core PUBLIC include)


add_executable(web_server main.cpp)
target_link_libraries(web_server pthread OpenSSL::SSL OpenSSL::Crypto server-core)

add_executable(web_server_tests tests/main.cpp src/WebServer.cpp src/HttpRequest.cpp src/HttpResponse.cpp)
target_include_directories(web_server_tests PRIVATE include)
target_link_libraries(web_server_tests PRIVATE GTest::gtest GTest::gmock pthread)

add_executable(socket_listener_tests tests/SocketListenerTests.cpp)
target_include_directories(socket_listener_tests PRIVATE include)
target_link_libraries(socket_listener_tests PRIVATE server-core GTest::gtest GTest::gmock)

add_test(NAME socket_listener_tests COMMAND socket_listener_tests)