cmake_minimum_required(VERSION 4.2.3)
project(Ir77Scene LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE Ir77Scene_SOURCES
    "*.cpp"
)

add_library(Ir77Scene STATIC ${Ir77Scene_SOURCES})

# Ir77RT
target_include_directories(Ir77Scene PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/dictionary
    ${CMAKE_CURRENT_SOURCE_DIR}/interface
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime
    ${CMAKE_CURRENT_SOURCE_DIR}/server
)

target_compile_options(Ir77Scene PRIVATE
    -fno-implicit-modules
    -fno-implicit-module-maps
)

# Vulkan
find_package(Vulkan REQUIRED)
target_include_directories(Ir77Scene PUBLIC ${Vulkan_INCLUDE_DIRS})
target_link_libraries(Ir77Scene PUBLIC Vulkan::Vulkan)

include(FetchContent)

# VMA
FetchContent_Declare(
    vma
    GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
    GIT_TAG        v3.4.0
)

FetchContent_MakeAvailable(vma)

target_include_directories(Ir77Scene PUBLIC ${vma_SOURCE_DIR}/include)

FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        1.0.1
)

FetchContent_MakeAvailable(glm)

target_link_libraries(Ir77Scene PUBLIC glm::glm)

FetchContent_Declare(
    fastgltf
    GIT_REPOSITORY https://github.com/spnda/fastgltf.git
    GIT_TAG        v0.8.0
)

FetchContent_MakeAvailable(fastgltf)

target_link_libraries(Ir77Scene PUBLIC fastgltf::fastgltf)

# SDL3
find_package(SDL3 REQUIRED)
target_link_libraries(Ir77Scene PRIVATE SDL3::SDL3)

target_link_libraries(Ir77Scene PUBLIC Ir77RT)