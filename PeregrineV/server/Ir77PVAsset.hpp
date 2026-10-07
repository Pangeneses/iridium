#pragma once

#include <SDL3/SDL_video.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <meshoptimizer.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "../server/Ir77PVTypes.hpp"

#include "../../Ir77RT/dictionary/IDIIr77MPVM.hpp"

#include "../dictionary/IDIIr77PeregrineV.hpp"
#include "../dictionary/IDIr77PeregrineV.hpp"

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"

#include "IDIr77PVContext.hpp"

#include "IIr77PVShader.hpp"
#include "Ir77PeregrineV.hpp"
#include "Ir77PVPaint.hpp"

#include "../runtime/Ir77PVBuffer.hpp"
#include "../runtime/Ir77PVDescriptorSet.hpp"
#include "../runtime/Ir77PVShader.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

// =============================================================================================================================================
// Client-facing types -- move to Ir77PVTypes.hpp when convenient
// =============================================================================================================================================

inline constexpr std::uint32_t IR77PV_NO_TEXTURE = UINT32_MAX;

// Texture ids for set 2 bindings 1..5. IR77PV_NO_TEXTURE = placeholder (flat normal for the normal slot).
typedef struct Ir77PVMaterialTextures {
    std::uint32_t albedo{IR77PV_NO_TEXTURE};
    std::uint32_t normal{IR77PV_NO_TEXTURE};
    std::uint32_t orm{IR77PV_NO_TEXTURE};
    std::uint32_t emissive{IR77PV_NO_TEXTURE};
    std::uint32_t ao{IR77PV_NO_TEXTURE};
}* pIr77PVMaterialTextures;

// =============================================================================================================================================
// Ir77PVAsset -- the data binding point between a client (scene loader / game layer) and PeregrineV.
//
//   startup:   SetPeregrineV -> CreateShaders -> (CreatePipelines) -> placeholder textures
//   load:      CreateTexture* -> CreateMaterial / UploadMaterials -> UploadScene (vertex path) or UploadMeshlets (mesh path)
//   frame:     Tick(frame) -> camera / Instances via Paint -> Make*DrawItem per material run -> Paint::SetDraws
//   runtime:   SetSceneObjectVisible / SetSceneObjectInstances / SetMeshletObjectVisible / SetMeshletRecordVisible
//   unload:    remove the draw items from Paint, then UnloadScene / UnloadMeshlets / UnloadMesh (freed after the frames in flight)
//
// Every upload takes `wait`. true = blocks until the copy is done (load screens). false = returns at once; the copy runs in submission
// order ahead of later frames, and Tick frees its staging when the GPU is finished with it.
// =============================================================================================================================================
class Ir77PVAsset : public Ir77Enlisted, public std::enable_shared_from_this<Ir77PVAsset> {
   private:
    // vertex path scene -- one buffer per channel for every stacked object
    struct Ir77PVSceneSlot {
        std::shared_ptr<IIr77PVBuffer> vertex{};
        std::shared_ptr<IIr77PVBuffer> index{};
        std::shared_ptr<IIr77PVBuffer> vertex_skinned{};
        std::shared_ptr<IIr77PVBuffer> index_skinned{};
        std::shared_ptr<IIr77PVBuffer> indirect{};             // Dynamic -- rewritten by Tick when dirty
        std::vector<VkDrawIndexedIndirectCommand> commands{};  // live values -- instanceCount 0 = hidden
        std::vector<std::uint32_t> instance_counts{};          // what a visible command draws
        std::vector<std::uint8_t> visible{};
        std::uint32_t dirty{0};  // frame copies still to rewrite
        bool alive{false};
    };

    // mesh path scene -- set 3 buffers plus the per-record command state
    struct Ir77PVMeshletSlot {
        Ir77PVMeshletMesh mesh{};
        std::vector<VkDrawMeshTasksIndirectCommandEXT> commands{};  // live, one per record -- groupCountX 0 = hidden
        std::vector<std::uint32_t> groups{};                        // what a visible record dispatches
        std::vector<std::uint32_t> first_record{};                  // per scene command, plus one end entry
        std::uint32_t dirty{0};
        bool alive{false};
    };

    struct Ir77PVTextureSlot {
        VkImage image{VK_NULL_HANDLE};
        VmaAllocation allocation{VK_NULL_HANDLE};
        VkImageView view{VK_NULL_HANDLE};
        bool alive{false};
    };

    // an upload submitted with wait = false -- released by Tick once its fence signals
    struct Ir77PVPending {
        VkFence fence{VK_NULL_HANDLE};
        VkCommandBuffer cmd{VK_NULL_HANDLE};
        VkCommandPool pool{VK_NULL_HANDLE};
        VkBuffer staging{VK_NULL_HANDLE};
        VmaAllocation staging_allocation{VK_NULL_HANDLE};
    };

    // unloaded resources -- held until every frame that could still read them has finished
    struct Ir77PVGrave {
        std::vector<std::shared_ptr<IIr77PVBuffer>> buffers{};
        std::shared_ptr<IIr77PVDescriptorSet> set{};
        std::uint32_t frames_left{0};
    };

    struct Ir77PVTestbed {
        std::uint32_t mesh{0};
        std::uint32_t scene{0};
        std::uint32_t meshlets{0};
        std::uint32_t material{0};
        std::uint32_t grid{3};
        bool use_mesh{false};
        bool use_indirect{false};
        bool ready{false};
    };

   public:
    Ir77PVAsset() {
        try {
            m_enlisted_uuid.Generate();
        } catch (std::invalid_argument a) {
            throw a;
        }

        m_enlisted = std::chrono::system_clock::now();
    }

    // m_context outlives this object (held by shared_ptr), so devices and allocators are still alive here
    ~Ir77PVAsset() {
        if (!m_context) return;

        for (auto const& [device_id, device] : m_context->m_devices) {
            VkDevice vk_device{VK_NULL_HANDLE};
            device->GetDevice(&vk_device);
            if (vk_device == VK_NULL_HANDLE) continue;

            vkDeviceWaitIdle(vk_device);

            auto const allocator_found = m_context->m_allocators.find(device_id);
            VmaAllocator const allocator = allocator_found != m_context->m_allocators.end() ? allocator_found->second : VK_NULL_HANDLE;

            // uploads still pending -- finished after the wait
            auto const pending = m_pending.find(device_id);
            if (pending != m_pending.end()) {
                for (auto const& p : pending->second) ReleasePending(vk_device, allocator, p);
                pending->second.clear();
            }

            auto const textures = m_textures.find(device_id);
            if (textures != m_textures.end()) {
                for (auto& t : textures->second) {
                    if (t.view != VK_NULL_HANDLE) vkDestroyImageView(vk_device, t.view, nullptr);
                    if (t.image != VK_NULL_HANDLE && allocator != VK_NULL_HANDLE) vmaDestroyImage(allocator, t.image, t.allocation);
                }
                textures->second.clear();
            }

            auto const sampler = m_samplers.find(device_id);
            if (sampler != m_samplers.end()) vkDestroySampler(vk_device, sampler->second, nullptr);

            auto const pool = m_upload_pools.find(device_id);
            if (pool != m_upload_pools.end()) vkDestroyCommandPool(vk_device, pool->second, nullptr);
        }

        m_graveyard.clear();  // buffers are still listed in the context slots, which tear them down
    }

