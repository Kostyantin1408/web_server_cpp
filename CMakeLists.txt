cmake_minimum_required(VERSION 3.22)
project(web-server)

set(CMAKE_CXX_STANDARD 20)
option(ENABLE_BENCHMARK "Enable benchmark" OFF)
option(ENABLE_TESTS "Enable tests" OFF)
option(BUILD_EXAMPLES "Build examples" OFF)

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
        include/server/WebServer.hpp
        include/server/HttpRequest.hpp
        src/SocketListener.cpp
        include/server/Threadpool.hpp
        src/threadpool.cpp)

target_include_directories(server-core PUBLIC include ${CMAKE_SOURCE_DIR}/3dparty/sha1)

if (ENABLE_BENCHMARK)
    add_subdirectory(benchmark)
endif ()

if (ENABLE_TESTS)
    add_subdirectory(tests)
endif ()

if (BUILD_EXAMPLES)
    add_subdirectory(examples)
endif ()
