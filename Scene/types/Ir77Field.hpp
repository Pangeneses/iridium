#pragma once

#include <glm/glm.hpp>

#include <array>
#include <cstdint>
#include <string>

namespace NSIr77Scene {

// Everything field.compile carries. Ranges map element ids to Ir77FieldIn.coord; a surface usually puts coord.x / coord.y on world X / Z
// and its height on Y, so z doubles as the surface's height range for the shadow box.
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

}  // namespace NSIr77Scene