   public:
    std::shared_ptr<IIr77Return const> EnlistedAs(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIIr77Enlisted>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> MemberUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77PVAsset>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CollectionUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77PeregrineV>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    Ir77GUID QueryInterface(Ir77GUID iid, std::shared_ptr<void>& obj) {
        if (iid == GUIDIIr77Enlisted)
            obj = std::shared_ptr<IIr77Enlisted>(shared_from_this(), static_cast<IIr77Enlisted*>(this));

        else if (iid == GUIDIr77PVAsset)
            obj = std::shared_ptr<Ir77PVAsset>(shared_from_this(), static_cast<Ir77PVAsset*>(this));

        else
            return GUIDQueryFailed;

        return GUIDQuerySucceeded;
    }

   public:
    std::shared_ptr<IIr77Return const> SetPeregrineV(std::shared_ptr<Ir77PeregrineV>& context) {
        m_context = context;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SetShaderRoot(std::string const& root) {
        m_shader_root = root;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Camera for set 0 binding 0. Vulkan clip space is Y-down, so the projection is flipped.
    static Ir77PVCamera MakeCamera(glm::vec3 const& eye, glm::vec3 const& target, float const& fov_degrees, float const& aspect) {
        Ir77PVCamera camera{};

        camera.eye = glm::vec4(eye, 1.0f);
        camera.view = glm::lookAtRH(eye, target, glm::vec3(0.0f, 1.0f, 0.0f));
        camera.proj = glm::perspectiveRH_ZO(glm::radians(fov_degrees), aspect, 0.1f, 100.0f);
        camera.proj[1][1] *= -1.0f;
        camera.view_proj = camera.proj * camera.view;

        return camera;
    }

    // =========================================================================================================================================
    // per frame -- call once per frame, before Paint records, with the frame-in-flight index Paint is about to record.
    //   1. frees staging / command buffers of uploads whose fence has signalled
    //   2. writes this frame's copy of every dirty indirect buffer (visibility / instance changes)
    //   3. ages the graveyard and releases anything no frame in flight can still read
    // =========================================================================================================================================
    std::shared_ptr<IIr77Return const> Tick(std::uint32_t const& frame) {
        if (frame >= MAX_FRAMES_IN_FLIGHT) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: frame index out of range.");

        std::uint64_t const device_id = m_context->m_current_device;

        VkDevice device{VK_NULL_HANDLE};
        m_context->m_devices.at(device_id)->GetDevice(&device);
        VmaAllocator const allocator = m_context->m_allocators.at(device_id);

        // 1. finished uploads
        auto& pending = m_pending[device_id];
        std::size_t keep{0};
        for (std::size_t i = 0; i < pending.size(); i++) {
            if (vkGetFenceStatus(device, pending[i].fence) == VK_SUCCESS)
                ReleasePending(device, allocator, pending[i]);
            else
                pending[keep++] = pending[i];
        }
        pending.resize(keep);

        // 2. dirty indirect copies -- one frame copy per Tick until every copy matches
        for (auto& scene : m_scenes[device_id]) {
            if (!scene.alive || scene.dirty == 0 || !scene.indirect) continue;

            VkDeviceSize const size = static_cast<VkDeviceSize>(scene.commands.size() * sizeof(VkDrawIndexedIndirectCommand));
            if (scene.indirect->Update(frame, scene.commands.data(), size)->ID() != GUIDIr77OperationSucceeded)
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: scene indirect write failed.");

            scene.dirty--;
        }

        for (auto& slot : m_meshlet_scenes[device_id]) {
            if (!slot.alive || slot.dirty == 0 || !slot.mesh.indirect) continue;

            VkDeviceSize const size = static_cast<VkDeviceSize>(slot.commands.size() * sizeof(VkDrawMeshTasksIndirectCommandEXT));
            if (slot.mesh.indirect->Update(frame, slot.commands.data(), size)->ID() != GUIDIr77OperationSucceeded)
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: meshlet indirect write failed.");

            slot.dirty--;
        }

        // 3. graveyard
        auto& graves = m_graveyard[device_id];
        auto& vertex_slot = m_context->m_buffers_vertex[device_id];
        auto& index_slot = m_context->m_buffers_index[device_id];

        keep = 0;
        for (std::size_t i = 0; i < graves.size(); i++) {
            if (graves[i].frames_left > 0) {
                graves[i].frames_left--;
                if (keep != i) graves[keep] = std::move(graves[i]);  // self-move would empty the vectors
                keep++;
                continue;
            }

            for (auto const& buffer : graves[i].buffers) {
                std::erase(vertex_slot, buffer);
                std::erase(index_slot, buffer);
            }
        }
        graves.resize(keep);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // =========================================================================================================================================
    // shaders
    // =========================================================================================================================================

    // Loads every shader file that exists under the root. Required ones fail the call; optional ones (skinned,
    // shadow, mesh) are skipped, and the pipeline kinds they unlock are reported by GetPipelineKinds.
    std::shared_ptr<IIr77Return const> CreateShaders() {
        bool mesh_shading{false};
        m_context->m_devices.at(m_context->m_current_device)->GetMeshShading(mesh_shading);

        std::vector<Ir77PVShaderFile> const files{
            {"vert.spv", ID_SHADER_VERT, Ir77PVShaderStage::Vertex, true},
            {"frag.spv", ID_SHADER_FRAG, Ir77PVShaderStage::Fragment, true},
            {"cef.vert.spv", ID_SHADER_CEF_VERT, Ir77PVShaderStage::Vertex, true},
            {"cef.frag.spv", ID_SHADER_CEF_FRAG, Ir77PVShaderStage::Fragment, true},
            {"skinned.vert.spv", ID_SHADER_SKINNED_VERT, Ir77PVShaderStage::Vertex, false},
            {"shadow.vert.spv", ID_SHADER_SHADOW_VERT, Ir77PVShaderStage::Vertex, false},
            {"shadow_skinned.vert.spv", ID_SHADER_SHADOW_SKINNED_VERT, Ir77PVShaderStage::Vertex, false},
            {"mesh.task.spv", ID_SHADER_MESH_TASK, Ir77PVShaderStage::Task, false, true},
            {"mesh.mesh.spv", ID_SHADER_MESH, Ir77PVShaderStage::Mesh, false, true},
        };

        auto shader = std::static_pointer_cast<IIr77PVShader>(std::make_shared<Ir77PVShader>());

        shader->SetDevice(m_context->m_devices.at(m_context->m_current_device));

        std::map<std::uint64_t, bool> loaded{};

        for (auto const& entry : files) {
            if (entry.mesh_shading && !mesh_shading) continue;

            std::filesystem::path const path = std::filesystem::path(m_shader_root) / entry.file;

            if (!std::filesystem::exists(path)) {
                if (entry.required) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: required shader missing.");
                continue;
            }

            if (shader->LoadShader(path.string(), entry.id, entry.stage)->ID() != GUIDIr77OperationSucceeded) {
                if (entry.required) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: required shader failed to load.");
                continue;
            }

            loaded[entry.id] = true;
        }

        // pipeline kinds whose every stage loaded
        std::vector<Ir77PVPipelineKind> kinds{Ir77PVPipelineKind::Static, Ir77PVPipelineKind::Transparent, Ir77PVPipelineKind::CEF};

        if (loaded.count(ID_SHADER_SKINNED_VERT)) kinds.push_back(Ir77PVPipelineKind::Skinned);
        if (loaded.count(ID_SHADER_SHADOW_VERT)) kinds.push_back(Ir77PVPipelineKind::Shadow);
        if (loaded.count(ID_SHADER_SHADOW_SKINNED_VERT)) kinds.push_back(Ir77PVPipelineKind::ShadowSkinned);
        if (loaded.count(ID_SHADER_MESH_TASK) && loaded.count(ID_SHADER_MESH)) kinds.push_back(Ir77PVPipelineKind::Mesh);

        m_pipeline_kinds[m_context->m_current_device] = kinds;

        m_context->m_shaders[m_context->m_current_device] = shader;

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: Create Shaders.");
    }

    std::shared_ptr<IIr77Return const> GetPipelineKinds(std::vector<Ir77PVPipelineKind>& kinds) {
        auto const found = m_pipeline_kinds.find(m_context->m_current_device);
        if (found == m_pipeline_kinds.end()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: CreateShaders not called.");

        kinds = found->second;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // =========================================================================================================================================
    // single-mesh path -- one Static vertex + index buffer pair per mesh. Testbed, procedural / debug geometry, one-off loads.
    // =========================================================================================================================================
    std::shared_ptr<IIr77Return const> UploadMesh(Ir77PVInputBuffer const& input, std::uint32_t& mesh_id, bool const& wait = true) {
        if (input.vertex_data.empty() || input.index_data.empty()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: empty mesh.");

        VkCommandPool pool{VK_NULL_HANDLE};
        VkQueue queue{VK_NULL_HANDLE};
        if (!UploadQueue(&pool, &queue)) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: no upload queue.");

        VkDeviceSize const vertex_size = static_cast<VkDeviceSize>(input.vertex_data.size() * sizeof(Ir77PVVertex));
        VkDeviceSize const index_size = static_cast<VkDeviceSize>(input.index_data.size() * sizeof(std::uint32_t));

        Ir77PVMesh mesh{};

        mesh.vertex = CreateStaticBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vertex_size);
        mesh.index = CreateStaticBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, index_size);

        if (!mesh.vertex || !mesh.index) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: mesh buffer creation failed.");

        VkBuffer vertex_dst{VK_NULL_HANDLE};
        VkBuffer index_dst{VK_NULL_HANDLE};
        mesh.vertex->GetBuffer(0, &vertex_dst);
        mesh.index->GetBuffer(0, &index_dst);

        if (UploadBatch(pool, queue, {{vertex_dst, input.vertex_data.data(), vertex_size}, {index_dst, input.index_data.data(), index_size}}, wait)
                ->ID() != GUIDIr77OperationSucceeded) {
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: mesh upload failed.");
        }

        mesh.index_count = static_cast<std::uint32_t>(input.index_data.size());
        mesh.index_type = VK_INDEX_TYPE_UINT32;

        auto& meshes = m_meshes[m_context->m_current_device];
        mesh_id = static_cast<std::uint32_t>(meshes.size());
        meshes.push_back(mesh);

        // mirrored into the context slots so its teardown order covers them
        m_context->m_buffers_vertex[m_context->m_current_device].push_back(mesh.vertex);
        m_context->m_buffers_index[m_context->m_current_device].push_back(mesh.index);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetMesh(std::uint32_t const& mesh_id, Ir77PVMesh& mesh) {
        auto const found = m_meshes.find(m_context->m_current_device);
        if (found == m_meshes.end() || mesh_id >= found->second.size() || !found->second[mesh_id].vertex)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: unknown mesh.");

        mesh = found->second[mesh_id];

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // The id stays reserved; GetMesh fails for it afterwards. Remove its draw items from Paint first.
    std::shared_ptr<IIr77Return const> UnloadMesh(std::uint32_t const& mesh_id) {
        auto const found = m_meshes.find(m_context->m_current_device);
        if (found == m_meshes.end() || mesh_id >= found->second.size() || !found->second[mesh_id].vertex)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: unknown mesh.");

        Ir77PVMesh& mesh = found->second[mesh_id];
        Bury({mesh.vertex, mesh.index}, nullptr);
        mesh = Ir77PVMesh{};

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // =========================================================================================================================================
    // scene path (vertex) -- the client stacks every object into one Ir77PVSceneUpload and prebuilds one indirect command per object:
    //   firstIndex / indexCount   -> its slice of indices (indices stay local, 0-based)
    //   vertexOffset              -> where its vertices start
    //   instanceCount / firstInstance -> how many copies, and their slots in models[] (set 0 binding 2)
    // Sort objects by material so each material is one contiguous run of commands; one draw item per run (MakeIndirectDrawItem).
    // The indirect buffer is Dynamic, so objects can be hidden / shown / resized per frame without re-uploading geometry.
    // =========================================================================================================================================
    std::shared_ptr<IIr77Return const> UploadScene(Ir77PVSceneUpload const& data, std::uint32_t& scene_id, bool const& wait = true) {
        if (data.vertices.empty() && data.vertices_skinned.empty()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: empty scene.");

        VkCommandPool pool{VK_NULL_HANDLE};
        VkQueue queue{VK_NULL_HANDLE};
        if (!UploadQueue(&pool, &queue)) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: no upload queue.");

        Ir77PVSceneSlot scene{};
        std::vector<Ir77PVUploadEntry> entries{};

        if (!data.vertices.empty()) {
            VkDeviceSize const size = static_cast<VkDeviceSize>(data.vertices.size() * sizeof(Ir77PVVertex));
            scene.vertex = CreateStaticBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, size);
            if (!scene.vertex) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: scene vertex buffer creation failed.");

            VkBuffer dst{VK_NULL_HANDLE};
            scene.vertex->GetBuffer(0, &dst);
            entries.push_back({dst, data.vertices.data(), size});
        }

        if (!data.indices.empty()) {
            VkDeviceSize const size = static_cast<VkDeviceSize>(data.indices.size() * sizeof(std::uint32_t));
            scene.index = CreateStaticBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, size);
            if (!scene.index) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: scene index buffer creation failed.");

            VkBuffer dst{VK_NULL_HANDLE};
            scene.index->GetBuffer(0, &dst);
            entries.push_back({dst, data.indices.data(), size});
        }

        if (!data.vertices_skinned.empty()) {
            VkDeviceSize const size = static_cast<VkDeviceSize>(data.vertices_skinned.size() * sizeof(Ir77PVVertexSkinned));
            scene.vertex_skinned = CreateStaticBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, size);
            if (!scene.vertex_skinned) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: scene skinned vertex buffer creation failed.");

            VkBuffer dst{VK_NULL_HANDLE};
            scene.vertex_skinned->GetBuffer(0, &dst);
            entries.push_back({dst, data.vertices_skinned.data(), size});
        }

        if (!data.indices_skinned.empty()) {
            VkDeviceSize const size = static_cast<VkDeviceSize>(data.indices_skinned.size() * sizeof(std::uint32_t));
            scene.index_skinned = CreateStaticBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, size);
            if (!scene.index_skinned) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: scene skinned index buffer creation failed.");

            VkBuffer dst{VK_NULL_HANDLE};
            scene.index_skinned->GetBuffer(0, &dst);
            entries.push_back({dst, data.indices_skinned.data(), size});
        }

        // indirect -- Dynamic, every frame copy written directly (not staged)
        if (!data.indirect_commands.empty()) {
            VkDeviceSize const size = static_cast<VkDeviceSize>(data.indirect_commands.size() * sizeof(VkDrawIndexedIndirectCommand));
            scene.indirect = CreateDynamicBuffer(VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, size);
            if (!scene.indirect) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: scene indirect buffer creation failed.");

            for (std::uint32_t frame = 0; frame < MAX_FRAMES_IN_FLIGHT; frame++) {
                if (scene.indirect->Update(frame, data.indirect_commands.data(), size)->ID() != GUIDIr77OperationSucceeded)
                    return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: scene indirect write failed.");
            }

            scene.commands = data.indirect_commands;
            scene.instance_counts.reserve(scene.commands.size());
            for (auto const& cmd : scene.commands) scene.instance_counts.push_back(cmd.instanceCount);
            scene.visible.assign(scene.commands.size(), 1);
        }

        if (UploadBatch(pool, queue, entries, wait)->ID() != GUIDIr77OperationSucceeded)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: scene upload failed.");

        scene.alive = true;

        std::uint64_t const device_id = m_context->m_current_device;

        auto& vertex_slot = m_context->m_buffers_vertex[device_id];
        auto& index_slot = m_context->m_buffers_index[device_id];
        if (scene.vertex) vertex_slot.push_back(scene.vertex);
        if (scene.vertex_skinned) vertex_slot.push_back(scene.vertex_skinned);
        if (scene.indirect) vertex_slot.push_back(scene.indirect);
        if (scene.index) index_slot.push_back(scene.index);
        if (scene.index_skinned) index_slot.push_back(scene.index_skinned);

        auto& scenes = m_scenes[device_id];
        scene_id = static_cast<std::uint32_t>(scenes.size());
        scenes.push_back(std::move(scene));

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Draw item for commands [first_command, first_command + command_count) of a scene -- one material run.
    std::shared_ptr<IIr77Return const> MakeIndirectDrawItem(std::size_t const& window, Ir77PVPipelineKind const& kind, std::uint32_t const& scene_id,
                                                            std::uint32_t const& material_id, std::uint32_t const& first_command,
                                                            std::uint32_t const& command_count, bool const& skinned, Ir77PVDrawItem& item) {
        Ir77PVSceneSlot* const scene = FindScene(scene_id);
        if (!scene) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: unknown scene.");

        std::shared_ptr<IIr77PVBuffer> const& vertex = skinned ? scene->vertex_skinned : scene->vertex;
        std::shared_ptr<IIr77PVBuffer> const& index = skinned ? scene->index_skinned : scene->index;

        if (!vertex || !index || !scene->indirect) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: scene channel empty.");

        if (static_cast<std::size_t>(first_command) + command_count > scene->commands.size())
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: command range outside the scene.");

        Ir77PVMaterial material{};
        if (GetMaterial(material_id, material)->ID() != GUIDIr77OperationSucceeded)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: unknown material.");

        item = Ir77PVDrawItem{};

        if (m_context->GetPipeline(window, kind, item.pipeline)->ID() != GUIDIr77OperationSucceeded)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: pipeline kind not created for window.");

        if (m_context->GetLayout(LayoutKindFor(kind), item.layout)->ID() != GUIDIr77OperationSucceeded)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: layout not created.");

        item.material = material.set;
        item.vertex = vertex;
        item.index = index;
        item.index_type = VK_INDEX_TYPE_UINT32;
        item.indirect = scene->indirect;
        item.indirect_offset = static_cast<VkDeviceSize>(first_command) * sizeof(VkDrawIndexedIndirectCommand);
        item.indirect_count = command_count;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Hide / show one object (every instance of it). Applied to each frame copy by Tick.
    std::shared_ptr<IIr77Return const> SetSceneObjectVisible(std::uint32_t const& scene_id, std::uint32_t const& command, bool const& visible) {
        Ir77PVSceneSlot* const scene = FindScene(scene_id);
        if (!scene || command >= scene->commands.size()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: unknown scene object.");

        scene->visible[command] = visible ? 1 : 0;
        scene->commands[command].instanceCount = visible ? scene->instance_counts[command] : 0;
        scene->dirty = MAX_FRAMES_IN_FLIGHT;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Change how many copies of an object draw -- spawn / despawn inside the models[] slots the client reserved for it
    // ([firstInstance, firstInstance + count) must not run into the next object's slots).
    std::shared_ptr<IIr77Return const> SetSceneObjectInstances(std::uint32_t const& scene_id, std::uint32_t const& command, std::uint32_t const& count) {
        Ir77PVSceneSlot* const scene = FindScene(scene_id);
        if (!scene || command >= scene->commands.size()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: unknown scene object.");

        scene->instance_counts[command] = count;
        if (scene->visible[command]) scene->commands[command].instanceCount = count;
        scene->dirty = MAX_FRAMES_IN_FLIGHT;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Frees the scene's buffers once no frame in flight can read them. Remove its draw items from Paint first.
    std::shared_ptr<IIr77Return const> UnloadScene(std::uint32_t const& scene_id) {
        Ir77PVSceneSlot* const scene = FindScene(scene_id);
        if (!scene) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: unknown scene.");

        Bury({scene->vertex, scene->index, scene->vertex_skinned, scene->index_skinned, scene->indirect}, nullptr);
        *scene = Ir77PVSceneSlot{};

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // =========================================================================================================================================
    // meshlet path -- Mesh pipeline kind only (VK_EXT_mesh_shader). Takes the same stacked Ir77PVSceneUpload as UploadScene.
    // Each indirect command is one object's range; meshlets are cut from that range only, so none ever spans two objects.
    // One draw record + one mesh command per instance. Static (non-skinned) vertices only.
    // record_count = records written. Fails when the Mesh layout wasn't created -- the device has no mesh shading.
    // =========================================================================================================================================
    std::shared_ptr<IIr77Return const> UploadMeshlets(Ir77PVSceneUpload const& scene, std::uint32_t& meshlet_id, std::uint32_t& record_count,
                                                      bool const& wait = true) {
        if (scene.vertices.empty() || scene.indices.empty() || scene.indirect_commands.empty())
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: empty scene.");

        std::shared_ptr<IIr77PVLayout> layout{};
        if (m_context->GetLayout(Ir77PVLayoutKind::Mesh, layout)->ID() != GUIDIr77OperationSucceeded)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: Mesh layout not created -- mesh shading unsupported.");

        VkCommandPool pool{VK_NULL_HANDLE};
        VkQueue queue{VK_NULL_HANDLE};
        if (!UploadQueue(&pool, &queue)) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: no upload queue.");

        // shared results -- every object appended here
        std::vector<Ir77PVMeshlet> meshlets{};
        std::vector<std::uint32_t> meshlet_vertices{};
        std::vector<std::uint8_t> meshlet_triangles{};
        std::vector<Ir77PVMeshDraw> draws{};
        std::vector<VkDrawMeshTasksIndirectCommandEXT> commands{};
        std::vector<std::uint32_t> groups_per_record{};
        std::vector<std::uint32_t> first_record{};
        first_record.reserve(scene.indirect_commands.size() + 1);

        // per-object scratch, reused
        std::vector<meshopt_Meshlet> built{};
        std::vector<std::uint32_t> local_vertices{};
        std::vector<std::uint8_t> local_triangles{};

        for (std::size_t c = 0; c < scene.indirect_commands.size(); c++) {
            first_record.push_back(static_cast<std::uint32_t>(draws.size()));  // every command gets an entry, even when it draws nothing

            VkDrawIndexedIndirectCommand const& cmd = scene.indirect_commands[c];
            if (cmd.indexCount == 0) continue;

            // 1. this object's slice
            if (cmd.vertexOffset < 0 || static_cast<std::size_t>(cmd.firstIndex) + cmd.indexCount > scene.indices.size() ||
                static_cast<std::size_t>(cmd.vertexOffset) >= scene.vertices.size())
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: indirect command outside the scene buffers.");

            std::uint32_t const* indices = &scene.indices[cmd.firstIndex];
            Ir77PVVertex const* vertices = &scene.vertices[static_cast<std::size_t>(cmd.vertexOffset)];

            // 2. vertices this object uses -- indices are local, so highest + 1
            std::size_t const local_count = static_cast<std::size_t>(*std::max_element(indices, indices + cmd.indexCount)) + 1;
            if (static_cast<std::size_t>(cmd.vertexOffset) + local_count > scene.vertices.size())
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: object indices run past the vertex buffer.");

            // 3. build on the slice only -- the builder can't see any other object
            std::size_t const max_meshlets = meshopt_buildMeshletsBound(cmd.indexCount, MESHLET_MAX_VERTICES, MESHLET_MAX_TRIANGLES);
            built.assign(max_meshlets, meshopt_Meshlet{});
            local_vertices.assign(max_meshlets * MESHLET_MAX_VERTICES, 0);
            local_triangles.assign(max_meshlets * MESHLET_MAX_TRIANGLES * 3, 0);

            float const* positions = &vertices[0].position.x;

            std::size_t const count = meshopt_buildMeshlets(built.data(), local_vertices.data(), local_triangles.data(), indices, cmd.indexCount, positions,
                                                            local_count, sizeof(Ir77PVVertex), MESHLET_MAX_VERTICES, MESHLET_MAX_TRIANGLES, MESHLET_CONE_WEIGHT);
            if (count == 0) continue;
            built.resize(count);

            // where this object starts in the shared arrays
            std::uint32_t const meshlet_base = static_cast<std::uint32_t>(meshlets.size());
            std::uint32_t const vertex_base = static_cast<std::uint32_t>(meshlet_vertices.size());
            std::uint32_t const triangle_base = static_cast<std::uint32_t>(meshlet_triangles.size());  // stays a 4-byte multiple

            // 4. optimize + bounds (object-local space, same space models[] transforms from)
            for (std::size_t i = 0; i < count; i++) {
                meshopt_Meshlet const& m = built[i];

                meshopt_optimizeMeshlet(&local_vertices[m.vertex_offset], &local_triangles[m.triangle_offset], m.triangle_count, m.vertex_count);

                meshopt_Bounds const bounds = meshopt_computeMeshletBounds(&local_vertices[m.vertex_offset], &local_triangles[m.triangle_offset],
                                                                           m.triangle_count, positions, local_count, sizeof(Ir77PVVertex));

                Ir77PVMeshlet out{};
                out.sphere = {bounds.center[0], bounds.center[1], bounds.center[2], bounds.radius};
                out.cone = {bounds.cone_axis[0], bounds.cone_axis[1], bounds.cone_axis[2], bounds.cone_cutoff};
                out.vertex_offset = vertex_base + m.vertex_offset;  // shifted into the shared array
                out.triangle_offset = triangle_base + m.triangle_offset;
                out.vertex_count = m.vertex_count;
                out.triangle_count = m.triangle_count;
                meshlets.push_back(out);
            }

            // 5. append -- local vertex index + vertexOffset = index into the full vertex buffer
            meshopt_Meshlet const& last = built.back();
            std::size_t const used_vertices = last.vertex_offset + last.vertex_count;
            std::size_t const used_triangles = last.triangle_offset + ((last.triangle_count * 3 + 3) & ~3u);

            for (std::size_t j = 0; j < used_vertices; j++)
                meshlet_vertices.push_back(local_vertices[j] + static_cast<std::uint32_t>(cmd.vertexOffset));

            meshlet_triangles.insert(meshlet_triangles.end(), local_triangles.begin(), local_triangles.begin() + used_triangles);

            // 6. this object = meshlets [meshlet_base, meshlet_base + count). One record per instance, same meaning as the vertex path.
            std::uint32_t const groups = (static_cast<std::uint32_t>(count) + 31) / 32;  // TASK_GROUP_SIZE

            for (std::uint32_t inst = 0; inst < cmd.instanceCount; inst++) {
                Ir77PVMeshDraw draw{};
                draw.instance = cmd.firstInstance + inst;
                draw.meshlet_offset = meshlet_base;
                draw.meshlet_count = static_cast<std::uint32_t>(count);

                draws.push_back(draw);
                commands.push_back(VkDrawMeshTasksIndirectCommandEXT{groups, 1, 1});
                groups_per_record.push_back(groups);
            }
        }

        first_record.push_back(static_cast<std::uint32_t>(draws.size()));  // end entry

        if (meshlets.empty() || draws.empty()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: scene produced no meshlets or draws.");

        // 7. upload
        VkDeviceSize const meshlets_size = static_cast<VkDeviceSize>(meshlets.size() * sizeof(Ir77PVMeshlet));
        VkDeviceSize const indices_size = static_cast<VkDeviceSize>(meshlet_vertices.size() * sizeof(std::uint32_t));
        VkDeviceSize const triangles_size = static_cast<VkDeviceSize>(meshlet_triangles.size());
        VkDeviceSize const vertices_size = static_cast<VkDeviceSize>(scene.vertices.size() * sizeof(Ir77PVVertex));
        VkDeviceSize const draws_size = static_cast<VkDeviceSize>(draws.size() * sizeof(Ir77PVMeshDraw));
        VkDeviceSize const indirect_size = static_cast<VkDeviceSize>(commands.size() * sizeof(VkDrawMeshTasksIndirectCommandEXT));

        Ir77PVMeshletSlot slot{};
        Ir77PVMeshletMesh& mesh = slot.mesh;

        mesh.meshlets = CreateStaticBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, meshlets_size);
        mesh.meshlet_vertices = CreateStaticBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, indices_size);
        mesh.meshlet_triangles = CreateStaticBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, triangles_size);
        mesh.vertices = CreateStaticBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, vertices_size);
        mesh.draws = CreateStaticBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, draws_size);

        if (!mesh.meshlets || !mesh.meshlet_vertices || !mesh.meshlet_triangles || !mesh.vertices || !mesh.draws)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: meshlet buffer creation failed.");

        // indirect -- Dynamic, so records can be hidden per frame ({0,1,1}). Written directly, not staged.
        mesh.indirect = CreateDynamicBuffer(VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT, indirect_size);
        if (!mesh.indirect) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: meshlet indirect buffer creation failed.");

        for (std::uint32_t frame = 0; frame < MAX_FRAMES_IN_FLIGHT; frame++) {
            if (mesh.indirect->Update(frame, commands.data(), indirect_size)->ID() != GUIDIr77OperationSucceeded)
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: meshlet indirect write failed.");
        }

        VkBuffer meshlets_dst{VK_NULL_HANDLE};
        VkBuffer indices_dst{VK_NULL_HANDLE};
        VkBuffer triangles_dst{VK_NULL_HANDLE};
        VkBuffer vertices_dst{VK_NULL_HANDLE};
        VkBuffer draws_dst{VK_NULL_HANDLE};
        mesh.meshlets->GetBuffer(0, &meshlets_dst);
        mesh.meshlet_vertices->GetBuffer(0, &indices_dst);
        mesh.meshlet_triangles->GetBuffer(0, &triangles_dst);
        mesh.vertices->GetBuffer(0, &vertices_dst);
        mesh.draws->GetBuffer(0, &draws_dst);

        if (UploadBatch(pool, queue,
                        {{meshlets_dst, meshlets.data(), meshlets_size},
                         {indices_dst, meshlet_vertices.data(), indices_size},
                         {triangles_dst, meshlet_triangles.data(), triangles_size},
                         {vertices_dst, scene.vertices.data(), vertices_size},
                         {draws_dst, draws.data(), draws_size}},
                        wait)
                ->ID() != GUIDIr77OperationSucceeded) {
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: meshlet upload failed.");
        }

        // 8. set 3
        mesh.set = std::static_pointer_cast<IIr77PVDescriptorSet>(std::make_shared<Ir77PVDescriptorSet>());

        mesh.set->SetDevice(m_context->m_devices.at(m_context->m_current_device));
        mesh.set->SetLayout(layout, 3);

        if (mesh.set->CreateSets(1)->ID() != GUIDIr77OperationSucceeded)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: meshlet set allocation failed -- raise the Mesh layout capacity.");

        mesh.set->BindBuffer(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, mesh.meshlets);
        mesh.set->BindBuffer(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, mesh.meshlet_vertices);
        mesh.set->BindBuffer(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, mesh.meshlet_triangles);
        mesh.set->BindBuffer(3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, mesh.vertices);
        mesh.set->BindBuffer(4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, mesh.draws);

        if (mesh.set->Write()->ID() != GUIDIr77OperationSucceeded) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: meshlet set write failed.");

        mesh.meshlet_count = static_cast<std::uint32_t>(meshlets.size());
        record_count = static_cast<std::uint32_t>(draws.size());

        slot.commands = std::move(commands);
        slot.groups = std::move(groups_per_record);
        slot.first_record = std::move(first_record);
        slot.alive = true;

        std::uint64_t const device_id = m_context->m_current_device;

        // mirrored into the context slots so its teardown order covers them
        auto& vertex_slot = m_context->m_buffers_vertex[device_id];
        vertex_slot.push_back(mesh.meshlets);
        vertex_slot.push_back(mesh.meshlet_vertices);
        vertex_slot.push_back(mesh.meshlet_triangles);
        vertex_slot.push_back(mesh.vertices);
        vertex_slot.push_back(mesh.draws);
        vertex_slot.push_back(mesh.indirect);

        auto& slots = m_meshlet_scenes[device_id];
        meshlet_id = static_cast<std::uint32_t>(slots.size());
        slots.push_back(std::move(slot));

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetMeshlets(std::uint32_t const& meshlet_id, Ir77PVMeshletMesh& mesh) {
        Ir77PVMeshletSlot* const slot = FindMeshlets(meshlet_id);
        if (!slot) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: unknown meshlet mesh.");

        mesh = slot->mesh;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Direct (non-indirect) draw of meshlets [0, meshlet_count) -- single-object meshlet meshes only.
    std::shared_ptr<IIr77Return const> MakeMeshletDrawItem(std::size_t const& window, std::uint32_t const& meshlet_id, std::uint32_t const& material_id,
                                                           Ir77PVDrawItem& item) {
        Ir77PVMeshletMesh mesh{};
        Ir77PVMaterial material{};

        if (GetMeshlets(meshlet_id, mesh)->ID() != GUIDIr77OperationSucceeded) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: unknown meshlet mesh.");
        if (GetMaterial(material_id, material)->ID() != GUIDIr77OperationSucceeded)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: unknown material.");

        item = Ir77PVDrawItem{};

        if (m_context->GetPipeline(window, Ir77PVPipelineKind::Mesh, item.pipeline)->ID() != GUIDIr77OperationSucceeded)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: Mesh pipeline not created for window.");

        if (m_context->GetLayout(Ir77PVLayoutKind::Mesh, item.layout)->ID() != GUIDIr77OperationSucceeded)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: Mesh layout not created.");

        item.material = material.set;
        item.meshlets = mesh.set;
        item.meshlet_offset = 0;
        item.meshlet_count = mesh.meshlet_count;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // One indirect draw for records [first_record, first_record + record_count).
    // indirect_offset and draw_base both start at first_record, so gl_DrawID 0..record_count-1 lines up with the records.
    std::shared_ptr<IIr77Return const> MakeMeshletIndirectDrawItem(std::size_t const& window, std::uint32_t const& meshlet_id, std::uint32_t const& material_id,
                                                                   std::uint32_t const& first_record, std::uint32_t const& record_count, Ir77PVDrawItem& item) {
        Ir77PVMeshletSlot* const slot = FindMeshlets(meshlet_id);
        if (!slot) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: unknown meshlet mesh.");

        if (static_cast<std::size_t>(first_record) + record_count > slot->commands.size())
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: record range outside the meshlet mesh.");

        auto const result = MakeMeshletDrawItem(window, meshlet_id, material_id, item);
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        item.indirect = slot->mesh.indirect;
        item.indirect_offset = static_cast<VkDeviceSize>(first_record) * sizeof(VkDrawMeshTasksIndirectCommandEXT);
        item.indirect_count = record_count;
        item.draw_base = first_record;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Same as MakeIndirectDrawItem but on the mesh path: commands [first_command, first_command + command_count) of the uploaded scene,
    // mapped to their records. This is how a material run becomes one mesh draw.
    std::shared_ptr<IIr77Return const> MakeMeshletSceneDrawItem(std::size_t const& window, std::uint32_t const& meshlet_id, std::uint32_t const& material_id,
                                                                std::uint32_t const& first_command, std::uint32_t const& command_count, Ir77PVDrawItem& item) {
        Ir77PVMeshletSlot* const slot = FindMeshlets(meshlet_id);
        if (!slot) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: unknown meshlet mesh.");

        if (static_cast<std::size_t>(first_command) + command_count >= slot->first_record.size())
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: command range outside the meshlet scene.");

        std::uint32_t const first = slot->first_record[first_command];
        std::uint32_t const end = slot->first_record[first_command + command_count];

        return MakeMeshletIndirectDrawItem(window, meshlet_id, material_id, first, end - first, item);
    }

    // Hide / show every instance of one scene object on the mesh path. Applied to each frame copy by Tick.
    std::shared_ptr<IIr77Return const> SetMeshletObjectVisible(std::uint32_t const& meshlet_id, std::uint32_t const& command, bool const& visible) {
        Ir77PVMeshletSlot* const slot = FindMeshlets(meshlet_id);
        if (!slot || static_cast<std::size_t>(command) + 1 >= slot->first_record.size())
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: unknown meshlet object.");

        for (std::uint32_t r = slot->first_record[command]; r < slot->first_record[command + 1]; r++)
            slot->commands[r].groupCountX = visible ? slot->groups[r] : 0;

        slot->dirty = MAX_FRAMES_IN_FLIGHT;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Hide / show one instance (record). The record count is fixed at build time -- spawn capacity is reserved by uploading
    // extra instances and starting them hidden.
    std::shared_ptr<IIr77Return const> SetMeshletRecordVisible(std::uint32_t const& meshlet_id, std::uint32_t const& record, bool const& visible) {
        Ir77PVMeshletSlot* const slot = FindMeshlets(meshlet_id);
        if (!slot || record >= slot->commands.size()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: unknown meshlet record.");

        slot->commands[record].groupCountX = visible ? slot->groups[record] : 0;
        slot->dirty = MAX_FRAMES_IN_FLIGHT;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // First record of a scene command -- record = first_record + instance offset, for SetMeshletRecordVisible.
    std::shared_ptr<IIr77Return const> GetMeshletFirstRecord(std::uint32_t const& meshlet_id, std::uint32_t const& command, std::uint32_t& first_record) {
        Ir77PVMeshletSlot* const slot = FindMeshlets(meshlet_id);
        if (!slot || static_cast<std::size_t>(command) + 1 >= slot->first_record.size())
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: unknown meshlet object.");

        first_record = slot->first_record[command];

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Frees the set 3 buffers once no frame in flight can read them. Remove its draw items from Paint first.
    std::shared_ptr<IIr77Return const> UnloadMeshlets(std::uint32_t const& meshlet_id) {
        Ir77PVMeshletSlot* const slot = FindMeshlets(meshlet_id);
        if (!slot) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: unknown meshlet mesh.");

        Ir77PVMeshletMesh const& mesh = slot->mesh;
        Bury({mesh.meshlets, mesh.meshlet_vertices, mesh.meshlet_triangles, mesh.vertices, mesh.draws, mesh.indirect}, mesh.set);
        *slot = Ir77PVMeshletSlot{};

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // =========================================================================================================================================
    // textures -- RGBA8, one mip, sampled in the fragment stage. srgb = true for color data (albedo, emissive), false for data maps
    // (normal, ORM, AO). One shared linear / repeat sampler per device.
    // =========================================================================================================================================
    std::shared_ptr<IIr77Return const> CreateTexture(std::uint8_t const* rgba, std::uint32_t const& width, std::uint32_t const& height, bool const& srgb,
                                                     std::uint32_t& texture_id, bool const& wait = true) {
        if (rgba == nullptr || width == 0 || height == 0) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: empty texture.");

        VkCommandPool pool{VK_NULL_HANDLE};
        VkQueue queue{VK_NULL_HANDLE};
        if (!UploadQueue(&pool, &queue)) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: no upload queue.");

        std::uint64_t const device_id = m_context->m_current_device;

        VkDevice device{VK_NULL_HANDLE};
        m_context->m_devices.at(device_id)->GetDevice(&device);
        VmaAllocator const allocator = m_context->m_allocators.at(device_id);

        if (EnsureSampler() == VK_NULL_HANDLE) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: sampler creation failed.");

        VkFormat const format = srgb ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;
        VkDeviceSize const size = static_cast<VkDeviceSize>(width) * height * 4;

        // image
        VkImageCreateInfo image_info{};
        image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image_info.imageType = VK_IMAGE_TYPE_2D;
        image_info.format = format;
        image_info.extent = {width, height, 1};
        image_info.mipLevels = 1;
        image_info.arrayLayers = 1;
        image_info.samples = VK_SAMPLE_COUNT_1_BIT;
        image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
        image_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        VmaAllocationCreateInfo image_alloc{};
        image_alloc.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

        Ir77PVTextureSlot texture{};

        if (vmaCreateImage(allocator, &image_info, &image_alloc, &texture.image, &texture.allocation, nullptr) != VK_SUCCESS)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: texture image creation failed.");

        VkImageSubresourceRange const range{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

        VkImageViewCreateInfo view_info{};
        view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view_info.image = texture.image;
        view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_info.format = format;
        view_info.subresourceRange = range;

        if (vkCreateImageView(device, &view_info, nullptr, &texture.view) != VK_SUCCESS) {
            vmaDestroyImage(allocator, texture.image, texture.allocation);
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: texture view creation failed.");
        }

        // staging
        VkBuffer staging{VK_NULL_HANDLE};
        VmaAllocation staging_allocation{VK_NULL_HANDLE};
        void* mapped{nullptr};

        if (!CreateStaging(size, staging, staging_allocation, mapped)) {
            vkDestroyImageView(device, texture.view, nullptr);
            vmaDestroyImage(allocator, texture.image, texture.allocation);
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: texture staging creation failed.");
        }

        std::memcpy(mapped, rgba, static_cast<std::size_t>(size));
        vmaFlushAllocation(allocator, staging_allocation, 0, size);

        VkCommandBuffer cmd{VK_NULL_HANDLE};
        if (!BeginOneShot(pool, cmd)) {
            vmaDestroyBuffer(allocator, staging, staging_allocation);
            vkDestroyImageView(device, texture.view, nullptr);
            vmaDestroyImage(allocator, texture.image, texture.allocation);
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: texture command buffer failed.");
        }

        // UNDEFINED -> TRANSFER_DST
        VkImageMemoryBarrier to_dst{};
        to_dst.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        to_dst.srcAccessMask = 0;
        to_dst.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        to_dst.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        to_dst.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        to_dst.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        to_dst.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        to_dst.image = texture.image;
        to_dst.subresourceRange = range;

        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &to_dst);

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        region.imageExtent = {width, height, 1};

        vkCmdCopyBufferToImage(cmd, staging, texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        // TRANSFER_DST -> SHADER_READ_ONLY
        VkImageMemoryBarrier to_read = to_dst;
        to_read.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        to_read.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        to_read.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        to_read.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &to_read);

        if (SubmitOneShot(pool, queue, cmd, staging, staging_allocation, wait)->ID() != GUIDIr77OperationSucceeded) {
            vkDestroyImageView(device, texture.view, nullptr);
            vmaDestroyImage(allocator, texture.image, texture.allocation);
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: texture upload failed.");
        }

        texture.alive = true;

        auto& textures = m_textures[device_id];
        texture_id = static_cast<std::uint32_t>(textures.size());
        textures.push_back(texture);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetTextureInfo(std::uint32_t const& texture_id, VkDescriptorImageInfo& info) {
        auto const found = m_textures.find(m_context->m_current_device);
        if (found == m_textures.end() || texture_id >= found->second.size() || !found->second[texture_id].alive)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: unknown texture.");

        info = VkDescriptorImageInfo{};
        info.sampler = EnsureSampler();
        info.imageView = found->second[texture_id].view;
        info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // =========================================================================================================================================
    // materials -- set 2 from the Static layout: constants UBO + 5 texture slots (placeholders where no texture is given)
    // =========================================================================================================================================
    std::shared_ptr<IIr77Return const> CreateMaterial(Ir77PVMaterialConstants const& constants, std::uint32_t& material_id) {
        return CreateMaterial(constants, Ir77PVMaterialTextures{}, material_id);
    }

    std::shared_ptr<IIr77Return const> CreateMaterial(Ir77PVMaterialConstants const& constants, Ir77PVMaterialTextures const& textures,
                                                      std::uint32_t& material_id) {
        std::vector<std::uint32_t> ids{};
        auto const result = UploadMaterials({constants}, {textures}, ids);
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        material_id = ids.front();

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> UploadMaterials(std::vector<Ir77PVMaterialConstants> const& constants, std::vector<std::uint32_t>& material_ids) {
        return UploadMaterials(constants, std::vector<Ir77PVMaterialTextures>(constants.size()), material_ids);
    }

    // One staging copy for every material's constants; one buffer + one set per material (each binds its own textures).
    std::shared_ptr<IIr77Return const> UploadMaterials(std::vector<Ir77PVMaterialConstants> const& constants, std::vector<Ir77PVMaterialTextures> const& textures,
                                                       std::vector<std::uint32_t>& material_ids) {
        if (constants.size() != textures.size()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: constants / textures count mismatch.");

        VkCommandPool pool{VK_NULL_HANDLE};
        VkQueue queue{VK_NULL_HANDLE};
        if (!UploadQueue(&pool, &queue)) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: no upload queue.");

        std::vector<Ir77PVMaterial> built(constants.size());
        std::vector<Ir77PVUploadEntry> entries{};
        entries.reserve(constants.size());

        for (std::size_t i = 0; i < constants.size(); i++) {
            auto const result = BuildMaterial(textures[i], built[i]);
            if (result->ID() != GUIDIr77OperationSucceeded) return result;

            VkBuffer dst{VK_NULL_HANDLE};
            built[i].constants->GetBuffer(0, &dst);
            entries.push_back({dst, &constants[i], sizeof(Ir77PVMaterialConstants)});
        }

        if (UploadBatch(pool, queue, entries)->ID() != GUIDIr77OperationSucceeded)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: material upload failed.");

        auto& materials = m_materials[m_context->m_current_device];
        material_ids.clear();
        material_ids.reserve(built.size());

        for (auto& material : built) {
            material_ids.push_back(static_cast<std::uint32_t>(materials.size()));
            materials.push_back(material);
        }

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: Upload Materials.");
    }

    std::shared_ptr<IIr77Return const> GetMaterial(std::uint32_t const& material_id, Ir77PVMaterial& material) {
        auto const found = m_materials.find(m_context->m_current_device);
        if (found == m_materials.end() || material_id >= found->second.size()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: unknown material.");

        material = found->second[material_id];

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Draw item for mesh + material through a given pipeline kind on a window. Caller fills model / instances.
    // Needs CreatePipelines first; the pipeline lookup fails for kinds whose render pass wasn't created.
    std::shared_ptr<IIr77Return const> MakeDrawItem(std::size_t const& window, Ir77PVPipelineKind const& kind, std::uint32_t const& mesh_id,
                                                    std::uint32_t const& material_id, Ir77PVDrawItem& item) {
        if (kind == Ir77PVPipelineKind::Mesh)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: Mesh kind takes a meshlet mesh -- use MakeMeshletDrawItem.");

        Ir77PVMesh mesh{};
        Ir77PVMaterial material{};

        if (GetMesh(mesh_id, mesh)->ID() != GUIDIr77OperationSucceeded) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: unknown mesh.");
        if (GetMaterial(material_id, material)->ID() != GUIDIr77OperationSucceeded)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: unknown material.");

        item = Ir77PVDrawItem{};

        if (m_context->GetPipeline(window, kind, item.pipeline)->ID() != GUIDIr77OperationSucceeded)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: pipeline kind not created for window.");

        if (m_context->GetLayout(LayoutKindFor(kind), item.layout)->ID() != GUIDIr77OperationSucceeded)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: layout not created.");

        item.material = material.set;
        item.vertex = mesh.vertex;
        item.index = mesh.index;
        item.index_type = mesh.index_type;
        item.index_count = mesh.index_count;
        item.instance_count = 1;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // =========================================================================================================================================
    // TESTBED -- a lit-by-normals grid of spinning cubes. Development only; nothing in the engine depends on it. It is a client:
    // it uses exactly the calls a scene loader would.
    //
    //   asset->CreateTestbed(grid, use_mesh, use_indirect);   // after CreatePipelines
    //   loop: asset->Tick(frame); asset->UpdateTestbed(paint, seconds);   // before paint->Draw()
    //
    // Three paths, all reading each cube's model from the Instances buffer (set 0 binding 2):
    //   default       -- one direct draw per cube (UploadMesh / MakeDrawItem), firstInstance = cube slot
    //   use_mesh      -- task / mesh shaders (UploadMeshlets / MakeMeshletDrawItem), draw_base = cube slot;
    //                    falls back to the default when the device has no mesh shading
    //   use_indirect  -- one indirect call for every cube: UploadScene / MakeIndirectDrawItem on the vertex path,
    //                    UploadMeshlets / MakeMeshletSceneDrawItem on the mesh path
    // =========================================================================================================================================
    std::shared_ptr<IIr77Return const> CreateTestbed(std::uint32_t const& grid = 3, bool const& use_mesh = false, bool const& use_indirect = false) {
        auto const cube = BuildCube(0.5f);

        std::uint32_t const side = std::max<std::uint32_t>(grid, 1);
        std::uint32_t const cubes = side * side;

        // vertex path always exists -- it's the fallback
        auto result = UploadMesh(cube, m_testbed.mesh);
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        // one scene, both paths: the cube as one object, `cubes` instances.
        // vertex indirect: gl_InstanceIndex 0..cubes-1 -> models[0..cubes-1]
        // mesh: one draw record per instance, record i -> models[i]
        Ir77PVSceneUpload scene{};
        scene.vertices = cube.vertex_data;
        scene.indices = cube.index_data;
        scene.indirect_commands.push_back(VkDrawIndexedIndirectCommand{static_cast<std::uint32_t>(cube.index_data.size()), cubes, 0, 0, 0});

        // mesh path -- falls back when the device has no mesh shading
        bool mesh = false;
        std::uint32_t records{0};
        if (use_mesh) mesh = UploadMeshlets(scene, m_testbed.meshlets, records)->ID() == GUIDIr77OperationSucceeded;

        // vertex indirect -- only when mesh isn't taking over
        bool indirect = false;
        if (use_indirect && !mesh) indirect = UploadScene(scene, m_testbed.scene)->ID() == GUIDIr77OperationSucceeded;

        Ir77PVMaterialConstants constants{};
        constants.base_color = {0.8f, 0.8f, 0.8f, 1.0f};

        result = CreateMaterial(constants, m_testbed.material);
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        m_testbed.grid = side;
        m_testbed.use_mesh = mesh;
        m_testbed.use_indirect = mesh ? use_indirect : indirect;  // mesh has its indirect buffer from UploadMeshlets already
        m_testbed.ready = true;

        std::printf("[DEBUG] testbed path = %s%s\n", mesh ? "mesh shading" : "vertex", m_testbed.use_indirect ? " indirect" : "");

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: Create Testbed.");
    }

    // Writes the camera, the instance models and the cube draw list for every window
    std::shared_ptr<IIr77Return const> UpdateTestbed(std::shared_ptr<Ir77PVPaint> const& paint, float const& seconds) {
        if (!m_testbed.ready) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: CreateTestbed not called.");

        std::size_t const windows = m_context->m_windows.at(m_context->m_current_device).size();

        for (std::size_t w = 0; w < windows; w++) {
            // camera -- orbits the grid; each window offset a little so multi-window setups are distinguishable
            VkExtent2D extent{};
            m_context->m_swapchains.at(m_context->m_current_device).at(w)->GetSwapchainExtents(extent);

            float const aspect = extent.height > 0 ? static_cast<float>(extent.width) / static_cast<float>(extent.height) : 1.78f;
            float const orbit = seconds * 0.25f + static_cast<float>(w) * 0.5f;
            float const radius = 2.5f + static_cast<float>(m_testbed.grid);

            glm::vec3 const eye(std::sin(orbit) * radius, radius * 0.6f, std::cos(orbit) * radius);

            Ir77PVCamera const camera = MakeCamera(eye, glm::vec3(0.0f), 60.0f, aspect);

            paint->SetFrameData(w, Ir77PVBufferSlot::Camera, &camera, sizeof(camera));

            // cubes
            Ir77PVDrawItem base{};
            auto const made = m_testbed.use_mesh ? MakeMeshletDrawItem(w, m_testbed.meshlets, m_testbed.material, base)
                                                 : MakeDrawItem(w, Ir77PVPipelineKind::Static, m_testbed.mesh, m_testbed.material, base);
            if (made->ID() != GUIDIr77OperationSucceeded) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: testbed draw item failed.");

            std::vector<Ir77PVDrawItem> draws{};
            std::vector<glm::mat4> models{};
            float const half = static_cast<float>(m_testbed.grid - 1) * 0.5f;

            for (std::uint32_t x = 0; x < m_testbed.grid; x++) {
                for (std::uint32_t z = 0; z < m_testbed.grid; z++) {
                    float const phase = static_cast<float>(x * m_testbed.grid + z);

                    glm::mat4 model(1.0f);
                    model = glm::translate(model, glm::vec3((static_cast<float>(x) - half) * 1.5f, 0.0f, (static_cast<float>(z) - half) * 1.5f));
                    model = glm::rotate(model, seconds * (0.6f + phase * 0.1f), glm::normalize(glm::vec3(0.3f, 1.0f, 0.2f)));

                    std::uint32_t const slot = static_cast<std::uint32_t>(models.size());

                    Ir77PVDrawItem item = base;
                    std::memcpy(item.model.data(), glm::value_ptr(model), sizeof(float) * 16);  // push constant -- unused by the current shaders
                    item.first_instance = slot;                                                 // vertex path -- gl_InstanceIndex -> models[slot]
                    item.draw_base = slot;                                                      // mesh path -- record slot -> models[slot]

                    models.push_back(model);
                    draws.push_back(item);
                }
            }

            // set 0 binding 2 -- every path reads its model from here
            paint->SetFrameData(w, Ir77PVBufferSlot::Instances, models.data(), models.size() * sizeof(glm::mat4));

            // one indirect draw replaces the per-cube items -- the whole scene is one object, so one run: command 0
            if (m_testbed.use_indirect) {
                Ir77PVDrawItem item{};

                auto const made_indirect = m_testbed.use_mesh
                                               ? MakeMeshletSceneDrawItem(w, m_testbed.meshlets, m_testbed.material, 0, 1, item)
                                               : MakeIndirectDrawItem(w, Ir77PVPipelineKind::Static, m_testbed.scene, m_testbed.material, 0, 1, false, item);
                if (made_indirect->ID() != GUIDIr77OperationSucceeded)
                    return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: testbed indirect draw item failed.");

                draws = {item};
            }

            paint->SetDraws(w, Ir77PVPass::Opaque, draws);
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   private:
    // Outward-facing CCW cube by construction: each face's winding is verified against its own normal
    // rather than assumed, so the Y-flipped projection + frontFace=CLOCKWISE pairing is correct on
    // every face regardless of per-face handedness.
    static Ir77PVInputBuffer BuildCube(float const& h) {
        struct Face {
            glm::vec3 normal;
            glm::vec3 u;
            glm::vec3 v;
        };

        std::array<Face, 6> const faces{{
            {{1, 0, 0}, {0, 0, -1}, {0, 1, 0}},
            {{-1, 0, 0}, {0, 0, 1}, {0, 1, 0}},
            {{0, 1, 0}, {1, 0, 0}, {0, 0, -1}},
            {{0, -1, 0}, {1, 0, 0}, {0, 0, 1}},
            {{0, 0, 1}, {1, 0, 0}, {0, 1, 0}},
            {{0, 0, -1}, {-1, 0, 0}, {0, 1, 0}},
        }};

        std::array<glm::vec2, 4> const corners{{{-1, -1}, {1, -1}, {1, 1}, {-1, 1}}};

        Ir77PVInputBuffer out{};

        for (auto const& face : faces) {
            std::uint32_t const base = static_cast<std::uint32_t>(out.vertex_data.size());

            for (auto const& c : corners) {
                Ir77PVVertex vertex{};
                vertex.position = (face.normal + face.u * c.x + face.v * c.y) * h;
                vertex.normal = face.normal;
                vertex.uv = glm::vec2((c.x + 1.0f) * 0.5f, 1.0f - (c.y + 1.0f) * 0.5f);

                out.vertex_data.push_back(vertex);
            }

            // Verify winding against the intended outward normal rather than assuming a fixed corner order
            glm::vec3 const& p0 = out.vertex_data[base + 2].position;
            glm::vec3 const& p1 = out.vertex_data[base + 1].position;
            glm::vec3 const& p2 = out.vertex_data[base + 0].position;
            glm::vec3 const computed_normal = glm::normalize(glm::cross(p1 - p0, p2 - p0));

            bool const correct_winding = glm::dot(computed_normal, face.normal) > 0.0f;

            if (correct_winding) {
                for (std::uint32_t const i : {0u, 1u, 2u, 0u, 2u, 3u}) out.index_data.push_back(base + i);
            } else {
                for (std::uint32_t const i : {0u, 2u, 1u, 0u, 3u, 2u}) out.index_data.push_back(base + i);
            }
        }

        return out;
    }

    static Ir77PVLayoutKind LayoutKindFor(Ir77PVPipelineKind const& kind) {
        switch (kind) {
            case Ir77PVPipelineKind::Skinned:
                return Ir77PVLayoutKind::Skinned;
            case Ir77PVPipelineKind::Shadow:
                return Ir77PVLayoutKind::Shadow;
            case Ir77PVPipelineKind::ShadowSkinned:
                return Ir77PVLayoutKind::ShadowSkinned;
            case Ir77PVPipelineKind::CEF:
                return Ir77PVLayoutKind::CEF;
            case Ir77PVPipelineKind::Mesh:
                return Ir77PVLayoutKind::Mesh;
            default:
                return Ir77PVLayoutKind::Static;
        }
    }

    Ir77PVSceneSlot* FindScene(std::uint32_t const& scene_id) {
        auto const found = m_scenes.find(m_context->m_current_device);
        if (found == m_scenes.end() || scene_id >= found->second.size() || !found->second[scene_id].alive) return nullptr;

        return &found->second[scene_id];
    }

    Ir77PVMeshletSlot* FindMeshlets(std::uint32_t const& meshlet_id) {
        auto const found = m_meshlet_scenes.find(m_context->m_current_device);
        if (found == m_meshlet_scenes.end() || meshlet_id >= found->second.size() || !found->second[meshlet_id].alive) return nullptr;

        return &found->second[meshlet_id];
    }

    // Held until MAX_FRAMES_IN_FLIGHT + 1 Ticks have passed -- every frame that could have recorded them has finished by then
    void Bury(std::vector<std::shared_ptr<IIr77PVBuffer>> const& buffers, std::shared_ptr<IIr77PVDescriptorSet> const& set) {
        Ir77PVGrave grave{};
        for (auto const& buffer : buffers)
            if (buffer) grave.buffers.push_back(buffer);
        grave.set = set;
        grave.frames_left = MAX_FRAMES_IN_FLIGHT + 1;

        m_graveyard[m_context->m_current_device].push_back(std::move(grave));
    }

    std::shared_ptr<IIr77PVBuffer> CreateStaticBuffer(VkBufferUsageFlags const& usage, VkDeviceSize const& size) {
        auto buffer = std::static_pointer_cast<IIr77PVBuffer>(std::make_shared<Ir77PVBuffer>());

        buffer->SetDevice(m_context->m_devices.at(m_context->m_current_device));

        buffer->SetAllocator(m_context->m_allocators.at(m_context->m_current_device));

        buffer->SetUsage(usage);

        buffer->SetMode(Ir77PVBufferMode::Static);

        if (buffer->CreateResources(size, 1)->ID() != GUIDIr77OperationSucceeded) return nullptr;

        return buffer;
    }

    // Host-written every frame -- one copy per frame in flight, persistently mapped. Write with Update(frame, ...).
    std::shared_ptr<IIr77PVBuffer> CreateDynamicBuffer(VkBufferUsageFlags const& usage, VkDeviceSize const& size) {
        auto buffer = std::static_pointer_cast<IIr77PVBuffer>(std::make_shared<Ir77PVBuffer>());

        buffer->SetDevice(m_context->m_devices.at(m_context->m_current_device));

        buffer->SetAllocator(m_context->m_allocators.at(m_context->m_current_device));

        buffer->SetUsage(usage);

        buffer->SetMode(Ir77PVBufferMode::Dynamic);

        if (buffer->CreateResources(size, MAX_FRAMES_IN_FLIGHT)->ID() != GUIDIr77OperationSucceeded) return nullptr;

        return buffer;
    }

    // Constants buffer + set 2 for one material. The constants are uploaded by the caller (UploadMaterials batches them).
    std::shared_ptr<IIr77Return const> BuildMaterial(Ir77PVMaterialTextures const& textures, Ir77PVMaterial& material) {
        std::shared_ptr<IIr77PVLayout> layout{};
        if (m_context->GetLayout(Ir77PVLayoutKind::Static, layout)->ID() != GUIDIr77OperationSucceeded)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: Static layout not created.");

        VkDescriptorImageInfo const placeholder = m_context->GetPlaceholderImageInfo();
        if (placeholder.imageView == VK_NULL_HANDLE) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: CreatePlaceholderTexture not called.");

        VkDescriptorImageInfo const flat_normal = m_context->GetPlaceholderImageInfo(Ir77PVPlaceholderKind::Normal);

        // resolve each slot -- texture when given, placeholder otherwise
        std::array<std::uint32_t, 5> const ids{textures.albedo, textures.normal, textures.orm, textures.emissive, textures.ao};
        std::array<VkDescriptorImageInfo, 5> images{placeholder, flat_normal, placeholder, placeholder, placeholder};

        for (std::size_t i = 0; i < ids.size(); i++) {
            if (ids[i] == IR77PV_NO_TEXTURE) continue;
            if (GetTextureInfo(ids[i], images[i])->ID() != GUIDIr77OperationSucceeded)
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: material references an unknown texture.");
        }

        material.constants = CreateStaticBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(Ir77PVMaterialConstants));
        if (!material.constants) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: material buffer creation failed.");

        material.set = std::static_pointer_cast<IIr77PVDescriptorSet>(std::make_shared<Ir77PVDescriptorSet>());

        material.set->SetDevice(m_context->m_devices.at(m_context->m_current_device));
        material.set->SetLayout(layout, 2);

        if (material.set->CreateSets(1)->ID() != GUIDIr77OperationSucceeded)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: material set allocation failed -- raise MATERIAL_CAPACITY.");

        material.set->BindBuffer(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, material.constants);

        material.set->BindImage(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, images[0]);  // albedo
        material.set->BindImage(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, images[1]);  // normal
        material.set->BindImage(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, images[2]);  // ORM
        material.set->BindImage(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, images[3]);  // emissive
        material.set->BindImage(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, images[4]);  // AO

        if (material.set->Write()->ID() != GUIDIr77OperationSucceeded) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: material set write failed.");

        m_context->m_buffers_vertex[m_context->m_current_device].push_back(material.constants);  // teardown order

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // One linear / repeat sampler per device, created on first use
    VkSampler EnsureSampler() {
        std::uint64_t const device_id = m_context->m_current_device;

        auto const found = m_samplers.find(device_id);
        if (found != m_samplers.end()) return found->second;

        VkDevice device{VK_NULL_HANDLE};
        m_context->m_devices.at(device_id)->GetDevice(&device);

        VkSamplerCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        info.magFilter = VK_FILTER_LINEAR;
        info.minFilter = VK_FILTER_LINEAR;
        info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        info.anisotropyEnable = VK_FALSE;
        info.maxLod = VK_LOD_CLAMP_NONE;
        info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

        VkSampler sampler{VK_NULL_HANDLE};
        if (vkCreateSampler(device, &info, nullptr, &sampler) != VK_SUCCESS) return VK_NULL_HANDLE;

        m_samplers[device_id] = sampler;

        return sampler;
    }

    // Asset's own transient pool on the presentation family, created on first upload per device
    bool UploadQueue(VkCommandPool* pool, VkQueue* queue) {
        std::uint64_t const device_id = m_context->m_current_device;

        std::vector<Ir77PVQueueFamily> families{};
        m_context->m_devices.at(device_id)->GetQueueFamilies(families);

        std::uint32_t family{UINT32_MAX};
        for (std::size_t i = 0; i < families.size(); i++) {
            if (families[i].presentation == VK_TRUE) {
                family = static_cast<std::uint32_t>(i);
                *queue = families[i].queue;
                break;
            }
        }

        if (family == UINT32_MAX) return false;

        auto const found = m_upload_pools.find(device_id);
        if (found != m_upload_pools.end()) {
            *pool = found->second;
            return true;
        }

        VkDevice device;
        m_context->m_devices.at(device_id)->GetDevice(&device);

        VkCommandPoolCreateInfo pool_info{};
        pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        pool_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        pool_info.queueFamilyIndex = family;

        if (vkCreateCommandPool(device, &pool_info, nullptr, pool) != VK_SUCCESS) return false;

        m_upload_pools[device_id] = *pool;

        return true;
    }

    // Host-visible, mapped, written once front to back
    bool CreateStaging(VkDeviceSize const& size, VkBuffer& staging, VmaAllocation& allocation, void*& mapped) {
        VmaAllocator const allocator = m_context->m_allocators.at(m_context->m_current_device);

        VkBufferCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        info.size = size;
        info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo alloc_info{};
        alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
        alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VmaAllocationInfo result{};
        if (vmaCreateBuffer(allocator, &info, &alloc_info, &staging, &allocation, &result) != VK_SUCCESS) return false;

        mapped = result.pMappedData;

        return true;
    }

    bool BeginOneShot(VkCommandPool const& pool, VkCommandBuffer& cmd) {
        VkDevice device{VK_NULL_HANDLE};
        m_context->m_devices.at(m_context->m_current_device)->GetDevice(&device);

        VkCommandBufferAllocateInfo alloc{};
        alloc.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        alloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        alloc.commandPool = pool;
        alloc.commandBufferCount = 1;

        if (vkAllocateCommandBuffers(device, &alloc, &cmd) != VK_SUCCESS) return false;

        VkCommandBufferBeginInfo begin{};
        begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        if (vkBeginCommandBuffer(cmd, &begin) != VK_SUCCESS) {
            vkFreeCommandBuffers(device, pool, 1, &cmd);
            cmd = VK_NULL_HANDLE;
            return false;
        }

        return true;
    }

    // Ends, submits with its own fence, then either waits and frees (wait) or hands everything to Tick (no wait).
    // Takes ownership of cmd and the staging buffer on every path.
    std::shared_ptr<IIr77Return const> SubmitOneShot(VkCommandPool const& pool, VkQueue const& queue, VkCommandBuffer const& cmd, VkBuffer const& staging,
                                                     VmaAllocation const& staging_allocation, bool const& wait) {
        std::uint64_t const device_id = m_context->m_current_device;

        VkDevice device{VK_NULL_HANDLE};
        m_context->m_devices.at(device_id)->GetDevice(&device);
        VmaAllocator const allocator = m_context->m_allocators.at(device_id);

        Ir77PVPending pending{};
        pending.cmd = cmd;
        pending.pool = pool;
        pending.staging = staging;
        pending.staging_allocation = staging_allocation;

        VkResult result = vkEndCommandBuffer(cmd);

        if (result == VK_SUCCESS) {
            VkFenceCreateInfo fence_info{};
            fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

            result = vkCreateFence(device, &fence_info, nullptr, &pending.fence);
        }

        if (result == VK_SUCCESS) {
            VkSubmitInfo submit_info{};
            submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submit_info.commandBufferCount = 1;
            submit_info.pCommandBuffers = &cmd;

            result = vkQueueSubmit(queue, 1, &submit_info, pending.fence);
        }

        if (result != VK_SUCCESS) {
            ReleasePending(device, allocator, pending);
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: upload submit failed.");
        }

        if (!wait) {
            m_pending[device_id].push_back(pending);
            return Ir77RETURN<Ir77OperationSucceeded>();
        }

        result = vkWaitForFences(device, 1, &pending.fence, VK_TRUE, UINT64_MAX);
        ReleasePending(device, allocator, pending);

        if (result != VK_SUCCESS) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: upload wait failed.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    static void ReleasePending(VkDevice const& device, VmaAllocator const& allocator, Ir77PVPending const& pending) {
        if (pending.fence != VK_NULL_HANDLE) vkDestroyFence(device, pending.fence, nullptr);
        if (pending.cmd != VK_NULL_HANDLE) vkFreeCommandBuffers(device, pending.pool, 1, &pending.cmd);
        if (pending.staging != VK_NULL_HANDLE && allocator != VK_NULL_HANDLE) vmaDestroyBuffer(allocator, pending.staging, pending.staging_allocation);
    }

    // One staging buffer, one command buffer, one submit for every entry. Each entry lands at its own dst_offset, so new data can go
    // into a free range of an existing buffer. The barrier makes the copies visible to everything submitted after it on this queue,
    // so wait = false is safe for data drawn by later frames.
    std::shared_ptr<IIr77Return const> UploadBatch(VkCommandPool const& pool, VkQueue const& queue, std::vector<Ir77PVUploadEntry> const& entries,
                                                   bool const& wait = true) {
        VkDeviceSize total{0};
        for (auto const& e : entries) {
            if (e.size == 0) continue;
            if (e.dst == VK_NULL_HANDLE || e.data == nullptr) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: upload entry has no destination or data.");
            total += e.size;
        }
        if (total == 0) return Ir77RETURN<Ir77OperationSucceeded>();

        VmaAllocator const allocator = m_context->m_allocators.at(m_context->m_current_device);

        VkBuffer staging{VK_NULL_HANDLE};
        VmaAllocation staging_allocation{VK_NULL_HANDLE};
        void* mapped{nullptr};

        if (!CreateStaging(total, staging, staging_allocation, mapped))
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: batch staging buffer creation failed.");

        // pack every entry back to back; one copy region per entry
        std::vector<VkBufferCopy> regions(entries.size());
        VkDeviceSize cursor{0};

        for (std::size_t i = 0; i < entries.size(); i++) {
            if (entries[i].size == 0) continue;

            std::memcpy(static_cast<std::uint8_t*>(mapped) + cursor, entries[i].data, static_cast<std::size_t>(entries[i].size));

            regions[i].srcOffset = cursor;
            regions[i].dstOffset = entries[i].dst_offset;
            regions[i].size = entries[i].size;

            cursor += entries[i].size;
        }

        vmaFlushAllocation(allocator, staging_allocation, 0, total);  // no-op on coherent memory

        VkCommandBuffer cmd{VK_NULL_HANDLE};
        if (!BeginOneShot(pool, cmd)) {
            vmaDestroyBuffer(allocator, staging, staging_allocation);
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: batch upload command buffer failed.");
        }

        for (std::size_t i = 0; i < entries.size(); i++) {
            if (entries[i].size == 0) continue;
            vkCmdCopyBuffer(cmd, staging, entries[i].dst, 1, &regions[i]);
        }

        // make the copied data visible to whatever reads it next -- vertex fetch, index fetch, indirect, shaders, later copies
        VkMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 1, &barrier, 0, nullptr, 0, nullptr);

        return SubmitOneShot(pool, queue, cmd, staging, staging_allocation, wait);
    }

   private:
    std::shared_ptr<Ir77PeregrineV> m_context{nullptr};

    std::string m_shader_root{"/home/alpha/workspace/iridium/Shader"};

    // [device]
    std::map<std::uint64_t, std::vector<Ir77PVPipelineKind>> m_pipeline_kinds{};

    std::map<std::uint64_t, std::vector<Ir77PVMesh>> m_meshes{};  // single-mesh path

    std::map<std::uint64_t, std::vector<Ir77PVSceneSlot>> m_scenes{};  // scene path (vertex)

    std::map<std::uint64_t, std::vector<Ir77PVMeshletSlot>> m_meshlet_scenes{};  // meshlet path

    std::map<std::uint64_t, std::vector<Ir77PVMaterial>> m_materials{};

    std::map<std::uint64_t, std::vector<Ir77PVTextureSlot>> m_textures{};

    std::map<std::uint64_t, VkSampler> m_samplers{};

    std::map<std::uint64_t, VkCommandPool> m_upload_pools{};

    std::map<std::uint64_t, std::vector<Ir77PVPending>> m_pending{};  // uploads submitted with wait = false

    std::map<std::uint64_t, std::vector<Ir77PVGrave>> m_graveyard{};  // unloaded, waiting out the frames in flight

    Ir77PVTestbed m_testbed{};
};
}  // namespace NSIr77PeregrineV