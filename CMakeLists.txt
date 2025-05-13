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


add_library(server-core
        3dparty/sha1/sha1.hpp
        src/WebServer.cpp src/HttpRequest.cpp
        include/server/HttpResponse.hpp
        src/HttpResponse.cpp
        src/WebSocket.cpp
        include/server/WebSocket.hpp
        include/server/WSApplication.hpp
        src/WSApplication.cpp
        include/server/SocketListener.hpp
        src/SocketListener.cpp
        include/server/threadpool.hpp
        src/threadpool.cpp)

target_include_directories(server-core PUBLIC include ${CMAKE_SOURCE_DIR}/3dparty/sha1)


add_executable(web_server main.cpp)
target_link_libraries(web_server pthread server-core)

add_executable(web_server_tests tests/main.cpp src/WebServer.cpp src/HttpRequest.cpp src/HttpResponse.cpp)
target_include_directories(web_server_tests PRIVATE include)
target_link_libraries(web_server_tests PRIVATE GTest::gtest GTest::gmock pthread)

add_executable(socket_listener_tests tests/SocketListenerTests.cpp)
target_include_directories(socket_listener_tests PRIVATE include)
target_link_libraries(socket_listener_tests PRIVATE server-core GTest::gtest GTest::gmock)

add_test(NAME socket_listener_tests COMMAND socket_listener_tests)