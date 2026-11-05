cmake_minimum_required(VERSION 4.2.3)
project(Ir77Fields LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE Ir77Fields_SOURCES
    "*.cpp"
)

add_library(Ir77Fields STATIC ${Ir77Fields_SOURCES})

# Ir77RT
target_include_directories(Ir77Fields PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/dictionary
    ${CMAKE_CURRENT_SOURCE_DIR}/interface
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime
    ${CMAKE_CURRENT_SOURCE_DIR}/server
    ${CMAKE_CURRENT_SOURCE_DIR}/types
)

target_compile_options(Ir77Fields PRIVATE
    -fno-implicit-modules
    -fno-implicit-module-maps
)

# Vulkan
find_package(Vulkan REQUIRED)
target_include_directories(Ir77Fields PUBLIC ${Vulkan_INCLUDE_DIRS})
target_link_libraries(Ir77Fields PUBLIC Vulkan::Vulkan)

include(FetchContent)

# glm -- same declaration as Scene's; whichever module is configured first fetches it, the other reuses it
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        1.0.1
)

FetchContent_MakeAvailable(glm)

target_link_libraries(Ir77Fields PUBLIC glm::glm)

target_link_libraries(Ir77Fields PUBLIC Ir77RT)

target_link_libraries(Ir77Fields PUBLIC PeregrineV)
