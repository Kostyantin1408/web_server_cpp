cmake_minimum_required(VERSION 3.22)
project(web-server)

set(CMAKE_CXX_STANDARD 20)
option(ENABLE_BENCHMARK "Enable benchmark" OFF)
option(ENABLE_TESTS "Enable tests" OFF)
option(BUILD_EXAMPLES "Build examples" OFF)

include(FetchContent)

add_library(custom-web-server src/WebServer.cpp src/HttpRequest.cpp
        include/HttpResponse.hpp include/HttpRequest.hpp include/WebServer.hpp include/sha1.hpp include/encryption.hpp
        src/HttpResponse.cpp
        src/HttpRequestReader.cpp
        include/HttpRequestReader.hpp)

target_include_directories(custom-web-server PUBLIC include)

#target_link_libraries(custom-web-server Threads::Threads)

if (ENABLE_BENCHMARK)
    add_subdirectory(benchmark)
endif ()

if (ENABLE_TESTS)
    add_subdirectory(tests)
endif ()
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

if (BUILD_EXAMPLES)
    add_subdirectory(examples)
endif ()
