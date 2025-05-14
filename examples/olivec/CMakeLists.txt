cmake_minimum_required(VERSION 3.22)

include(FetchContent)

FetchContent_Declare(
        json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(json)

add_executable(olivec olivec.cpp)
target_link_libraries(olivec PRIVATE lws nlohmann_json::nlohmann_json uuid)