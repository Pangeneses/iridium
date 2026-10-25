cmake_minimum_required(VERSION 4.2.3)
project(Ir77 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE Ir77_SOURCES
    "*.cpp"
)

add_executable(Ir77 ${Ir77_SOURCES})

set_target_properties(Ir77 PROPERTIES
    BUILD_RPATH "$ORIGIN;/usr/lib/x86_64-linux-gnu"
    INSTALL_RPATH "$ORIGIN;/usr/lib/x86_64-linux-gnu"
    BUILD_RPATH_USE_ORIGIN TRUE
)

target_compile_options(Ir77 PRIVATE
    -fno-implicit-modules
    -fno-implicit-module-maps
)

target_include_directories(Ir77 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Vulkan
find_package(Vulkan REQUIRED)
target_link_libraries(Ir77 PRIVATE Vulkan::Vulkan)

# SDL3
find_package(SDL3 REQUIRED)
target_link_libraries(Ir77 PRIVATE SDL3::SDL3)

# --- CEF ---
set(CEF_ROOT "$ENV{HOME}/workspace/cef")

set(CEF_WRAPPER_LIB "${CEF_ROOT}/build/libcef_dll_wrapper/libcef_dll_wrapper.a")

target_include_directories(Ir77 PRIVATE
    ${CEF_ROOT}
    ${CEF_ROOT}/include
)

target_link_libraries(Ir77 PRIVATE
    ${CEF_WRAPPER_LIB}
    ${CEF_ROOT}/Release/libcef.so
)

target_compile_definitions(Ir77 PRIVATE WRAPPING_CEF_SHARED)

FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
    SYSTEM
)
FetchContent_MakeAvailable(nlohmann_json)

target_link_libraries(Ir77 PRIVATE nlohmann_json::nlohmann_json)

# Copy CEF's runtime next to the executable -- every .so / .bin / .json in Release, plus Resources
add_custom_command(TARGET Ir77 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CEF_ROOT}/Release $<TARGET_FILE_DIR:Ir77>
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CEF_ROOT}/Resources $<TARGET_FILE_DIR:Ir77>
)

# --- engine libraries ---
# Ir77RT is whole-archived (keeps every object even if nothing references it); the rest link normally.
# Ir77Scene brings fastgltf, glm and stb_image with it (PUBLIC in its own CMakeLists).
target_link_libraries(Ir77 PRIVATE
    -Wl,--whole-archive
    Ir77RT
    -Wl,--no-whole-archive
    PeregrineV
    Windows
    Ir77Scene
)

# iridium.json next to the executable. Development: a link, so edits apply on the next run without a build.
# Distribution (-DIR77_LINK_CONFIG=OFF): a real copy, refreshed by every build.
option(IR77_LINK_CONFIG "Link iridium.json into the build instead of copying it" ON)

file(REMOVE ${CMAKE_CURRENT_BINARY_DIR}/iridium.json)

if (IR77_LINK_CONFIG)
    file(CREATE_LINK ${CMAKE_CURRENT_SOURCE_DIR}/iridium.json ${CMAKE_CURRENT_BINARY_DIR}/iridium.json SYMBOLIC)
else ()
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/iridium.json ${CMAKE_CURRENT_BINARY_DIR}/iridium.json COPYONLY)
endif ()