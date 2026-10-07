cmake_minimum_required(VERSION 4.2.3)
project(PeregrineV LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# -----------------------------------------------------------------------------
# library
# -----------------------------------------------------------------------------
file(GLOB_RECURSE PV_SOURCES CONFIGURE_DEPENDS
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

# -----------------------------------------------------------------------------
# system packages
# -----------------------------------------------------------------------------
find_package(Vulkan REQUIRED)
find_package(SDL3 REQUIRED)

# -----------------------------------------------------------------------------
# fetched dependencies
# -----------------------------------------------------------------------------
FetchContent_Declare(
    vma
    GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
    GIT_TAG        v3.4.0
)

FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        1.0.1
)

FetchContent_Declare(
    meshoptimizer
    GIT_REPOSITORY https://github.com/zeux/meshoptimizer.git
    GIT_TAG        v0.23
)

FetchContent_MakeAvailable(vma glm meshoptimizer)

# -----------------------------------------------------------------------------
# linkage -- PUBLIC because the headers that use these are included by consumers
# -----------------------------------------------------------------------------
target_include_directories(PeregrineV PUBLIC ${vma_SOURCE_DIR}/include)

target_link_libraries(PeregrineV PUBLIC
    Vulkan::Vulkan
    SDL3::SDL3
    glm::glm
    meshoptimizer
    Ir77RT
)