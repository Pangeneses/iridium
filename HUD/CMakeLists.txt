cmake_minimum_required(VERSION 3.28)

project(Ir77 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE Ir77_SOURCES "src/*.cpp" "src/*.ixx" "header/*.hpp")

add_executable(Ir77 ${Ir77_SOURCES})

target_include_directories(Ir77 PRIVATE header)

# Enable C++20 modules
target_compile_features(Ir77 PRIVATE cxx_std_20)

add_library(HUD MODULE
    src/hud.cpp
)

target_link_libraries(HUD
    PRIVATE
        cef
        capnp
)


