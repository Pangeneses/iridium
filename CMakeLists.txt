cmake_minimum_required(VERSION 4.2.3)
project(Iridium LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL3 REQUIRED)

add_subdirectory(Ir77RT)
add_subdirectory(Iridium)
add_subdirectory(Windows)
add_subdirectory(REDOS)
add_subdirectory(PeregrineV)

target_link_libraries(iridium PRIVATE SDL3::SDL3)
