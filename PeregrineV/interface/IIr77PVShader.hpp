#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "IIr77PVDevice.hpp"

#include "../types/Ir77PVTypes.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

// The device's shader registry -- every shader module by name. A handle is an index into the registry, stable for its life, so pipeline
// descriptions can hold handles instead of compiled-in IDs. Each shader's stage and interface come from its own SPIR-V, and a shader that
// breaks the shader contract (Ir77PVTypes.hpp) is rejected when it is registered.
//   Ir77PVInit: SetDevice / SetShaderRoot -> LoadShaders -> [pipeline cache]. CompileShader + Ir77PVInit::ReloadShader for hot compiles.
typedef struct IIr77PVShader : virtual public IIr77Enlisted {
    IIr77PVShader() = default;

    virtual std::shared_ptr<IIr77Return const> SetDevice(std::shared_ptr<IIr77PVDevice> device) = 0;

    // Where LoadShaders reads .spv files and CompileShader writes generated sources
    virtual std::shared_ptr<IIr77Return const> SetShaderRoot(std::string const& root) = 0;

    // ---- the shader root ---------------------------------------------------------------------------------------------------------------------
    // Registers every .spv under the shader root, each under its file name without ".spv" ("vert", "skinned.vert", "heightmap.frag"); the
    // stage comes from the SPIR-V. A file that breaks the shader contract (Ir77PVTypes.hpp) -- or a task / mesh shader on a device without mesh
    // shading -- is skipped with a warning. Which shaders combine into which pipelines is the manifest's job (Ir77PVInit picks it: an explicit
    // SetPipelineManifest, else <shader root>/pipelines.json). More shaders can be registered at any time through the registry's
    // LoadShader / AddShader. Names already registered are skipped, so a second call only picks up new files.
    virtual std::shared_ptr<IIr77Return const> LoadShaders() = 0;

    // GLSL text -> SPIR-V, through the shader compiler (glslangValidator, on PATH). The text is written to <shader root>/generated/<name> and
    // compiled with the shader root on the include path, so it can #include "include/bindless.glsl". name is the registry name to be, so it
    // ends in its stage ("field.comp"). errors = the compiler's ERROR / WARNING lines, with the file path cut down to the name.
    // Blocking (a compiler process) -- for hot compiles between frames, not every frame.
    virtual std::shared_ptr<IIr77Return const> CompileShader(std::string const& name, std::string const& text, std::vector<std::uint32_t>& spirv, std::string& errors) = 0;

    // ---- register ----------------------------------------------------------------------------------------------------------------------------
    // A SPIR-V file under a name. Fails if the name is already registered or the shader breaks the contract.
    virtual std::shared_ptr<IIr77Return const> LoadShader(std::string const& name, std::string const& filename, Ir77PVShaderHandle& handle) = 0;

    // SPIR-V already in memory (runtime compile, project files) under a name. Fails if the name is already registered or the shader breaks
    // the contract.
    virtual std::shared_ptr<IIr77Return const> AddShader(std::string const& name, std::vector<std::uint32_t> const& spirv, Ir77PVShaderHandle& handle) = 0;

    // New SPIR-V behind an existing handle (hot compile) -- the name and handle stay. Device idle; rebuild its pipelines afterwards
    // (IIr77PVPipelineCache::InvalidateShader). Fails, changing nothing, when the new SPIR-V breaks the contract.
    virtual std::shared_ptr<IIr77Return const> ReplaceShader(Ir77PVShaderHandle const& handle, std::vector<std::uint32_t> const& spirv) = 0;

    // ---- queries -----------------------------------------------------------------------------------------------------------------------------
    // IR77PV_NO_SHADER and a failure when the name isn't registered
    virtual std::shared_ptr<IIr77Return const> FindShader(std::string const& name, Ir77PVShaderHandle& handle) = 0;

    virtual std::shared_ptr<IIr77Return const> GetShaderInfo(Ir77PVShaderHandle const& handle, Ir77PVShaderInfo& info) = 0;

    // Stage and reflection only -- no SPIR-V copy
    virtual std::shared_ptr<IIr77Return const> GetShaderReflection(Ir77PVShaderHandle const& handle, Ir77PVShaderStage& stage,
                                                                   Ir77PVShaderReflection& reflection) = 0;

    // Stage create infos for a pipeline, in the order given. Fails on any unknown handle.
    virtual std::shared_ptr<IIr77Return const> GetPipelineShaderStageInfos(std::vector<VkPipelineShaderStageCreateInfo>& infos,
                                                                           std::vector<Ir77PVShaderHandle> const& handles) = 0;

    virtual ~IIr77PVShader() = default;
}* pIIr77PVShader;
}  // namespace NSIr77PeregrineV
