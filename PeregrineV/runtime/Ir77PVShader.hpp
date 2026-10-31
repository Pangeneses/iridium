#pragma once

#include <vulkan/vulkan.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <sys/wait.h>
#include <vector>

#include "../../Ir77RT/dictionary/IDIIr77MPVM.hpp"

#include "../dictionary/IDIIr77PeregrineV.hpp"
#include "../dictionary/IDIr77PeregrineV.hpp"

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../../Ir77RT/runtime/Ir77GUID.hpp"
#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"
#include "../../Ir77RT/runtime/Ir77Return.hpp"

#include "../interface/IIr77PVShader.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

// =============================================================================================================================================
// Ir77PVShader -- the device's shader registry. Every module is registered under a name and gets a handle (its index); pipelines link
// stages by handle, presets find them by name.
//
//   register:  LoadShader (file) / AddShader (SPIR-V in memory) -> DefineReflection -> DefineContract -> DefineModule
//   replace:   ReplaceShader (new SPIR-V behind an existing name and handle -- hot compiles) -> the same three steps, then the old module goes
//
// DefineReflection reads the SPIR-V itself (no reflection library): the "main" entry point's execution model gives the stage; Input /
// Output locations, the push-constant block size and every descriptor's set / binding / kind come from the decorated variables.
// DefineContract rejects a shader that breaks the contract (Ir77PVTypes.hpp) with a message naming the shader and the rule.
//   query:     FindShader (name -> handle) / GetShaderInfo / GetPipelineShaderStageInfos (handles -> stage create infos)
//   startup:   Ir77PVInit -- SetDevice / SetShaderRoot -> LoadShaders (every <name>.<stage>.spv in the root)
//   compile:   CompileShader (GLSL text -> SPIR-V through glslangValidator) -> Ir77PVInit::ReloadShader puts it in and rebuilds pipelines.
//              The device's defines go in here (DeviceDefines) -- a caller sends what its shader says, never what the GPU can do.
//
// Modules live until the registry is destroyed or replaced -- pipelines are built from them on demand, at any time.
// =============================================================================================================================================
class Ir77PVShader : public Ir77Enlisted, public IIr77PVShader, public std::enable_shared_from_this<Ir77PVShader> {
   public:
    Ir77PVShader() {
        try {
            m_enlisted_uuid.Generate();
        } catch (std::invalid_argument a) {
            throw a;
        }

        m_enlisted = std::chrono::system_clock::now();
    }

    // the owner waits for the device to go idle first
    ~Ir77PVShader() {
        if (!m_device) return;

        VkDevice device{VK_NULL_HANDLE};
        m_device->GetDevice(&device);

        for (auto const& info : m_shaders)
            if (info.stage_create_info.module != VK_NULL_HANDLE) vkDestroyShaderModule(device, info.stage_create_info.module, nullptr);
    }

