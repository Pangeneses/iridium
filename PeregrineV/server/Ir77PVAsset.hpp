#pragma once

#include <SDL3/SDL_video.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
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

class Ir77PVAsset : public Ir77Enlisted, public std::enable_shared_from_this<Ir77PVAsset> {
   public:
    Ir77PVAsset() {
        try {
            m_enlisted_uuid.Generate();
        } catch (std::invalid_argument a) {
            throw a;
        }

        m_enlisted = std::chrono::system_clock::now();
    }

    // m_context outlives this object (held by shared_ptr), so the device is still alive here
    ~Ir77PVAsset() {
        if (!m_context) return;

        for (auto& [device_id, pool] : m_upload_pools) {
            auto const device = m_context->m_devices.find(device_id);
            if (device == m_context->m_devices.end()) continue;

            VkDevice vk_device{VK_NULL_HANDLE};
            device->second->GetDevice(&vk_device);

            vkDeviceWaitIdle(vk_device);
            vkDestroyCommandPool(vk_device, pool, nullptr);
        }
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
    static Ir77PVTestCamera MakeCamera(glm::vec3 const& eye, glm::vec3 const& target, float const& fov_degrees, float const& aspect) {
        Ir77PVTestCamera camera{};

        camera.eye = glm::vec4(eye, 1.0f);
        camera.view = glm::lookAtRH(eye, target, glm::vec3(0.0f, 1.0f, 0.0f));
        camera.proj = glm::perspectiveRH_ZO(glm::radians(fov_degrees), aspect, 0.1f, 100.0f);
        camera.proj[1][1] *= -1.0f;
        camera.view_proj = camera.proj * camera.view;

        return camera;
    }

    // =========================================================================================================================================
    // shaders
    // =========================================================================================================================================

    // Loads every shader file that exists under the root. Required ones fail the call; optional ones (skinned,
    // shadow) are skipped, and the pipeline kinds they unlock are reported by GetPipelineKinds.
    std::shared_ptr<IIr77Return const> CreateShaders() {
        std::vector<Ir77PVShaderFile> const files{
            {"vert.spv", ID_SHADER_VERT, Ir77PVShaderStage::Vertex, true},
            {"frag.spv", ID_SHADER_FRAG, Ir77PVShaderStage::Fragment, true},
            {"cef.vert.spv", ID_SHADER_CEF_VERT, Ir77PVShaderStage::Vertex, true},
            {"cef.frag.spv", ID_SHADER_CEF_FRAG, Ir77PVShaderStage::Fragment, true},
            {"skinned.vert.spv", ID_SHADER_SKINNED_VERT, Ir77PVShaderStage::Vertex, false},
            {"shadow.vert.spv", ID_SHADER_SHADOW_VERT, Ir77PVShaderStage::Vertex, false},
            {"shadow_skinned.vert.spv", ID_SHADER_SHADOW_SKINNED_VERT, Ir77PVShaderStage::Vertex, false},
        };

        auto shader = std::static_pointer_cast<IIr77PVShader>(std::make_shared<Ir77PVShader>());

        shader->SetDevice(m_context->m_devices.at(m_context->m_current_device));

        std::map<std::uint64_t, bool> loaded{};

        for (auto const& entry : files) {
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

        m_pipeline_kinds[m_context->m_current_device] = kinds;

        m_context->m_shaders[m_context->m_current_device] = shader;

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: Create Shaders.");
    }

    // Feed straight into Ir77PeregrineV::CreatePipelines. Shadow kinds are still skipped there until a Shadow render pass exists.
    std::shared_ptr<IIr77Return const> GetPipelineKinds(std::vector<Ir77PVPipelineKind>& kinds) {
        auto const found = m_pipeline_kinds.find(m_context->m_current_device);
        if (found == m_pipeline_kinds.end()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: CreateShaders not called.");

        kinds = found->second;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // =========================================================================================================================================
    // single-mesh path -- Static (device-local) buffers, uploaded once through staging.
    // Used by the testbed and any one-off load. A full scene load should use UploadScene below instead,
    // which stacks every primitive into one shared buffer per channel rather than one pair per mesh.
    // =========================================================================================================================================
    std::shared_ptr<IIr77Return const> UploadMesh(Ir77PVInputBuffer const& input, std::uint32_t& mesh_id) {
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

        if (UploadBatch(pool, queue, {{vertex_dst, input.vertex_data.data(), vertex_size}, {index_dst, input.index_data.data(), index_size}})->ID() !=
            GUIDIr77OperationSucceeded) {
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

    // Kept for the old call site: uploads each input as its own mesh
    std::shared_ptr<IIr77Return const> UploadVertexBuffer(std::vector<Ir77PVInputBuffer> const& buffers) {
        for (auto const& input : buffers) {
            std::uint32_t mesh_id{0};

            auto const result = UploadMesh(input, mesh_id);
            if (result->ID() != GUIDIr77OperationSucceeded) return result;
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetMesh(std::uint32_t const& mesh_id, Ir77PVMesh& mesh) {
        auto const found = m_meshes.find(m_context->m_current_device);
        if (found == m_meshes.end() || mesh_id >= found->second.size()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: unknown mesh.");

        mesh = found->second[mesh_id];

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // =========================================================================================================================================
    // SCENE UPLOAD -- entire scene's vertex-domain data, stacked and transferred as one call per channel.
    // Caller (Scene/glTF loader) appends every primitive's vertices/indices into these arrays itself and
    // prebuilds indirect_commands using the offsets it already knows from that stacking -- Asset performs
    // no per-mesh range computation and hands back no CPU-side mesh IDs. GPU-side indexing at draw time
    // comes entirely from the indirect buffer via vkCmdDrawIndexedIndirect (see MakeIndirectDrawItem).
    //
    // Each non-empty array becomes exactly one CreateStaticBuffer + one Upload call, regardless of how
    // many primitives were stacked into it -- one transfer per channel, not one per mesh.
    // =========================================================================================================================================
    std::shared_ptr<IIr77Return const> UploadScene(Ir77PVSceneUpload const& data) {
        VkCommandPool pool{VK_NULL_HANDLE};
        VkQueue queue{VK_NULL_HANDLE};
        if (!UploadQueue(&pool, &queue)) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: no upload queue.");

        std::uint64_t const device_id = m_context->m_current_device;
        std::vector<Ir77PVUploadEntry> entries{};

        if (!data.vertices.empty()) {
            VkDeviceSize const size = static_cast<VkDeviceSize>(data.vertices.size() * sizeof(Ir77PVVertex));
            m_scene_vertex[device_id] = CreateStaticBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, size);
            if (!m_scene_vertex[device_id]) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: scene vertex buffer creation failed.");

            VkBuffer dst{VK_NULL_HANDLE};
            m_scene_vertex[device_id]->GetBuffer(0, &dst);
            entries.push_back({dst, data.vertices.data(), size});
        }

        if (!data.indices.empty()) {
            VkDeviceSize const size = static_cast<VkDeviceSize>(data.indices.size() * sizeof(std::uint32_t));
            m_scene_index[device_id] = CreateStaticBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, size);
            if (!m_scene_index[device_id]) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: scene index buffer creation failed.");

            VkBuffer dst{VK_NULL_HANDLE};
            m_scene_index[device_id]->GetBuffer(0, &dst);
            entries.push_back({dst, data.indices.data(), size});
        }

        if (!data.vertices_skinned.empty()) {
            VkDeviceSize const size = static_cast<VkDeviceSize>(data.vertices_skinned.size() * sizeof(Ir77PVVertexSkinned));
            m_scene_vertex_skinned[device_id] = CreateStaticBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, size);
            if (!m_scene_vertex_skinned[device_id]) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: scene skinned vertex buffer creation failed.");

            VkBuffer dst{VK_NULL_HANDLE};
            m_scene_vertex_skinned[device_id]->GetBuffer(0, &dst);
            entries.push_back({dst, data.vertices_skinned.data(), size});
        }

        if (!data.indices_skinned.empty()) {
            VkDeviceSize const size = static_cast<VkDeviceSize>(data.indices_skinned.size() * sizeof(std::uint32_t));
            m_scene_index_skinned[device_id] = CreateStaticBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, size);
            if (!m_scene_index_skinned[device_id]) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: scene skinned index buffer creation failed.");

            VkBuffer dst{VK_NULL_HANDLE};
            m_scene_index_skinned[device_id]->GetBuffer(0, &dst);
            entries.push_back({dst, data.indices_skinned.data(), size});
        }

        if (!data.indirect_commands.empty()) {
            VkDeviceSize const size = static_cast<VkDeviceSize>(data.indirect_commands.size() * sizeof(VkDrawIndexedIndirectCommand));
            m_scene_indirect[device_id] = CreateStaticBuffer(VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, size);
            if (!m_scene_indirect[device_id]) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: scene indirect buffer creation failed.");

            VkBuffer dst{VK_NULL_HANDLE};
            m_scene_indirect[device_id]->GetBuffer(0, &dst);
            entries.push_back({dst, data.indirect_commands.data(), size});
        }

        return UploadBatch(pool, queue, entries);
    }

    // Draw item against the whole-scene buffers, indexed purely by indirect_offset/indirect_count --
    // no mesh ID involved. One call per contiguous run of indirect commands that share a pipeline kind
    // and material (Scene decides the runs when it sorts/builds indirect_commands).
    std::shared_ptr<IIr77Return const> MakeIndirectDrawItem(std::size_t const& window, Ir77PVPipelineKind const& kind, std::uint32_t const& material_id,
                                                            VkDeviceSize const& indirect_offset, std::uint32_t const& indirect_count, bool const& skinned,
                                                            Ir77PVDrawItem& item) {
        std::uint64_t const device_id = m_context->m_current_device;

        auto const vertex_buffer = skinned ? m_scene_vertex_skinned.find(device_id) : m_scene_vertex.find(device_id);
        auto const index_buffer = skinned ? m_scene_index_skinned.find(device_id) : m_scene_index.find(device_id);
        auto const indirect_buffer = m_scene_indirect.find(device_id);

        bool const has_vertex = skinned ? vertex_buffer != m_scene_vertex_skinned.end() : vertex_buffer != m_scene_vertex.end();
        bool const has_index = skinned ? index_buffer != m_scene_index_skinned.end() : index_buffer != m_scene_index.end();

        if (!has_vertex || !has_index || indirect_buffer == m_scene_indirect.end())
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: UploadScene not called, or channel empty.");

        Ir77PVMaterial material{};
        if (GetMaterial(material_id, material)->ID() != GUIDIr77OperationSucceeded)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: unknown material.");

        item = Ir77PVDrawItem{};

        if (m_context->GetPipeline(window, kind, item.pipeline)->ID() != GUIDIr77OperationSucceeded)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: pipeline kind not created for window.");

        if (m_context->GetLayout(LayoutKindFor(kind), item.layout)->ID() != GUIDIr77OperationSucceeded)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: layout not created.");

        item.material = material.set;
        item.vertex = skinned ? m_scene_vertex_skinned.at(device_id) : m_scene_vertex.at(device_id);
        item.index = skinned ? m_scene_index_skinned.at(device_id) : m_scene_index.at(device_id);
        item.index_type = VK_INDEX_TYPE_UINT32;
        item.indirect = m_scene_indirect.at(device_id);
        item.indirect_offset = indirect_offset;
        item.indirect_count = indirect_count;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // =========================================================================================================================================
    // materials -- set 2 from the Static layout: constants UBO + 5 texture slots (placeholders for now)
    // =========================================================================================================================================
    std::shared_ptr<IIr77Return const> CreateMaterial(Ir77PVMaterialConstants const& constants, std::uint32_t& material_id) {
        VkCommandPool pool{VK_NULL_HANDLE};
        VkQueue queue{VK_NULL_HANDLE};
        if (!UploadQueue(&pool, &queue)) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: no upload queue.");

        Ir77PVMaterial material{};
        auto const result = BuildMaterial(pool, queue, constants, material);
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        auto& materials = m_materials[m_context->m_current_device];
        material_id = static_cast<std::uint32_t>(materials.size());
        materials.push_back(material);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Batched version of CreateMaterial -- one outward call for the whole scene's material list.
    // Internally still one buffer + one descriptor set per material (each material needs its own
    // descriptor set to bind its own textures independently), but reuses one command pool/queue
    // lookup and appends to m_materials in one pass, instead of the caller looping CreateMaterial itself.
    std::shared_ptr<IIr77Return const> UploadMaterials(std::vector<Ir77PVMaterialConstants> const& materials_in, std::vector<std::uint32_t>& material_ids) {
        VkCommandPool pool{VK_NULL_HANDLE};
        VkQueue queue{VK_NULL_HANDLE};
        if (!UploadQueue(&pool, &queue)) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: no upload queue.");

        auto& materials = m_materials[m_context->m_current_device];
        material_ids.clear();
        material_ids.reserve(materials_in.size());

        for (auto const& constants : materials_in) {
            Ir77PVMaterial material{};
            auto const result = BuildMaterial(pool, queue, constants, material);
            if (result->ID() != GUIDIr77OperationSucceeded) return result;

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
    // TESTBED -- a lit-by-normals grid of spinning cubes. Development only; nothing in the engine depends on it.
    //
    //   asset->CreateTestbed();                       // after CreatePipelines + CreateCommandBuffers
    //   loop: asset->UpdateTestbed(paint, seconds);   // before paint->Draw(); replaces the Opaque draw list and the camera
    //
    // Unchanged -- still goes through the single-mesh path (UploadMesh/CreateMaterial/MakeDrawItem) above,
    // not the scene upload. Winding is verified per face below against each face's intended outward
    // normal, since a single fixed corner order does not produce consistent CCW winding across all six
    // faces once the camera's Y-flip is combined with frontFace = CLOCKWISE.
    // =========================================================================================================================================
    std::shared_ptr<IIr77Return const> CreateTestbed(std::uint32_t const& grid = 3) {
        auto const cube = BuildCube(0.5f);

        auto result = UploadMesh(cube, m_testbed.mesh);
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        Ir77PVMaterialConstants constants{};
        constants.base_color = {0.8f, 0.8f, 0.8f, 1.0f};

        result = CreateMaterial(constants, m_testbed.material);
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        m_testbed.grid = std::max<std::uint32_t>(grid, 1);
        m_testbed.ready = true;

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: Create Testbed.");
    }

    // Writes the camera and the cube draw list for every window
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

            Ir77PVTestCamera const camera = MakeCamera(eye, glm::vec3(0.0f), 60.0f, aspect);

            paint->SetFrameData(w, Ir77PVBufferSlot::Camera, &camera, sizeof(camera));

            // cubes
            Ir77PVDrawItem base{};
            if (MakeDrawItem(w, Ir77PVPipelineKind::Static, m_testbed.mesh, m_testbed.material, base)->ID() != GUIDIr77OperationSucceeded)
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: testbed draw item failed.");

            std::vector<Ir77PVDrawItem> draws{};
            float const half = static_cast<float>(m_testbed.grid - 1) * 0.5f;

            for (std::uint32_t x = 0; x < m_testbed.grid; x++) {
                for (std::uint32_t z = 0; z < m_testbed.grid; z++) {
                    float const phase = static_cast<float>(x * m_testbed.grid + z);

                    glm::mat4 model(1.0f);
                    model = glm::translate(model, glm::vec3((static_cast<float>(x) - half) * 1.5f, 0.0f, (static_cast<float>(z) - half) * 1.5f));
                    model = glm::rotate(model, seconds * (0.6f + phase * 0.1f), glm::normalize(glm::vec3(0.3f, 1.0f, 0.2f)));

                    Ir77PVDrawItem item = base;
                    std::memcpy(item.model.data(), glm::value_ptr(model), sizeof(float) * 16);

                    draws.push_back(item);
                }
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
            default:
                return Ir77PVLayoutKind::Static;
        }
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

    // Shared by CreateMaterial and UploadMaterials so the per-material buffer+set logic exists once
    std::shared_ptr<IIr77Return const> BuildMaterial(VkCommandPool const& pool, VkQueue const& queue, Ir77PVMaterialConstants const& constants,
                                                     Ir77PVMaterial& material) {
        std::shared_ptr<IIr77PVLayout> layout{};
        if (m_context->GetLayout(Ir77PVLayoutKind::Static, layout)->ID() != GUIDIr77OperationSucceeded)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: Static layout not created.");

        VkDescriptorImageInfo const placeholder = m_context->GetPlaceholderImageInfo();
        if (placeholder.imageView == VK_NULL_HANDLE) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: CreatePlaceholderTexture not called.");

        material.constants = CreateStaticBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(Ir77PVMaterialConstants));
        if (!material.constants) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: material buffer creation failed.");

        VkBuffer constants_dst{VK_NULL_HANDLE};
        material.constants->GetBuffer(0, &constants_dst);

        if (UploadBatch(pool, queue, {{constants_dst, &constants, sizeof(Ir77PVMaterialConstants)}})->ID() != GUIDIr77OperationSucceeded)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: material upload failed.");

        material.set = std::static_pointer_cast<IIr77PVDescriptorSet>(std::make_shared<Ir77PVDescriptorSet>());

        material.set->SetDevice(m_context->m_devices.at(m_context->m_current_device));
        material.set->SetLayout(layout, 2);

        if (material.set->CreateSets(1)->ID() != GUIDIr77OperationSucceeded)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: material set allocation failed -- raise MATERIAL_CAPACITY.");

        material.set->BindBuffer(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, material.constants);

        VkDescriptorImageInfo const flat_normal = m_context->GetPlaceholderImageInfo(Ir77PVPlaceholderKind::Normal);

        material.set->BindImage(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, placeholder);  // albedo
        material.set->BindImage(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, flat_normal);  // normal
        material.set->BindImage(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, placeholder);  // ORM
        material.set->BindImage(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, placeholder);  // emissive
        material.set->BindImage(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, placeholder);  // AO

        if (material.set->Write()->ID() != GUIDIr77OperationSucceeded) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: material set write failed.");

        return Ir77RETURN<Ir77OperationSucceeded>();
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

    // One staging buffer, one command buffer, one submit, covering every entry at once --
    // UploadScene's actual "one transfer" instead of one Ir77PVBuffer::Upload call per channel.
    std::shared_ptr<IIr77Return const> UploadBatch(VkCommandPool const& pool, VkQueue const& queue, std::vector<Ir77PVUploadEntry> const& entries) {
        VkDeviceSize total{0};
        for (auto const& e : entries) total += e.size;
        if (total == 0) return Ir77RETURN<Ir77OperationSucceeded>();

        VkDevice device;
        m_context->m_devices.at(m_context->m_current_device)->GetDevice(&device);

        VmaAllocator const allocator = m_context->m_allocators.at(m_context->m_current_device);

        VkBufferCreateInfo staging_info{};
        staging_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        staging_info.size = total;
        staging_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        staging_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo staging_alloc_info{};
        staging_alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
        staging_alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VkBuffer staging{VK_NULL_HANDLE};
        VmaAllocation staging_allocation{VK_NULL_HANDLE};
        VmaAllocationInfo staging_result{};

        if (vmaCreateBuffer(allocator, &staging_info, &staging_alloc_info, &staging, &staging_allocation, &staging_result) != VK_SUCCESS)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: batch staging buffer creation failed.");

        std::vector<VkDeviceSize> offsets(entries.size());
        VkDeviceSize cursor{0};
        for (std::size_t i = 0; i < entries.size(); i++) {
            offsets[i] = cursor;
            std::memcpy(static_cast<std::uint8_t*>(staging_result.pMappedData) + cursor, entries[i].data, static_cast<std::size_t>(entries[i].size));
            cursor += entries[i].size;
        }
        vmaFlushAllocation(allocator, staging_allocation, 0, total);

        VkCommandBufferAllocateInfo cmd_alloc{};
        cmd_alloc.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmd_alloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmd_alloc.commandPool = pool;
        cmd_alloc.commandBufferCount = 1;

        VkCommandBuffer cmd{VK_NULL_HANDLE};
        if (vkAllocateCommandBuffers(device, &cmd_alloc, &cmd) != VK_SUCCESS) {
            vmaDestroyBuffer(allocator, staging, staging_allocation);
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: batch upload command buffer allocation failed.");
        }

        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(cmd, &begin_info);

        for (std::size_t i = 0; i < entries.size(); i++) {
            VkBufferCopy region{};
            region.srcOffset = offsets[i];
            region.dstOffset = 0;
            region.size = entries[i].size;
            vkCmdCopyBuffer(cmd, staging, entries[i].dst, 1, &region);
        }

        vkEndCommandBuffer(cmd);

        VkSubmitInfo submit_info{};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &cmd;

        VkResult const result = vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE);
        if (result == VK_SUCCESS) vkQueueWaitIdle(queue);

        vkFreeCommandBuffers(device, pool, 1, &cmd);
        vmaDestroyBuffer(allocator, staging, staging_allocation);

        if (result != VK_SUCCESS) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAsset: batch upload submit failed.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   private:
    struct Ir77PVTestbed {
        std::uint32_t mesh{0};

        std::uint32_t material{0};

        std::uint32_t grid{3};

        bool ready{false};
    };

    std::shared_ptr<Ir77PeregrineV> m_context{nullptr};

    std::string m_shader_root{"/home/alpha/workspace/iridium/Shader"};

    // [device]
    std::map<std::uint64_t, std::vector<Ir77PVPipelineKind>> m_pipeline_kinds{};

    // single-mesh path
    std::map<std::uint64_t, std::vector<Ir77PVMesh>> m_meshes{};

    std::map<std::uint64_t, std::vector<Ir77PVMaterial>> m_materials{};

    std::map<std::uint64_t, VkCommandPool> m_upload_pools{};

    // whole-scene path -- one buffer per channel, shared across every mesh in the scene
    std::map<std::uint64_t, std::shared_ptr<IIr77PVBuffer>> m_scene_vertex{};

    std::map<std::uint64_t, std::shared_ptr<IIr77PVBuffer>> m_scene_index{};

    std::map<std::uint64_t, std::shared_ptr<IIr77PVBuffer>> m_scene_vertex_skinned{};

    std::map<std::uint64_t, std::shared_ptr<IIr77PVBuffer>> m_scene_index_skinned{};

    std::map<std::uint64_t, std::shared_ptr<IIr77PVBuffer>> m_scene_indirect{};

    Ir77PVTestbed m_testbed{};
};
}  // namespace NSIr77PeregrineV