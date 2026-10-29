#pragma once

#include <glm/glm.hpp>

#include <array>
#include <cstdint>
#include <string>

namespace NSIr77Scene {

// What an import brings besides geometry, textures and animations when the op doesn't say -- a file's lights and cameras. Stripped: the scene
// graph places lights and cameras itself (Aurum, stored in the project's MongoDB).
inline constexpr bool SCENE_IMPORT_LIGHTS{false};

inline constexpr bool SCENE_IMPORT_CAMERAS{false};

}
