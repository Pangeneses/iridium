#pragma once

#include <vulkan/vulkan.h>

#include <memory>
#include <string>
#include <vector>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "IIr77PVDevice.hpp"
#include "IIr77PVRenderPass.hpp"
#include "IIr77PVLayout.hpp"
#include "IIr77PVShader.hpp"
#include "IIr77PVPipeline.hpp"
#include "IIr77PVPipelineCPT.hpp"

#include "../types/Ir77PVTypes.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

// Every pipeline of one device, graphics and compute, keyed by Ir77PVPipelineDesc -- built on first request, shared by every window. Owns the device's
// VkPipelineCache, loaded from disk at create and written back by SaveCache / on destruction, so driver compiles survive restarts.
typedef struct IIr77PVPipelineCache : virtual public IIr77Enlisted {
    IIr77PVPipelineCache() = default;

    // ---- configuration -----------------------------------------------------------------------------------------------------------------------
    virtual std::shared_ptr<IIr77Return const> SetDevice(std::shared_ptr<IIr77PVDevice> device) = 0;

    virtual std::shared_ptr<IIr77Return const> SetLayout(std::shared_ptr<IIr77PVLayout> layout) = 0;

    virtual std::shared_ptr<IIr77Return const> SetShader(std::shared_ptr<IIr77PVShader> shader_stack) = 0;

    // one per pass kind a description can name
    virtual std::shared_ptr<IIr77Return const> SetRenderPass(Ir77PVRenderPassKind const& kind, std::shared_ptr<IIr77PVRenderPass> render_pass) = 0;

    // Optional -- default $XDG_CACHE_HOME (or ~/.cache) / iridium / pipelines-<vendor>-<device>.bin
    virtual std::shared_ptr<IIr77Return const> SetCachePath(std::string const& path) = 0;

    // ---- create ------------------------------------------------------------------------------------------------------------------------------
    // The VkPipelineCache, seeded from the file when it was written by this driver on this GPU
    virtual std::shared_ptr<IIr77Return const> CreatePipelineCache() = 0;

    // ---- pipelines -- by handle only ---------------------------------------------------------------------------------------------------------
    // A manifest pipeline's handle -- one per name for the cache's life; resolves whenever the manifest defines the name
    virtual std::shared_ptr<IIr77Return const> FindPipeline(std::string const& name, Ir77PVPipelineHandle& handle) = 0;

    // A role's handle -- follows the manifest's "roles" across reloads
    virtual std::shared_ptr<IIr77Return const> FindPipeline(Ir77PVPipelineKind const& kind, Ir77PVPipelineHandle& handle) = 0;

    // The pipeline a handle resolves to under the current manifest -- built now on its first use. Fails quietly after the first report until
    // the next manifest load.
    virtual std::shared_ptr<IIr77Return const> GetPipeline(Ir77PVPipelineHandle const& handle, std::shared_ptr<IIr77PVPipeline>& pipeline) = 0;

    // Per dispatch -- the compute pipeline a handle resolves to (a manifest entry with "compute")
    virtual std::shared_ptr<IIr77Return const> GetComputePipeline(Ir77PVPipelineHandle const& handle, std::shared_ptr<IIr77PVPipelineCPT>& pipeline) = 0;

    // The pipeline or role name a handle was issued for
    virtual std::shared_ptr<IIr77Return const> GetPipelineName(Ir77PVPipelineHandle const& handle, std::string& name) = 0;

    // ---- queries -- read only ----------------------------------------------------------------------------------------------------------------
    // A manifest pipeline's settings, stages resolved. Fails with the reason when a shader it names isn't registered.
    virtual std::shared_ptr<IIr77Return const> DescribeNamed(std::string const& name, Ir77PVPipelineDesc& desc) = 0;

    // The names a pipeline gives pc.user[4..7], in slot order
    virtual std::shared_ptr<IIr77Return const> GetUserParameters(std::string const& name, std::vector<std::string>& parameters) = 0;

    // ---- manifest ----------------------------------------------------------------------------------------------------------------------------
    // Reads pipelines.json -- named pipelines and the role -> pipeline map. Shaders must be registered first. A bad file fails and leaves the
    // previous manifest in force; static and cef roles are required. Every handle re-resolves against the new manifest.
    virtual std::shared_ptr<IIr77Return const> LoadManifest(std::string const& path) = 0;

    // ---- persistence -------------------------------------------------------------------------------------------------------------------------
    // Writes the driver's cache data to the cache path (also done on destruction)
    virtual std::shared_ptr<IIr77Return const> SaveCache() = 0;

    virtual ~IIr77PVPipelineCache() = default;
}* pIIr77PVPipelineCache;
}  // namespace NSIr77PeregrineV
