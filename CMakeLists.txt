cmake_minimum_required(VERSION 3.26)
project(web_server)

set(CMAKE_CXX_STANDARD 20)

include(FetchContent)

add_executable(web_server main.cpp src/WebServer.cpp src/HttpRequest.cpp)
target_include_directories(web_server PRIVATE include)
target_link_libraries(web_server pthread)

