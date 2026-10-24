#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <fastgltf/core.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/tools.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "../../Ir77RT/dictionary/IDIIr77MPVM.hpp"

#include "../dictionary/IDIIr77Scene.hpp"
#include "../dictionary/IDIr77Scene.hpp"

#include "../../Ir77RT/types/Ir77Types.hpp"

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"
#include "../../Ir77RT/runtime/Ir77Return.hpp"

#include "../interface/IIr77Camera.hpp"
#include "../interface/IIr77Light.hpp"
#include "../interface/IIr77Mesh.hpp"
#include "../interface/IIr77Material.hpp"
#include "../interface/IIr77Texture.hpp"
#include "../interface/IIr77Animation.hpp"
#include "../interface/IIr77Pose.hpp"

#include "./Ir77Camera.hpp"
#include "./Ir77Light.hpp"
#include "./Ir77Mesh.hpp"
#include "./Ir77Material.hpp"
#include "./Ir77Texture.hpp"
#include "./Ir77Animation.hpp"

using namespace NSIr77RT;

namespace NSIr77Scene {

// =============================================================================================================================================
// Ir77Pose -- the loaded glTF and its live node tree. Parses the file into its components (meshes, materials, textures, lights, cameras,
// animations), keeps every node's local TRS, plays clips into them, and rebuilds world matrices (parents first) and joint palettes each frame.
// CPU only -- it never sees the renderer. Owned by Ir77Animate, which reads the components and the frame's results out of it.
//
//   load:      ParseAsset(path) -> components (GetMeshes / GetCameras / ...)
//   playback:  PlayAnimation / PlayAllAnimations / StopAnimation / StopAllAnimations
//   per frame: Update(dt) -> WriteInstanceTransforms / WritePackedBones
// =============================================================================================================================================
class Ir77Pose : public Ir77Enlisted, public IIr77Pose, public std::enable_shared_from_this<Ir77Pose> {
   private:
    // one per glTF node -- local TRS is what animation writes; world is rebuilt every frame
    struct Ir77SceneNode {
        std::uint32_t parent{UINT32_MAX};  // UINT32_MAX = root
        glm::vec3 translation{0.0f};
        glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
        glm::vec3 scale{1.0f};
        glm::mat4 world{1.0f};
        std::uint32_t mesh{UINT32_MAX};           // glTF mesh on this node
        std::uint32_t mesh_ordinal{0};            // which instance of that mesh this node is
        std::uint32_t instance_slot{UINT32_MAX};  // its slot in models[]
        std::uint32_t skin{UINT32_MAX};           // glTF skin on this (mesh) node
        std::uint32_t bone_base{UINT32_MAX};      // first entry of its joint palette in the bones array
    };

    // one per glTF skin -- the joint nodes and their inverse bind matrices, same order
    struct Ir77SceneSkin {
        std::vector<std::uint32_t> joints{};
        std::vector<glm::mat4> inverse_bind{};
    };

    // one playing clip -- the scene runs any number of these at once; each drives the nodes its clip targets
    struct Ir77ScenePlayer {
        std::uint32_t clip{UINT32_MAX};
        float time{0.0f};
        float duration{0.0f};
        float speed{1.0f};
        bool loop{true};
    };

   public:
    Ir77Pose() {
        try {
            m_enlisted_uuid.Generate();
        } catch (std::invalid_argument a) {
            throw a;
        }

        m_enlisted = std::chrono::system_clock::now();
    }

