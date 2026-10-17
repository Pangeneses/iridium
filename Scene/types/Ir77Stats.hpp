#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "../PeregrineV/types/Ir77PVTypes.hpp" // Ir77PVLight -- same include iridium.hpp uses

namespace NSIr77Scene {

// One playing clip, as of this frame
typedef struct Ir77ClipStat {
    std::string name{};
    float time{0.0f};      // seconds into the clip
    float duration{0.0f};  // seconds
    float speed{1.0f};
    bool loop{true};
}* pIr77ClipStat;

// Filled once by LoadScene. The app reads it with iridium::GetSceneStats.
typedef struct Ir77SceneStats {
    std::string path{};
    std::string gpu{};          // VkPhysicalDeviceProperties::deviceName
    std::string render_path{};  // "mesh shading indirect" / "vertex indirect"

    // what the glTF holds
    std::uint32_t nodes{0};
    std::uint32_t meshes{0};
    std::uint32_t materials{0};
    std::uint32_t textures{0};
    std::uint32_t cameras{0};
    std::uint32_t skins{0};
    std::uint32_t bones{0};  // joint matrices across every skin
    std::uint32_t clips{0};

    // what was uploaded
    std::uint64_t static_vertices{0};
    std::uint64_t static_indices{0};
    std::uint64_t skinned_vertices{0};
    std::uint64_t skinned_indices{0};
    std::uint32_t static_primitives{0};
    std::uint32_t skinned_primitives{0};
    std::uint32_t instances{0};
    std::uint32_t opaque_runs{0};
    std::uint32_t skinned_runs{0};
    std::uint32_t transparent_objects{0};

    std::vector<std::string> clip_names{};
    std::vector<NSIr77PeregrineV::Ir77PVLight> lights{};  // as uploaded -- includes the default sun when the file has none
    std::vector<std::string> warnings{};
}* pIr77SceneStats;

// Updated every frame by VulkanFrameStart. The app reads it with iridium::GetFrameStats.
typedef struct Ir77FrameStats {
    std::uint64_t frame{0};
    float dt_ms{0.0f};
    float fps{0.0f};       // smoothed
    float scene_ms{0.0f};  // Scene::Update + UploadScene (CPU)
    float draw_ms{0.0f};   // Paint::Draw -- includes the fence wait, so GPU-bound frames show up here
    std::uint32_t opaque_items{0};
    std::uint32_t transparent_items{0};
    std::uint32_t camera_index{0};
    bool orbit_camera{false};
    std::vector<Ir77ClipStat> playing{};
}* pIr77FrameStats;

}  // namespace Ir77