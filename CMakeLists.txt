cmake_minimum_required(VERSION 3.22)
project(web-server)

set(CMAKE_CXX_STANDARD 20)
option(ENABLE_BENCHMARK "Enable benchmark" OFF)
option(ENABLE_TESTS "Enable tests" OFF)
option(BUILD_EXAMPLES "Build examples" OFF)

#find_package(Threads REQUIRED)

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

if (BUILD_EXAMPLES)
    add_subdirectory(examples)
endif ()