   public:
    std::shared_ptr<IIr77Return const> EnlistedAs(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIIr77Enlisted>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> MemberUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77Pose>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CollectionUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77Scene>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    Ir77GUID QueryInterface(Ir77GUID iid, std::shared_ptr<void>& obj) {
        if (iid == GUIDIIr77Enlisted)
            obj = std::shared_ptr<IIr77Enlisted>(shared_from_this(), static_cast<IIr77Enlisted*>(this));

        else if (iid == GUIDIIr77Pose)
            obj = std::shared_ptr<IIr77Pose>(shared_from_this(), static_cast<IIr77Pose*>(this));

        else if (iid == GUIDIr77Pose)
            obj = std::shared_ptr<Ir77Pose>(shared_from_this(), static_cast<Ir77Pose*>(this));

        else
            return GUIDQueryFailed;

        return GUIDQuerySucceeded;
    }

   public:
    // An empty path is an empty scene: no geometry, no nodes, no clips -- the default sun, the field and the cameras still run.
    std::shared_ptr<IIr77Return const> ParseAsset(const std::string& path) {
        // fresh, empty components first -- an empty path, or a file that fails to load below, leaves an empty scene rather than half of the
        // previous one
        m_asset = fastgltf::Asset{};

        m_meshes = std::make_shared<Ir77Mesh>();
        m_lights = std::make_shared<Ir77Light>();
        m_cameras = std::make_shared<Ir77Camera>();
        m_materials = std::make_shared<Ir77Material>();
        m_textures = std::make_shared<Ir77Texture>();
        m_animations = std::make_shared<Ir77Animation>();

        m_nodes.clear();
        m_order.clear();
        m_mesh_instance_count.clear();
        m_skins.clear();
        m_bones.clear();
        m_bone_count = 0;
        m_rest_translations.clear();
        m_rest_rotations.clear();
        m_rest_scales.clear();
        m_players.clear();
        m_pose_dirty = false;

        if (path.empty()) return Ir77RETURN<Ir77OperationSucceeded>(this, "Ir77Scene: empty scene.");

        // extensions this loader handles -- a file that *requires* one not listed fails to parse; optional ones are ignored
        constexpr auto extensions =
            fastgltf::Extensions::KHR_lights_punctual | fastgltf::Extensions::KHR_mesh_quantization | fastgltf::Extensions::KHR_texture_transform;

        // LoadExternalBuffers / LoadExternalImages -- .bin and image files beside a .gltf are read into memory (.glb contents always are)
        // GenerateMeshIndices   -- non-indexed primitives get 0..n-1, so every primitive has an index accessor
        // DecomposeNodeMatrices -- matrix nodes become TRS, so every node stores (and animates) the same way
        constexpr auto options = fastgltf::Options::LoadExternalBuffers | fastgltf::Options::LoadExternalImages | fastgltf::Options::GenerateMeshIndices |
                                 fastgltf::Options::DecomposeNodeMatrices;

        fastgltf::Parser parser(extensions);

        auto data = fastgltf::GltfDataBuffer::FromPath(path);
        if (data.error() != fastgltf::Error::None) {
            auto const message = fastgltf::getErrorMessage(data.error());
            return Ir77RETURN<Ir77FileDoesntExist>(this, "Ir77Scene: failed to load glTF file " + path + ": " + std::string{message});
        }

        auto asset = parser.loadGltf(data.get(), std::filesystem::path(path).parent_path(), options);
        if (asset.error() != fastgltf::Error::None) {
            auto const message = fastgltf::getErrorMessage(asset.error());
            return Ir77RETURN<Ir77FileBadFormatting>(this, "Ir77Scene: failed to parse glTF asset " + path + ": " + std::string{message});
        }

        m_asset = std::move(asset.get());

        std::size_t const scene_index = m_asset.defaultScene.value_or(0);
        if (scene_index >= m_asset.scenes.size()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Scene: no valid scene.");

        const auto& scene = m_asset.scenes.at(scene_index);

        std::vector<bool> mesh_used(m_asset.meshes.size(), false);

        // node tree -- sized up front, filled by the walk
        m_nodes.assign(m_asset.nodes.size(), Ir77SceneNode{});

        for (auto root_index : scene.nodeIndices) {
            WalkNode(root_index, UINT32_MAX, glm::mat4(1.0f), mesh_used);
        }

        for (std::size_t i = 0; i < m_asset.meshes.size(); i++) {
            if (mesh_used[i]) m_meshes->AddMesh(m_asset, static_cast<std::uint32_t>(i));
        }

        m_meshes->PopulateMeshRanges();

        // mesh node -> its slot in models[]
        for (Ir77SceneNode& record : m_nodes) {
            if (record.mesh == UINT32_MAX) continue;

            std::uint32_t first{0};
            if (m_meshes->GetMeshFirstInstance(record.mesh, first)->ID() == GUIDIr77OperationSucceeded) record.instance_slot = first + record.mesh_ordinal;
        }

        // skins -- joint lists and inverse bind matrices
        m_skins.assign(m_asset.skins.size(), Ir77SceneSkin{});

        for (std::size_t k = 0; k < m_asset.skins.size(); k++) {
            fastgltf::Skin const& skin = m_asset.skins[k];
            Ir77SceneSkin& out = m_skins[k];

            for (auto const joint : skin.joints) out.joints.push_back(static_cast<std::uint32_t>(joint));

            out.inverse_bind.assign(out.joints.size(), glm::mat4(1.0f));  // glTF: identity when the accessor is absent
            if (skin.inverseBindMatrices.has_value()) {
                std::size_t j = 0;
                fastgltf::iterateAccessor<glm::mat4>(m_asset, m_asset.accessors.at(skin.inverseBindMatrices.value()), [&](glm::mat4 m) {
                    if (j < out.inverse_bind.size()) out.inverse_bind[j] = m;
                    j++;
                });
            }
        }

        // one joint palette per skinned mesh node, packed back to back
        m_bone_count = 0;
        for (Ir77SceneNode& record : m_nodes) {
            if (record.skin >= m_skins.size() || record.instance_slot == UINT32_MAX) continue;

            record.bone_base = m_bone_count;
            m_bone_count += static_cast<std::uint32_t>(m_skins[record.skin].joints.size());
        }

        m_bones.assign(m_bone_count, glm::mat4(1.0f));

        // rest pose -- what every frame starts from before a clip writes over it
        m_rest_translations.assign(m_nodes.size(), glm::vec3(0.0f));
        m_rest_rotations.assign(m_nodes.size(), glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
        m_rest_scales.assign(m_nodes.size(), glm::vec3(1.0f));

        for (std::size_t i = 0; i < m_nodes.size(); i++) {
            m_rest_translations[i] = m_nodes[i].translation;
            m_rest_rotations[i] = m_nodes[i].rotation;
            m_rest_scales[i] = m_nodes[i].scale;
        }

        m_materials->LoadMaterialsFromAsset(m_asset);
        m_textures->LoadTexturesFromAsset(m_asset);
        m_animations->LoadAnimationsFromAsset(m_asset);

        m_players.clear();
        m_pose_dirty = false;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // node tree -- per frame
    // -------------------------------------------------------------------------------------------------------------------------------------

    // Starts clip `clip` from the beginning, alongside whatever else is playing. Already playing -> restarts it with the new settings.
    // loop = wrap at the end, otherwise hold the last frame. Clips are sampled in the order they were started; where two drive the
    // same node, the later one wins.
    std::shared_ptr<IIr77Return const> PlayAnimation(std::uint32_t const& clip, bool const& loop, float const& speed = 1.0f) {
        std::uint32_t count{0};
        m_animations->GetClipCount(count);
        if (clip >= count) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Scene: not in animation index.");

        Ir77ScenePlayer player{};
        player.clip = clip;
        player.loop = loop;
        player.speed = speed;
        m_animations->GetClipDuration(clip, player.duration);

        for (Ir77ScenePlayer& existing : m_players) {
            if (existing.clip == clip) {
                existing = player;
                return Ir77RETURN<Ir77OperationSucceeded>();
            }
        }

        m_players.push_back(player);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // By name -- the glTF animation / Blender action name
    std::shared_ptr<IIr77Return const> PlayAnimation(std::string const& name, bool const& loop, float const& speed = 1.0f) {
        std::uint32_t clip{0};
        if (m_animations->GetClipIndex(name, clip)->ID() != GUIDIr77OperationSucceeded)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Scene: no animation with that name.");

        return PlayAnimation(clip, loop, speed);
    }

    // Every clip in the file at once -- what a "Mode: Actions" export of several animated objects needs
    std::shared_ptr<IIr77Return const> PlayAllAnimations(bool const& loop, float const& speed = 1.0f) {
        std::uint32_t count{0};
        m_animations->GetClipCount(count);

        for (std::uint32_t clip = 0; clip < count; clip++) PlayAnimation(clip, loop, speed);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Stops one clip; the nodes it drove go back to rest on the next Update
    std::shared_ptr<IIr77Return const> StopAnimation(std::uint32_t const& clip) {
        std::erase_if(m_players, [&](Ir77ScenePlayer const& player) { return player.clip == clip; });

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> StopAnimation(std::string const& name) {
        std::uint32_t clip{0};
        if (m_animations->GetClipIndex(name, clip)->ID() != GUIDIr77OperationSucceeded)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Scene: no animation with that name.");

        return StopAnimation(clip);
    }

    // Everything back to the rest pose, nothing playing
    std::shared_ptr<IIr77Return const> StopAllAnimations() {
        m_players.clear();
        m_pose_dirty = true;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Once per frame: advances animation, rebuilds every world matrix, then every joint palette. Afterwards the frame's results are read out
    // (WriteInstanceTransforms, WritePackedBones).
    std::shared_ptr<IIr77Return const> Update(float const& dt) {
        AdvanceAnimation(dt);
        UpdateWorld();
        UpdateBones();

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Copies mesh-node world matrices into their models[] slots
    std::shared_ptr<IIr77Return const> WriteInstanceTransforms(std::vector<glm::mat4>& models) {
        for (Ir77SceneNode const& node : m_nodes)
            if (node.instance_slot < models.size()) models[node.instance_slot] = node.world;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Every skinned node's joint palette, back to back -- what the Bones buffer holds. Node k's joints start at its bone base.
    std::shared_ptr<IIr77Return const> WriteBoneMatrices(std::vector<glm::mat4>& bones) {
        bones = m_bones;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Per models[] slot: where that instance's joint palette starts in the bones array (0 for unskinned instances)
    std::shared_ptr<IIr77Return const> WriteInstanceBoneBases(std::vector<std::uint32_t>& bases, std::size_t const& instance_count) {
        bases.assign(instance_count, 0);

        for (Ir77SceneNode const& node : m_nodes)
            if (node.bone_base != UINT32_MAX && node.instance_slot < bases.size()) bases[node.instance_slot] = node.bone_base;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // bone_base[BONE_BASE_SLOTS] then every joint palette -- the layout skinned.vert reads at set 0 binding 4. instance_count = models[] slots;
    // bases past BONE_BASE_SLOTS and palettes past BONE_CAPACITY are cut.
    std::shared_ptr<IIr77Return const> WritePackedBones(std::size_t const& instance_count, std::vector<std::uint8_t>& packed) {
        WriteInstanceBoneBases(m_bone_bases, std::min(instance_count, BONE_BASE_SLOTS));

        std::size_t const bone_count = std::min(m_bones.size(), BONE_CAPACITY);

        packed.assign(BONE_BASE_SLOTS * sizeof(std::uint32_t) + bone_count * sizeof(glm::mat4), 0);
        std::memcpy(packed.data(), m_bone_bases.data(), m_bone_bases.size() * sizeof(std::uint32_t));
        std::memcpy(packed.data() + BONE_BASE_SLOTS * sizeof(std::uint32_t), m_bones.data(), bone_count * sizeof(glm::mat4));

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // components
    // -------------------------------------------------------------------------------------------------------------------------------------
    std::shared_ptr<IIr77Return const> GetMeshes(std::shared_ptr<IIr77Mesh>& meshes) {
        meshes = m_meshes;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetCameras(std::shared_ptr<IIr77Camera>& cameras) {
        cameras = m_cameras;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetLights(std::shared_ptr<IIr77Light>& lights) {
        lights = m_lights;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetMaterials(std::shared_ptr<IIr77Material>& materials) {
        materials = m_materials;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetTextures(std::shared_ptr<IIr77Texture>& textures) {
        textures = m_textures;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetAnimations(std::shared_ptr<IIr77Animation>& animations) {
        animations = m_animations;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // stats -- counts from the loaded file
    std::shared_ptr<IIr77Return const> GetAssetCounts(std::uint32_t& nodes, std::uint32_t& meshes, std::uint32_t& materials, std::uint32_t& textures,
                                                      std::uint32_t& cameras, std::uint32_t& skins, std::uint32_t& bones, std::uint32_t& clips) {
        nodes = static_cast<std::uint32_t>(m_asset.nodes.size());
        meshes = static_cast<std::uint32_t>(m_asset.meshes.size());
        materials = static_cast<std::uint32_t>(m_asset.materials.size());
        textures = static_cast<std::uint32_t>(m_asset.textures.size());
        cameras = static_cast<std::uint32_t>(m_asset.cameras.size());
        skins = static_cast<std::uint32_t>(m_skins.size());
        bones = m_bone_count;
        clips = static_cast<std::uint32_t>(m_asset.animations.size());

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // stats -- the clips currently playing
    std::shared_ptr<IIr77Return const> GetPlayerCount(std::uint32_t& count) {
        count = static_cast<std::uint32_t>(m_players.size());

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetPlayer(std::uint32_t const& index, std::uint32_t& clip, float& time, float& duration, float& speed, bool& loop) {
        if (index >= m_players.size()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Scene: player index out of range.");

        Ir77ScenePlayer const& player = m_players[index];
        clip = player.clip;
        time = player.time;
        duration = player.duration;
        speed = player.speed;
        loop = player.loop;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   private:

    // Moves every player's clock by dt and writes the combined pose into the nodes' local TRS. Runs inside Update, before UpdateWorld.
    // Starts from the rest pose each frame, so a node no clip drives (or a stopped clip's nodes) sits at rest.
    std::shared_ptr<IIr77Return const> AdvanceAnimation(float const& dt) {
        if (m_players.empty() && !m_pose_dirty) return Ir77RETURN<Ir77OperationSucceeded>();  // nothing playing, already at rest

        m_pose_translations = m_rest_translations;
        m_pose_rotations = m_rest_rotations;
        m_pose_scales = m_rest_scales;

        for (Ir77ScenePlayer& player : m_players) {
            player.time += dt * player.speed;

            if (player.duration > 0.0f) {
                if (player.loop) {
                    player.time = std::fmod(player.time, player.duration);
                    if (player.time < 0.0f) player.time += player.duration;  // negative speed plays backwards
                } else {
                    player.time = std::clamp(player.time, 0.0f, player.duration);
                }
            }

            m_animations->Sample(player.clip, player.time, m_pose_translations, m_pose_rotations, m_pose_scales);
        }

        for (std::size_t i = 0; i < m_nodes.size(); i++) {
            m_nodes[i].translation = m_pose_translations[i];
            m_nodes[i].rotation = m_pose_rotations[i];
            m_nodes[i].scale = m_pose_scales[i];
        }

        m_pose_dirty = !m_players.empty();  // one more pass after the last clip stops puts everything back at rest

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Rebuilds every world matrix from the local TRS, parents first. Animation writes TRS before this runs.
    std::shared_ptr<IIr77Return const> UpdateWorld() {
        for (std::uint32_t const index : m_order) {
            Ir77SceneNode& node = m_nodes[index];

            glm::mat4 const local = glm::translate(glm::mat4(1.0f), node.translation) * glm::mat4_cast(node.rotation) * glm::scale(glm::mat4(1.0f), node.scale);

            node.world = node.parent == UINT32_MAX ? local : m_nodes[node.parent].world * local;
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // glTF skinning: joint matrix = inverse(mesh node world) * joint world * inverse bind. The mesh node's own world is applied
    // afterwards as the instance's model matrix, so the two cancel to joint world * inverse bind.
    std::shared_ptr<IIr77Return const> UpdateBones() {
        for (Ir77SceneNode const& node : m_nodes) {
            if (node.bone_base == UINT32_MAX) continue;

            Ir77SceneSkin const& skin = m_skins[node.skin];
            glm::mat4 const inverse_mesh = glm::inverse(node.world);

            for (std::size_t j = 0; j < skin.joints.size(); j++) {
                std::uint32_t const joint = skin.joints[j];
                if (joint >= m_nodes.size()) continue;

                m_bones[node.bone_base + j] = inverse_mesh * m_nodes[joint].world * skin.inverse_bind[j];
            }
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Records each node's parent and local TRS, keeps parents-before-children order, and hands lights / cameras / mesh instances
    // to their components with the load-time world transform.
    void WalkNode(std::size_t node_index, std::uint32_t parent_index, const glm::mat4& parent_transform, std::vector<bool>& mesh_used) {
        const auto& node = m_asset.nodes.at(node_index);
        Ir77SceneNode& record = m_nodes[node_index];

        record.parent = parent_index;

        if (auto const* trs = std::get_if<fastgltf::TRS>(&node.transform)) {  // DecomposeNodeMatrices guarantees TRS
            record.translation = glm::vec3(trs->translation[0], trs->translation[1], trs->translation[2]);
            record.rotation = glm::quat(trs->rotation[3], trs->rotation[0], trs->rotation[1], trs->rotation[2]);
            record.scale = glm::vec3(trs->scale[0], trs->scale[1], trs->scale[2]);
        }

        m_order.push_back(static_cast<std::uint32_t>(node_index));

        glm::mat4 const world_transform = parent_transform * ComputeLocalTransform(node);
        record.world = world_transform;

        if (node.lightIndex.has_value()) {
            m_lights->AddLight(m_asset.lights.at(node.lightIndex.value()), world_transform);
        }

        if (node.cameraIndex.has_value()) {
            m_cameras->AddCamera(m_asset.cameras.at(node.cameraIndex.value()), world_transform);
        }

        if (node.meshIndex.has_value()) {
            std::uint32_t const mesh = static_cast<std::uint32_t>(node.meshIndex.value());

            mesh_used[mesh] = true;
            record.mesh = mesh;
            if (node.skinIndex.has_value()) record.skin = static_cast<std::uint32_t>(node.skinIndex.value());
            record.mesh_ordinal = m_mesh_instance_count[mesh]++;  // same order AddMeshInstance appends in

            m_meshes->AddMeshInstance(mesh, world_transform);
        }

        for (auto child_index : node.children) {
            WalkNode(child_index, static_cast<std::uint32_t>(node_index), world_transform, mesh_used);
        }
    }

    glm::mat4 ComputeLocalTransform(const fastgltf::Node& node) {
        if (auto* trs = std::get_if<fastgltf::TRS>(&node.transform)) {
            glm::mat4 translation = glm::translate(glm::mat4(1.0f), glm::vec3(trs->translation[0], trs->translation[1], trs->translation[2]));

            glm::quat rotation(trs->rotation[3], trs->rotation[0], trs->rotation[1], trs->rotation[2]);

            glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(trs->scale[0], trs->scale[1], trs->scale[2]));

            return translation * glm::mat4_cast(rotation) * scale;
        }

        if (auto* mat = std::get_if<fastgltf::math::fmat4x4>(&node.transform)) {
            return glm::make_mat4(mat->data());
        }

        return glm::mat4(1.0f);
    }

   private:
    fastgltf::Asset m_asset;

    std::shared_ptr<IIr77Mesh> m_meshes{nullptr};

    std::shared_ptr<IIr77Camera> m_cameras{nullptr};

    std::shared_ptr<IIr77Light> m_lights{nullptr};

    std::shared_ptr<IIr77Material> m_materials{nullptr};

    std::shared_ptr<IIr77Texture> m_textures{nullptr};

    std::shared_ptr<IIr77Animation> m_animations{nullptr};

    // node tree
    std::vector<Ir77SceneNode> m_nodes{};

    std::vector<std::uint32_t> m_order{};  // parents before children

    std::map<std::uint32_t, std::uint32_t> m_mesh_instance_count{};  // [glTF mesh] -> instances seen so far in the walk

    // animation -- rest pose from the file, working pose the clip writes into, and the player
    std::vector<glm::vec3> m_rest_translations{};

    std::vector<glm::quat> m_rest_rotations{};

    std::vector<glm::vec3> m_rest_scales{};

    std::vector<glm::vec3> m_pose_translations{};

    std::vector<glm::quat> m_pose_rotations{};

    std::vector<glm::vec3> m_pose_scales{};

    // skinning
    std::vector<Ir77SceneSkin> m_skins{};

    std::vector<glm::mat4> m_bones{};  // every skinned node's joint palette, back to back

    std::uint32_t m_bone_count{0};

    std::vector<Ir77ScenePlayer> m_players{};  // everything currently playing, in start order

    bool m_pose_dirty{false};  // nodes hold an animated pose that a pass with no players must reset

    std::vector<std::uint32_t> m_bone_bases{};  // WritePackedBones' scratch

    static constexpr std::size_t BONE_BASE_SLOTS{8192};  // must match bone_base[] in skinned.vert

    static constexpr std::size_t BONE_CAPACITY{256 * 128};  // Ir77PVFrameSizes::bones capacity after the bases
};
}  // namespace NSIr77Scene
