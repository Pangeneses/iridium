cmake_minimum_required(VERSION 4.2.3)
project(Windows LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE PV_SOURCES
    "*.cpp"
    "operation/*.cpp"
    "runtime/*.cpp"
)

add_library(Windows STATIC ${PV_SOURCES})

target_compile_options(Windows PRIVATE
    -fno-implicit-modules
    -fno-implicit-module-maps
)

target_include_directories(Windows PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/dictionary
    ${CMAKE_CURRENT_SOURCE_DIR}/interface
    ${CMAKE_CURRENT_SOURCE_DIR}/operation
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime
    ${CMAKE_CURRENT_SOURCE_DIR}/service
)

# SDL3
find_package(SDL3 REQUIRED)
target_link_libraries(Ir77 PRIVATE SDL3::SDL3)

# CEF
set(CEF_ROOT "$ENV{HOME}/workspace/cef")
set(CEF_WRAPPER_LIB "${CEF_ROOT}/build/libcef_dll_wrapper/libcef_dll_wrapper.a")
target_include_directories(Windows PRIVATE ${CEF_ROOT})
target_link_libraries(Windows PRIVATE
    ${CEF_WRAPPER_LIB}
    ${CEF_ROOT}/Release/libcef.so
)

# Ir77RT
target_include_directories(Windows PUBLIC
    ${CMAKE_SOURCE_DIR}/Ir77RT/interface
    ${CMAKE_SOURCE_DIR}/Ir77RT/runtime
    ${CMAKE_SOURCE_DIR}/Ir77RT/dictionary
    ${CMAKE_SOURCE_DIR}/Ir77RT/service
)
target_link_libraries(Windows PUBLIC Ir77RT)