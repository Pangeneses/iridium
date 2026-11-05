#pragma once

#include <glm/glm.hpp>

#include <array>
#include <cstdint>
#include <string>

namespace NSIr77Fields {

// =============================================================================================================================================
// settings -- everything field.compile carries
// =============================================================================================================================================

// Ranges map element ids to Ir77FieldIn.coord; a surface usually puts coord.x / coord.y on world X / Z and its height on Y, so z doubles as
// the surface's height range for the shadow box.
struct Ir77FieldSettings {
    std::string source{};
    std::uint32_t topology{0};  // 0 surface (z = 0 slice of the grid), 1 particles
    glm::uvec3 dims{128, 128, 1};
    glm::vec2 x{-4.0f, 4.0f};
    glm::vec2 y{-4.0f, 4.0f};
    glm::vec2 z{-1.0f, 1.0f};
    glm::vec2 value{-1.0f, 1.0f};  // colour map range of Ir77FieldPoint.data.x
    std::uint32_t shading{0};       // 0 lit colour, 1 colour map, 2 colour map + contours
    std::uint32_t colormap{0};      // 0 viridis, 1 turbo, 2 grey
    float contour_spacing{0.1f};
    float contour_width{1.0f};  // pixels
    std::array<float, 8> params{};
    std::uint32_t canvas_size{2048};  // first compile with a Pen sets it; later sizes are ignored
    std::uint32_t pens{1};
    std::array<glm::vec4, 8> pen_color{glm::vec4(1.0f), glm::vec4(1.0f), glm::vec4(1.0f), glm::vec4(1.0f),
                                       glm::vec4(1.0f), glm::vec4(1.0f), glm::vec4(1.0f), glm::vec4(1.0f)};
    std::array<float, 8> pen_width{2.0f, 2.0f, 2.0f, 2.0f, 2.0f, 2.0f, 2.0f, 2.0f};  // pixels
    std::uint32_t pen_target{0};  // 0 canvas (a picture over the surface, width in pixels), 1 points (the grid's own point ink, width in cells)
    float pen_speed{1.0f};        // pen t per second
    float sweep{0.0f};            // > 0: oscilloscope -- domain widths per unit of t; 0: spirograph
    float decay{0.0f};            // canvas fade per second; 0 = permanent
    bool wrap_clear{false};       // oscilloscope: blank the canvas at each wrap instead of fading
    std::uint32_t steps{256};     // pen samples per frame
};

// =============================================================================================================================================
// GPU layouts -- std430, must match include/field.glsl
// =============================================================================================================================================

// One element of the point buffer
struct Ir77FieldPointGPU {
    glm::vec4 position_size{0.0f};
    glm::vec4 color{0.0f};
    glm::vec4 data{0.0f};
};

static_assert(sizeof(Ir77FieldPointGPU) == 48, "must match Ir77FieldPoint in field.glsl");

// The config row -- the settings as the shaders read them (pc.user[4])
struct Ir77FieldConfig {
    glm::vec4 domain_x{-1.0f, 1.0f, -1.0f, 1.0f};
    glm::vec4 domain_z{-1.0f, 1.0f, -1.0f, 1.0f};
    glm::uvec4 dims{2u, 2u, 1u, 0u};
    glm::uvec4 shading{0u};
    glm::vec4 contour{0.1f, 1.0f, 0.0f, 0.0f};
    glm::uvec4 canvas{0u};
    glm::vec4 trace{1.0f, 0.0f, 0.0f, 0.0f};
    glm::uvec4 steps{256u, 0u, 0u, 0u};
    std::array<glm::vec4, 2> params{};
    std::array<glm::vec4, 8> pen_color{};
    std::array<glm::vec4, 2> pen_width{};
    glm::uvec4 ink{0u};  // x point-ink slot, y pen target (0 canvas, 1 points)
    glm::uvec4 values{0u};  // x data slot (field.data -- the config's own when there's none), y how many floats
};

static_assert(sizeof(Ir77FieldConfig) == 352, "must match Ir77FieldConfig in field.glsl");

// =============================================================================================================================================
// limits
// =============================================================================================================================================

inline constexpr std::uint64_t FIELD_MAX_POINTS{16u * 1024u * 1024u};

// field.data -- the most floats a program's data buffer holds (64 MB)
inline constexpr std::uint64_t FIELD_MAX_DATA{16u * 1024u * 1024u};

// a surface past this stays out of the rays (its grid index copy: 96 MB)
inline constexpr std::uint64_t FIELD_MAX_RAY_CELLS{4u * 1024u * 1024u};

// field_compute.glsl's jobs -- pc.user[2] of a field dispatch
inline constexpr std::uint32_t FIELD_JOB_POINTS{0};
inline constexpr std::uint32_t FIELD_JOB_CANVAS{1};
inline constexpr std::uint32_t FIELD_JOB_PEN{2};

// 64 threads a group (local_size_x in field_compute.glsl)
inline constexpr std::uint64_t FIELD_GROUP_SIZE{64};

}  // namespace NSIr77Fields
