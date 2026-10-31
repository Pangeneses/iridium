#pragma once

#include <vulkan/vulkan.h>
#include <SDL3/SDL_video.h>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../types/Ir77PVTypes.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

struct IIr77PVShader;
struct IIr77PVCEF;
struct IIr77PVPaint;
struct IIr77PVTransfer;
struct IIr77PVMaterial;
struct IIr77PVGeometry;
struct IIr77PVAcceleration;
struct IIr77PVWorld;

// The renderer's lifetime controller: device, swapchains, bindless layout + table, frame buffers, shader registry, pipeline cache -- and CEF,
// Paint, Transfer, Materials, Geometry, Acceleration and World, which it creates, wires with their Set* calls as each piece exists, and
// destroys. They never see Init; only the server holds it.
//   SetWindows -> CreateRenderer -> [content] -> CreateCommandBuffers ... DestroyRenderer (before SDL destroys the windows)
//   per frame: Tick(frame) -> [Paint records]
typedef struct IIr77PVInit : virtual public IIr77Enlisted {
    IIr77PVInit() = default;

    // ---- lifetime ----------------------------------------------------------------------------------------------------------------------------

    // The windows the display device must present to -- pointers, so a direct link, not an arg
    virtual std::shared_ptr<IIr77Return const> SetWindows(std::vector<SDL_Window*> const& windows) = 0;

    // Instance through pipelines, then the peers. device_id names the display device -- the key for the caller's window map.
    virtual std::shared_ptr<IIr77Return const> CreateRenderer(std::string const& shader_root, std::uint64_t& device_id) = 0;

    // After content is loaded -- the command buffers wire the overlay and the pipelines
    virtual std::shared_ptr<IIr77Return const> CreateCommandBuffers() = 0;

    // Reverse of creation. Anything else still holding a peer (Scene, through the server) must have let go first, or it outlives the allocator.
    virtual std::shared_ptr<IIr77Return const> DestroyRenderer() = 0;

    // ---- the peers ---------------------------------------------------------------------------------------------------------------------------

    virtual std::shared_ptr<IIr77Return const> GetCEF(std::shared_ptr<IIr77PVCEF>& cef) = 0;

    virtual std::shared_ptr<IIr77Return const> GetPaint(std::shared_ptr<IIr77PVPaint>& paint) = 0;

    virtual std::shared_ptr<IIr77Return const> GetTransfer(std::shared_ptr<IIr77PVTransfer>& transfer) = 0;

    virtual std::shared_ptr<IIr77Return const> GetMaterials(std::shared_ptr<IIr77PVMaterial>& materials) = 0;

    virtual std::shared_ptr<IIr77Return const> GetGeometry(std::shared_ptr<IIr77PVGeometry>& geometry) = 0;

    // NotConfigured on a device without ray queries
    virtual std::shared_ptr<IIr77Return const> GetAcceleration(std::shared_ptr<IIr77PVAcceleration>& acceleration) = 0;

    // Assets and frames -- LoadAsset / SubmitFrame
    virtual std::shared_ptr<IIr77Return const> GetWorld(std::shared_ptr<IIr77PVWorld>& world) = 0;

    // The display device's shader registry -- CompileShader
    virtual std::shared_ptr<IIr77Return const> GetShader(std::shared_ptr<IIr77PVShader>& shader) = 0;

    // ---- the device --------------------------------------------------------------------------------------------------------------------------

    // The display device has ray queries -- and so an Acceleration
    virtual std::shared_ptr<IIr77Return const> ImplementsRayQuery(bool& enabled) = 0;

    // The display device has mesh shading
    virtual std::shared_ptr<IIr77Return const> GetMeshShading(bool& enabled) = 0;

    // Name of the GPU this renderer runs on -- for stats / logs
    virtual std::shared_ptr<IIr77Return const> GetDeviceName(std::string& name) = 0;

    // ---- per frame / runtime -----------------------------------------------------------------------------------------------------------------

    // Once per frame, before Paint records, with the frame-in-flight index Paint is about to record. Transfer first (slot aging, finished
    // staging, the graveyard), then Geometry (this frame's copy of every dirty indirect buffer).
    virtual std::shared_ptr<IIr77Return const> Tick(std::uint32_t const& frame) = 0;

    // Puts SPIR-V into the registry under name -- replacing the shader of that name (same handle) or adding it -- then rebuilds every pipeline
    // that used it and reloads the manifest, so pipelines naming a shader that wasn't registered before resolve now. Waits for the device to
    // go idle first: the old pipelines may still be recording. A shader that breaks the contract fails and changes nothing.
    virtual std::shared_ptr<IIr77Return const> ReloadShader(std::string const& name, std::vector<std::uint32_t> const& spirv) = 0;

    // ---- pipelines ---------------------------------------------------------------------------------------------------------------------------

    // A manifest pipeline's handle -- what draw items carry
    virtual std::shared_ptr<IIr77Return const> FindPipeline(std::string const& name, Ir77PVPipelineHandle& handle) = 0;

    // An engine role's handle -- follows the manifest's "roles"
    virtual std::shared_ptr<IIr77Return const> FindPipeline(Ir77PVPipelineKind const& kind, Ir77PVPipelineHandle& handle) = 0;

    // Before CreateRenderer: overrides <shader root>/pipelines.json. After: reloads now -- a bad file fails and the old manifest stays.
    virtual std::shared_ptr<IIr77Return const> SetPipelineManifest(std::string const& path) = 0;

    virtual ~IIr77PVInit() = default;
}* pIIr77PVInit;
}  // namespace NSIr77PeregrineV