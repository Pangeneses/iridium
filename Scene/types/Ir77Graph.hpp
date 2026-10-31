#pragma once

#include <cstdint>
#include <string>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "../../PeregrineV/types/Ir77PVTypes.hpp"

using namespace NSIr77PeregrineV;

namespace NSIr77Scene {

// =============================================================================================================================================
// The scene graph -- everything in a scene is an entity. The scene itself is the root: it isn't an entity, it holds the project's metadata,
// and its children are the entities whose parent is ''. An entity is the same core for every kind, plus that kind's payload:
//   collection  a container -- its children sit relative to it (it starts on the origin)
//   model       one .glb of the project's Models/ -- geometry, materials, skin, clips. Several entities may share one file (instances).
//   light       directional / point / spot -- it shines down its local -Z
//   camera      a lens -- stored with the scene; looking through one comes with world cameras
// Any entity may parent any other (a wheel under its car); transforms compose down the tree, visibility too (a hidden collection hides
// everything under it). Removing an entity removes everything under it.
// =============================================================================================================================================
enum class Ir77GraphKind : std::uint8_t { Collection, Model, Light, Camera };

inline constexpr std::uint32_t GRAPH_NO_RUNTIME{UINT32_MAX};  // nothing behind it in Animate (a collection, a camera)

inline constexpr std::size_t GRAPH_ID_LENGTH{16};  // 64-bit, lower-case hex

struct Ir77GraphCamera {
    float fov{60.0f};  // vertical, degrees
    bool ortho{false};
    float near{0.1f};
    float far{1000.0f};
};

struct Ir77GraphEntity {
    // core -- every kind
    std::string id{};      // 16 hex digits, made once (Ir77Graph::NewId) and kept through save and load
    std::string name{};
    Ir77GraphKind kind{Ir77GraphKind::Collection};
    std::string parent{};  // '' = the scene root
    std::uint32_t order{0};  // among its siblings -- the outliner's order
    bool visible{true};
    glm::vec3 t{0.0f};  // relative to the parent
    glm::quat r{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 s{1.0f};

    // payloads
    std::string model{};       // model -- <project>/Models/<model>.glb
    Ir77PVLight light{};       // light -- type, color, intensity (glTF units), range, cones
    Ir77GraphCamera camera{};  // camera

    // what Animate has behind it -- a model's entity, a light's world light; handed out again on every load, never stored
    std::uint32_t runtime{GRAPH_NO_RUNTIME};
};

inline char const* GraphKindName(Ir77GraphKind const kind) {
    switch (kind) {
        case Ir77GraphKind::Model: return "model";
        case Ir77GraphKind::Light: return "light";
        case Ir77GraphKind::Camera: return "camera";
        default: return "collection";
    }
}

// "collection" | "model" | "light" | "camera" -> kind; false for anything else
inline bool GraphKindFromName(std::string_view const name, Ir77GraphKind& kind) {
    if (name == "collection") kind = Ir77GraphKind::Collection;
    else if (name == "model") kind = Ir77GraphKind::Model;
    else if (name == "light") kind = Ir77GraphKind::Light;
    else if (name == "camera") kind = Ir77GraphKind::Camera;
    else return false;

    return true;
}

}  // namespace NSIr77Scene
