#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <fastgltf/core.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/tools.hpp>

#include <glm/gtc/constants.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <map>
#include <memory>
#include <numeric>
#include <string>
#include <string_view>
#include <vector>

#include "../../Ir77RT/dictionary/IDIIr77MPVM.hpp"

#include "../dictionary/IDIIr77Scene.hpp"
#include "../dictionary/IDIr77Scene.hpp"

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"

#include "../interface/IIr77Camera.hpp"
#include "../interface/IIr77Light.hpp"
#include "../interface/IIr77Mesh.hpp"
#include "../interface/IIr77Material.hpp"
#include "../interface/IIr77Texture.hpp"
#include "../interface/IIr77Animation.hpp"

#include "../runtime/Ir77Camera.hpp"
#include "../runtime/Ir77Light.hpp"
#include "../runtime/Ir77Mesh.hpp"
#include "../runtime/Ir77Material.hpp"
#include "../runtime/Ir77Texture.hpp"
#include "../runtime/Ir77Animation.hpp"

#include "../types/Ir77Stats.hpp"

// the direct link -- Scene -> PeregrineV, one way: Scene calls Asset / Paint, PeregrineV never sees Scene
#include "../../PeregrineV/server/Ir77PVAsset.hpp"
#include "../../PeregrineV/server/Ir77PVPaint.hpp"

using namespace NSIr77RT;
using namespace NSIr77PeregrineV;

namespace NSIr77Scene {

// =============================================================================================================================================
// Ir77Scene -- server for the scene module (build-time linked, no abstract interface). Parses a glTF into its components (meshes, materials, textures, lights, cameras,
// animations) and keeps the node tree live: every node's local TRS is stored, and UpdateWorld rebuilds world matrices each
// frame (parents first), so animation only ever writes local TRS.
//
// Direct link to PeregrineV (one way): CreateScene uploads the file through Ir77PVAsset; CreateFrame hands the frame -- camera, instances,
// lights, bones, draw items -- to Ir77PVPaint. It also owns the cameras: the file's, and the controlled orbit / fly camera the UI drives.
//
//   setup:     SetAsset / SetPaint / SetViewSize / SetPipeline -> CreateScene(path, use_mesh)
//   per frame: Update(dt) -> CreateFrame(seconds)
// =============================================================================================================================================
class Ir77Animate : public Ir77Enlisted, public std::enable_shared_from_this<Ir77Animate> {
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

    // one material run -- contiguous indirect commands sharing a glTF material
    struct Ir77SceneRun {
        std::uint32_t gltf_material{UINT32_MAX};
        std::uint32_t first_command{0};
        std::uint32_t command_count{0};
    };

    // one transparent object -- a single indirect command, sorted by camera distance every frame
    struct Ir77SceneBlendDraw {
        std::uint32_t gltf_material{UINT32_MAX};
        std::uint32_t command{0};         // in the static blend scene, or in the skinned scene when skinned
        std::uint32_t first_instance{0};  // models[] slot -- its translation is the sort position
        bool skinned{false};
    };

   public:
    Ir77Animate() {
        try {
            m_enlisted_uuid.Generate();
        } catch (std::invalid_argument a) {
            throw a;
        }

        m_enlisted = std::chrono::system_clock::now();
    }

    ~Ir77Animate() {}

   public:
    std::shared_ptr<IIr77Return const> EnlistedAs(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIIr77Enlisted>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> MemberUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77Scene>(uid);

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

        else if (iid == GUIDIr77Scene)
            obj = std::shared_ptr<Ir77Animate>(shared_from_this(), static_cast<Ir77Animate*>(this));

        else
            return GUIDQueryFailed;

        return GUIDQuerySucceeded;
    }

