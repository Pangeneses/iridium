#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include <fastgltf/core.hpp>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

using namespace NSIr77RT;

namespace NSIr77Scene {

// Scene-side animation clips: one per glTF animation (same index as asset.animations). Read-only after load; Sample() writes a clip
// at a time into a pose (per-node local TRS), which the scene's world pass turns into world matrices.
typedef struct IIr77Animation : virtual public IIr77Enlisted {
    IIr77Animation() = default;

    virtual std::shared_ptr<IIr77Return const> LoadAnimationsFromAsset(fastgltf::Asset const& asset) = 0;

    virtual std::shared_ptr<IIr77Return const> GetClipCount(std::uint32_t& count) = 0;

    // By the glTF animation name (the Blender action name)
    virtual std::shared_ptr<IIr77Return const> GetClipIndex(std::string const& name, std::uint32_t& index) = 0;

    // Seconds -- the last key time across the clip's tracks
    virtual std::shared_ptr<IIr77Return const> GetClipDuration(std::uint32_t const& index, float& duration) = 0;

    // The glTF animation name (the Blender action name)
    virtual std::shared_ptr<IIr77Return const> GetClipName(std::uint32_t const& index, std::string& name) = 0;

    // Writes clip `index` at `time` into the pose arrays (indexed by glTF node). Untouched slots keep what the caller put there.
    virtual std::shared_ptr<IIr77Return const> Sample(std::uint32_t const& index, float const& time, std::vector<glm::vec3>& translations,
                                                      std::vector<glm::quat>& rotations, std::vector<glm::vec3>& scales) = 0;

    virtual ~IIr77Animation() = default;
}* pIIr77Animation;
}  // namespace NSIr77Scene