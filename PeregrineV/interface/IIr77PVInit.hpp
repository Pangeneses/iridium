#pragma once

#include <vulkan/vulkan.h>
#include <SDL3/SDL_video.h>

#include <array>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../types/Ir77PVTypes.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

struct IIr77PVLayout;
struct IIr77PVDescriptorSet;
struct IIr77PVPipeline;
struct IIr77PVBuffer;
struct IIr77PVShader;
struct IIr77PVCEF;
struct IIr77PVPaint;
struct IIr77PVTransfer;
struct IIr77PVMaterial;
struct IIr77PVGeometry;
struct IIr77PVAcceleration;


// The renderer's lifetime controller: device, swapchains, bindless layout + table, frame buffers, shader registry, pipeline cache -- and CEF,
// Paint, Transfer, Materials and Geometry, which it creates, wires with their Set* calls as each piece exists, and destroys. They never see Init.
//   SetWindows -> CreateRenderer -> [content] -> CreateCommandBuffers ... DestroyRenderer (before SDL destroys the windows)
//   per frame: Tick(frame) -> [Paint records]
typedef struct IIr77PVInit : virtual public IIr77Enlisted {
    IIr77PVInit() = default;

    virtual std::shared_ptr<IIr77Return const> SetCurrentDevice(std::uint64_t const& device_id) = 0;

    // ---- lifetime ----------------------------------------------------------------------------------------------------------------------------

    // The windows the display device must present to -- pointers, so a direct link, not an arg
    virtual std::shared_ptr<IIr77Return const> SetWindows(std::vector<SDL_Window*> const& windows) = 0;

    // Instance through pipelines, in order. device_id names the display device -- the key for the caller's window map.
    virtual std::shared_ptr<IIr77Return const> CreateRenderer(std::string const& shader_root, std::uint64_t& device_id) = 0;

    // After content is loaded -- the command buffers wire the overlay and the pipelines
    virtual std::shared_ptr<IIr77Return const> CreateCommandBuffers() = 0;

    // Reverse of creation. Anything else still holding a peer (Scene, through the server) must have let go first, or it outlives the allocator.
    virtual std::shared_ptr<IIr77Return const> DestroyRenderer() = 0;

    virtual std::shared_ptr<IIr77Return const> GetCEF(std::shared_ptr<IIr77PVCEF>& cef) = 0;

    virtual std::shared_ptr<IIr77Return const> GetPaint(std::shared_ptr<IIr77PVPaint>& paint) = 0;

    virtual std::shared_ptr<IIr77Return const> GetTransfer(std::shared_ptr<IIr77PVTransfer>& transfer) = 0;

    virtual std::shared_ptr<IIr77Return const> GetMaterials(std::shared_ptr<IIr77PVMaterial>& materials) = 0;

    virtual std::shared_ptr<IIr77Return const> GetGeometry(std::shared_ptr<IIr77PVGeometry>& geometry) = 0;

    virtual std::shared_ptr<IIr77Return const> GetAcceleration(std::shared_ptr<IIr77PVAcceleration>& acceleration) = 0;

    virtual std::shared_ptr<IIr77Return const> GetRayQuery(bool& enabled) = 0;

    // The display device's shader registry -- CompileShader, LoadShader / AddShader, queries
    virtual std::shared_ptr<IIr77Return const> GetShader(std::shared_ptr<IIr77PVShader>& shader) = 0;

    // ---- per frame / runtime -----------------------------------------------------------------------------------------------------------------

    // Once per frame, before Paint records, with the frame-in-flight index Paint is about to record. Transfer first (slot aging, finished
    // staging, the graveyard), then Geometry (this frame's copy of every dirty indirect buffer).
    virtual std::shared_ptr<IIr77Return const> Tick(std::uint32_t const& frame) = 0;

    // Puts SPIR-V into the registry under name -- replacing the shader of that name (same handle) or adding it -- then rebuilds every pipeline
    // that used it and reloads the manifest, so pipelines naming a shader that wasn't registered before resolve now. Waits for the device to
    // go idle first: the old pipelines may still be recording. A shader that breaks the contract fails and changes nothing.
    virtual std::shared_ptr<IIr77Return const> ReloadShader(std::string const& name, std::vector<std::uint32_t> const& spirv) = 0;

    // Every engine role -- CreateRenderer warms the cache with them. A role the manifest leaves out, or points at an unavailable pipeline,
    // is skipped there (static and cef can't be: the manifest fails to load without them).
    virtual std::shared_ptr<IIr77Return const> GetPipelineKinds(std::vector<Ir77PVPipelineKind>& kinds) = 0;

    // Name of the GPU this renderer runs on -- for stats / logs
    virtual std::shared_ptr<IIr77Return const> GetDeviceName(std::string& name) = 0;

    // ---- creation steps -- CreateRenderer runs these in order ----------------------------------------------------------------------------

    virtual std::shared_ptr<IIr77Return const> CreateInstance() = 0;

    virtual std::shared_ptr<IIr77Return const> EnumeratePhysicalDevices() = 0;

    // Picks and creates the device that drives the display -- discrete > integrated > virtual > other > CPU
    virtual std::shared_ptr<IIr77Return const> CreateDisplayDevice(std::vector<SDL_Window*> const& windows, std::uint64_t& device_id) = 0;

    virtual std::shared_ptr<IIr77Return const> CreateSurfaces(std::map<std::uint64_t, std::vector<SDL_Window*>> const& windows) = 0;

    virtual std::shared_ptr<IIr77Return const> EnumerateDeviceQueues() = 0;

    virtual std::shared_ptr<IIr77Return const> CreateLogicalDevices() = 0;

    virtual std::shared_ptr<IIr77Return const> CreateAllocator() = 0;

    // One bindless layout + table per device, shared by every window and pipeline
    virtual std::shared_ptr<IIr77Return const> CreateLayouts() = 0;

    // Main, Shadow, Offscreen, Post
    virtual std::shared_ptr<IIr77Return const> CreateRenderPasses() = 0;

    virtual std::shared_ptr<IIr77Return const> CreateSwapchains() = 0;

    // White + flat normal -- every image binding can be written before real textures exist
    virtual std::shared_ptr<IIr77Return const> CreatePlaceholderTexture() = 0;

    // One depth texture per device (the shadow atlas). After CreateRenderPasses, before CreateDescriptorSets.
    virtual std::shared_ptr<IIr77Return const> CreateShadowMap(std::uint32_t const& size = 2048) = 0;

    // One set of frame buffers per window, one copy per frame in flight
    virtual std::shared_ptr<IIr77Return const> CreateFrameBuffers(Ir77PVFrameSizes const& sizes = Ir77PVFrameSizes{}) = 0;

    // Placeholders, material table, frame buffers + frame tables -> bindless slots
    virtual std::shared_ptr<IIr77Return const> CreateDescriptorSets() = 0;

    // The device's pipeline cache, warmed with these roles. Needs the shader registry (CreateRenderer loads it first).
    virtual std::shared_ptr<IIr77Return const> CreatePipelines(std::vector<Ir77PVPipelineKind> const& kinds) = 0;

    // ---- lookups -------------------------------------------------------------------------------------------------

    virtual std::shared_ptr<IIr77Return const> GetLayout(std::shared_ptr<IIr77PVLayout>& layout) = 0;

    virtual std::shared_ptr<IIr77Return const> GetDescriptorSet(std::shared_ptr<IIr77PVDescriptorSet>& table) = 0;

    // Frame slots of one window, one entry per frame in flight -- for its command buffer
    virtual std::shared_ptr<IIr77Return const> GetFrameSlots(std::size_t const& window, std::array<Ir77PVFrameSlots, MAX_FRAMES_IN_FLIGHT>& slots) = 0;

    virtual std::shared_ptr<IIr77Return const> GetMaterialTableSlot(std::uint32_t& slot) = 0;

    virtual std::shared_ptr<IIr77Return const> GetPlaceholderSlot(Ir77PVPlaceholderKind const& kind, std::uint32_t& slot) = 0;

    // A manifest pipeline's handle -- what draw items carry
    virtual std::shared_ptr<IIr77Return const> FindPipeline(std::string const& name, Ir77PVPipelineHandle& handle) = 0;

    // An engine role's handle -- follows the manifest's "roles"
    virtual std::shared_ptr<IIr77Return const> FindPipeline(Ir77PVPipelineKind const& kind, Ir77PVPipelineHandle& handle) = 0;

    // The pipeline a handle resolves to under the current manifest
    virtual std::shared_ptr<IIr77Return const> GetPipeline(Ir77PVPipelineHandle const& handle, std::shared_ptr<IIr77PVPipeline>& pipeline) = 0;

    // Before CreatePipelines: overrides <shader root>/pipelines.json. After: reloads now -- a bad file fails and the old manifest stays.
    virtual std::shared_ptr<IIr77Return const> SetPipelineManifest(std::string const& path) = 0;

    virtual std::shared_ptr<IIr77Return const> GetFrameBuffer(std::size_t const& window, Ir77PVBufferSlot const& slot,
                                                              std::shared_ptr<IIr77PVBuffer>& buffer) = 0;

    virtual VkDescriptorImageInfo GetPlaceholderImageInfo(Ir77PVPlaceholderKind const& kind = Ir77PVPlaceholderKind::White) const = 0;

    virtual ~IIr77PVInit() = default;
}* pIIr77PVInit;
}  // namespace NSIr77PeregrineV