   public:
    std::shared_ptr<IIr77Return const> ParseAsset(const std::string& path) {
        // extensions this loader handles -- a file that *requires* one not listed fails to parse; optional ones are ignored
        constexpr auto extensions = fastgltf::Extensions::KHR_lights_punctual | fastgltf::Extensions::KHR_mesh_quantization |
                                    fastgltf::Extensions::KHR_texture_transform;

        // LoadExternalBuffers / LoadExternalImages -- .bin and image files beside a .gltf are read into memory (.glb contents always are)
        // GenerateMeshIndices   -- non-indexed primitives get 0..n-1, so every primitive has an index accessor
        // DecomposeNodeMatrices -- matrix nodes become TRS, so every node stores (and animates) the same way
        constexpr auto options = fastgltf::Options::LoadExternalBuffers | fastgltf::Options::LoadExternalImages |
                                 fastgltf::Options::GenerateMeshIndices | fastgltf::Options::DecomposeNodeMatrices;

        fastgltf::Parser parser(extensions);

        auto data = fastgltf::GltfDataBuffer::FromPath(path);
        if (data.error() != fastgltf::Error::None) {
            auto const message = fastgltf::getErrorMessage(data.error());
            std::printf("[Ir77Scene] %s: %.*s\n", path.c_str(), static_cast<int>(message.size()), message.data());
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Scene: failed to load glTF file.");
        }

        auto asset = parser.loadGltf(data.get(), std::filesystem::path(path).parent_path(), options);
        if (asset.error() != fastgltf::Error::None) {
            auto const message = fastgltf::getErrorMessage(asset.error());
            std::printf("[Ir77Scene] %s: %.*s\n", path.c_str(), static_cast<int>(message.size()), message.data());
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Scene: failed to parse glTF asset.");
        }

        m_asset = std::move(asset.get());

        std::size_t const scene_index = m_asset.defaultScene.value_or(0);
        if (scene_index >= m_asset.scenes.size()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Scene: no valid scene.");

        const auto& scene = m_asset.scenes.at(scene_index);

        std::vector<bool> mesh_used(m_asset.meshes.size(), false);

        m_meshes = std::make_shared<Ir77Mesh>();
        m_lights = std::make_shared<Ir77Light>();
        m_cameras = std::make_shared<Ir77Camera>();
        m_materials = std::make_shared<Ir77Material>();
        m_textures = std::make_shared<Ir77Texture>();
        m_animations = std::make_shared<Ir77Animation>();

        // node tree -- sized up front, filled by the walk
        m_nodes.assign(m_asset.nodes.size(), Ir77SceneNode{});
        m_order.clear();
        m_mesh_instance_count.clear();

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
        if (m_animations->GetClipIndex(name, clip)->ID() != GUIDIr77OperationSucceeded) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Scene: no animation with that name.");

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
        if (m_animations->GetClipIndex(name, clip)->ID() != GUIDIr77OperationSucceeded) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Scene: no animation with that name.");

        return StopAnimation(clip);
    }

    // Everything back to the rest pose, nothing playing
    std::shared_ptr<IIr77Return const> StopAllAnimations() {
        m_players.clear();
        m_pose_dirty = true;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // The scene's one per-frame call: advances animation, rebuilds every world matrix, then every joint palette. Everything animation-related happens
    // in here; afterwards the frame's results are read out (WriteInstanceTransforms, ...) and uploaded by the app.
    std::shared_ptr<IIr77Return const> Update(float const& dt) {
        AdvanceAnimation(dt);
        UpdateWorld();
        UpdateBones();
        UpdateCameraControl(dt);  // held keys from the UI move the controlled camera

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

    // -------------------------------------------------------------------------------------------------------------------------------------
    // direct link -- Scene -> PeregrineV
    // -------------------------------------------------------------------------------------------------------------------------------------

    // Where the scene's textures, materials and geometry go (CreateScene) and which pipelines its draw items use
    std::shared_ptr<IIr77Return const> SetAsset(std::shared_ptr<Ir77PVAsset> const& asset) {
        m_pv_asset = asset;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Where each frame goes (CreateFrame) -- frame data and draw lists, per window
    std::shared_ptr<IIr77Return const> SetPaint(std::shared_ptr<Ir77PVPaint> const& paint) {
        m_pv_paint = paint;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // The windows CreateFrame fills, and their size -- the cameras' aspect. Any time; the next frame uses it.
    std::shared_ptr<IIr77Return const> SetViewSize(std::size_t const& windows, int const& width, int const& height) {
        m_view_windows = windows;
        m_view_width = width;
        m_view_height = height;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // What the static (non-skinned, opaque) runs draw with -- a manifest pipeline or role. Unset -> the "static" role.
    std::shared_ptr<IIr77Return const> SetPipeline(Ir77PVPipelineHandle const& pipeline) {
        m_pipeline = pipeline;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Once, after CreatePipelines. Parses the glTF, stacks every primitive into one upload, sorts the commands into material runs, uploads
    // textures / materials / geometry through the Asset and packs the lights. use_mesh -> mesh shading for the static stream when the device has it.
    std::shared_ptr<IIr77Return const> CreateScene(std::string const& path, bool const& use_mesh) {
        if (!m_pv_asset || !m_pv_paint) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Scene: SetAsset / SetPaint not called.");

        auto result = ParseAsset(path);
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        if (m_pipeline == IR77PV_NO_PIPELINE) {
            result = m_pv_asset->FindPipeline(Ir77PVPipelineKind::Static, m_pipeline);
            if (result->ID() != GUIDIr77OperationSucceeded) return result;
        }

        // commands ordered by material -> each material is one contiguous run, one draw item
        auto const build_runs = [](std::vector<Ir77MeshRange> const& ranges, std::vector<std::uint32_t> const& materials,
                                   std::vector<VkDrawIndexedIndirectCommand>& commands, std::vector<Ir77SceneRun>& runs) {
            std::vector<std::size_t> order(ranges.size());
            std::iota(order.begin(), order.end(), 0);
            std::stable_sort(order.begin(), order.end(), [&](std::size_t a, std::size_t b) { return materials[a] < materials[b]; });

            runs.clear();
            for (std::size_t k = 0; k < order.size(); k++) {
                Ir77MeshRange const& r = ranges[order[k]];
                commands.push_back(VkDrawIndexedIndirectCommand{r.index_count, r.instance_count, r.index_offset, r.vertex_offset, r.first_instance});

                std::uint32_t const material = materials[order[k]];
                if (runs.empty() || runs.back().gltf_material != material) runs.push_back({material, static_cast<std::uint32_t>(k), 0});
                runs.back().command_count++;
            }
        };

        // static stream -- vertex path or mesh shading
        Ir77PVSceneUpload upload{};
        std::vector<Ir77MeshRange> ranges{};
        std::vector<std::uint32_t> materials{};

        m_meshes->GetVertexBuffer(upload.vertices);
        m_meshes->GetIndexBuffer(upload.indices);
        m_meshes->GetMeshRanges(ranges);
        m_meshes->GetRangeMaterials(materials);
        m_meshes->GetInstanceBuffer(m_transforms);
        m_meshes->GetBounds(m_bounds_min, m_bounds_max);

        // BLEND materials -> Transparent pass (sorted per object each frame); everything else stays in the batched opaque runs
        auto const split_blend = [&](std::vector<Ir77MeshRange>& in_ranges, std::vector<std::uint32_t>& in_materials, std::vector<Ir77MeshRange>& blend_ranges,
                                     std::vector<std::uint32_t>& blend_materials) {
            std::vector<Ir77MeshRange> keep_ranges{};
            std::vector<std::uint32_t> keep_materials{};

            for (std::size_t r = 0; r < in_ranges.size(); r++) {
                bool blend{false};
                m_materials->GetMaterialBlend(in_materials[r], blend);  // no material / out of range -> stays opaque

                if (blend) {
                    blend_ranges.push_back(in_ranges[r]);
                    blend_materials.push_back(in_materials[r]);
                } else {
                    keep_ranges.push_back(in_ranges[r]);
                    keep_materials.push_back(in_materials[r]);
                }
            }

            in_ranges = std::move(keep_ranges);
            in_materials = std::move(keep_materials);
        };

        m_blend_draws.clear();

        std::vector<Ir77MeshRange> blend_ranges{};
        std::vector<std::uint32_t> blend_materials{};
        split_blend(ranges, materials, blend_ranges, blend_materials);

        build_runs(ranges, materials, upload.indirect_commands, m_runs);

        // static blend ranges -- compacted into their own small vertex-path scene (the mesh path has no Transparent pipeline)
        Ir77PVSceneUpload blend_upload{};

        for (std::size_t b = 0; b < blend_ranges.size(); b++) {
            Ir77MeshRange const& r = blend_ranges[b];
            if (r.index_count == 0) continue;

            std::uint32_t highest{0};  // indices are relative to vertex_offset -- the highest one bounds the vertex span
            for (std::uint32_t i = 0; i < r.index_count; i++) highest = std::max(highest, upload.indices[r.index_offset + i]);

            m_blend_draws.push_back({blend_materials[b], static_cast<std::uint32_t>(blend_upload.indirect_commands.size()), r.first_instance, false});

            blend_upload.indirect_commands.push_back(VkDrawIndexedIndirectCommand{r.index_count, r.instance_count,
                                                                                  static_cast<std::uint32_t>(blend_upload.indices.size()),
                                                                                  static_cast<std::int32_t>(blend_upload.vertices.size()), r.first_instance});

            blend_upload.indices.insert(blend_upload.indices.end(), upload.indices.begin() + r.index_offset,
                                        upload.indices.begin() + r.index_offset + r.index_count);
            blend_upload.vertices.insert(blend_upload.vertices.end(), upload.vertices.begin() + r.vertex_offset,
                                         upload.vertices.begin() + r.vertex_offset + highest + 1);
        }

        // skinned stream -- vertex path only, Skinned pipeline
        Ir77PVSceneUpload skinned_upload{};
        std::vector<Ir77MeshRange> skinned_ranges{};
        std::vector<std::uint32_t> skinned_materials{};

        m_meshes->GetSkinnedVertexBuffer(skinned_upload.vertices_skinned);
        m_meshes->GetSkinnedIndexBuffer(skinned_upload.indices_skinned);
        m_meshes->GetSkinnedMeshRanges(skinned_ranges);
        m_meshes->GetSkinnedRangeMaterials(skinned_materials);

        std::vector<Ir77MeshRange> skinned_blend_ranges{};
        std::vector<std::uint32_t> skinned_blend_materials{};
        split_blend(skinned_ranges, skinned_materials, skinned_blend_ranges, skinned_blend_materials);

        build_runs(skinned_ranges, skinned_materials, skinned_upload.indirect_commands, m_skinned_runs);

        // skinned blend ranges -- appended after the opaque runs in the skinned scene (same buffers, SkinnedTransparent pipeline)
        for (std::size_t b = 0; b < skinned_blend_ranges.size(); b++) {
            Ir77MeshRange const& r = skinned_blend_ranges[b];

            m_blend_draws.push_back({skinned_blend_materials[b], static_cast<std::uint32_t>(skinned_upload.indirect_commands.size()), r.first_instance, true});

            skinned_upload.indirect_commands.push_back(
                VkDrawIndexedIndirectCommand{r.index_count, r.instance_count, r.index_offset, r.vertex_offset, r.first_instance});
        }

        m_has_skinned = !skinned_upload.indirect_commands.empty();

        if (upload.indirect_commands.empty() && skinned_upload.indirect_commands.empty() && blend_upload.indirect_commands.empty())
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Scene: no drawable primitives.");

        // grey fallback -- primitives with no glTF material
        Ir77PVMaterialConstants constants{};
        constants.base_color = {0.8f, 0.8f, 0.8f, 1.0f};

        result = m_pv_asset->CreateMaterial(constants, m_default_material);
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        // textures -- glTF texture index -> Asset texture id; IR77PV_NO_TEXTURE where decode / upload failed
        std::uint32_t texture_count{0};
        m_textures->GetTextureCount(texture_count);

        std::vector<std::uint32_t> texture_ids(texture_count, IR77PV_NO_TEXTURE);

        for (std::uint32_t t = 0; t < texture_count; t++) {
            std::uint32_t width{0}, height{0};
            bool srgb{false};
            std::vector<std::uint8_t> rgba{};

            if (m_textures->GetTexture(t, width, height, srgb, rgba)->ID() != GUIDIr77OperationSucceeded) continue;
            if (m_pv_asset->CreateTexture(rgba.data(), width, height, srgb, texture_ids[t])->ID() != GUIDIr77OperationSucceeded)
                texture_ids[t] = IR77PV_NO_TEXTURE;
        }

        // materials -- glTF material index -> Asset material id, uploaded in one batch
        std::uint32_t material_count{0};
        m_materials->GetMaterialCount(material_count);

        std::vector<Ir77PVMaterialConstants> material_constants(material_count);
        std::vector<Ir77PVMaterialTextures> material_textures(material_count);

        auto const texture_id = [&](std::uint32_t const& gltf_texture) {
            return gltf_texture < texture_ids.size() ? texture_ids[gltf_texture] : IR77PV_NO_TEXTURE;
        };

        for (std::uint32_t m = 0; m < material_count; m++) {
            std::array<std::uint32_t, 5> slots{};
            bool double_sided{false};
            m_materials->GetMaterial(m, material_constants[m], slots, double_sided);

            material_textures[m].albedo = texture_id(slots[0]);
            material_textures[m].normal = texture_id(slots[1]);
            material_textures[m].orm = texture_id(slots[2]);
            material_textures[m].emissive = texture_id(slots[3]);
            material_textures[m].ao = texture_id(slots[4]);
        }

        m_material_ids.clear();
        if (material_count > 0) {
            result = m_pv_asset->UploadMaterials(material_constants, material_textures, m_material_ids);
            if (result->ID() != GUIDIr77OperationSucceeded) return result;
        }

        // static: mesh path when the device has it, vertex path otherwise -- same upload either way
        m_mesh_path = false;
        if (!ranges.empty()) {
            std::uint32_t records{0};
            m_mesh_path = use_mesh && m_pv_asset->UploadMeshlets(upload, m_static_id, records)->ID() == GUIDIr77OperationSucceeded;

            if (!m_mesh_path) {
                result = m_pv_asset->UploadScene(upload, m_static_id);
                if (result->ID() != GUIDIr77OperationSucceeded) return result;
            }
        }

        // skinned: its own scene upload (skinned vertex / index channels + its own indirect commands)
        if (m_has_skinned) {
            result = m_pv_asset->UploadScene(skinned_upload, m_skinned_id);
            if (result->ID() != GUIDIr77OperationSucceeded) return result;
        }

        // static transparent: its own compacted scene
        if (!blend_upload.indirect_commands.empty()) {
            result = m_pv_asset->UploadScene(blend_upload, m_blend_id);
            if (result->ID() != GUIDIr77OperationSucceeded) return result;
        }

        // lights -- header + array, packed once; a default sun when the file has none so the scene isn't black
        std::vector<Ir77PVLight> light_list{};
        m_lights->GetLightsBuffer(light_list);

        if (light_list.empty()) {
            Ir77PVLight sun{};
            sun.direction = glm::normalize(glm::vec3(-0.4f, -1.0f, -0.3f));
            sun.color = glm::vec3(1.0f);
            sun.intensity = 1.0f;
            sun.type = 0;  // directional
            light_list.push_back(sun);
        }

        if (light_list.size() > 64) light_list.resize(64);  // Ir77PVFrameSizes::lights capacity

        Ir77PVLightsHeader header{};
        header.count = static_cast<std::uint32_t>(light_list.size());

        m_lights_packed.resize(sizeof(header) + light_list.size() * sizeof(Ir77PVLight));
        std::memcpy(m_lights_packed.data(), &header, sizeof(header));
        std::memcpy(m_lights_packed.data() + sizeof(header), light_list.data(), light_list.size() * sizeof(Ir77PVLight));

        // file cameras -- index m_camera_count is the controlled camera; start on the file's first camera if it has one
        m_cameras->GetCameraCount(m_camera_count);
        m_camera_index = 0;

        ResetCameraControl();  // the controlled camera starts framed on the scene

        // every clip in the file plays by default, looping -- the app takes over with PlayAnimation / StopAnimation
        PlayAllAnimations(true);

        // stats -- filled once here, read by the app through GetSceneStats
        m_stats = Ir77SceneStats{};
        m_stats.path = path;
        m_stats.render_path = m_mesh_path ? "mesh shading indirect" : "vertex indirect";

        m_pv_asset->GetDeviceName(m_stats.gpu);

        GetAssetCounts(m_stats.nodes, m_stats.meshes, m_stats.materials, m_stats.textures, m_stats.cameras, m_stats.skins, m_stats.bones, m_stats.clips);

        m_stats.clip_names.assign(m_stats.clips, std::string{});
        for (std::uint32_t c = 0; c < m_stats.clips; c++) m_animations->GetClipName(c, m_stats.clip_names[c]);

        m_stats.static_vertices = upload.vertices.size();
        m_stats.static_indices = upload.indices.size();
        m_stats.skinned_vertices = skinned_upload.vertices_skinned.size();
        m_stats.skinned_indices = skinned_upload.indices_skinned.size();
        m_stats.static_primitives = static_cast<std::uint32_t>(ranges.size() + blend_ranges.size());
        m_stats.skinned_primitives = static_cast<std::uint32_t>(skinned_ranges.size() + skinned_blend_ranges.size());
        m_stats.instances = static_cast<std::uint32_t>(m_transforms.size());
        m_stats.opaque_runs = static_cast<std::uint32_t>(m_runs.size());
        m_stats.skinned_runs = static_cast<std::uint32_t>(m_skinned_runs.size());
        m_stats.transparent_objects = static_cast<std::uint32_t>(m_blend_draws.size());
        m_stats.lights = light_list;

        if (m_transforms.size() > 8192) m_stats.warnings.push_back("more than 8192 instances -- raise the Instances capacity");

        PrintSceneStats();  // console copy, for when the app UI isn't up

        m_scene_ready = true;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Every frame, after Update: hands the frame's results to Paint -- camera, instance transforms, lights, bones, one draw item per material
    // run and the sorted transparent draws, for every window.
    std::shared_ptr<IIr77Return const> CreateFrame(float const& seconds) {
        if (!m_scene_ready) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Scene: CreateScene not called.");

        WriteInstanceTransforms(m_transforms);

        // bones -- uint bone_base[8192] then every joint palette, the layout skinned.vert reads at set 0 binding 4
        bool const skinned = m_has_skinned;
        if (skinned) {
            constexpr std::size_t base_slots = 8192;      // must match bone_base[] in skinned.vert
            constexpr std::size_t max_bones = 256 * 128;  // Ir77PVFrameSizes::bones capacity after the bases

            WriteInstanceBoneBases(m_bone_bases, std::min(m_transforms.size(), base_slots));

            std::size_t const bone_count = std::min(m_bones.size(), max_bones);

            m_bones_packed.assign(base_slots * sizeof(std::uint32_t) + bone_count * sizeof(glm::mat4), 0);
            std::memcpy(m_bones_packed.data(), m_bone_bases.data(), m_bone_bases.size() * sizeof(std::uint32_t));
            std::memcpy(m_bones_packed.data() + base_slots * sizeof(std::uint32_t), m_bones.data(), bone_count * sizeof(glm::mat4));
        }

        float const radius = std::max(glm::length(m_bounds_max - m_bounds_min) * 0.5f, 0.01f);
        float const aspect = m_view_height > 0 ? static_cast<float>(m_view_width) / static_cast<float>(m_view_height) : 1.78f;

        for (std::size_t w = 0; w < m_view_windows; w++) {
            // file camera (rebuilt with this window's aspect), or the controlled camera (orbit / fly, driven from the UI)
            Ir77PVCamera camera{};
            bool built = false;

            if (m_camera_index < m_camera_count) {
                float const fallback_far = std::max(100.0f, radius * 20.0f);  // file camera without zfar
                built = m_cameras->BuildCamera(m_camera_index, aspect, fallback_far, camera)->ID() == GUIDIr77OperationSucceeded;
            }

            if (!built) camera = MakeControlledCamera(aspect);

            m_camera_view = camera.view;  // where the next switch to orbit / fly starts from
            m_camera_fov = glm::degrees(2.0f * std::atan(1.0f / std::abs(camera.proj[1][1])));  // the lens too

            camera.time = glm::vec4(seconds, seconds - m_last_seconds, static_cast<float>(m_frame_number), 0.0f);

            m_pv_paint->SetFrameData(w, Ir77PVBufferSlot::Camera, &camera, sizeof(camera));
            m_pv_paint->SetFrameData(w, Ir77PVBufferSlot::Instances, m_transforms.data(), m_transforms.size() * sizeof(glm::mat4));
            m_pv_paint->SetFrameData(w, Ir77PVBufferSlot::Lights, m_lights_packed.data(), m_lights_packed.size());
            if (skinned) m_pv_paint->SetFrameData(w, Ir77PVBufferSlot::Bones, m_bones_packed.data(), m_bones_packed.size());

            // one draw item per material run
            std::vector<Ir77PVDrawItem> items{};
            items.reserve(m_runs.size() + m_skinned_runs.size());

            for (Ir77SceneRun const& run : m_runs) {
                Ir77PVDrawItem item{};
                auto const made = m_mesh_path ? m_pv_asset->MakeMeshletSceneDrawItem(w, m_static_id, MaterialId(run.gltf_material), run.first_command,
                                                                                    run.command_count, item)
                                              : m_pv_asset->MakeIndirectDrawItem(w, m_pipeline, m_static_id, MaterialId(run.gltf_material), run.first_command,
                                                                                 run.command_count, false, item);
                if (made->ID() != GUIDIr77OperationSucceeded) return made;

                items.push_back(item);
            }

            // skinned runs -- Skinned pipeline over the skinned scene's buffers
            for (Ir77SceneRun const& run : m_skinned_runs) {
                Ir77PVDrawItem item{};
                auto const made = m_pv_asset->MakeIndirectDrawItem(w, Ir77PVPipelineKind::Skinned, m_skinned_id, MaterialId(run.gltf_material),
                                                                   run.first_command, run.command_count, true, item);
                if (made->ID() != GUIDIr77OperationSucceeded) return made;

                items.push_back(item);
            }

            m_pv_paint->SetDraws(w, Ir77PVPass::Opaque, items);
            m_opaque_items = static_cast<std::uint32_t>(items.size());

            // transparent -- one draw per object, back to front from this window's eye (instances within one object keep their order)
            glm::vec3 const eye = glm::vec3(glm::inverse(camera.view)[3]);

            std::vector<float> distance(m_blend_draws.size());
            for (std::size_t d = 0; d < m_blend_draws.size(); d++) {
                glm::vec3 const position = glm::vec3(m_transforms[m_blend_draws[d].first_instance][3]);
                distance[d] = glm::dot(position - eye, position - eye);
            }

            std::vector<std::size_t> order(m_blend_draws.size());
            std::iota(order.begin(), order.end(), 0);
            std::sort(order.begin(), order.end(), [&](std::size_t a, std::size_t b) { return distance[a] > distance[b]; });

            std::vector<Ir77PVDrawItem> blend_items{};
            blend_items.reserve(order.size() * 2);

            for (std::size_t const d : order) {
                Ir77SceneBlendDraw const& draw = m_blend_draws[d];
                std::uint32_t const scene = draw.skinned ? m_skinned_id : m_blend_id;

                // far side first (front faces culled), then near side -- one pipeline, the cull flips per draw
                Ir77PVPipelineKind const kind{draw.skinned ? Ir77PVPipelineKind::SkinnedTransparent : Ir77PVPipelineKind::Transparent};

                for (bool const back_faces : {true, false}) {
                    Ir77PVDrawItem item{};
                    auto const made = m_pv_asset->MakeIndirectDrawItem(w, kind, scene, MaterialId(draw.gltf_material), draw.command, 1, draw.skinned, item);
                    if (made->ID() != GUIDIr77OperationSucceeded) return made;

                    item.back_faces = back_faces;
                    blend_items.push_back(item);
                }
            }

            m_pv_paint->SetDraws(w, Ir77PVPass::Transparent, blend_items);
            m_transparent_items = static_cast<std::uint32_t>(blend_items.size());
        }

        m_last_seconds = seconds;
        m_frame_number++;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // cameras -- the file's, then the controlled one (index == camera count). The UI drives these; they take effect on the next frame.
    // -------------------------------------------------------------------------------------------------------------------------------------

    // Held keys: right / up / forward in -1..1, applied every Update until set back to 0
    std::shared_ptr<IIr77Return const> SetCameraMove(glm::vec3 const& move) {
        DefineCameraTakeover();
        m_camera_move = glm::clamp(move, glm::vec3(-1.0f), glm::vec3(1.0f));

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Mouse drag in pixels -- turns the fly camera / swings the orbit camera
    std::shared_ptr<IIr77Return const> SetCameraLook(float const& dx, float const& dy) {
        DefineCameraTakeover();
        m_camera_yaw -= dx * CAMERA_LOOK_RADIANS_PER_PIXEL;
        m_camera_pitch = glm::clamp(m_camera_pitch - dy * CAMERA_LOOK_RADIANS_PER_PIXEL, -CAMERA_PITCH_LIMIT, CAMERA_PITCH_LIMIT);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Wheel steps -- orbit distance, 10% per step
    std::shared_ptr<IIr77Return const> SetCameraZoom(float const& delta) {
        DefineCameraTakeover();
        m_camera_distance = std::max(m_camera_distance * std::pow(1.1f, -delta), 0.01f);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Multiplies the move speed
    std::shared_ptr<IIr77Return const> SetCameraSpeed(float const& factor) {
        if (factor <= 0.0f) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Scene: camera speed factor must be above 0.");

        m_camera_speed *= factor;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // "orbit" | "fly" -- start from the current view, so switching never jumps; "file" -- the file's first camera
    std::shared_ptr<IIr77Return const> SetCameraMode(std::string_view const& mode) {
        if (mode == "orbit" || mode == "fly") {
            DefineCameraFromView(mode == "fly");
            m_camera_index = m_camera_count;
        } else if (mode == "file") {
            if (m_camera_count == 0) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Scene: the file has no cameras.");
            m_camera_index = 0;
        } else {
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Scene: camera mode must be orbit, fly or file.");
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // index == camera count -> the controlled camera
    std::shared_ptr<IIr77Return const> SelectCamera(std::uint32_t const& index) {
        if (index > m_camera_count) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Scene: no such camera.");

        m_camera_index = index;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // The next file camera, then the controlled camera, then around again (F2)
    std::shared_ptr<IIr77Return const> NextCamera() {
        m_camera_index = (m_camera_index + 1) % (m_camera_count + 1);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Orbit, framed on the scene
    std::shared_ptr<IIr77Return const> ResetCamera() {
        ResetCameraControl();
        m_camera_index = m_camera_count;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // stats -- copies, safe to keep
    // -------------------------------------------------------------------------------------------------------------------------------------
    std::shared_ptr<IIr77Return const> GetSceneStats(Ir77SceneStats& stats) const {
        stats = m_stats;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // The scene's part of the frame stats -- active camera, draw counts, playing clips. Timings stay with whoever runs the loop.
    std::shared_ptr<IIr77Return const> WriteFrameStats(Ir77FrameStats& stats) const {
        stats.camera_index = m_camera_index;
        stats.orbit_camera = m_camera_index >= m_camera_count;
        stats.opaque_items = m_opaque_items;
        stats.transparent_items = m_transparent_items;

        stats.playing.clear();

        for (Ir77ScenePlayer const& player : m_players) {
            Ir77ClipStat clip_stat{};
            clip_stat.time = player.time;
            clip_stat.duration = player.duration;
            clip_stat.speed = player.speed;
            clip_stat.loop = player.loop;
            if (player.clip < m_stats.clip_names.size()) clip_stat.name = m_stats.clip_names[player.clip];

            stats.playing.push_back(clip_stat);
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   private:
    // glTF material -> Asset material id; primitives without one use the grey default
    std::uint32_t MaterialId(std::uint32_t const& gltf_material) const {
        return gltf_material < m_material_ids.size() ? m_material_ids[gltf_material] : m_default_material;
    }

    // Driving the camera while a file camera is up takes it over -- the controlled camera starts from the current view
    void DefineCameraTakeover() {
        if (m_camera_index >= m_camera_count) return;

        DefineCameraFromView(m_camera_fly);
        m_camera_index = m_camera_count;
    }

    // Orbit on the scene: target its centre, back off to frame its bounds, a little above
    void ResetCameraControl() {
        float const radius = std::max(glm::length(m_bounds_max - m_bounds_min) * 0.5f, 0.01f);

        m_camera_fly = false;
        m_camera_target = (m_bounds_min + m_bounds_max) * 0.5f;
        m_camera_distance = radius * 2.5f;
        m_camera_yaw = glm::pi<float>();  // looking down -Z, from the +Z side
        m_camera_pitch = -0.35f;
        m_camera_speed = radius * 0.5f;  // units per second at full key
        m_camera_move = glm::vec3(0.0f);
    }

    // Takes over from whatever was on screen last frame -- file camera, orbit or fly
    void DefineCameraFromView(bool const fly) {
        glm::mat4 const world = glm::inverse(m_camera_view);
        glm::vec3 const eye = glm::vec3(world[3]);
        glm::vec3 const forward = glm::normalize(-glm::vec3(world[2]));  // the view looks down its -Z

        m_camera_fly = fly;
        m_camera_yaw = std::atan2(forward.x, forward.z);
        m_camera_pitch = glm::clamp(std::asin(glm::clamp(forward.y, -1.0f, 1.0f)), -CAMERA_PITCH_LIMIT, CAMERA_PITCH_LIMIT);

        if (fly) {
            m_camera_position = eye;
        } else {
            m_camera_distance = std::max(glm::length(m_camera_target - eye), 0.01f);
            m_camera_target = eye + forward * m_camera_distance;  // keep the view; orbit around the point straight ahead
        }
    }

    // Held keys, every Update. Fly: walk along the view. Orbit: forward dollies in / out, right / up pan the target.
    void UpdateCameraControl(float const dt) {
        if (m_camera_move == glm::vec3(0.0f) || dt <= 0.0f) return;

        glm::vec3 const forward = CameraForward();
        glm::vec3 const right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
        glm::vec3 const up = glm::cross(right, forward);

        float const step = m_camera_speed * dt;

        if (m_camera_fly) {
            m_camera_position += (right * m_camera_move.x + up * m_camera_move.y + forward * m_camera_move.z) * step;
        } else {
            m_camera_target += (right * m_camera_move.x + up * m_camera_move.y) * step;
            m_camera_distance = std::max(m_camera_distance * std::exp(-m_camera_move.z * dt), 0.01f);
        }
    }

    glm::vec3 CameraForward() const {
        return glm::vec3(std::cos(m_camera_pitch) * std::sin(m_camera_yaw), std::sin(m_camera_pitch), std::cos(m_camera_pitch) * std::cos(m_camera_yaw));
    }

    // The controlled camera's matrices. Planes hug the scene: whole bounds in front of far, near as large as depth precision allows.
    Ir77PVCamera MakeControlledCamera(float const aspect) const {
        glm::vec3 const center = (m_bounds_min + m_bounds_max) * 0.5f;
        float const radius = std::max(glm::length(m_bounds_max - m_bounds_min) * 0.5f, 0.01f);

        glm::vec3 const forward = CameraForward();
        glm::vec3 const eye = m_camera_fly ? m_camera_position : m_camera_target - forward * m_camera_distance;

        float const zfar = glm::length(eye - center) + radius * 2.0f;
        float const znear = std::max(zfar * 0.0001f, 0.001f);

        return Ir77PVAsset::MakeCamera(eye, eye + forward, m_camera_fov, aspect, znear, zfar);
    }

    // The scene stats as console lines -- until the app displays them
    void PrintSceneStats() const {
        Ir77SceneStats const& s = m_stats;

        std::printf("[STATS] %s\n", s.path.c_str());
        std::printf("[STATS] gpu %s, %s\n", s.gpu.c_str(), s.render_path.c_str());
        std::printf("[STATS] nodes %u, meshes %u, materials %u, textures %u, cameras %u, skins %u, bones %u, clips %u\n", s.nodes, s.meshes, s.materials,
                    s.textures, s.cameras, s.skins, s.bones, s.clips);
        std::printf("[STATS] static %llu verts / %u prims, skinned %llu verts / %u prims, %u instances\n", static_cast<unsigned long long>(s.static_vertices),
                    s.static_primitives, static_cast<unsigned long long>(s.skinned_vertices), s.skinned_primitives, s.instances);
        std::printf("[STATS] runs: %u opaque, %u skinned; %u transparent objects\n", s.opaque_runs, s.skinned_runs, s.transparent_objects);

        for (std::size_t l = 0; l < s.lights.size(); l++)
            std::printf("[STATS] light %zu: type %d, intensity %.2f, pos (%.2f, %.2f, %.2f)\n", l, s.lights[l].type, s.lights[l].intensity,
                        s.lights[l].position.x, s.lights[l].position.y, s.lights[l].position.z);

        for (std::size_t c = 0; c < s.clip_names.size(); c++) std::printf("[STATS] clip %zu: '%s'\n", c, s.clip_names[c].c_str());

        for (std::string const& warning : s.warnings) std::printf("[WARN] %s\n", warning.c_str());
    }

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
    // direct link -- Scene -> PeregrineV
    std::shared_ptr<Ir77PVAsset> m_pv_asset{};

    std::shared_ptr<Ir77PVPaint> m_pv_paint{};

    std::size_t m_view_windows{1};

    int m_view_width{1280};

    int m_view_height{720};

    Ir77PVPipelineHandle m_pipeline{IR77PV_NO_PIPELINE};  // what the static runs draw with

    // what CreateScene uploaded
    bool m_scene_ready{false};

    bool m_mesh_path{false};  // static stream on the mesh shading path

    std::uint32_t m_static_id{0};  // UploadScene id, or UploadMeshlets id when m_mesh_path

    std::uint32_t m_skinned_id{0};  // UploadScene id of the skinned stream

    std::uint32_t m_blend_id{0};  // UploadScene id of the compacted static transparent scene

    bool m_has_skinned{false};  // skinned scene uploaded (opaque or transparent)

    std::vector<Ir77SceneRun> m_runs{};

    std::vector<Ir77SceneRun> m_skinned_runs{};  // commands index the skinned scene's buffers

    std::vector<Ir77SceneBlendDraw> m_blend_draws{};

    std::uint32_t m_default_material{0};

    std::vector<std::uint32_t> m_material_ids{};  // [glTF material] -> Asset material id

    glm::vec3 m_bounds_min{-1.0f};

    glm::vec3 m_bounds_max{1.0f};

    Ir77SceneStats m_stats{};  // filled by CreateScene

    // per frame -- what CreateFrame hands to Paint
    std::vector<glm::mat4> m_transforms{};

    std::vector<std::uint32_t> m_bone_bases{};

    std::vector<std::uint8_t> m_bones_packed{};  // bone_base[8192] + palettes, as set 0 binding 4 expects

    std::vector<std::uint8_t> m_lights_packed{};  // Ir77PVLightsHeader + Ir77PVLight[], as set 0 binding 1 expects

    float m_last_seconds{0.0f};  // camera.time.y

    std::uint32_t m_frame_number{0};  // camera.time.z

    std::uint32_t m_opaque_items{0};

    std::uint32_t m_transparent_items{0};

    // cameras -- index m_camera_count is the controlled camera; "next" / F2 cycles
    std::uint32_t m_camera_count{0};

    std::uint32_t m_camera_index{0};

    // controlled camera -- orbit (target, distance) or fly (position); yaw / pitch aim both
    bool m_camera_fly{false};

    glm::vec3 m_camera_target{0.0f};

    glm::vec3 m_camera_position{0.0f};

    float m_camera_distance{1.0f};

    float m_camera_yaw{0.0f};

    float m_camera_pitch{0.0f};

    float m_camera_speed{1.0f};  // units per second at full key

    glm::vec3 m_camera_move{0.0f};  // held keys: right, up, forward in -1..1

    glm::mat4 m_camera_view{1.0f};  // last frame's view, whichever camera drew it

    float m_camera_fov{60.0f};  // vertical, degrees -- taken from whichever camera drew last, so a takeover keeps the lens

    static constexpr float CAMERA_LOOK_RADIANS_PER_PIXEL{0.004f};

    static constexpr float CAMERA_PITCH_LIMIT{1.5f};  // just short of straight up / down
};
}  // namespace NSIr77Scene