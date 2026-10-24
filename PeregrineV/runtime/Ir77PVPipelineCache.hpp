#pragma once

#include <vulkan/vulkan.h>

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <set>
#include <string_view>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include <simdjson.h>

#include "../types/Ir77PVTypes.hpp"

#include "../../Ir77RT/dictionary/IDIIr77MPVM.hpp"

#include "../dictionary/IDIIr77PeregrineV.hpp"
#include "../dictionary/IDIr77PeregrineV.hpp"

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../../Ir77RT/runtime/Ir77GUID.hpp"
#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"

#include "../interface/IIr77PVPipelineCache.hpp"

#include "Ir77PVPipeline.hpp"
#include "Ir77PVPipelineCPT.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

// =============================================================================================================================================
// Ir77PVPipelineCache -- every pipeline of one device, graphics and compute, keyed by its Ir77PVPipelineDesc (stage handles included), built
// on first request. Named pipelines and the engine roles come from the pipeline manifest (pipelines.json), not from code.
//
//   configure: SetDevice / SetLayout / SetShader / SetRenderPass (one per pass kind) / [SetCachePath]
//   create:    CreatePipelineCache -> DefineCachePath -> DefineCache
//   manifest:  LoadManifest -> DefineManifest -> per pipeline DefineNamedState + DefineNamedStages -> DefineRoles
//   use:       FindPipeline(name / kind) once -> Ir77PVPipelineHandle (draw items and dispatches carry it) -> GetPipeline(handle) per draw,
//              GetComputePipeline(handle) per dispatch. The only way in: a handle names a pipeline (or a role) and resolves against the current
//              manifest, so a manifest reload re-points everything.
//   resolve:   handle -> DefineSlot -> DescribeNamed / DescribeKind -> BuildPipeline or BuildComputePipeline (existing, or Define*Pipeline on a
//              miss; a failed one isn't retried)
//
// Manifest (version 1):
//   "pipelines": { name: { vertex / fragment / task / mesh: shader registry names,
//                           pass: main | shadow | offscreen | post, blend: opaque | alpha | additive, cull: back | front | none,
//                           depth_test / depth_write / depth_bias / flip_back_faces: bool, user: [up to 4 names for pc.user[4..7]] }
//                  name: { compute: shader registry name, user: [...] } -- a compute pipeline: nothing else applies }
//   "roles":     { role: pipeline name } -- roles are the engine's Ir77PVPipelineKind values (static, skinned, transparent,
//                 skinned.transparent, mesh, cef, shadow, shadow.skinned); static and cef are required
// A malformed entry fails the load. An entry naming a shader that isn't registered is kept as unavailable (with the reason) and warned about.
//   hot swap:  InvalidateShader(handle) after the registry replaced a shader -- every pipeline built from it is dropped and rebuilt on next use
//   shutdown:  the destructor saves the VkPipelineCache data, then destroys the pipelines and the cache
//
// The VkPipelineCache is seeded from disk only when the file's header names this GPU and driver (vendor, device, pipelineCacheUUID) --
// a driver update makes the old file useless, and it is simply overwritten on the next save.
// =============================================================================================================================================
class Ir77PVPipelineCache : public Ir77Enlisted, public IIr77PVPipelineCache, public std::enable_shared_from_this<Ir77PVPipelineCache> {
   public:
    Ir77PVPipelineCache() {
        try {
            m_enlisted_uuid.Generate();
        } catch (std::invalid_argument a) {
            throw a;
        }

        m_enlisted = std::chrono::system_clock::now();
    }

    ~Ir77PVPipelineCache() {
        if (!m_device) return;

        VkDevice device{VK_NULL_HANDLE};
        m_device->GetDevice(&device);

        if (m_cache == VK_NULL_HANDLE) return;

        SaveCache();

        m_handles.clear();
        m_pipelines.clear();  // each destroys its VkPipeline
        m_compute_pipelines.clear();

        vkDestroyPipelineCache(device, m_cache, nullptr);
    }

