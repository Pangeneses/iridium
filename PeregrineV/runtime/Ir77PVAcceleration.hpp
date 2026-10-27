#pragma once

#include <vulkan/vulkan.h>

#include <glm/glm.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "../types/Ir77PVTypes.hpp"

#include "../../Ir77RT/dictionary/IDIIr77MPVM.hpp"

#include "../dictionary/IDIIr77PeregrineV.hpp"
#include "../dictionary/IDIr77PeregrineV.hpp"

#include "../../Ir77RT/types/Ir77Types.hpp"

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"
#include "../../Ir77RT/runtime/Ir77Return.hpp"

#include "../interface/IIr77PVDevice.hpp"
#include "../interface/IIr77PVDescriptorSet.hpp"
#include "../interface/IIr77PVBuffer.hpp"
#include "../interface/IIr77PVTransfer.hpp"
#include "../interface/IIr77PVGeometry.hpp"
#include "../interface/IIr77PVAcceleration.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

// =============================================================================================================================================
// Ir77PVAcceleration -- the acceleration structures ray queries trace against.
//
//   startup:   Ir77PVInit wires it (ray-query devices only) -- SetDevice / SetTransfer / SetGeometry / SetDescriptorSet -> CreateAcceleration
//   load:      BuildScene(scene_id, dynamic, alpha) per vertex-path scene, BuildMeshlets(meshlet_id, alpha) for the mesh path's static stream --
//              one BLAS per command, built now (blocking); its instances join the TLAS. SetObjectsVisible / ReleaseObjects on what they made.
//   surfaces:  CreateSurface (a field) -> SetSurfaceVisible -> ReleaseSurface -- one BLAS rebuilt from its buffers every frame
//   per frame: SetInstanceTransforms (models[]) -> the first window Paint records calls RecordBuild after its Compute pass:
//              refit the dynamic BLASes, rebuild the surfaces -> write the next ring entry's instances -> rebuild its TLAS -> barrier
//   shaders:   GetTopLevelSlot -- the binding-3 slot of the TLAS built last (IR77_FRAME.tlas); GetObjectSlot -- the object table
//              (IR77_FRAME.ray_objects). Paint writes both into every window's frame table.
//
// The TLASes are a ring of MAX_FRAMES_IN_FLIGHT, advanced once per build -- not tied to any one window's frame index, so the build can move
// to another window (window 0 minimized). Each TLAS has a fixed binding-3 slot; regrowing rewrites the slot's descriptor, never the slot.
//
// One BLAS per object: the triangles of one indirect command, read straight out of the scene's vertex / index buffers by GPU address
// (firstIndex / vertexOffset become the build range). Instances share it -- one TLAS entry per (command, instance), transform from its
// models[] slot. Static BLASes are built once for fast tracing; dynamic ones (posed geometry) allow update and are refit every frame;
// surfaces are rebuilt every frame.
//
// Alpha test: a command whose material is MASK is built non-opaque, and its material row goes in the object table -- one Ir77PVRayObject per
// BLAS, the row a hit's custom index names. include/raytrace.glsl rebuilds the hit's UV from the buffers' addresses there and tests the
// albedo's alpha against the cutoff. Every other BLAS is opaque and never runs the test.
// =============================================================================================================================================
class Ir77PVAcceleration : public Ir77Enlisted, public IIr77PVAcceleration, public std::enable_shared_from_this<Ir77PVAcceleration> {
   public:
    Ir77PVAcceleration() {
        try {
            m_enlisted_uuid.Generate();
        } catch (std::invalid_argument a) {
            throw a;
        }

        m_enlisted = std::chrono::system_clock::now();

        m_top_slots.fill(BINDLESS_NO_SLOT);
    }

    // Ir77PVInit releases this after Paint and before Geometry / Transfer -- the device and the buffers are still alive here
    ~Ir77PVAcceleration() { DefineRelease(); }

   public:
    std::shared_ptr<IIr77Return const> EnlistedAs(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIIr77Enlisted>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> MemberUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77PVAcceleration>(uid);

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

        else if (iid == GUIDIIr77PVAcceleration)
            obj = std::shared_ptr<IIr77PVAcceleration>(shared_from_this(), static_cast<IIr77PVAcceleration*>(this));

        else if (iid == GUIDIr77PVAcceleration)
            obj = std::shared_ptr<Ir77PVAcceleration>(shared_from_this(), static_cast<Ir77PVAcceleration*>(this));

        else
            return GUIDQueryFailed;

        return GUIDQuerySucceeded;
    }

