include(FetchContent)

FetchContent_Declare(
        httplib
        GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
        GIT_TAG v0.15.3
)

FetchContent_MakeAvailable(httplib)

add_executable(web_server_benchmark_tests benchmark_runner.cpp custom_server.hpp httplib_server.hpp
        utils.hpp)
target_link_libraries(web_server_benchmark_tests PRIVATE httplib custom-web-server)
