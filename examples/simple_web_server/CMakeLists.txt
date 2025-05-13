cmake_minimum_required(VERSION 3.22)

add_executable(web_server main.cpp)

target_link_libraries(web_server custom-web-server)