    // =========================================================================================================================================
    // wiring -- Ir77PVInit sets these as each piece exists
    // =========================================================================================================================================
   public:
    std::shared_ptr<IIr77Return const> SetDevice(std::shared_ptr<IIr77PVDevice> device) {
        m_device = device;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Storage, scratch and instance buffers are made through it
    std::shared_ptr<IIr77Return const> SetTransfer(std::shared_ptr<IIr77PVTransfer> transfer) {
        m_transfer = transfer;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Where the scenes' vertex / index buffers and commands come from
    std::shared_ptr<IIr77Return const> SetGeometry(std::shared_ptr<IIr77PVGeometry> geometry) {
        m_geometry = geometry;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // The bindless table -- every TLAS gets a binding-3 slot in it
    std::shared_ptr<IIr77Return const> SetDescriptorSet(std::shared_ptr<IIr77PVDescriptorSet> descriptor_set) {
        m_descriptor_set = descriptor_set;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // The extension commands and the device's limits. Fails on a device without ray queries -- Ir77PVInit only creates this when it has them.
    std::shared_ptr<IIr77Return const> CreateAcceleration() {
        if (!m_device) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAcceleration: SetDevice not called.");

        bool ray_query{false};
        m_device->GetRayQuery(ray_query);

        if (!ray_query) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAcceleration: the device has no ray queries.");

        VkDevice device{VK_NULL_HANDLE};
        m_device->GetDevice(&device);

        m_create_structure = reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(vkGetDeviceProcAddr(device, "vkCreateAccelerationStructureKHR"));
        m_destroy_structure = reinterpret_cast<PFN_vkDestroyAccelerationStructureKHR>(vkGetDeviceProcAddr(device, "vkDestroyAccelerationStructureKHR"));
        m_build_sizes = reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(vkGetDeviceProcAddr(device, "vkGetAccelerationStructureBuildSizesKHR"));
        m_cmd_build = reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(device, "vkCmdBuildAccelerationStructuresKHR"));
        m_structure_address =
            reinterpret_cast<PFN_vkGetAccelerationStructureDeviceAddressKHR>(vkGetDeviceProcAddr(device, "vkGetAccelerationStructureDeviceAddressKHR"));

        if (!m_create_structure || !m_destroy_structure || !m_build_sizes || !m_cmd_build || !m_structure_address)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAcceleration: acceleration-structure commands not found.");

        m_device->GetAccelerationStructureProperties(m_properties);
        m_scratch_alignment = std::max<VkDeviceSize>(m_properties.minAccelerationStructureScratchOffsetAlignment, 1);

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: Create Acceleration.");
    }

    // =========================================================================================================================================
    // scenes -- at load
    // =========================================================================================================================================

    // One BLAS per command of a vertex-path scene, built now in one blocking submit, and one TLAS instance per (command, instance). dynamic =
    // the vertices move every frame (the posed stream) -> built to allow update and refit by every RecordBuild. alpha = the material row per
    // command for MASK materials, BINDLESS_NO_SLOT for opaque ones; shorter than the commands (or empty) -> the rest are opaque. The TLAS
    // buffers grow to fit. objects = the BLASes it made, [first, first + count).
    std::shared_ptr<IIr77Return const> BuildScene(std::uint32_t const& scene_id, bool const& dynamic, std::vector<std::uint32_t> const& alpha,
                                                  glm::uvec2& objects) {
        if (!m_geometry) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAcceleration: SetGeometry not called.");

        std::shared_ptr<IIr77Return const> result{};

        Ir77PVSceneGeometry scene{};
        if (Failed(m_geometry->GetSceneGeometry(scene_id, scene), result)) return result;

        return DefineScene(scene_id, scene, dynamic, alpha, objects);
    }

    // The same for the mesh path's static stream (UploadMeshlets on a ray-query device keeps the scene's index buffer for this). Static.
    std::shared_ptr<IIr77Return const> BuildMeshlets(std::uint32_t const& meshlet_id, std::vector<std::uint32_t> const& alpha, glm::uvec2& objects) {
        if (!m_geometry) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAcceleration: SetGeometry not called.");

        std::shared_ptr<IIr77Return const> result{};

        Ir77PVSceneGeometry scene{};
        if (Failed(m_geometry->GetMeshletGeometry(meshlet_id, scene), result)) return result;

        return DefineScene(meshlet_id, scene, false, alpha, objects);
    }

    // A scene's BLASes (what BuildScene / BuildMeshlets made) in the TLAS or not, from the next frame. Dynamic ones keep being refit.
    std::shared_ptr<IIr77Return const> SetObjectsVisible(glm::uvec2 const& objects, bool const& visible) {
        if (static_cast<std::size_t>(objects.x) + objects.y > m_bottom.size())
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAcceleration: unknown objects.");

        for (std::uint32_t b = objects.x; b < objects.x + objects.y; b++) m_bottom[b].visible = visible;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // A scene's BLASes out of the TLAS for good; their structures are destroyed now (waits for the device). Before the scene is unloaded.
    std::shared_ptr<IIr77Return const> ReleaseObjects(glm::uvec2 const& objects) {
        if (static_cast<std::size_t>(objects.x) + objects.y > m_bottom.size())
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAcceleration: unknown objects.");
        if (objects.y == 0) return Ir77RETURN<Ir77OperationSucceeded>();

        return DestroyObjects(objects);
    }

    // =========================================================================================================================================
    // surfaces -- world-space geometry whose shape changes every frame (a field surface)
    // =========================================================================================================================================

    // One BLAS over vertex (positions = the first 3 floats of each vertex_stride-byte vertex, vertex_count of them) and index (triangle_count
    // triangles of uint32), in world space, opaque. Rebuilt from those buffers by every RecordBuild, so whatever wrote them this frame (a
    // Compute dispatch) is what the rays see. Both buffers need the build-input usage (Transfer's storage buffers have it on this device).
    // surface = its handle for SetSurfaceVisible / ReleaseSurface. Visible from the next frame.
    std::shared_ptr<IIr77Return const> CreateSurface(std::shared_ptr<IIr77PVBuffer> const& vertex, std::uint32_t const& vertex_stride,
                                                     std::uint32_t const& vertex_count, std::shared_ptr<IIr77PVBuffer> const& index,
                                                     std::uint32_t const& triangle_count, std::uint32_t& surface) {
        if (!m_cmd_build) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAcceleration: CreateAcceleration not called.");
        if (!vertex || !index || vertex_count == 0 || triangle_count == 0 || vertex_stride < 3 * sizeof(float))
            return Ir77RETURN<Ir77InvalidArgument>(this, "Ir77PVAcceleration: surface needs vertices, triangles and a stride of at least 12 bytes.");

        std::shared_ptr<IIr77Return const> result{};

        Ir77PVBottomLevel blas{};
        blas.rebuild = true;
        blas.vertex_address = GetBufferAddress(vertex, 0);
        blas.index_address = GetBufferAddress(index, 0);
        blas.vertex_stride = vertex_stride;
        blas.max_vertex = vertex_count - 1;
        blas.triangle_count = triangle_count;

        if (blas.vertex_address == 0 || blas.index_address == 0)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAcceleration: surface buffers have no device address.");

        if (Failed(DefineObject(blas, surface), result)) return result;

        m_instances.push_back(Ir77PVTopInstance{surface, UINT32_MAX});  // world space -- identity transform

        if (Failed(DefineFrameResources(), result)) return result;
        if (Failed(DefineObjects(), result)) return result;

        return Ir77RETURN<Ir77OperationSucceeded>(
            this, "Ir77PVAcceleration: surface " + std::to_string(surface) + " -- " + std::to_string(triangle_count) + " triangles, rebuilt every frame.");
    }

    // In the TLAS or not, from the next frame. Hidden surfaces aren't rebuilt either.
    std::shared_ptr<IIr77Return const> SetSurfaceVisible(std::uint32_t const& surface, bool const& visible) {
        if (surface >= m_bottom.size() || !m_bottom[surface].alive || !m_bottom[surface].rebuild)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAcceleration: unknown surface.");

        m_bottom[surface].visible = visible;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Out of the TLAS for good; its structure is destroyed now (waits for the device). Release before destroying the buffers it reads.
    std::shared_ptr<IIr77Return const> ReleaseSurface(std::uint32_t const& surface) {
        if (surface >= m_bottom.size() || !m_bottom[surface].alive || !m_bottom[surface].rebuild)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAcceleration: unknown surface.");

        return DestroyObjects(glm::uvec2(surface, 1u));
    }

    // =========================================================================================================================================
    // per frame
    // =========================================================================================================================================

    // models[] -- a TLAS instance takes its transform from its slot. Kept until replaced.
    std::shared_ptr<IIr77Return const> SetInstanceTransforms(std::vector<glm::mat4> const& transforms) {
        m_transforms = transforms;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Once a frame, in the first command buffer Paint records, after its Compute pass, outside any render pass:
    //   0. barrier -- the Compute pass's writes (skinned vertices, field points); earlier builds (they share the scratch); earlier frames' ray
    //      queries (the ring entry about to be rebuilt may be the one they traced)
    //   1. refit every dynamic BLAS in place, rebuild every visible surface
    //   2. write the ring entry's instance array (host-mapped copy -- Paint waits for the old builder's frames whenever the builder moves)
    //   3. rebuild the ring entry's TLAS from it
    //   4. barrier -- the TLAS visible to ray queries in fragment / compute shaders; GetTopLevelSlot now names it
    std::shared_ptr<IIr77Return const> RecordBuild(VkCommandBuffer const& cmd) {
        m_built_slot = BINDLESS_NO_SLOT;
        if (!m_cmd_build || m_instances.empty() || !m_instance_buffer) return Ir77RETURN<Ir77OperationSucceeded>();  // nothing to trace

        std::uint32_t const frame = m_ring;

        VkDeviceAddress const scratch = AlignUp(GetBufferAddress(m_frame_scratch, 0), m_scratch_alignment);

        // 0.
        DefineBarrier(
            cmd, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
            VK_ACCESS_2_SHADER_WRITE_BIT | VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR, VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
            VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR);

        // 1. -- refits update in place; surfaces build from scratch (their shape is new every frame). Each its own scratch region.
        if (m_frame_count > 0) {
            std::vector<VkAccelerationStructureGeometryKHR> geometries(m_frame_count);
            std::vector<VkAccelerationStructureBuildGeometryInfoKHR> builds(m_frame_count);
            std::vector<VkAccelerationStructureBuildRangeInfoKHR> ranges(m_frame_count);
            std::vector<VkAccelerationStructureBuildRangeInfoKHR const*> range_pointers(m_frame_count);

            VkDeviceAddress region = scratch;
            std::size_t k{0};

            for (Ir77PVBottomLevel const& blas : m_bottom) {
                if (!blas.alive || !(blas.dynamic || (blas.rebuild && blas.visible)) || k >= m_frame_count) continue;

                VkBuildAccelerationStructureModeKHR const mode{blas.dynamic ? VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR
                                                                            : VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR};

                DefineBottomGeometry(blas, geometries[k]);
                DefineBottomBuild(blas, &geometries[k], mode, builds[k]);
                builds[k].scratchData.deviceAddress = region;
                DefineBottomRange(blas, ranges[k]);
                range_pointers[k] = &ranges[k];

                region += AlignUp(blas.dynamic ? blas.update_scratch : blas.build_scratch, m_scratch_alignment);
                k++;
            }

            if (k > 0) m_cmd_build(cmd, static_cast<std::uint32_t>(k), builds.data(), range_pointers.data());

            // the refits write the BLASes the TLAS build reads, and the scratch it reuses
            DefineBarrier(cmd, VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR,
                          VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                          VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR);
        }

        // 2. -- live, visible BLASes only (a hidden or released surface drops out)
        m_instance_data.clear();

        for (Ir77PVTopInstance const& entry : m_instances) {
            Ir77PVBottomLevel const& blas = m_bottom[entry.bottom];
            if (!blas.alive || !blas.visible) continue;

            glm::mat4 const model = entry.slot < m_transforms.size() ? m_transforms[entry.slot] : glm::mat4(1.0f);

            VkAccelerationStructureInstanceKHR instance{};

            // 3 x 4, row-major -- glm is column-major
            for (int r = 0; r < 3; r++)
                for (int c = 0; c < 4; c++) instance.transform.matrix[r][c] = model[c][r];

            instance.instanceCustomIndex = entry.bottom & 0xFFFFFFu;  // the BLAS -- its Ir77PVRayObject row (the alpha test)
            instance.mask = 0xFF;
            instance.instanceShaderBindingTableRecordOffset = 0;
            instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
            instance.accelerationStructureReference = blas.address;

            m_instance_data.push_back(instance);
        }

        if (m_instance_data.empty()) return Ir77RETURN<Ir77OperationSucceeded>();  // everything hidden -- no TLAS this frame

        auto const written = m_instance_buffer->Update(frame, m_instance_data.data(),
                                                       static_cast<VkDeviceSize>(m_instance_data.size() * sizeof(VkAccelerationStructureInstanceKHR)));
        if (written->ID() != GUIDIr77OperationSucceeded) return written;

        // 3.
        VkAccelerationStructureGeometryKHR geometry{};
        geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
        geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
        geometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
        geometry.geometry.instances.arrayOfPointers = VK_FALSE;
        geometry.geometry.instances.data.deviceAddress = GetBufferAddress(m_instance_buffer, frame);

        VkAccelerationStructureBuildGeometryInfoKHR build{};
        DefineTopBuild(&geometry, build);
        build.dstAccelerationStructure = m_top[frame].structure;
        build.scratchData.deviceAddress = scratch;

        VkAccelerationStructureBuildRangeInfoKHR range{};
        range.primitiveCount = static_cast<std::uint32_t>(m_instance_data.size());

        VkAccelerationStructureBuildRangeInfoKHR const* range_pointer = &range;

        m_cmd_build(cmd, 1, &build, &range_pointer);

        // 4.
        DefineBarrier(cmd, VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR,
                      VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR);

        m_built_slot = m_top_slots[frame];
        m_ring = (m_ring + 1) % MAX_FRAMES_IN_FLIGHT;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // The binding-3 slot of the TLAS the last RecordBuild built -- what every window's frame table carries this frame (IR77_FRAME.tlas).
    // BINDLESS_NO_SLOT before the first build or with nothing to trace.
    std::shared_ptr<IIr77Return const> GetTopLevelSlot(std::uint32_t& slot) {
        slot = m_instances.empty() ? BINDLESS_NO_SLOT : m_built_slot;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // The binding-1 slot of the object table -- one Ir77PVRayObject per BLAS, row = a hit's custom index (IR77_FRAME.ray_objects).
    // BINDLESS_NO_SLOT before the first BLAS.
    std::shared_ptr<IIr77Return const> GetObjectSlot(std::uint32_t& slot) {
        slot = m_has_objects ? m_objects_slot : BINDLESS_NO_SLOT;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   private:
    // =========================================================================================================================================
    // steps
    // =========================================================================================================================================

    // BuildScene / BuildMeshlets: one BLAS per command of scene, built now in one submit, and its instances
    std::shared_ptr<IIr77Return const> DefineScene(std::uint32_t const& scene_id, Ir77PVSceneGeometry const& scene, bool const& dynamic,
                                                   std::vector<std::uint32_t> const& alpha, glm::uvec2& objects) {
        objects = glm::uvec2(static_cast<std::uint32_t>(m_bottom.size()), 0u);

        if (!m_cmd_build) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAcceleration: CreateAcceleration not called.");
        if (!m_transfer) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAcceleration: SetTransfer not called.");

        if (!scene.vertex || !scene.index || scene.vertex_count == 0)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAcceleration: scene has no vertex / index buffers to build from.");

        std::shared_ptr<IIr77Return const> result{};

        VkDeviceAddress const vertex_address = GetBufferAddress(scene.vertex, 0);
        VkDeviceAddress const index_address = GetBufferAddress(scene.index, 0);

        std::size_t const first = m_bottom.size();
        std::uint32_t tested{0};

        // 1. one BLAS per command -- what it builds from, its sizes, its storage and the structure itself
        for (std::size_t c = 0; c < scene.commands.size(); c++) {
            VkDrawIndexedIndirectCommand const& cmd = scene.commands[c];
            if (cmd.indexCount == 0 || cmd.indexCount % 3 != 0 || cmd.vertexOffset < 0) continue;

            Ir77PVBottomLevel blas{};
            blas.scene_id = scene_id;
            blas.command = static_cast<std::uint32_t>(c);
            blas.dynamic = dynamic;
            blas.vertex_address = vertex_address;
            blas.index_address = index_address;
            blas.max_vertex = scene.vertex_count - 1;  // the whole buffer -- an upper bound is all the build needs
            blas.first_index = cmd.firstIndex;
            blas.vertex_offset = cmd.vertexOffset;
            blas.triangle_count = cmd.indexCount / 3;
            blas.material = c < alpha.size() ? alpha[c] : BINDLESS_NO_SLOT;

            if (blas.material != BINDLESS_NO_SLOT) tested++;

            std::uint32_t bottom{0};
            if (Failed(DefineObject(blas, bottom), result)) return result;

            // its instances -- the same BLAS through every models[] slot the command draws
            std::uint32_t const instances = c < scene.instance_counts.size() ? scene.instance_counts[c] : cmd.instanceCount;
            for (std::uint32_t i = 0; i < instances; i++) m_instances.push_back(Ir77PVTopInstance{bottom, cmd.firstInstance + i});
        }

        // 2. build them -- one submit, each BLAS its own region of one scratch buffer
        if (Failed(DefineBottomBuilds(first), result)) return result;

        // 3. the per-frame resources have to fit every instance and every refit now; the shaders' object table gets the new rows
        if (Failed(DefineFrameResources(), result)) return result;
        if (Failed(DefineObjects(), result)) return result;

        objects = glm::uvec2(static_cast<std::uint32_t>(first), static_cast<std::uint32_t>(m_bottom.size() - first));

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Ir77PVAcceleration: scene " + std::to_string(scene_id) + " -- " +
                                                            std::to_string(m_bottom.size() - first) + " BLAS" + (dynamic ? " (refit every frame)" : "") +
                                                            (tested > 0 ? ", " + std::to_string(tested) + " alpha-tested" : std::string{}) + ", " +
                                                            std::to_string(m_instances.size()) + " TLAS instances in all.");
    }

    // BLASes [objects.x, objects.x + objects.y) gone: structures destroyed (the device idle first -- a frame in flight may still trace them),
    // storage buried, rows left empty so every other handle keeps its meaning, their instances out of the TLAS.
    std::shared_ptr<IIr77Return const> DestroyObjects(glm::uvec2 const& objects) {
        VkDevice device{VK_NULL_HANDLE};
        m_device->GetDevice(&device);

        vkDeviceWaitIdle(device);

        for (std::uint32_t b = objects.x; b < objects.x + objects.y; b++) {
            Ir77PVBottomLevel& blas = m_bottom[b];
            if (!blas.alive) continue;

            if (blas.structure != VK_NULL_HANDLE) m_destroy_structure(device, blas.structure, nullptr);
            m_transfer->Bury({.buffers = {blas.storage}});

            blas = Ir77PVBottomLevel{};
            blas.alive = false;
        }

        std::erase_if(m_instances, [&](Ir77PVTopInstance const& instance) { return instance.bottom >= objects.x && instance.bottom < objects.x + objects.y; });

        std::shared_ptr<IIr77Return const> result{};
        if (Failed(DefineFrameResources(), result)) return result;

        return DefineObjects();
    }

    // One BLAS: its build sizes, its storage and the structure, appended to m_bottom. bottom = its index (handle, custom index, object row).
    std::shared_ptr<IIr77Return const> DefineObject(Ir77PVBottomLevel& blas, std::uint32_t& bottom) {
        std::shared_ptr<IIr77Return const> result{};

        VkDevice device{VK_NULL_HANDLE};
        m_device->GetDevice(&device);

        VkAccelerationStructureGeometryKHR geometry{};
        DefineBottomGeometry(blas, geometry);

        VkAccelerationStructureBuildGeometryInfoKHR build{};
        DefineBottomBuild(blas, &geometry, VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR, build);

        VkAccelerationStructureBuildSizesInfoKHR sizes{};
        sizes.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
        m_build_sizes(device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &build, &blas.triangle_count, &sizes);

        blas.build_scratch = sizes.buildScratchSize;
        blas.update_scratch = sizes.updateScratchSize;

        if (Failed(
                DefineStructure(VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR, sizes.accelerationStructureSize, blas.storage, blas.structure, blas.address),
                result))
            return result;

        bottom = static_cast<std::uint32_t>(m_bottom.size());
        m_bottom.push_back(blas);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // The object table, rewritten whole -- one Ir77PVRayObject per BLAS (released ones empty). A new buffer each time: the old one is buried,
    // so a frame in flight still reads the table it was recorded with. Load / compile time only.
    std::shared_ptr<IIr77Return const> DefineObjects() {
        if (m_bottom.empty()) return Ir77RETURN<Ir77OperationSucceeded>();

        std::vector<Ir77PVRayObject> objects(m_bottom.size());

        for (std::size_t b = 0; b < m_bottom.size(); b++) {
            Ir77PVBottomLevel const& blas = m_bottom[b];
            if (!blas.alive) continue;

            Ir77PVRayObject& object = objects[b];
            object.vertex_address = {static_cast<std::uint32_t>(blas.vertex_address & 0xFFFFFFFFull), static_cast<std::uint32_t>(blas.vertex_address >> 32)};
            object.index_address = {static_cast<std::uint32_t>(blas.index_address & 0xFFFFFFFFull), static_cast<std::uint32_t>(blas.index_address >> 32)};
            object.first_index = blas.first_index;
            object.vertex_offset = blas.vertex_offset;
            object.vertex_floats = static_cast<std::uint32_t>(blas.vertex_stride / sizeof(float));
            object.material = blas.material;
        }

        std::shared_ptr<IIr77Return const> result{};

        std::uint32_t buffer_id{0};
        std::uint32_t slot{BINDLESS_NO_SLOT};
        if (Failed(m_transfer->CreateStorageBuffer(static_cast<VkDeviceSize>(objects.size() * sizeof(Ir77PVRayObject)), objects.data(), buffer_id, slot),
                   result))
            return result;

        if (m_has_objects) m_transfer->DestroyStorageBuffer(m_objects_buffer);

        m_objects_buffer = buffer_id;
        m_objects_slot = slot;
        m_has_objects = true;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // BLASes [first, end) in one blocking submit on the device's upload pool -- each its own region of one scratch buffer, buried after
    std::shared_ptr<IIr77Return const> DefineBottomBuilds(std::size_t const& first) {
        std::size_t const count = m_bottom.size() - first;
        if (count == 0) return Ir77RETURN<Ir77OperationSucceeded>();

        std::shared_ptr<IIr77Return const> result{};

        VkDeviceSize scratch_size{0};
        for (std::size_t k = first; k < m_bottom.size(); k++) scratch_size += AlignUp(m_bottom[k].build_scratch, m_scratch_alignment);

        std::shared_ptr<IIr77PVBuffer> scratch_buffer{};
        if (Failed(m_transfer->CreateStaticBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                                  scratch_size + m_scratch_alignment, scratch_buffer),
                   result))
            return result;

        std::vector<VkAccelerationStructureGeometryKHR> geometries(count);
        std::vector<VkAccelerationStructureBuildGeometryInfoKHR> builds(count);
        std::vector<VkAccelerationStructureBuildRangeInfoKHR> ranges(count);
        std::vector<VkAccelerationStructureBuildRangeInfoKHR const*> range_pointers(count);

        VkDeviceAddress region = AlignUp(GetBufferAddress(scratch_buffer, 0), m_scratch_alignment);

        for (std::size_t k = 0; k < count; k++) {
            Ir77PVBottomLevel const& blas = m_bottom[first + k];

            DefineBottomGeometry(blas, geometries[k]);
            DefineBottomBuild(blas, &geometries[k], VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR, builds[k]);
            builds[k].scratchData.deviceAddress = region;
            DefineBottomRange(blas, ranges[k]);
            range_pointers[k] = &ranges[k];

            region += AlignUp(blas.build_scratch, m_scratch_alignment);
        }

        // one-shot command buffer on the upload pool
        VkDevice device{VK_NULL_HANDLE};
        m_device->GetDevice(&device);

        VkCommandPool pool{VK_NULL_HANDLE};
        VkQueue queue{VK_NULL_HANDLE};
        if (Failed(m_device->GetUploadPool(pool, queue), result)) return result;

        VkCommandBufferAllocateInfo allocate{};
        allocate.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocate.commandPool = pool;
        allocate.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocate.commandBufferCount = 1;

        VkCommandBuffer cmd{VK_NULL_HANDLE};
        if (vkAllocateCommandBuffers(device, &allocate, &cmd) != VK_SUCCESS)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAcceleration: vkAllocateCommandBuffers failed.");

        VkCommandBufferBeginInfo begin{};
        begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(cmd, &begin);

        m_cmd_build(cmd, static_cast<std::uint32_t>(count), builds.data(), range_pointers.data());

        // built before any frame's TLAS build reads them
        DefineBarrier(
            cmd, VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR,
            VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR);

        vkEndCommandBuffer(cmd);

        VkFenceCreateInfo fence_info{};
        fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

        VkFence fence{VK_NULL_HANDLE};
        vkCreateFence(device, &fence_info, nullptr, &fence);

        VkSubmitInfo submit{};
        submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit.commandBufferCount = 1;
        submit.pCommandBuffers = &cmd;

        VkResult const submitted = vkQueueSubmit(queue, 1, &submit, fence);
        VkResult const waited = submitted == VK_SUCCESS ? vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX) : submitted;

        vkDestroyFence(device, fence, nullptr);
        vkFreeCommandBuffers(device, pool, 1, &cmd);

        m_transfer->Bury({.buffers = {scratch_buffer}});

        if (waited != VK_SUCCESS) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAcceleration: BLAS build submit failed.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // What every frame needs, sized for the instances and the per-frame BLASes there are now: one TLAS per frame in flight (storage +
    // structure), the host-written instance buffer (one copy per frame in flight), and the scratch the refits, the surface rebuilds and the
    // TLAS build share. Grows only; the old resources are destroyed after the device goes idle (load / compile time).
    std::shared_ptr<IIr77Return const> DefineFrameResources() {
        std::shared_ptr<IIr77Return const> result{};

        VkDevice device{VK_NULL_HANDLE};
        m_device->GetDevice(&device);

        std::uint32_t const capacity = static_cast<std::uint32_t>(m_instances.size());

        // per-frame BLAS work -- every refit and every surface rebuild its own region (hidden surfaces counted: they may show again)
        m_frame_count = 0;
        VkDeviceSize update_size{0};

        for (Ir77PVBottomLevel const& blas : m_bottom) {
            if (!blas.alive || !(blas.dynamic || blas.rebuild)) continue;

            update_size += AlignUp(blas.dynamic ? blas.update_scratch : blas.build_scratch, m_scratch_alignment);
            m_frame_count++;
        }

        if (capacity == 0 || (capacity <= m_top_capacity && update_size <= m_update_size)) return Ir77RETURN<Ir77OperationSucceeded>();

        // the TLAS at this capacity
        VkAccelerationStructureGeometryKHR geometry{};
        geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
        geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
        geometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;

        VkAccelerationStructureBuildGeometryInfoKHR build{};
        DefineTopBuild(&geometry, build);

        VkAccelerationStructureBuildSizesInfoKHR sizes{};
        sizes.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
        m_build_sizes(device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &build, &capacity, &sizes);

        // the old ones may still be in use by a frame in flight
        vkDeviceWaitIdle(device);

        DefineTopRelease();

        for (std::uint32_t f = 0; f < MAX_FRAMES_IN_FLIGHT; f++) {
            VkDeviceAddress address{0};
            if (Failed(DefineStructure(VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR, sizes.accelerationStructureSize, m_top[f].storage, m_top[f].structure,
                                       address),
                       result))
                return result;

            // its binding-3 slot -- registered once, rewritten in place when the TLAS regrows (the device is idle here)
            if (!m_descriptor_set) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAcceleration: SetDescriptorSet not called.");

            if (m_top_slots[f] == BINDLESS_NO_SLOT) {
                if (Failed(m_descriptor_set->RegisterAccelerationStructure(m_top[f].structure, m_top_slots[f]), result)) return result;
            } else {
                if (Failed(m_descriptor_set->UpdateAccelerationStructure(m_top_slots[f], m_top[f].structure), result)) return result;
            }
        }

        m_built_slot = BINDLESS_NO_SLOT;  // nothing built at the new size yet

        if (Failed(m_transfer->CreateDynamicBuffer(
                       VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                       static_cast<VkDeviceSize>(capacity) * sizeof(VkAccelerationStructureInstanceKHR), m_instance_buffer),
                   result))
            return result;

        // the BLAS work and the TLAS build run one after the other (a barrier between), so they share one scratch
        VkDeviceSize const scratch_size = std::max(update_size, sizes.buildScratchSize);

        if (Failed(m_transfer->CreateStaticBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                                  scratch_size + m_scratch_alignment, m_frame_scratch),
                   result))
            return result;

        m_top_capacity = capacity;
        m_update_size = update_size;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // A structure of `type` in its own storage buffer; address = its device address (what TLAS instances point at)
    std::shared_ptr<IIr77Return const> DefineStructure(VkAccelerationStructureTypeKHR const& type, VkDeviceSize const& size,
                                                       std::shared_ptr<IIr77PVBuffer>& storage, VkAccelerationStructureKHR& structure,
                                                       VkDeviceAddress& address) {
        std::shared_ptr<IIr77Return const> result{};

        if (Failed(m_transfer->CreateStaticBuffer(VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, size,
                                                  storage),
                   result))
            return result;

        VkBuffer buffer{VK_NULL_HANDLE};
        storage->GetBuffer(0, &buffer);

        VkAccelerationStructureCreateInfoKHR create{};
        create.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
        create.buffer = buffer;
        create.offset = 0;
        create.size = size;
        create.type = type;

        VkDevice device{VK_NULL_HANDLE};
        m_device->GetDevice(&device);

        if (m_create_structure(device, &create, nullptr, &structure) != VK_SUCCESS)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVAcceleration: vkCreateAccelerationStructureKHR failed.");

        VkAccelerationStructureDeviceAddressInfoKHR address_info{};
        address_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
        address_info.accelerationStructure = structure;

        address = m_structure_address(device, &address_info);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // One object's triangles -- positions are the first 3 floats of each vertex. Opaque unless it has an alpha-test material: then every
    // candidate hit goes to the shader's test (include/raytrace.glsl).
    void DefineBottomGeometry(Ir77PVBottomLevel const& blas, VkAccelerationStructureGeometryKHR& geometry) const {
        geometry = VkAccelerationStructureGeometryKHR{};
        geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
        geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
        geometry.flags = blas.material == BINDLESS_NO_SLOT ? VK_GEOMETRY_OPAQUE_BIT_KHR : VK_GEOMETRY_NO_DUPLICATE_ANY_HIT_INVOCATION_BIT_KHR;

        VkAccelerationStructureGeometryTrianglesDataKHR& triangles = geometry.geometry.triangles;
        triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
        triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
        triangles.vertexData.deviceAddress = blas.vertex_address;
        triangles.vertexStride = blas.vertex_stride;
        triangles.maxVertex = blas.max_vertex;
        triangles.indexType = VK_INDEX_TYPE_UINT32;
        triangles.indexData.deviceAddress = blas.index_address;
    }

    // static: built once, fastest to trace. dynamic: refit every frame, so it allows update and favours build speed. rebuild: built every
    // frame -- build speed.
    void DefineBottomBuild(Ir77PVBottomLevel const& blas, VkAccelerationStructureGeometryKHR const* geometry, VkBuildAccelerationStructureModeKHR const& mode,
                           VkAccelerationStructureBuildGeometryInfoKHR& build) const {
        build = VkAccelerationStructureBuildGeometryInfoKHR{};
        build.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        build.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        build.flags = blas.dynamic   ? VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR
                      : blas.rebuild ? VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR
                                     : VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        build.mode = mode;
        build.srcAccelerationStructure = mode == VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR ? blas.structure : VK_NULL_HANDLE;  // refit in place
        build.dstAccelerationStructure = blas.structure;
        build.geometryCount = 1;
        build.pGeometries = geometry;
    }

    // The command's slice: firstIndex (in bytes) into the index buffer, vertexOffset added to every index
    void DefineBottomRange(Ir77PVBottomLevel const& blas, VkAccelerationStructureBuildRangeInfoKHR& range) const {
        range = VkAccelerationStructureBuildRangeInfoKHR{};
        range.primitiveCount = blas.triangle_count;
        range.primitiveOffset = blas.first_index * static_cast<std::uint32_t>(sizeof(std::uint32_t));
        range.firstVertex = static_cast<std::uint32_t>(blas.vertex_offset);
        range.transformOffset = 0;
    }

    // Rebuilt from scratch every frame -- favours build speed
    void DefineTopBuild(VkAccelerationStructureGeometryKHR const* geometry, VkAccelerationStructureBuildGeometryInfoKHR& build) const {
        build = VkAccelerationStructureBuildGeometryInfoKHR{};
        build.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        build.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
        build.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR;
        build.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        build.geometryCount = 1;
        build.pGeometries = geometry;
    }

    void DefineBarrier(VkCommandBuffer const& cmd, VkPipelineStageFlags2 const& src_stage, VkAccessFlags2 const& src_access,
                       VkPipelineStageFlags2 const& dst_stage, VkAccessFlags2 const& dst_access) const {
        VkMemoryBarrier2 barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
        barrier.srcStageMask = src_stage;
        barrier.srcAccessMask = src_access;
        barrier.dstStageMask = dst_stage;
        barrier.dstAccessMask = dst_access;

        VkDependencyInfo dependency{};
        dependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        dependency.memoryBarrierCount = 1;
        dependency.pMemoryBarriers = &barrier;

        vkCmdPipelineBarrier2(cmd, &dependency);
    }

    // The TLASes -- the device is idle when this runs. Their buffers go to Transfer's graveyard.
    void DefineTopRelease() {
        VkDevice device{VK_NULL_HANDLE};
        if (m_device) m_device->GetDevice(&device);

        for (Ir77PVTopLevel& top : m_top) {
            if (top.structure != VK_NULL_HANDLE && device != VK_NULL_HANDLE && m_destroy_structure) m_destroy_structure(device, top.structure, nullptr);
            if (m_transfer) m_transfer->Bury({.buffers = {top.storage}});

            top = Ir77PVTopLevel{};
        }

        if (m_transfer) m_transfer->Bury({.buffers = {m_instance_buffer, m_frame_scratch}});

        m_instance_buffer.reset();
        m_frame_scratch.reset();
        m_top_capacity = 0;
        m_update_size = 0;
    }

    // Everything -- waits for the device first
    void DefineRelease() {
        if (!m_device) return;

        VkDevice device{VK_NULL_HANDLE};
        m_device->GetDevice(&device);
        if (device == VK_NULL_HANDLE) return;

        vkDeviceWaitIdle(device);

        DefineTopRelease();

        for (Ir77PVBottomLevel& blas : m_bottom) {
            if (!blas.alive) continue;  // released already

            if (blas.structure != VK_NULL_HANDLE && m_destroy_structure) m_destroy_structure(device, blas.structure, nullptr);
            if (m_transfer) m_transfer->Bury({.buffers = {blas.storage}});
        }

        m_bottom.clear();
        m_instances.clear();
        m_frame_count = 0;

        if (m_has_objects && m_transfer) m_transfer->DestroyStorageBuffer(m_objects_buffer);
        m_has_objects = false;
        m_objects_slot = BINDLESS_NO_SLOT;

        // the TLAS slots back to the table -- straight away: the device is idle
        for (std::uint32_t& slot : m_top_slots) {
            if (slot != BINDLESS_NO_SLOT && m_descriptor_set) m_descriptor_set->Release(BINDLESS_BINDING_ACCELERATION, slot);
            slot = BINDLESS_NO_SLOT;
        }

        m_built_slot = BINDLESS_NO_SLOT;
    }

    // A buffer copy's GPU address -- needs VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
    VkDeviceAddress GetBufferAddress(std::shared_ptr<IIr77PVBuffer> const& buffer, std::uint32_t const& copy) const {
        if (!buffer || !m_device) return 0;

        VkBuffer handle{VK_NULL_HANDLE};
        buffer->GetBuffer(copy, &handle);

        VkDevice device{VK_NULL_HANDLE};
        m_device->GetDevice(&device);

        VkBufferDeviceAddressInfo info{};
        info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
        info.buffer = handle;

        return vkGetBufferDeviceAddress(device, &info);
    }

    VkDeviceSize AlignUp(VkDeviceSize const value, VkDeviceSize const alignment) const { return (value + alignment - 1) / alignment * alignment; }

   private:
    // wiring -- Ir77PVInit
    std::shared_ptr<IIr77PVDevice> m_device{};

    std::shared_ptr<IIr77PVTransfer> m_transfer{};

    std::shared_ptr<IIr77PVGeometry> m_geometry{};

    std::shared_ptr<IIr77PVDescriptorSet> m_descriptor_set{};

    // extension commands -- CreateAcceleration
    PFN_vkCreateAccelerationStructureKHR m_create_structure{nullptr};

    PFN_vkDestroyAccelerationStructureKHR m_destroy_structure{nullptr};

    PFN_vkGetAccelerationStructureBuildSizesKHR m_build_sizes{nullptr};

    PFN_vkCmdBuildAccelerationStructuresKHR m_cmd_build{nullptr};

    PFN_vkGetAccelerationStructureDeviceAddressKHR m_structure_address{nullptr};

    VkPhysicalDeviceAccelerationStructurePropertiesKHR m_properties{};

    VkDeviceSize m_scratch_alignment{256};

    // bottom level -- one per object (built at load) or surface (built every frame); released rows stay, so indices never move
    std::vector<Ir77PVBottomLevel> m_bottom{};

    std::uint32_t m_frame_count{0};  // BLASes refit or rebuilt every frame -- live dynamic ones plus every live surface

    // the shaders' object table -- one Ir77PVRayObject per BLAS (Transfer storage buffer)
    bool m_has_objects{false};

    std::uint32_t m_objects_buffer{0};

    std::uint32_t m_objects_slot{BINDLESS_NO_SLOT};

    // top level -- one TLAS entry per (object, instance); one TLAS per frame in flight
    std::vector<Ir77PVTopInstance> m_instances{};

    std::array<Ir77PVTopLevel, MAX_FRAMES_IN_FLIGHT> m_top{};  // the ring

    std::array<std::uint32_t, MAX_FRAMES_IN_FLIGHT> m_top_slots{};  // binding-3 slot per ring entry -- BINDLESS_NO_SLOT until registered (constructor)

    std::uint32_t m_ring{0};  // the ring entry the next RecordBuild rebuilds

    std::uint32_t m_built_slot{BINDLESS_NO_SLOT};  // slot of the TLAS the last RecordBuild built

    std::uint32_t m_top_capacity{0};  // instances the TLASes and the instance buffer are sized for

    VkDeviceSize m_update_size{0};  // per-frame BLAS scratch the frame scratch is sized for

    std::shared_ptr<IIr77PVBuffer> m_instance_buffer{};  // Dynamic, one copy per ring entry

    std::shared_ptr<IIr77PVBuffer> m_frame_scratch{};  // refits, then the TLAS build

    std::vector<VkAccelerationStructureInstanceKHR> m_instance_data{};  // RecordBuild's scratch

    std::vector<glm::mat4> m_transforms{};  // models[] -- SetInstanceTransforms
};
}  // namespace NSIr77PeregrineV
