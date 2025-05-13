include(FetchContent)

FetchContent_Declare(
        httplib
        GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
        GIT_TAG v0.20.1
)

FetchContent_MakeAvailable(httplib)


include(FetchContent)

FetchContent_Declare(
        crow
        GIT_REPOSITORY https://github.com/CrowCpp/crow.git
        GIT_TAG v1.2.1
)

FetchContent_MakeAvailable(crow)

add_executable(web_server_benchmark_tests benchmark_runner.cpp custom_server.hpp httplib_server.hpp
        utils.hpp
        crow_server.hpp)

target_link_libraries(web_server_benchmark_tests PRIVATE httplib server-core Crow::Crow)
