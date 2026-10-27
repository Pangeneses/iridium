#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "IIr77Camera.hpp"
#include "IIr77Light.hpp"
#include "IIr77Mesh.hpp"
#include "IIr77Material.hpp"
#include "IIr77Texture.hpp"
#include "IIr77Animation.hpp"

using namespace NSIr77RT;

namespace NSIr77Scene {

// The loaded glTF and its live node tree -- components, clip playback, world matrices and joint palettes. CPU only.
//   ParseAsset -> [Play* / Stop*] -> per frame: Update(dt) -> WriteInstanceTransforms / WritePackedBones
typedef struct IIr77Pose : virtual public IIr77Enlisted {
    IIr77Pose() = default;

    // An empty path is an empty scene: no geometry, no nodes, no clips -- the default sun, the field and the cameras still run.
    virtual std::shared_ptr<IIr77Return const> ParseAsset(const std::string& path) = 0;

    // Starts clip `clip` from the beginning, alongside whatever else is playing. Already playing -> restarts it with the new settings.
    // loop = wrap at the end, otherwise hold the last frame. Clips are sampled in the order they were started; where two drive the
    // same node, the later one wins.
    virtual std::shared_ptr<IIr77Return const> PlayAnimation(std::uint32_t const& clip, bool const& loop, float const& speed = 1.0f) = 0;

    // By name -- the glTF animation / Blender action name
    virtual std::shared_ptr<IIr77Return const> PlayAnimation(std::string const& name, bool const& loop, float const& speed = 1.0f) = 0;

    // Every clip in the file at once -- what a "Mode: Actions" export of several animated objects needs
    virtual std::shared_ptr<IIr77Return const> PlayAllAnimations(bool const& loop, float const& speed = 1.0f) = 0;

    // Stops one clip; the nodes it drove go back to rest on the next Update
    virtual std::shared_ptr<IIr77Return const> StopAnimation(std::uint32_t const& clip) = 0;

    virtual std::shared_ptr<IIr77Return const> StopAnimation(std::string const& name) = 0;

    // Everything back to the rest pose, nothing playing
    virtual std::shared_ptr<IIr77Return const> StopAllAnimations() = 0;

    // Where the whole file sits in the world -- above its root nodes, so everything it draws moves with it. Next Update.
    virtual std::shared_ptr<IIr77Return const> SetRootTransform(glm::mat4 const& root) = 0;

    // One node's rest TRS (its parent's space). A clip that drives the node still overrides the channels it animates. Next Update.
    virtual std::shared_ptr<IIr77Return const> SetNodeTransform(std::uint32_t const& node, glm::vec3 const& translation, glm::quat const& rotation,
                                                                glm::vec3 const& scale) = 0;

    virtual std::shared_ptr<IIr77Return const> GetNodeCount(std::uint32_t& count) = 0;

    // One node as the file names it, its parent (UINT32_MAX = a root) and its rest TRS
    virtual std::shared_ptr<IIr77Return const> GetNode(std::uint32_t const& node, std::string& name, std::uint32_t& parent, glm::vec3& translation,
                                                       glm::quat& rotation, glm::vec3& scale) = 0;

    // Once per frame: advances animation, rebuilds every world matrix, then every joint palette. Afterwards the frame's results are read out
    // (WriteInstanceTransforms, WritePackedBones).
    virtual std::shared_ptr<IIr77Return const> Update(float const& dt) = 0;

    // Copies mesh-node world matrices into their models[] slots
    virtual std::shared_ptr<IIr77Return const> WriteInstanceTransforms(std::vector<glm::mat4>& models) = 0;

    // Every skinned node's joint palette, back to back -- what the Bones buffer holds. Node k's joints start at its bone base.
    virtual std::shared_ptr<IIr77Return const> WriteBoneMatrices(std::vector<glm::mat4>& bones) = 0;

    // Per models[] slot: where that instance's joint palette starts in the bones array (0 for unskinned instances)
    virtual std::shared_ptr<IIr77Return const> WriteInstanceBoneBases(std::vector<std::uint32_t>& bases, std::size_t const& instance_count) = 0;

    // bone_base[BONE_BASE_SLOTS] then every joint palette -- the layout skinned.vert reads at set 0 binding 4. instance_count = models[] slots;
    // bases past BONE_BASE_SLOTS and palettes past BONE_CAPACITY are cut.
    virtual std::shared_ptr<IIr77Return const> WritePackedBones(std::size_t const& instance_count, std::vector<std::uint8_t>& packed) = 0;

    // components
    virtual std::shared_ptr<IIr77Return const> GetMeshes(std::shared_ptr<IIr77Mesh>& meshes) = 0;

    virtual std::shared_ptr<IIr77Return const> GetCameras(std::shared_ptr<IIr77Camera>& cameras) = 0;

    virtual std::shared_ptr<IIr77Return const> GetLights(std::shared_ptr<IIr77Light>& lights) = 0;

    virtual std::shared_ptr<IIr77Return const> GetMaterials(std::shared_ptr<IIr77Material>& materials) = 0;

    virtual std::shared_ptr<IIr77Return const> GetTextures(std::shared_ptr<IIr77Texture>& textures) = 0;

    virtual std::shared_ptr<IIr77Return const> GetAnimations(std::shared_ptr<IIr77Animation>& animations) = 0;

    // stats -- counts from the loaded file
    virtual std::shared_ptr<IIr77Return const> GetAssetCounts(std::uint32_t& nodes, std::uint32_t& meshes, std::uint32_t& materials, std::uint32_t& textures, std::uint32_t& cameras, std::uint32_t& skins, std::uint32_t& bones, std::uint32_t& clips) = 0;

    // stats -- the clips currently playing
    virtual std::shared_ptr<IIr77Return const> GetPlayerCount(std::uint32_t& count) = 0;

    virtual std::shared_ptr<IIr77Return const> GetPlayer(std::uint32_t const& index, std::uint32_t& clip, float& time, float& duration, float& speed, bool& loop) = 0;

    virtual ~IIr77Pose() = default;
}* pIIr77Pose;
}  // namespace NSIr77Scene
