cmake_minimum_required(VERSION 4.2.3)
project(PeregrineV LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE PV_SOURCES
    "*.cpp"
    "operation/*.cpp"
    "runtime/*.cpp"
)

add_library(PeregrineV STATIC ${PV_SOURCES})

target_compile_options(PeregrineV PRIVATE
    -fno-implicit-modules
    -fno-implicit-module-maps
)

target_include_directories(PeregrineV PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/dictionary
    ${CMAKE_CURRENT_SOURCE_DIR}/interface
    ${CMAKE_CURRENT_SOURCE_DIR}/operation
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime
    ${CMAKE_CURRENT_SOURCE_DIR}/service
)

# Vulkan
find_package(Vulkan REQUIRED)
target_include_directories(PeregrineV PUBLIC ${Vulkan_INCLUDE_DIRS})
target_link_libraries(PeregrineV PUBLIC Vulkan::Vulkan)

# SDL3
find_package(SDL3 REQUIRED)
target_link_libraries(Ir77 PRIVATE SDL3::SDL3)

# Ir77RT
target_include_directories(PeregrineV PUBLIC
    ${CMAKE_SOURCE_DIR}/Ir77RT/interface
    ${CMAKE_SOURCE_DIR}/Ir77RT/runtime
    ${CMAKE_SOURCE_DIR}/Ir77RT/dictionary
    ${CMAKE_SOURCE_DIR}/Ir77RT/service
)
target_link_libraries(PeregrineV PUBLIC Ir77RT)