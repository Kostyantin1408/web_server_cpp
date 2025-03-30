cmake_minimum_required(VERSION 3.26)
project(websocket__)

set(CMAKE_CXX_STANDARD 20)

include(FetchContent)

FetchContent_Declare(
        websocketpp
        GIT_REPOSITORY https://github.com/toonetown/websocketpp.git
        GIT_TAG 0a12426ddcf0f431b56ef73c3e00d1065fdde465
)
FetchContent_MakeAvailable(websocketpp)

find_package(Boost REQUIRED COMPONENTS system thread)
if(NOT Boost_FOUND)
    message(FATAL_ERROR "Boost not found!")
endif()

#include_directories(${websocketpp_SOURCE_DIR})
include_directories(${Boost_INCLUDE_DIRS} ${websocketpp_SOURCE_DIR})

add_executable(websocket__ main.cpp src/WebServer.cpp)
target_include_directories(websocket__ PRIVATE include)
#target_link_libraries(websocket__ pthread)
target_link_libraries(websocket__ ${Boost_LIBRARIES} pthread)

