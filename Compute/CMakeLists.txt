cmake_minimum_required(VERSION 4.2.3)
project(Ir77Compute LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE Ir77Compute_SOURCES
    "*.cpp"
)

add_library(Ir77Compute STATIC ${Ir77Compute_SOURCES})

target_include_directories(Ir77Compute PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/dictionary
    ${CMAKE_CURRENT_SOURCE_DIR}/interface
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime
    ${CMAKE_CURRENT_SOURCE_DIR}/server
    ${CMAKE_CURRENT_SOURCE_DIR}/services
    ${CMAKE_CURRENT_SOURCE_DIR}/types
)

target_compile_options(Ir77Compute PRIVATE
    -fno-implicit-modules
    -fno-implicit-module-maps
)

# OpenCL -- the SDK when it's installed; otherwise Khronos' headers and ICD loader, fetched (the GPU driver supplies the actual device)
find_package(OpenCL QUIET)
if(NOT OpenCL_FOUND)
    include(FetchContent)

    FetchContent_Declare(
        OpenCLHeaders
        GIT_REPOSITORY https://github.com/KhronosGroup/OpenCL-Headers.git
        GIT_TAG        v2025.07.22
    )

    FetchContent_Declare(
        OpenCLICDLoader
        GIT_REPOSITORY https://github.com/KhronosGroup/OpenCL-ICD-Loader.git
        GIT_TAG        v2025.07.22
    )

    # the headers first -- the loader builds against them
    FetchContent_MakeAvailable(OpenCLHeaders)
    set(OPENCL_ICD_LOADER_HEADERS_DIR ${openclheaders_SOURCE_DIR} CACHE PATH "" FORCE)
    set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(OpenCLICDLoader)
endif()

target_compile_definitions(Ir77Compute PUBLIC CL_TARGET_OPENCL_VERSION=120)
target_link_libraries(Ir77Compute PUBLIC OpenCL::OpenCL)

target_link_libraries(Ir77Compute PUBLIC Ir77RT)

# the pipe's JSON -- the simdjson::simdjson target is PeregrineV's fetch (configured before Compute)
target_link_libraries(Ir77Compute PUBLIC simdjson::simdjson)
