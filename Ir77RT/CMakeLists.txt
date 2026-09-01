cmake_minimum_required(VERSION 4.2.3)

project(Ir77RT LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(Ir77RT STATIC
    service/Ir77RTService.cpp
)

target_include_directories(Ir77RT
    PUBLIC
        dictionary
        interface
        runtime
        service
)

target_compile_features(Ir77RT PRIVATE cxx_std_20)

include(FetchContent)

#FetchContent_Declare(
#    nlohmann_json
#    GIT_REPOSITORY https://github.com/nlohmann/json.git
#    GIT_TAG        v3.11.3
#)
#
#FetchContent_MakeAvailable(nlohmann_json)
#
#target_link_libraries(Ir77RT PUBLIC nlohmann_json::nlohmann_json)