   public:
    std::shared_ptr<IIr77Return const> EnlistedAs(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIIr77Enlisted>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> MemberUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77PVPipelineCache>(uid);

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

        else if (iid == GUIDIIr77PVPipelineCache)
            obj = std::shared_ptr<IIr77PVPipelineCache>(shared_from_this(), static_cast<IIr77PVPipelineCache*>(this));

        else if (iid == GUIDIr77PVPipelineCache)
            obj = std::shared_ptr<Ir77PVPipelineCache>(shared_from_this(), static_cast<Ir77PVPipelineCache*>(this));

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

    std::shared_ptr<IIr77Return const> SetLayout(std::shared_ptr<IIr77PVLayout> layout) {
        m_layout = layout;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SetShader(std::shared_ptr<IIr77PVShader> shader_stack) {
        m_shader_stack = shader_stack;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SetRenderPass(Ir77PVRenderPassKind const& kind, std::shared_ptr<IIr77PVRenderPass> render_pass) {
        m_render_passes[kind] = render_pass;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SetCachePath(std::string const& path) {
        m_cache_path = path;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // create
    // -------------------------------------------------------------------------------------------------------------------------------------
    std::shared_ptr<IIr77Return const> CreatePipelineCache() {
        if (!m_device) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPipelineCache: SetDevice not called.");
        if (!m_layout) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPipelineCache: SetLayout not called.");
        if (!m_shader_stack) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPipelineCache: SetShader not called.");
        if (m_cache != VK_NULL_HANDLE) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPipelineCache: pipeline cache already created.");

        auto const result = DefineCachePath();
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        return DefineCache();
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // pipelines -- by handle only
    // -------------------------------------------------------------------------------------------------------------------------------------
    // The handle of a manifest pipeline. Issued once per name for the cache's life -- asking again returns the same handle. The name doesn't
    // have to be in the manifest yet: the handle resolves when it is, so draw items can be made before a reload adds their pipeline.
    std::shared_ptr<IIr77Return const> FindPipeline(std::string const& name, Ir77PVPipelineHandle& handle) {
        return DefineHandle(name, false, handle);
    }

    // The handle of an engine role -- follows the manifest's "roles", so pointing the role at another pipeline moves every item holding it
    std::shared_ptr<IIr77Return const> FindPipeline(Ir77PVPipelineKind const& kind, Ir77PVPipelineHandle& handle) {
        return DefineHandle(RoleName(kind), true, handle);
    }

    // Per draw: one index after the first call; the first call (and the first after a manifest load) describes and builds. A compute handle
    // fails here -- see GetComputePipeline. A handle that can't resolve fails quietly (reported once) until the next manifest load.
    std::shared_ptr<IIr77Return const> GetPipeline(Ir77PVPipelineHandle const& handle, std::shared_ptr<IIr77PVPipeline>& pipeline) {
        if (handle >= m_handles.size()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPipelineCache: unknown pipeline handle.");

        auto& slot = m_handles[handle];

        if (slot.pipeline) {
            pipeline = slot.pipeline;
            return Ir77RETURN<Ir77OperationSucceeded>();
        }

        if (slot.failed || slot.compute) return Ir77RETURN<Ir77NotConfigured>();

        auto const result = DefineSlot(handle);
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        if (!slot.pipeline) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPipelineCache: '" + slot.name + "' is a compute pipeline -- dispatch it.");

        pipeline = slot.pipeline;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Per dispatch -- the same, for a manifest entry with "compute"
    std::shared_ptr<IIr77Return const> GetComputePipeline(Ir77PVPipelineHandle const& handle, std::shared_ptr<IIr77PVPipelineCPT>& pipeline) {
        if (handle >= m_handles.size()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPipelineCache: unknown pipeline handle.");

        auto& slot = m_handles[handle];

        if (slot.compute) {
            pipeline = slot.compute;
            return Ir77RETURN<Ir77OperationSucceeded>();
        }

        if (slot.failed || slot.pipeline) return Ir77RETURN<Ir77NotConfigured>();

        auto const result = DefineSlot(handle);
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        if (!slot.compute) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPipelineCache: '" + slot.name + "' is a graphics pipeline -- draw it.");

        pipeline = slot.compute;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // The name a handle was issued for -- a pipeline name, or a role name for a role handle
    std::shared_ptr<IIr77Return const> GetPipelineName(Ir77PVPipelineHandle const& handle, std::string& name) {
        if (handle >= m_handles.size()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPipelineCache: unknown pipeline handle.");

        name = m_handles[handle].name;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // A manifest pipeline's description, stage names already resolved to handles
    std::shared_ptr<IIr77Return const> DescribeNamed(std::string const& name, Ir77PVPipelineDesc& desc) {
        auto const found = m_named.find(name);
        if (found != m_named.end()) {
            desc = found->second;
            return Ir77RETURN<Ir77OperationSucceeded>();
        }

        auto const unavailable = m_unavailable.find(name);
        if (unavailable != m_unavailable.end())
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPipelineCache: pipeline '" + name + "' is unavailable -- " + unavailable->second);

        return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPipelineCache: no pipeline named '" + name + "' in the manifest.");
    }

    // The names a pipeline gives pc.user[4..7], in slot order -- for setting parameters by name
    std::shared_ptr<IIr77Return const> GetUserParameters(std::string const& name, std::vector<std::string>& parameters) {
        auto const found = m_user.find(name);
        if (found == m_user.end()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPipelineCache: no pipeline named '" + name + "' in the manifest.");

        parameters = found->second;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // manifest
    // -------------------------------------------------------------------------------------------------------------------------------------
    // Replaces every named pipeline and role. Needs the shader registry (SetShader) populated -- stage names resolve now. Pipelines already
    // built stay cached; handles re-resolve, so names and roles point at their new descriptions on the next draw.
    std::shared_ptr<IIr77Return const> LoadManifest(std::string const& path) {
        if (!m_shader_stack) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPipelineCache: SetShader not called.");

        simdjson::dom::parser parser{};
        simdjson::dom::element root{};

        if (auto const error = parser.load(path).get(root); error)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPipelineCache: cannot read manifest " + path + " -- " + simdjson::error_message(error));

        return DefineManifest(root, path);
    }

    // A shader's code changed behind its handle (Ir77PVShader::ReplaceShader): every pipeline built from it -- graphics or compute -- is
    // destroyed, its earlier build failure forgotten, and every handle re-resolves, building again on next use. The device must be idle.
    std::shared_ptr<IIr77Return const> InvalidateShader(Ir77PVShaderHandle const& shader) {
        auto const uses = [shader](Ir77PVPipelineDesc const& desc) {
            return desc.vertex == shader || desc.fragment == shader || desc.task == shader || desc.mesh == shader || desc.compute == shader;
        };

        for (auto& slot : m_handles) {
            slot.pipeline.reset();
            slot.compute.reset();
            slot.failed = false;
        }

        std::erase_if(m_pipelines, [&](auto const& entry) { return uses(entry.first); });
        std::erase_if(m_compute_pipelines, [&](auto const& entry) { return uses(entry.first); });
        std::erase_if(m_failed, uses);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // persistence
    // -------------------------------------------------------------------------------------------------------------------------------------
    // Written to <path>.tmp and renamed over <path>, so a crash mid-write never leaves a torn file
    std::shared_ptr<IIr77Return const> SaveCache() {
        if (m_cache == VK_NULL_HANDLE || m_cache_path.empty()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPipelineCache: nothing to save.");

        VkDevice device{VK_NULL_HANDLE};
        m_device->GetDevice(&device);

        std::size_t size{0};
        if (vkGetPipelineCacheData(device, m_cache, &size, nullptr) != VK_SUCCESS || size == 0)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPipelineCache: vkGetPipelineCacheData failed.");

        std::vector<char> data(size);
        if (vkGetPipelineCacheData(device, m_cache, &size, data.data()) != VK_SUCCESS)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPipelineCache: vkGetPipelineCacheData failed.");

        std::filesystem::path const path{m_cache_path};
        std::filesystem::path const temp{m_cache_path + ".tmp"};

        std::error_code error{};
        std::filesystem::create_directories(path.parent_path(), error);

        {
            std::ofstream file{temp, std::ios::binary | std::ios::trunc};
            if (!file) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPipelineCache: cannot write " + temp.string());

            file.write(data.data(), static_cast<std::streamsize>(size));
            if (!file) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPipelineCache: write failed " + temp.string());
        }

        std::filesystem::rename(temp, path, error);
        if (error) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPipelineCache: rename failed " + path.string());

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // steps
    // -------------------------------------------------------------------------------------------------------------------------------------
   private:
    // Keeps a path given through SetCachePath; otherwise $XDG_CACHE_HOME or ~/.cache, per GPU so two GPUs never fight over one file
    std::shared_ptr<IIr77Return const> DefineCachePath() {
        if (!m_cache_path.empty()) return Ir77RETURN<Ir77OperationSucceeded>();

        VkPhysicalDeviceProperties properties{};
        m_device->GetDeviceProperties(properties);

        std::filesystem::path base{};

        if (char const* xdg = std::getenv("XDG_CACHE_HOME"); xdg != nullptr && xdg[0] != '\0')
            base = xdg;
        else if (char const* home = std::getenv("HOME"); home != nullptr && home[0] != '\0')
            base = std::filesystem::path{home} / ".cache";
        else
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPipelineCache: no HOME / XDG_CACHE_HOME -- call SetCachePath.");

        char name[64]{};
        std::snprintf(name, sizeof(name), "pipelines-%04x-%04x.bin", properties.vendorID, properties.deviceID);

        m_cache_path = (base / "iridium" / name).string();

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Seeds the VkPipelineCache from the file when its header matches this GPU and driver; otherwise starts empty
    std::shared_ptr<IIr77Return const> DefineCache() {
        VkDevice device{VK_NULL_HANDLE};
        m_device->GetDevice(&device);

        std::vector<char> data{};

        std::ifstream file{m_cache_path, std::ios::binary | std::ios::ate};
        if (file) {
            std::streamsize const size{file.tellg()};
            file.seekg(0);

            data.resize(static_cast<std::size_t>(size > 0 ? size : 0));
            if (!data.empty() && !file.read(data.data(), size)) data.clear();
        }

        if (!CacheDataMatches(data)) data.clear();

        VkPipelineCacheCreateInfo cache_info{};
        cache_info.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
        cache_info.initialDataSize = data.size();
        cache_info.pInitialData = data.empty() ? nullptr : data.data();

        if (vkCreatePipelineCache(device, &cache_info, nullptr, &m_cache) != VK_SUCCESS)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPipelineCache: vkCreatePipelineCache failed.");

        return Ir77RETURN<Ir77OperationSucceeded>(this, data.empty() ? "Success: Create Pipeline Cache (empty)." : "Success: Create Pipeline Cache (from disk).");
    }

    // A cache-data header written by this GPU and driver -- anything else would be rejected or, worse, misread
    bool CacheDataMatches(std::vector<char> const& data) const {
        if (data.size() < sizeof(VkPipelineCacheHeaderVersionOne)) return false;

        VkPipelineCacheHeaderVersionOne header{};
        std::memcpy(&header, data.data(), sizeof(header));

        VkPhysicalDeviceProperties properties{};
        m_device->GetDeviceProperties(properties);

        return header.headerSize >= sizeof(VkPipelineCacheHeaderVersionOne) && header.headerVersion == VK_PIPELINE_CACHE_HEADER_VERSION_ONE &&
               header.vendorID == properties.vendorID && header.deviceID == properties.deviceID &&
               std::memcmp(header.pipelineCacheUUID, properties.pipelineCacheUUID, VK_UUID_SIZE) == 0;
    }

    // name -> handle, issuing a new one the first time a name (or role) is asked for
    std::shared_ptr<IIr77Return const> DefineHandle(std::string const& name, bool const role, Ir77PVPipelineHandle& handle) {
        auto& handles = role ? m_role_handles : m_name_handles;

        auto const found = handles.find(name);
        if (found != handles.end()) {
            handle = found->second;
            return Ir77RETURN<Ir77OperationSucceeded>();
        }

        if (m_handles.size() >= IR77PV_NO_PIPELINE) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPipelineCache: out of pipeline handles.");

        handle = static_cast<Ir77PVPipelineHandle>(m_handles.size());

        Ir77PVPipelineSlot slot{};
        slot.name = name;
        slot.role = role;

        m_handles.push_back(slot);
        handles[name] = handle;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // The whole manifest into fresh tables, swapped in only when it is valid -- a bad file leaves the previous one in force
    std::shared_ptr<IIr77Return const> DefineManifest(simdjson::dom::element const& root, std::string const& path) {
        std::string const where{"Ir77PVPipelineCache: " + path + " -- "};

        std::uint64_t version{0};
        if (root["version"].get(version) != simdjson::SUCCESS || version != MANIFEST_VERSION)
            return Ir77RETURN<Ir77NotConfigured>(this, where + "\"version\" must be " + std::to_string(MANIFEST_VERSION) + ".");

        simdjson::dom::object pipelines{};
        if (root["pipelines"].get(pipelines) != simdjson::SUCCESS) return Ir77RETURN<Ir77NotConfigured>(this, where + "\"pipelines\" must be an object.");

        std::map<std::string, Ir77PVPipelineDesc> named{};
        std::map<std::string, std::string> unavailable{};
        std::map<std::string, std::vector<std::string>> user{};

        for (auto const field : pipelines) {
            std::string const name{field.key};

            simdjson::dom::object entry{};
            if (field.value.get(entry) != simdjson::SUCCESS) return Ir77RETURN<Ir77NotConfigured>(this, where + "pipeline '" + name + "' must be an object.");

            Ir77PVPipelineDesc desc{};

            // malformed -- the whole load fails
            auto result = DefineNamedState(where + "pipeline '" + name + "': ", entry, desc, user[name]);
            if (result->ID() != GUIDIr77OperationSucceeded) return result;

            // a shader that isn't registered -- this pipeline is unavailable, the rest load
            std::string missing{};
            DefineNamedStages(entry, desc, missing);

            if (!missing.empty()) {
                unavailable[name] = missing;
                Ir77RETURN<Ir77OperationSucceeded>(this, "[WARN] pipeline '" + name + "' unavailable: " + missing);
                continue;
            }

            named[name] = desc;
        }

        std::map<std::string, std::string> roles{};

        auto const result = DefineRoles(root, where, named, unavailable, roles);
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        m_named = std::move(named);
        m_unavailable = std::move(unavailable);
        m_user = std::move(user);
        m_roles = std::move(roles);

        // every handle re-resolves against the new manifest on its next use
        for (auto& slot : m_handles) {
            slot.pipeline.reset();
            slot.compute.reset();
            slot.failed = false;
        }

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: Load Pipeline Manifest.");
    }

    // Everything in a pipeline entry except its stages. Unknown keys are errors -- a typo shouldn't silently fall back to a default.
    std::shared_ptr<IIr77Return const> DefineNamedState(std::string const& where, simdjson::dom::object const& entry, Ir77PVPipelineDesc& desc,
                                                        std::vector<std::string>& user) {
        bool compute{false};
        std::string graphics_key{};

        for (auto const field : entry) {
            std::string_view const key{field.key};

            if (key == "compute") compute = true;
            else if (key != "user" && graphics_key.empty()) graphics_key = key;

            if (key == "vertex" || key == "fragment" || key == "task" || key == "mesh" || key == "compute") {
                std::string_view value{};
                if (field.value.get(value) != simdjson::SUCCESS) return Ir77RETURN<Ir77NotConfigured>(this, where + "\"" + std::string{key} + "\" must be a shader name.");
            } else if (key == "pass") {
                std::string_view value{};
                if (field.value.get(value) != simdjson::SUCCESS || !PassOf(value, desc.pass))
                    return Ir77RETURN<Ir77NotConfigured>(this, where + "\"pass\" must be main, shadow, offscreen or post.");
            } else if (key == "blend") {
                std::string_view value{};
                if (field.value.get(value) != simdjson::SUCCESS || !BlendOf(value, desc.blend))
                    return Ir77RETURN<Ir77NotConfigured>(this, where + "\"blend\" must be opaque, alpha or additive.");
            } else if (key == "cull") {
                std::string_view value{};
                if (field.value.get(value) != simdjson::SUCCESS || !CullOf(value, desc.cull))
                    return Ir77RETURN<Ir77NotConfigured>(this, where + "\"cull\" must be back, front or none.");
            } else if (key == "depth_test" || key == "depth_write" || key == "depth_bias" || key == "flip_back_faces") {
                bool value{false};
                if (field.value.get(value) != simdjson::SUCCESS) return Ir77RETURN<Ir77NotConfigured>(this, where + "\"" + std::string{key} + "\" must be true or false.");

                if (key == "depth_test") desc.depth_test = value;
                if (key == "depth_write") desc.depth_write = value;
                if (key == "depth_bias") desc.depth_bias = value;
                if (key == "flip_back_faces") desc.flip_back_faces = value;
            } else if (key == "user") {
                simdjson::dom::array names{};
                if (field.value.get(names) != simdjson::SUCCESS) return Ir77RETURN<Ir77NotConfigured>(this, where + "\"user\" must be an array of names.");

                user.clear();

                for (auto const element : names) {
                    std::string_view value{};
                    if (element.get(value) != simdjson::SUCCESS) return Ir77RETURN<Ir77NotConfigured>(this, where + "\"user\" entries must be strings.");

                    user.emplace_back(value);
                }

                if (user.size() > USER_PARAMETER_COUNT)
                    return Ir77RETURN<Ir77NotConfigured>(this, where + "\"user\" has " + std::to_string(user.size()) + " names -- pc.user[4..7] holds " +
                                                                   std::to_string(USER_PARAMETER_COUNT) + ".");
            } else {
                return Ir77RETURN<Ir77NotConfigured>(this, where + "unknown key \"" + std::string{key} + "\".");
            }
        }

        if (compute && !graphics_key.empty())
            return Ir77RETURN<Ir77NotConfigured>(this, where + "a compute pipeline takes only \"compute\" and \"user\" -- not \"" + graphics_key + "\".");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Stage names -> registry handles. missing = why not, when a named shader isn't registered (empty when every stage resolved).
    void DefineNamedStages(simdjson::dom::object const& entry, Ir77PVPipelineDesc& desc, std::string& missing) {
        std::pair<char const*, Ir77PVShaderHandle*> const stages[]{
            {"vertex", &desc.vertex}, {"fragment", &desc.fragment}, {"task", &desc.task}, {"mesh", &desc.mesh}, {"compute", &desc.compute}};

        for (auto const& [key, handle] : stages) {
            std::string_view value{};
            if (entry[key].get(value) != simdjson::SUCCESS) continue;  // stage not used

            if (m_shader_stack->FindShader(std::string{value}, *handle)->ID() != GUIDIr77OperationSucceeded) {
                missing = "shader '" + std::string{value} + "' is not registered";
                return;
            }
        }

        if (desc.vertex == IR77PV_NO_SHADER && desc.mesh == IR77PV_NO_SHADER && desc.compute == IR77PV_NO_SHADER)
            missing = "it names no vertex, mesh or compute shader";
    }

    // "roles": every engine role it names must point at a pipeline in the file; static and cef must point at available ones
    std::shared_ptr<IIr77Return const> DefineRoles(simdjson::dom::element const& root, std::string const& where, std::map<std::string, Ir77PVPipelineDesc> const& named,
                                                   std::map<std::string, std::string> const& unavailable, std::map<std::string, std::string>& roles) {
        simdjson::dom::object object{};
        if (root["roles"].get(object) != simdjson::SUCCESS) return Ir77RETURN<Ir77NotConfigured>(this, where + "\"roles\" must be an object.");

        for (auto const field : object) {
            std::string const role{field.key};

            std::string_view pipeline{};
            if (field.value.get(pipeline) != simdjson::SUCCESS) return Ir77RETURN<Ir77NotConfigured>(this, where + "role '" + role + "' must name a pipeline.");

            if (!IsRole(role)) {
                Ir77RETURN<Ir77OperationSucceeded>(this, "[WARN] manifest role '" + role + "' isn't an engine role -- ignored");
                continue;
            }

            if (named.count(std::string{pipeline}) == 0 && unavailable.count(std::string{pipeline}) == 0)
                return Ir77RETURN<Ir77NotConfigured>(this, where + "role '" + role + "' names pipeline '" + std::string{pipeline} + "', which isn't in \"pipelines\".");

            auto const described = named.find(std::string{pipeline});
            if (described != named.end() && described->second.IsCompute())
                return Ir77RETURN<Ir77NotConfigured>(this, where + "role '" + role + "' names compute pipeline '" + std::string{pipeline} + "' -- roles are drawn.");

            roles[role] = std::string{pipeline};
        }

        for (auto const kind : {Ir77PVPipelineKind::Static, Ir77PVPipelineKind::CEF}) {
            auto const role = roles.find(RoleName(kind));

            if (role == roles.end()) return Ir77RETURN<Ir77NotConfigured>(this, where + "required role '" + RoleName(kind) + "' is missing.");

            if (named.count(role->second) == 0)
                return Ir77RETURN<Ir77NotConfigured>(this, where + "required role '" + RoleName(kind) + "' points at unavailable pipeline '" + role->second + "'.");
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // role -> the manifest's pipeline for it
    std::shared_ptr<IIr77Return const> DescribeKind(Ir77PVPipelineKind const& kind, Ir77PVPipelineDesc& desc) {
        auto const role = m_roles.find(RoleName(kind));
        if (role == m_roles.end()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPipelineCache: role '" + RoleName(kind) + "' has no pipeline in the manifest.");

        return DescribeNamed(role->second, desc);
    }

    // An unresolved slot -> its description -> its graphics or compute pipeline. A failure marks the slot until the next manifest load.
    std::shared_ptr<IIr77Return const> DefineSlot(Ir77PVPipelineHandle const& handle) {
        auto& slot = m_handles[handle];

        Ir77PVPipelineDesc desc{};

        auto result = slot.role ? DescribeKind(RoleOf(slot.name), desc) : DescribeNamed(slot.name, desc);

        if (result->ID() == GUIDIr77OperationSucceeded) result = desc.IsCompute() ? BuildComputePipeline(desc, slot.compute) : BuildPipeline(desc, slot.pipeline);

        if (result->ID() != GUIDIr77OperationSucceeded) slot.failed = true;

        return result;
    }

    // compute description -> its pipeline, the same way
    std::shared_ptr<IIr77Return const> BuildComputePipeline(Ir77PVPipelineDesc const& desc, std::shared_ptr<IIr77PVPipelineCPT>& pipeline) {
        auto const found = m_compute_pipelines.find(desc);
        if (found != m_compute_pipelines.end()) {
            pipeline = found->second;
            return Ir77RETURN<Ir77OperationSucceeded>();
        }

        if (m_failed.count(desc) != 0) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPipelineCache: this pipeline failed to build earlier.");

        auto created = std::static_pointer_cast<IIr77PVPipelineCPT>(std::make_shared<Ir77PVPipelineCPT>());

        created->SetDevice(m_device);
        created->SetLayout(m_layout);
        created->SetShader(m_shader_stack);
        created->SetComputeShader(desc.compute);

        auto const result = created->CreatePipeline();
        if (result->ID() != GUIDIr77OperationSucceeded) {
            m_failed.insert(desc);
            return result;
        }

        m_compute_pipelines.emplace(desc, created);
        pipeline = created;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // description -> its pipeline: the one already built, or built now. A description that failed once stays failed.
    std::shared_ptr<IIr77Return const> BuildPipeline(Ir77PVPipelineDesc const& desc, std::shared_ptr<IIr77PVPipeline>& pipeline) {
        if (m_cache == VK_NULL_HANDLE) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPipelineCache: CreatePipelineCache not called.");

        auto const found = m_pipelines.find(desc);
        if (found != m_pipelines.end()) {
            pipeline = found->second;
            return Ir77RETURN<Ir77OperationSucceeded>();
        }

        if (m_failed.count(desc) != 0) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPipelineCache: this pipeline failed to build earlier.");

        return DefinePipeline(desc, pipeline);
    }

    // A miss: build it against the pass the description names, keep it, or remember that it failed
    std::shared_ptr<IIr77Return const> DefinePipeline(Ir77PVPipelineDesc const& desc, std::shared_ptr<IIr77PVPipeline>& pipeline) {
        auto const pass = m_render_passes.find(desc.pass);
        if (pass == m_render_passes.end() || !pass->second) {
            m_failed.insert(desc);
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPipelineCache: the description's render pass was not set.");
        }

        auto created = std::static_pointer_cast<IIr77PVPipeline>(std::make_shared<Ir77PVPipeline>());

        created->SetDevice(m_device);
        created->SetRenderPass(pass->second);
        created->SetLayout(m_layout);
        created->SetShader(m_shader_stack);
        created->SetDesc(desc);
        created->SetPipelineCache(m_cache);

        auto const result = created->CreatePipeline();
        if (result->ID() != GUIDIr77OperationSucceeded) {
            m_failed.insert(desc);
            return result;
        }

        m_pipelines.emplace(desc, created);
        pipeline = created;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // manifest vocabulary -- values only
    // -------------------------------------------------------------------------------------------------------------------------------------
    // The manifest name of each engine role
    std::string RoleName(Ir77PVPipelineKind const& kind) const {
        switch (kind) {
            case Ir77PVPipelineKind::Static: return "static";
            case Ir77PVPipelineKind::Skinned: return "skinned";
            case Ir77PVPipelineKind::Shadow: return "shadow";
            case Ir77PVPipelineKind::ShadowSkinned: return "shadow.skinned";
            case Ir77PVPipelineKind::Transparent: return "transparent";
            case Ir77PVPipelineKind::CEF: return "cef";
            case Ir77PVPipelineKind::Mesh: return "mesh";
            case Ir77PVPipelineKind::SkinnedTransparent: return "skinned.transparent";
        }

        return "";
    }

    // role name -> kind; only called with names RoleName produced
    Ir77PVPipelineKind RoleOf(std::string const& role) const {
        for (auto const kind : {Ir77PVPipelineKind::Static, Ir77PVPipelineKind::Skinned, Ir77PVPipelineKind::Shadow, Ir77PVPipelineKind::ShadowSkinned,
                                Ir77PVPipelineKind::Transparent, Ir77PVPipelineKind::CEF, Ir77PVPipelineKind::Mesh, Ir77PVPipelineKind::SkinnedTransparent})
            if (RoleName(kind) == role) return kind;

        return Ir77PVPipelineKind::Static;
    }

    bool IsRole(std::string const& role) const {
        for (auto const kind : {Ir77PVPipelineKind::Static, Ir77PVPipelineKind::Skinned, Ir77PVPipelineKind::Shadow, Ir77PVPipelineKind::ShadowSkinned,
                                Ir77PVPipelineKind::Transparent, Ir77PVPipelineKind::CEF, Ir77PVPipelineKind::Mesh, Ir77PVPipelineKind::SkinnedTransparent})
            if (RoleName(kind) == role) return true;

        return false;
    }

    bool PassOf(std::string_view const value, Ir77PVRenderPassKind& pass) const {
        if (value == "main") pass = Ir77PVRenderPassKind::Main;
        else if (value == "shadow") pass = Ir77PVRenderPassKind::Shadow;
        else if (value == "offscreen") pass = Ir77PVRenderPassKind::Offscreen;
        else if (value == "post") pass = Ir77PVRenderPassKind::Post;
        else return false;

        return true;
    }

    bool BlendOf(std::string_view const value, Ir77PVBlend& blend) const {
        if (value == "opaque") blend = Ir77PVBlend::Opaque;
        else if (value == "alpha") blend = Ir77PVBlend::Alpha;
        else if (value == "additive") blend = Ir77PVBlend::Additive;
        else return false;

        return true;
    }

    bool CullOf(std::string_view const value, VkCullModeFlags& cull) const {
        if (value == "back") cull = VK_CULL_MODE_BACK_BIT;
        else if (value == "front") cull = VK_CULL_MODE_FRONT_BIT;
        else if (value == "none") cull = VK_CULL_MODE_NONE;
        else return false;

        return true;
    }

   private:
    // One issued handle -- what it names, and the pipeline it resolved to under the current manifest
    typedef struct Ir77PVPipelineSlot {
        std::string name{};  // pipeline name, or role name when role
        bool role{false};
        std::shared_ptr<IIr77PVPipeline> pipeline{};    // graphics -- empty until first use, and again after every manifest load
        std::shared_ptr<IIr77PVPipelineCPT> compute{};  // compute -- the same
        bool failed{false};                          // didn't resolve under this manifest -- not retried until the next load
    }* pIr77PVPipelineSlot;

    static constexpr std::uint64_t MANIFEST_VERSION = 1;

    // pc.user[4..7] -- user[0..3] belong to the engine
    static constexpr std::size_t USER_PARAMETER_COUNT = 4;

    std::shared_ptr<IIr77PVDevice> m_device{};

    std::shared_ptr<IIr77PVLayout> m_layout{};

    std::shared_ptr<IIr77PVShader> m_shader_stack{};

    std::map<Ir77PVRenderPassKind, std::shared_ptr<IIr77PVRenderPass>> m_render_passes{};

    std::string m_cache_path{};

    VkPipelineCache m_cache{VK_NULL_HANDLE};

    // [description] -- ordered by Ir77PVPipelineDesc::operator<=>
    std::map<Ir77PVPipelineDesc, std::shared_ptr<IIr77PVPipeline>> m_pipelines{};

    std::map<Ir77PVPipelineDesc, std::shared_ptr<IIr77PVPipelineCPT>> m_compute_pipelines{};

    // descriptions that failed to build (bad stages / missing pass) -- not retried every frame
    std::set<Ir77PVPipelineDesc> m_failed{};

    // manifest -- name -> description (stages resolved), name -> why unavailable, name -> pc.user[4..7] names, role -> pipeline name
    std::map<std::string, Ir77PVPipelineDesc> m_named{};

    std::map<std::string, std::string> m_unavailable{};

    std::map<std::string, std::vector<std::string>> m_user{};

    std::map<std::string, std::string> m_roles{};

    // handles -- [handle] slot; name -> handle for pipeline names and for role names (separate, so a pipeline may share a role's name)
    std::vector<Ir77PVPipelineSlot> m_handles{};

    std::map<std::string, Ir77PVPipelineHandle> m_name_handles{};

    std::map<std::string, Ir77PVPipelineHandle> m_role_handles{};
};

}  // namespace NSIr77PeregrineV
