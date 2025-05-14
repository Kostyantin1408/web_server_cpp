cmake_minimum_required(VERSION 3.22)
project(lws)

set(CMAKE_CXX_STANDARD 20)
option(ENABLE_BENCHMARK "Enable benchmark" OFF)
option(ENABLE_TESTS "Enable tests" OFF)
option(BUILD_EXAMPLES "Build examples" OFF)

add_library(lws
        3dparty/sha1/sha1.hpp
        include/server/HttpRequest.hpp
        include/server/HttpResponse.hpp
        include/server/SocketListener.hpp
        include/server/Threadpool.hpp
        include/server/WebServer.hpp
        include/server/WebSocket.hpp
        include/server/WSApplication.hpp
        src/HttpRequest.cpp
        src/HttpResponse.cpp
        src/SocketListener.cpp
        src/Threadpool.cpp
        src/WebServer.cpp
        src/WebSocket.cpp
        src/WSApplication.cpp
)

target_include_directories(lws PUBLIC include ${CMAKE_SOURCE_DIR}/3dparty/sha1)

if (ENABLE_BENCHMARK)
    add_subdirectory(benchmark)
endif ()

if (ENABLE_TESTS)
    add_subdirectory(tests)
endif ()

if (BUILD_EXAMPLES)
    add_subdirectory(examples)
endif ()
