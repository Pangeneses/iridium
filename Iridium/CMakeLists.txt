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

find_package(Vulkan REQUIRED)
target_link_libraries(Ir77 PRIVATE Vulkan::Vulkan)

# SDL3
find_package(SDL3 REQUIRED)
target_link_libraries(Ir77 PRIVATE SDL3::SDL3)

# --- CEF ---
set(CEF_ROOT "$ENV{HOME}/workspace/cef")

set(CEF_WRAPPER_LIB "${CEF_ROOT}/build/libcef_dll_wrapper/libcef_dll_wrapper.a")

# CEF headers
target_include_directories(Ir77 PRIVATE 
    ${CEF_ROOT}
    ${CEF_ROOT}/include)

# Link engine + wrapper
target_link_libraries(Ir77 PRIVATE
    ${CEF_WRAPPER_LIB}
    ${CEF_ROOT}/Release/libcef.so
)

target_compile_definitions(Ir77 PRIVATE WRAPPING_CEF_SHARED)

# Copy runtime resources next to the executable
add_custom_command(TARGET Ir77 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CEF_ROOT}/Release/libcef.so $<TARGET_FILE_DIR:Ir77>
    COMMAND ${CMAKE_COMMAND} -E copy ${CEF_ROOT}/Release/v8_context_snapshot.bin $<TARGET_FILE_DIR:Ir77>
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CEF_ROOT}/Resources $<TARGET_FILE_DIR:Ir77>
)

target_link_libraries(Ir77 PRIVATE
    -Wl,--whole-archive
    Ir77RT
    -Wl,--no-whole-archive
    PeregrineV
    -Wl,--no-whole-archive
    Windows
)