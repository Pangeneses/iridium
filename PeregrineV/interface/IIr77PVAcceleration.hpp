#pragma once

#include <vulkan/vulkan.h>

#include <glm/glm.hpp>

#include <cstdint>
#include <memory>
#include <vector>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "IIr77PVDevice.hpp"
#include "IIr77PVDescriptorSet.hpp"
#include "IIr77PVTransfer.hpp"
#include "IIr77PVGeometry.hpp"
#include "IIr77PVBuffer.hpp"

#include "../types/Ir77PVTypes.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

// Ray-query acceleration structures over Geometry's scenes: one bottom level (BLAS) per object -- a scene command's triangles -- built at
// load, surfaces (a field) rebuilt every frame, and a ring of top levels (TLAS), one rebuilt every frame from the instance transforms. Dynamic
// scenes (posed geometry) are refit every frame instead of rebuilt. MASK materials are built non-opaque and alpha-tested by the shaders through
// the object table. Every buffer is made through Transfer; every TLAS has a fixed bindless slot (binding 3).
//   Ir77PVInit wires it (ray-query devices only): SetDevice / SetTransfer / SetGeometry / SetDescriptorSet -> CreateAcceleration. Each frame
//   Paint hands it to the first command buffer it records, which calls RecordBuild after its Compute pass; Paint then writes GetTopLevelSlot
//   into every window's frame table.
typedef struct IIr77PVAcceleration : virtual public IIr77Enlisted {
    IIr77PVAcceleration() = default;

    // ---- wiring -- Ir77PVInit ------------------------------------------------------------------------------------------------------------------

    virtual std::shared_ptr<IIr77Return const> SetDevice(std::shared_ptr<IIr77PVDevice> device) = 0;

    // Storage, scratch and instance buffers are made through it
    virtual std::shared_ptr<IIr77Return const> SetTransfer(std::shared_ptr<IIr77PVTransfer> transfer) = 0;

    // Where the scenes' vertex / index buffers and commands come from
    virtual std::shared_ptr<IIr77Return const> SetGeometry(std::shared_ptr<IIr77PVGeometry> geometry) = 0;

    // The bindless table -- every TLAS gets a binding-3 slot in it
    virtual std::shared_ptr<IIr77Return const> SetDescriptorSet(std::shared_ptr<IIr77PVDescriptorSet> descriptor_set) = 0;

    // Loads the extension commands. Fails on a device without ray queries.
    virtual std::shared_ptr<IIr77Return const> CreateAcceleration() = 0;

    // ---- scenes ------------------------------------------------------------------------------------------------------------------------------

    // One BLAS per command of a vertex-path scene (UploadScene), and one TLAS instance per (command, instance). Blocking. dynamic = the
    // vertices move every frame (compute skinning) -> refit every frame. alpha = per command, the material row of a MASK material or
    // BINDLESS_NO_SLOT (opaque); missing entries are opaque. The TLAS buffers grow to fit. objects = the BLASes it made, [first, first + count)
    // -- for SetObjectsVisible / ReleaseObjects.
    virtual std::shared_ptr<IIr77Return const> BuildScene(std::uint32_t const& scene_id, bool const& dynamic, std::vector<std::uint32_t> const& alpha,
                                                          glm::uvec2& objects) = 0;

    // The same for the mesh path's static stream (UploadMeshlets) -- static
    virtual std::shared_ptr<IIr77Return const> BuildMeshlets(std::uint32_t const& meshlet_id, std::vector<std::uint32_t> const& alpha,
                                                             glm::uvec2& objects) = 0;

    // A scene's BLASes in the TLAS or not, from the next frame -- an asset left out of a frame
    virtual std::shared_ptr<IIr77Return const> SetObjectsVisible(glm::uvec2 const& objects, bool const& visible) = 0;

    // A scene's BLASes gone for good -- waits for the device. Release before unloading the scene.
    virtual std::shared_ptr<IIr77Return const> ReleaseObjects(glm::uvec2 const& objects) = 0;

    // ---- surfaces -- world-space geometry rebuilt every frame (a field) ----------------------------------------------------------------------

    // One opaque BLAS over vertex (positions = first 3 floats per vertex_stride bytes) and index (uint32 triangles), world space, rebuilt by
    // every RecordBuild from what the buffers hold then. Both need the build-input usage. surface = its handle.
    virtual std::shared_ptr<IIr77Return const> CreateSurface(std::shared_ptr<IIr77PVBuffer> const& vertex, std::uint32_t const& vertex_stride,
                                                             std::uint32_t const& vertex_count, std::shared_ptr<IIr77PVBuffer> const& index,
                                                             std::uint32_t const& triangle_count, std::uint32_t& surface) = 0;

    // In the TLAS (and rebuilt) or not, from the next frame
    virtual std::shared_ptr<IIr77Return const> SetSurfaceVisible(std::uint32_t const& surface, bool const& visible) = 0;

    // Gone for good -- waits for the device. Release before destroying its buffers.
    virtual std::shared_ptr<IIr77Return const> ReleaseSurface(std::uint32_t const& surface) = 0;

    // ---- per frame ---------------------------------------------------------------------------------------------------------------------------

    // models[] -- the instance table's matrices; a TLAS instance takes its transform from its slot. Kept until replaced.
    virtual std::shared_ptr<IIr77Return const> SetInstanceTransforms(std::vector<glm::mat4> const& transforms) = 0;

    // Once a frame, outside any render pass, after the Compute pass: refit the dynamic BLASes, write the next ring entry's instances, rebuild
    // its TLAS and make it visible to ray queries. Nothing to do -> nothing recorded.
    virtual std::shared_ptr<IIr77Return const> RecordBuild(VkCommandBuffer const& cmd) = 0;

    // Binding-3 slot of the TLAS the last RecordBuild built -- BINDLESS_NO_SLOT before the first build or with nothing to trace
    virtual std::shared_ptr<IIr77Return const> GetTopLevelSlot(std::uint32_t& slot) = 0;

    // Binding-1 slot of the object table -- one Ir77PVRayObject per BLAS, row = a hit's custom index. BINDLESS_NO_SLOT before the first BLAS.
    virtual std::shared_ptr<IIr77Return const> GetObjectSlot(std::uint32_t& slot) = 0;

    virtual ~IIr77PVAcceleration() = default;
}* pIIr77PVAcceleration;
}  // namespace NSIr77PeregrineV
