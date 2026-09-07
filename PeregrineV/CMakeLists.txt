cmake_minimum_required(VERSION 4.2.3)
project(PeregrineV LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE PV_SOURCES
    "*.cpp"
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
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime
    ${CMAKE_CURRENT_SOURCE_DIR}/server
)

# Vulkan
find_package(Vulkan REQUIRED)
target_include_directories(PeregrineV PUBLIC ${Vulkan_INCLUDE_DIRS})
target_link_libraries(PeregrineV PUBLIC Vulkan::Vulkan)

include(FetchContent)

# VMA
FetchContent_Declare(
    vma
    GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
    GIT_TAG        v3.4.0
)

FetchContent_MakeAvailable(vma)

target_include_directories(PeregrineV PUBLIC ${vma_SOURCE_DIR}/include)

FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        1.0.1
)

FetchContent_MakeAvailable(glm)

target_link_libraries(PeregrineV PUBLIC glm::glm)

# SDL3
find_package(SDL3 REQUIRED)
target_link_libraries(PeregrineV PUBLIC SDL3::SDL3)

target_link_libraries(PeregrineV PUBLIC Ir77RT)