   public:
    std::shared_ptr<IIr77Return const> EnlistedAs(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIIr77Enlisted>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> MemberUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77PVShader>(uid);

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

        else if (iid == GUIDIIr77PVShader)
            obj = std::shared_ptr<IIr77PVShader>(shared_from_this(), static_cast<IIr77PVShader*>(this));

        else if (iid == GUIDIr77PVShader)
            obj = std::shared_ptr<Ir77PVShader>(shared_from_this(), static_cast<Ir77PVShader*>(this));

        else
            return GUIDQueryFailed;

        return GUIDQuerySucceeded;
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // configuration
    // -------------------------------------------------------------------------------------------------------------------------------------
   public:
    std::shared_ptr<IIr77Return const> SetDevice(std::shared_ptr<IIr77PVDevice> device) {
        m_device = device;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Where LoadShaders reads .spv files and CompileShader writes generated sources
    std::shared_ptr<IIr77Return const> SetShaderRoot(std::string const& root) {
        m_shader_root = root;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // the shader root
    // -------------------------------------------------------------------------------------------------------------------------------------
    // Registers every .spv under the shader root, each under its file name without ".spv" ("vert", "skinned.vert", "heightmap.frag"); the
    // stage comes from the SPIR-V. A file that breaks the shader contract (Ir77PVTypes.hpp) -- or a task / mesh shader on a device without mesh
    // shading -- is skipped with a warning. Which shaders combine into which pipelines is the manifest's job (Ir77PVInit picks it: an explicit
    // SetPipelineManifest, else <shader root>/pipelines.json). More shaders can be registered at any time through the registry's
    // LoadShader / AddShader. Names already registered are skipped, so a second call only picks up new files.
    std::shared_ptr<IIr77Return const> LoadShaders() {
        if (!m_device) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVShader: device not set.");

        std::error_code error{};
        if (!std::filesystem::is_directory(m_shader_root, error))
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVShader: shader root not found: " + m_shader_root);

        // only <name>.<stage>.spv -- what compile_shaders.sh writes. A bare <name>.spv is a leftover from the old naming and would shadow nothing
        // but still register, so a manifest naming it would silently get stale code; it's skipped with a warning instead.
        static std::array<std::string_view, 8> const stages{".vert", ".frag", ".comp", ".task", ".mesh", ".geom", ".tesc", ".tese"};

        for (auto const& entry : std::filesystem::directory_iterator(m_shader_root, error)) {
            if (!entry.is_regular_file() || entry.path().extension() != ".spv") continue;

            std::string const name{entry.path().stem().string()};  // "skinned.vert.spv" -> "skinned.vert"
            std::string const stage{std::filesystem::path(name).extension().string()};

            if (std::find(stages.begin(), stages.end(), stage) == stages.end()) {
                Ir77RETURN<Ir77OperationFailed>(nullptr, std::string{"[WARN] shader skipped: "} + entry.path().filename().string() +
                                                             " -- name it <name>.<stage>.spv (e.g. " + name + ".frag.spv)\n");
                continue;
            }

            Ir77PVShaderHandle handle{IR77PV_NO_SHADER};
            if (m_names.count(name) != 0) continue;  // a second LoadShaders, or registered by hand first

            if (LoadShader(name, entry.path().string(), handle)->ID() != GUIDIr77OperationSucceeded)
                Ir77RETURN<Ir77NotConfigured>(this, "[WARN] shader skipped: " + entry.path().string());
        }

        if (error) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVShader: cannot read the shader root: " + m_shader_root);

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: Load Shaders.");
    }

    // GLSL text -> SPIR-V, through the shader compiler (glslangValidator, on PATH). The text is written to <shader root>/generated/<name> and
    // compiled with the shader root on the include path, so it can #include "include/bindless.glsl", and with the device's defines
    // (DeviceDefines). name is the registry name to be, so it ends in its stage ("field.comp"). errors = the compiler's ERROR / WARNING lines,
    // with the file path cut down to the name.
    // Blocking (a compiler process) -- for hot compiles between frames, not every frame.
    std::shared_ptr<IIr77Return const> CompileShader(std::string const& name, std::string const& text, std::vector<std::uint32_t>& spirv, std::string& errors) {
        spirv.clear();
        errors.clear();

        std::error_code error{};
        std::filesystem::path const folder = std::filesystem::path(m_shader_root) / "generated";
        std::filesystem::create_directories(folder, error);
        if (error) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVShader: cannot create " + folder.string());

        std::filesystem::path const source = folder / name;
        std::filesystem::path const output = folder / (name + ".spv");

        {
            std::ofstream file{source, std::ios::trunc};
            if (!file) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVShader: cannot write " + source.string());
            file << text;
        }

        std::filesystem::remove(output, error);

        std::string const command = m_shader_compiler + " -V --target-env vulkan1.3" + DeviceDefines() + " -I\"" + m_shader_root + "\" \"" + source.string() +
                                    "\" -o \"" + output.string() + "\" 2>&1";

        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVShader: cannot run " + m_shader_compiler);

        std::string log{};
        char buffer[512]{};
        while (std::fgets(buffer, sizeof(buffer), pipe)) log += buffer;

        int const status = pclose(pipe);

        // keep what a programmer needs: the ERROR / WARNING lines, "<name>:<line>: ..." instead of the full path
        std::string const path = source.string();
        std::size_t at{0};

        while (at < log.size()) {
            std::size_t const end = std::min(log.find('\n', at), log.size());
            std::string line = log.substr(at, end - at);
            at = end + 1;

            if (line.rfind("ERROR:", 0) != 0 && line.rfind("WARNING:", 0) != 0) continue;
            if (line.find("compilation errors") != std::string::npos || line.find("Linking") != std::string::npos) continue;

            for (std::size_t found = line.find(path); found != std::string::npos; found = line.find(path)) line.replace(found, path.size(), name);

            errors += line + "\n";
        }

        if (status == -1 || !WIFEXITED(status) || WEXITSTATUS(status) != 0) {
            if (errors.empty()) errors = log.empty() ? m_shader_compiler + " failed to run\n" : log;
            return Ir77RETURN<Ir77OperationFailed>(this, "Ir77PVShader: " + name + " failed to compile.");
        }

        std::ifstream file{output, std::ios::ate | std::ios::binary};
        if (!file) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVShader: no SPIR-V from " + m_shader_compiler + " for " + name);

        std::streamsize const size{file.tellg()};
        if (size <= 0 || size % 4 != 0) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVShader: bad SPIR-V size for " + name);

        spirv.resize(static_cast<std::size_t>(size) / 4);
        file.seekg(0);
        if (!file.read(reinterpret_cast<char*>(spirv.data()), size)) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVShader: cannot read SPIR-V for " + name);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // register
    // -------------------------------------------------------------------------------------------------------------------------------------
    std::shared_ptr<IIr77Return const> LoadShader(std::string const& name, std::string const& filename, Ir77PVShaderHandle& handle) {
        handle = IR77PV_NO_SHADER;

        std::ifstream file{filename, std::ios::ate | std::ios::binary};
        if (!file.is_open()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVShader: cannot open " + filename);

        std::streamsize const size{file.tellg()};
        if (size <= 0 || size % 4 != 0) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVShader: not a SPIR-V file (size) " + filename);

        std::vector<std::uint32_t> spirv(static_cast<std::size_t>(size) / 4);

        file.seekg(0);
        if (!file.read(reinterpret_cast<char*>(spirv.data()), size)) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVShader: read failed " + filename);

        return AddShader(name, spirv, handle);
    }

    std::shared_ptr<IIr77Return const> AddShader(std::string const& name, std::vector<std::uint32_t> const& spirv, Ir77PVShaderHandle& handle) {
        handle = IR77PV_NO_SHADER;

        if (!m_device) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVShader: SetDevice not called.");
        if (name.empty()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVShader: a shader needs a name.");
        if (m_names.count(name) != 0) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVShader: '" + name + "' is already registered.");
        if (spirv.size() < SPIRV_HEADER_WORDS || spirv[0] != SPIRV_MAGIC)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVShader: '" + name + "' is not SPIR-V.");

        Ir77PVShaderInfo info{};
        info.name = name;
        info.spirv = spirv;

        auto result = DefineReflection(info);
        if (result->ID() == GUIDIr77OperationSucceeded) result = DefineContract(info);
        if (result->ID() == GUIDIr77OperationSucceeded) result = DefineModule(info);
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        handle = static_cast<Ir77PVShaderHandle>(m_shaders.size());

        m_shaders.push_back(std::move(info));
        m_names.emplace(name, handle);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // New SPIR-V behind an existing name -- the handle stays, so every description holding it gets the new code once its pipeline is rebuilt
    // (Ir77PVPipelineCache::InvalidateShader). The device must be idle: the old module is destroyed here. Fails, changing nothing, when the new
    // SPIR-V breaks the contract.
    std::shared_ptr<IIr77Return const> ReplaceShader(Ir77PVShaderHandle const& handle, std::vector<std::uint32_t> const& spirv) {
        if (!m_device) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVShader: SetDevice not called.");
        if (handle >= m_shaders.size()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVShader: unknown shader handle.");

        Ir77PVShaderInfo info{};
        info.name = m_shaders[handle].name;
        info.spirv = spirv;

        if (spirv.size() < SPIRV_HEADER_WORDS || spirv[0] != SPIRV_MAGIC)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVShader: '" + info.name + "' is not SPIR-V.");

        auto result = DefineReflection(info);
        if (result->ID() == GUIDIr77OperationSucceeded) result = DefineContract(info);
        if (result->ID() == GUIDIr77OperationSucceeded) result = DefineModule(info);
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        VkDevice device{VK_NULL_HANDLE};
        m_device->GetDevice(&device);

        if (m_shaders[handle].stage_create_info.module != VK_NULL_HANDLE) vkDestroyShaderModule(device, m_shaders[handle].stage_create_info.module, nullptr);

        m_shaders[handle] = std::move(info);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // queries
    // -------------------------------------------------------------------------------------------------------------------------------------
    std::shared_ptr<IIr77Return const> FindShader(std::string const& name, Ir77PVShaderHandle& handle) {
        auto const found = m_names.find(name);
        if (found == m_names.end()) {
            handle = IR77PV_NO_SHADER;
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVShader: '" + name + "' is not registered.");
        }

        handle = found->second;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetShaderInfo(Ir77PVShaderHandle const& handle, Ir77PVShaderInfo& info) {
        if (handle >= m_shaders.size()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVShader: unknown shader handle.");

        info = m_shaders[handle];

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetShaderReflection(Ir77PVShaderHandle const& handle, Ir77PVShaderStage& stage, Ir77PVShaderReflection& reflection) {
        if (handle >= m_shaders.size()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVShader: unknown shader handle.");

        stage = m_shaders[handle].stage;
        reflection = m_shaders[handle].reflection;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetPipelineShaderStageInfos(std::vector<VkPipelineShaderStageCreateInfo>& infos,
                                                                   std::vector<Ir77PVShaderHandle> const& handles) {
        infos.clear();
        infos.reserve(handles.size());

        for (auto const handle : handles) {
            if (handle >= m_shaders.size()) {
                infos.clear();
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVShader: unknown shader handle.");
            }

            infos.push_back(m_shaders[handle].stage_create_info);
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // steps
    // -------------------------------------------------------------------------------------------------------------------------------------
   private:
    enum class Ir77PVSpirvDescriptorKind : std::uint8_t { Texture, StorageBuffer, StorageImage, AccelerationStructure, Other };

    struct Ir77PVSpirvVariable {
        std::uint32_t id{0};
        std::uint32_t pointer_type{0};
        std::uint32_t storage{0};
    };

    struct Ir77PVSpirvDescriptor {
        Ir77PVSpirvDescriptorKind kind{Ir77PVSpirvDescriptorKind::Other};
        std::uint32_t set{UINT32_MAX};
        std::uint32_t binding{UINT32_MAX};
    };

    // the parts of a module reflection needs -- type instructions are kept whole (word 0 = count | opcode)
    struct Ir77PVSpirvModule {
        bool has_main{false};
        std::uint32_t execution_model{UINT32_MAX};
        std::map<std::uint32_t, std::vector<std::uint32_t>> types{};
        std::map<std::uint32_t, std::uint32_t> constants{};
        std::map<std::uint32_t, std::map<std::uint32_t, std::uint32_t>> decorations{};                                  // id -> decoration -> literal
        std::map<std::uint32_t, std::map<std::uint32_t, std::map<std::uint32_t, std::uint32_t>>> member_decorations{};  // struct -> member -> ...
        std::vector<Ir77PVSpirvVariable> variables{};
    };

    // What the SPIR-V says: stage (from the "main" entry point), interface locations, push-constant size, and the descriptors
    // (m_descriptors, checked by DefineContract)
    std::shared_ptr<IIr77Return const> DefineReflection(Ir77PVShaderInfo& info) {
        Ir77PVSpirvModule module{};

        auto const result = DefineSpirvModule(info, module);
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        if (!module.has_main) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVShader: '" + info.name + "' has no entry point named main.");

        if (!StageOf(module.execution_model, info.stage))
            return Ir77RETURN<Ir77NotConfigured>(
                this, "Ir77PVShader: '" + info.name + "' has an unsupported execution model (" + std::to_string(module.execution_model) + ").");

        info.reflection = Ir77PVShaderReflection{};
        m_descriptors.clear();

        for (auto const& variable : module.variables) {
            auto const decorations = module.decorations.find(variable.id);
            bool const builtin{decorations != module.decorations.end() && decorations->second.count(SPV_DECORATION_BUILTIN) != 0};

            switch (variable.storage) {
                case SPV_STORAGE_INPUT:
                case SPV_STORAGE_OUTPUT: {
                    if (builtin || decorations == module.decorations.end()) break;

                    auto const location = decorations->second.find(SPV_DECORATION_LOCATION);
                    if (location == decorations->second.end()) break;

                    auto& list = variable.storage == SPV_STORAGE_INPUT ? info.reflection.inputs : info.reflection.outputs;
                    list.push_back(location->second);
                    break;
                }

                case SPV_STORAGE_PUSH_CONSTANT:
                    info.reflection.push_constant_size = std::max(info.reflection.push_constant_size, SizeOf(module, Pointee(module, variable.pointer_type)));
                    break;

                case SPV_STORAGE_UNIFORM_CONSTANT:
                case SPV_STORAGE_UNIFORM:
                case SPV_STORAGE_STORAGE_BUFFER: {
                    Ir77PVSpirvDescriptor descriptor{};
                    descriptor.kind = DescriptorKind(module, variable);

                    if (decorations != module.decorations.end()) {
                        if (auto const set = decorations->second.find(SPV_DECORATION_DESCRIPTOR_SET); set != decorations->second.end())
                            descriptor.set = set->second;
                        if (auto const binding = decorations->second.find(SPV_DECORATION_BINDING); binding != decorations->second.end())
                            descriptor.binding = binding->second;
                    }

                    m_descriptors.push_back(descriptor);
                    break;
                }

                default:
                    break;
            }
        }

        std::sort(info.reflection.inputs.begin(), info.reflection.inputs.end());
        std::sort(info.reflection.outputs.begin(), info.reflection.outputs.end());

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // The shader contract (Ir77PVTypes.hpp) -- every failure names the shader and the rule it breaks
    std::shared_ptr<IIr77Return const> DefineContract(Ir77PVShaderInfo const& info) {
        std::string const shader{"Ir77PVShader: '" + info.name + "' "};

        for (auto const& descriptor : m_descriptors) {
            std::uint32_t expected{UINT32_MAX};

            switch (descriptor.kind) {
                case Ir77PVSpirvDescriptorKind::Texture:
                    expected = BINDLESS_BINDING_TEXTURES;
                    break;
                case Ir77PVSpirvDescriptorKind::StorageBuffer:
                    expected = BINDLESS_BINDING_BUFFERS;
                    break;
                case Ir77PVSpirvDescriptorKind::StorageImage:
                    expected = BINDLESS_BINDING_STORAGE_IMAGES;
                    break;
                case Ir77PVSpirvDescriptorKind::AccelerationStructure: {
                    bool ray_query{false};
                    m_device->ImplementsRayQuery(ray_query);

                    if (!ray_query)
                        return Ir77RETURN<Ir77NotConfigured>(this, shader + "traces rays (include/raytrace.glsl) and this device has no ray queries.");

                    expected = BINDLESS_BINDING_ACCELERATION;
                    break;
                }
                default:
                    return Ir77RETURN<Ir77NotConfigured>(
                        this, shader + "uses a descriptor type the bindless set doesn't have (set " + std::to_string(descriptor.set) + ", binding " +
                                  std::to_string(descriptor.binding) +
                                  ") -- use sampler2D (binding 0), storage buffers (1), storage images (2) or acceleration structures (3).");
            }

            if (descriptor.set != 0 || descriptor.binding != expected)
                return Ir77RETURN<Ir77NotConfigured>(
                    this, shader + "binds set " + std::to_string(descriptor.set) + ", binding " + std::to_string(descriptor.binding) +
                              " -- only the bindless set 0 exists (0 textures, 1 storage buffers, 2 storage images, 3 acceleration "
                              "structures); include bindless.glsl / raytrace.glsl.");
        }

        if (info.reflection.push_constant_size > PUSH_CONSTANT_LIMIT)
            return Ir77RETURN<Ir77NotConfigured>(this, shader + "has a " + std::to_string(info.reflection.push_constant_size) +
                                                           "-byte push-constant block -- the limit is " + std::to_string(PUSH_CONSTANT_LIMIT) +
                                                           " (Ir77PushConstants).");

        if (info.stage == Ir77PVShaderStage::Vertex)
            for (auto const location : info.reflection.inputs)
                if (location > VERTEX_LOCATION_TANGENT)
                    return Ir77RETURN<Ir77NotConfigured>(this, shader + "reads vertex location " + std::to_string(location) +
                                                                   " -- vertex inputs are 0 position, 1 normal, 2 uv, 3 joints, 4 weights, 5 tangent.");

        if (info.stage == Ir77PVShaderStage::Task || info.stage == Ir77PVShaderStage::Mesh) {
            bool mesh_shading{false};
            m_device->GetMeshShading(mesh_shading);

            if (!mesh_shading) return Ir77RETURN<Ir77NotConfigured>(this, shader + "is a task / mesh shader and this device has no mesh shading.");
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // One pass over the instruction stream: the main entry point, decorations, types, constants and module-scope variables
    std::shared_ptr<IIr77Return const> DefineSpirvModule(Ir77PVShaderInfo const& info, Ir77PVSpirvModule& module) const {
        std::vector<std::uint32_t> const& words = info.spirv;

        std::size_t at{SPIRV_HEADER_WORDS};

        while (at < words.size()) {
            std::uint32_t const count{words[at] >> 16};
            std::uint32_t const opcode{words[at] & 0xFFFFu};

            if (count == 0 || at + count > words.size())
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVShader: '" + info.name + "' has a malformed instruction stream.");

            std::uint32_t const* const op = &words[at];

            switch (opcode) {
                case SPV_OP_ENTRY_POINT:
                    // execution model, entry id, name (literal string)...
                    if (count >= 4 && LiteralString(op + 3, count - 3) == "main") {
                        module.has_main = true;
                        module.execution_model = op[1];
                    }
                    break;

                case SPV_OP_DECORATE:
                    if (count >= 3) module.decorations[op[1]][op[2]] = count >= 4 ? op[3] : 0;
                    break;

                case SPV_OP_MEMBER_DECORATE:
                    if (count >= 5) module.member_decorations[op[1]][op[2]][op[3]] = op[4];
                    break;

                case SPV_OP_CONSTANT:
                    if (count >= 4) module.constants[op[2]] = op[3];
                    break;

                case SPV_OP_VARIABLE:
                    if (count >= 4) module.variables.push_back(Ir77PVSpirvVariable{op[2], op[1], op[3]});
                    break;

                default:
                    if (((opcode >= SPV_OP_TYPE_FIRST && opcode <= SPV_OP_TYPE_LAST) || opcode == SPV_OP_TYPE_ACCELERATION_STRUCTURE) && count >= 2)
                        module.types[op[1]].assign(op, op + count);
                    break;
            }

            at += count;
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // The VkShaderModule and its stage create info (entry point "main")
    std::shared_ptr<IIr77Return const> DefineModule(Ir77PVShaderInfo& info) {
        VkDevice device{VK_NULL_HANDLE};
        m_device->GetDevice(&device);

        VkShaderModuleCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        create_info.codeSize = info.spirv.size() * sizeof(std::uint32_t);
        create_info.pCode = info.spirv.data();

        VkShaderModule module{VK_NULL_HANDLE};
        if (vkCreateShaderModule(device, &create_info, nullptr, &module) != VK_SUCCESS)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVShader: vkCreateShaderModule failed for '" + info.name + "'.");

        info.stage_create_info = VkPipelineShaderStageCreateInfo{};
        info.stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        info.stage_create_info.stage = ToVkStage(info.stage);
        info.stage_create_info.module = module;
        info.stage_create_info.pName = "main";

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // SPIR-V reading -- values only
    // -------------------------------------------------------------------------------------------------------------------------------------
   private:
    // What this device can do, as compiler defines for runtime-compiled shaders -- the same variants shader.sh builds ahead of time:
    //   IR77_RAY_SHADOWS  ray queries -- lighting.glsl traces its shadows and ambient occlusion against the TLAS
    std::string DeviceDefines() const {
        bool ray_query{false};
        if (m_device) m_device->ImplementsRayQuery(ray_query);

        return ray_query ? " -DIR77_RAY_SHADOWS" : "";
    }

    // "main\0" packed four chars per word, little-endian
    std::string LiteralString(std::uint32_t const* words, std::uint32_t const count) const {
        std::string text{};

        for (std::uint32_t w = 0; w < count; w++)
            for (std::uint32_t b = 0; b < 4; b++) {
                char const c{static_cast<char>((words[w] >> (8 * b)) & 0xFFu)};
                if (c == '\0') return text;
                text.push_back(c);
            }

        return text;
    }

    bool StageOf(std::uint32_t const model, Ir77PVShaderStage& stage) const {
        switch (model) {
            case 0:
                stage = Ir77PVShaderStage::Vertex;
                return true;
            case 1:
                stage = Ir77PVShaderStage::TessellationControl;
                return true;
            case 2:
                stage = Ir77PVShaderStage::TessellationEvaluation;
                return true;
            case 3:
                stage = Ir77PVShaderStage::Geometry;
                return true;
            case 4:
                stage = Ir77PVShaderStage::Fragment;
                return true;
            case 5:
                stage = Ir77PVShaderStage::Compute;
                return true;
            case 5364:
                stage = Ir77PVShaderStage::Task;
                return true;  // TaskEXT
            case 5365:
                stage = Ir77PVShaderStage::Mesh;
                return true;  // MeshEXT
            case 5313:
                stage = Ir77PVShaderStage::RayGeneration;
                return true;
            case 5314:
                stage = Ir77PVShaderStage::RayIntersection;
                return true;
            case 5315:
                stage = Ir77PVShaderStage::RayAnyHit;
                return true;
            case 5316:
                stage = Ir77PVShaderStage::RayClosestHit;
                return true;
            case 5317:
                stage = Ir77PVShaderStage::RayMiss;
                return true;
            case 5318:
                stage = Ir77PVShaderStage::RayCallable;
                return true;
            default:
                return false;
        }
    }

    std::uint32_t Opcode(std::vector<std::uint32_t> const& type) const { return type.empty() ? 0 : type[0] & 0xFFFFu; }

    // OpTypePointer -> the type it points to
    std::uint32_t Pointee(Ir77PVSpirvModule const& module, std::uint32_t const pointer) const {
        auto const found = module.types.find(pointer);
        if (found == module.types.end() || Opcode(found->second) != SPV_OP_TYPE_POINTER || found->second.size() < 4) return 0;

        return found->second[3];
    }

    std::uint32_t Decoration(Ir77PVSpirvModule const& module, std::uint32_t const id, std::uint32_t const decoration) const {
        auto const target = module.decorations.find(id);
        if (target == module.decorations.end()) return 0;

        auto const found = target->second.find(decoration);
        return found == target->second.end() ? 0 : found->second;
    }

    // Byte size of a type as laid out by its offset / stride decorations -- what a push-constant block occupies. Runtime arrays count 0.
    std::uint32_t SizeOf(Ir77PVSpirvModule const& module, std::uint32_t const type_id) const {
        auto const found = module.types.find(type_id);
        if (found == module.types.end()) return 0;

        std::vector<std::uint32_t> const& type = found->second;

        switch (Opcode(type)) {
            case SPV_OP_TYPE_BOOL:
                return 4;

            case SPV_OP_TYPE_INT:
            case SPV_OP_TYPE_FLOAT:
                return type.size() >= 3 ? type[2] / 8 : 0;

            case SPV_OP_TYPE_VECTOR:
            case SPV_OP_TYPE_MATRIX:
                return type.size() >= 4 ? SizeOf(module, type[2]) * type[3] : 0;

            case SPV_OP_TYPE_ARRAY: {
                if (type.size() < 4) return 0;

                auto const length = module.constants.find(type[3]);
                std::uint32_t const stride{Decoration(module, type_id, SPV_DECORATION_ARRAY_STRIDE)};

                return length == module.constants.end() ? 0 : length->second * (stride != 0 ? stride : SizeOf(module, type[2]));
            }

            case SPV_OP_TYPE_STRUCT: {
                std::uint32_t size{0};
                auto const members = module.member_decorations.find(type_id);

                for (std::size_t m = 2; m < type.size(); m++) {
                    std::uint32_t const member{static_cast<std::uint32_t>(m - 2)};
                    std::uint32_t offset{0};
                    std::uint32_t member_size{SizeOf(module, type[m])};

                    if (members != module.member_decorations.end()) {
                        auto const decorations = members->second.find(member);

                        if (decorations != members->second.end()) {
                            if (auto const o = decorations->second.find(SPV_DECORATION_OFFSET); o != decorations->second.end()) offset = o->second;

                            // matrices are laid out by their stride, not their tight size
                            auto const matrix = module.types.find(type[m]);
                            auto const stride = decorations->second.find(SPV_DECORATION_MATRIX_STRIDE);
                            if (matrix != module.types.end() && Opcode(matrix->second) == SPV_OP_TYPE_MATRIX && matrix->second.size() >= 4 &&
                                stride != decorations->second.end())
                                member_size = stride->second * matrix->second[3];
                        }
                    }

                    size = std::max(size, offset + member_size);
                }

                return size;
            }

            default:
                return 0;
        }
    }

    // What a descriptor variable binds: arrays are looked through to their element type
    Ir77PVSpirvDescriptorKind DescriptorKind(Ir77PVSpirvModule const& module, Ir77PVSpirvVariable const& variable) const {
        std::uint32_t type_id{Pointee(module, variable.pointer_type)};

        for (int depth = 0; depth < 8; depth++) {
            auto const found = module.types.find(type_id);
            if (found == module.types.end()) return Ir77PVSpirvDescriptorKind::Other;

            std::vector<std::uint32_t> const& type = found->second;

            switch (Opcode(type)) {
                case SPV_OP_TYPE_ARRAY:
                case SPV_OP_TYPE_RUNTIME_ARRAY:
                    type_id = type.size() >= 3 ? type[2] : 0;
                    continue;

                case SPV_OP_TYPE_SAMPLED_IMAGE:
                    return Ir77PVSpirvDescriptorKind::Texture;

                case SPV_OP_TYPE_ACCELERATION_STRUCTURE:
                    return Ir77PVSpirvDescriptorKind::AccelerationStructure;

                case SPV_OP_TYPE_IMAGE:
                    // operand 7 "sampled": 2 = storage image
                    return type.size() >= 8 && type[7] == 2 ? Ir77PVSpirvDescriptorKind::StorageImage : Ir77PVSpirvDescriptorKind::Other;

                case SPV_OP_TYPE_STRUCT:
                    if (variable.storage == SPV_STORAGE_STORAGE_BUFFER) return Ir77PVSpirvDescriptorKind::StorageBuffer;
                    if (variable.storage == SPV_STORAGE_UNIFORM && module.decorations.count(type_id) != 0 &&
                        module.decorations.at(type_id).count(SPV_DECORATION_BUFFER_BLOCK) != 0)
                        return Ir77PVSpirvDescriptorKind::StorageBuffer;
                    return Ir77PVSpirvDescriptorKind::Other;  // uniform buffer

                default:
                    return Ir77PVSpirvDescriptorKind::Other;
            }
        }

        return Ir77PVSpirvDescriptorKind::Other;
    }

   private:
    static constexpr std::uint32_t SPIRV_MAGIC = 0x07230203;
    static constexpr std::size_t SPIRV_HEADER_WORDS = 5;

    // opcodes
    static constexpr std::uint32_t SPV_OP_ENTRY_POINT = 15;
    static constexpr std::uint32_t SPV_OP_TYPE_FIRST = 19;  // OpTypeVoid
    static constexpr std::uint32_t SPV_OP_TYPE_BOOL = 20;
    static constexpr std::uint32_t SPV_OP_TYPE_INT = 21;
    static constexpr std::uint32_t SPV_OP_TYPE_FLOAT = 22;
    static constexpr std::uint32_t SPV_OP_TYPE_VECTOR = 23;
    static constexpr std::uint32_t SPV_OP_TYPE_MATRIX = 24;
    static constexpr std::uint32_t SPV_OP_TYPE_IMAGE = 25;
    static constexpr std::uint32_t SPV_OP_TYPE_SAMPLED_IMAGE = 27;
    static constexpr std::uint32_t SPV_OP_TYPE_ARRAY = 28;
    static constexpr std::uint32_t SPV_OP_TYPE_RUNTIME_ARRAY = 29;
    static constexpr std::uint32_t SPV_OP_TYPE_STRUCT = 30;
    static constexpr std::uint32_t SPV_OP_TYPE_POINTER = 32;
    static constexpr std::uint32_t SPV_OP_TYPE_LAST = 32;
    static constexpr std::uint32_t SPV_OP_TYPE_ACCELERATION_STRUCTURE = 5341;  // OpTypeAccelerationStructureKHR
    static constexpr std::uint32_t SPV_OP_CONSTANT = 43;
    static constexpr std::uint32_t SPV_OP_VARIABLE = 59;
    static constexpr std::uint32_t SPV_OP_DECORATE = 71;
    static constexpr std::uint32_t SPV_OP_MEMBER_DECORATE = 72;

    // decorations
    static constexpr std::uint32_t SPV_DECORATION_BUFFER_BLOCK = 3;
    static constexpr std::uint32_t SPV_DECORATION_ARRAY_STRIDE = 6;
    static constexpr std::uint32_t SPV_DECORATION_MATRIX_STRIDE = 7;
    static constexpr std::uint32_t SPV_DECORATION_BUILTIN = 11;
    static constexpr std::uint32_t SPV_DECORATION_LOCATION = 30;
    static constexpr std::uint32_t SPV_DECORATION_BINDING = 33;
    static constexpr std::uint32_t SPV_DECORATION_DESCRIPTOR_SET = 34;
    static constexpr std::uint32_t SPV_DECORATION_OFFSET = 35;

    // storage classes
    static constexpr std::uint32_t SPV_STORAGE_UNIFORM_CONSTANT = 0;
    static constexpr std::uint32_t SPV_STORAGE_INPUT = 1;
    static constexpr std::uint32_t SPV_STORAGE_UNIFORM = 2;
    static constexpr std::uint32_t SPV_STORAGE_OUTPUT = 3;
    static constexpr std::uint32_t SPV_STORAGE_PUSH_CONSTANT = 9;
    static constexpr std::uint32_t SPV_STORAGE_STORAGE_BUFFER = 12;

    std::shared_ptr<IIr77PVDevice> m_device{};

    std::string m_shader_root{};  // SetShaderRoot -- LoadShaders reads it, CompileShader writes <root>/generated

    std::string m_shader_compiler{"glslangValidator"};  // CompileShader runs it -- on PATH

    // [handle]
    std::vector<Ir77PVShaderInfo> m_shaders{};

    // name -> handle
    std::map<std::string, Ir77PVShaderHandle> m_names{};

    // descriptors of the shader being registered -- filled by DefineReflection, checked by DefineContract
    std::vector<Ir77PVSpirvDescriptor> m_descriptors{};
};
}  // namespace NSIr77PeregrineV
