cmake_minimum_required(VERSION 3.28)

project(Ir77TBasic LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE Ir77TBasic_SOURCES
    "*.cpp"
)

target_include_directories(Ir77TBasic
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/dictionary
    ${CMAKE_CURRENT_SOURCE_DIR}/interface
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime
    ${CMAKE_CURRENT_SOURCE_DIR}/server
)

target_compile_features(Ir77TBasic PRIVATE cxx_std_20)

add_subdirectory(Ir77TBasic)

target_link_libraries(Ir77TBasic PRIVATE
    -Wl,--whole-archive
    Ir77RT
)