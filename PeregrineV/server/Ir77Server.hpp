#pragma once

#include <cstdint>
#include <cstdio>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <SDL3/SDL_video.h>

#include <simdjson.h>

#include "../../Ir77RT/types//Ir77Types.hpp"

#include "../../Ir77RT/runtime/Ir77Return.hpp"

#include "../dictionary/IDIIr77PeregrineV.hpp"

#include "../interface/IIr77PVInit.hpp"
#include "../interface/IIr77PVCEF.hpp"
#include "../interface/IIr77PVPaint.hpp"
#include "../interface/IIr77PVShader.hpp"
#include "../interface/IIr77PVTransfer.hpp"
#include "../interface/IIr77PVMaterial.hpp"
#include "../interface/IIr77PVGeometry.hpp"
#include "../interface/IIr77PVAcceleration.hpp"
#include "../interface/IIr77PVWorld.hpp"
#include "../interface/IIr77PVOverlay.hpp"

#include "../runtime/Ir77PVInit.hpp"

#include "../types/Ir77PVTypes.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

// =============================================================================================================================================
// Ir77Server -- the renderer module's switchboard. Owns Ir77PVInit, the lifetime controller; Init owns and wires CEF, Paint, Transfer,
// Materials, Geometry, (with ray queries) Acceleration and World, and holds the shader registry and pipeline cache.
// Nothing below holds this. Everything past Init is reached through its interface. The module's one door:
//
//   pipe:        Sink(json) in, the post function out (SetPost) -- setup, teardown, queries and reloads are ops
//   direct link: what can't travel as JSON or runs every frame --
//                  setup / loop:       SetWindows, Tick, Draw
//                  world:              LoadAsset, UnloadAsset, SubmitFrame
//                  overlay:            UploadOverlay, IsOverlayResizing
//                  runtime resources:  CompileShader, ReloadShader, FindPipeline, Create / Upload / DestroyStorageBuffer, CreateCanvas,
//                                      MakeProceduralDrawItem, Create / SetVisible / Release SurfaceAcceleration
//                The peers never leave this module.
//
//   setup:     SetWindows -> renderer.create -> [other modules hold this server, LoadAsset]
//              -> renderer.command_buffers ... renderer.destroy (before SDL destroys the windows)
//   per frame: Tick -> SubmitFrame -> Draw
//
// Message in:  {"id":3, "from":"ui", "to":"peregrinev", "op":"device.info"}
// Reply out:   {"id":3, "from":"peregrinev", "to":"ui", "op":"device.info", "ok":true, "error":"", "data":{"gpu":"..."}}
//
// ops:
//   renderer.create           { shader_root } -> data { device_id }
//   renderer.command_buffers
//   renderer.destroy
//   device.info               -> data { gpu, ray_query, mesh_shading }
//   pipelines.manifest        { path } -- reloads the pipeline manifest from path (a bad file fails, the old one stays)
// =============================================================================================================================================
class Ir77Server : public Ir77Enlisted, public std::enable_shared_from_this<Ir77Server> {
   public:
    Ir77Server() {
        m_init = std::static_pointer_cast<IIr77PVInit>(std::make_shared<Ir77PVInit>());

        if (m_empty_parser.parse(std::string_view{"{}"}).get(m_empty_args) != simdjson::SUCCESS)  // what a handler sees when a message has no "args"
            throw std::runtime_error{"Ir77Server: empty args"};

        RegisterPipeOperations();
    }

   public:
    std::shared_ptr<IIr77Return const> EnlistedAs(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIIr77Enlisted>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> MemberUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIIr77Server>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CollectionUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIIr77PeregrineV>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    Ir77GUID QueryInterface(Ir77GUID iid, std::shared_ptr<void>& obj) {
        if (iid == GUIDIIr77Enlisted)
            obj = std::shared_ptr<IIr77Enlisted>(shared_from_this(), static_cast<IIr77Enlisted*>(this));

        else if (iid == GUIDIIr77Server)
            obj = std::shared_ptr<Ir77Server>(shared_from_this(), static_cast<Ir77Server*>(this));

        else
            return GUIDQueryFailed;

        return GUIDQuerySucceeded;
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // direct links -- setup
    // -------------------------------------------------------------------------------------------------------------------------------------

    // The windows the display device presents to -- before renderer.create
   public:
    std::shared_ptr<IIr77Return const> SetWindows(std::vector<SDL_Window*> const& windows) { return m_init->SetWindows(windows); }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // forwarding -- the only way into the renderer for other modules. The peers stay inside; each call finds its peer through
    // Init, so a peer that isn't created yet (or was destroyed) answers NotConfigured instead of crashing.
    // -------------------------------------------------------------------------------------------------------------------------------------

    // ---- world -- assets once, then one frame at a time ------------------------------------------------------------------------------------

    // One asset (a glTF file) onto the device, blocking -- textures, materials, geometry, BLASes, its own range of models[] and joint
    // matrices. asset_id for SubmitFrame / UnloadAsset; report = what it became (path, runs, where it sits).
    std::shared_ptr<IIr77Return const> LoadAsset(Ir77PVAssetBundle const& bundle, std::uint32_t& asset_id, Ir77PVAssetReport& report) {
        std::shared_ptr<IIr77Return const> result{};

        std::shared_ptr<IIr77PVWorld> world{};
        if (Failed(m_init->GetWorld(world), result)) return result;

        return world->LoadAsset(bundle, asset_id, report);
    }

    // Its geometry, tables and BLASes freed; its draws go with the next SubmitFrame
    std::shared_ptr<IIr77Return const> UnloadAsset(std::uint32_t const& asset_id) {
        std::shared_ptr<IIr77Return const> result{};

        std::shared_ptr<IIr77PVWorld> world{};
        if (Failed(m_init->GetWorld(world), result)) return result;

        return world->UnloadAsset(asset_id);
    }

    // Once a frame, between Tick and Draw: cameras, lights, each asset's transforms and bones, the frame's own dispatches and draws.
    // An asset left out isn't drawn and isn't in the rays.
    std::shared_ptr<IIr77Return const> SubmitFrame(Ir77PVFrame const& frame, Ir77PVFrameReport& report) {
        std::shared_ptr<IIr77Return const> result{};

        std::shared_ptr<IIr77PVWorld> world{};
        if (Failed(m_init->GetWorld(world), result)) return result;

        return world->SubmitFrame(frame, report);
    }

    // ---- overlay -- the windows module's CEF paints ----------------------------------------------------------------------------------------

    // One CEF paint (BGRA, tightly packed) into window `window`'s overlay. Skipped by the overlay while it is resizing.
    std::shared_ptr<IIr77Return const> UploadOverlay(std::uint32_t const& window, void const* buffer, int const& width, int const& height) {
        std::shared_ptr<IIr77Return const> result{};

        std::shared_ptr<IIr77PVOverlay> overlay{};
        if (Failed(FindOverlay(window, overlay), result)) return result;

        return overlay->UploadFrame(buffer, width, height);
    }

    // Window `window`'s overlay is mid-rebuild -- a paint now would be lost
    std::shared_ptr<IIr77Return const> IsOverlayResizing(std::uint32_t const& window, bool& resizing) {
        std::shared_ptr<IIr77Return const> result{};

        std::shared_ptr<IIr77PVOverlay> overlay{};
        if (Failed(FindOverlay(window, overlay), result)) return result;

        return overlay->IsResizing(resizing);
    }

    // ---- runtime resources -- what a program compiled at runtime (the field, shader nodes) draws with -------------------------------------

    // GLSL text -> SPIR-V, through the shader compiler (glslangValidator, on PATH). The text is written to <shader root>/generated/<name> and
    // compiled with the shader root on the include path, so it can #include "include/bindless.glsl". name is the registry name to be, so it
    // ends in its stage ("field.comp"). errors = the compiler's ERROR / WARNING lines, with the file path cut down to the name. The device's
    // defines (IR77_RAY_SHADOWS on a ray-query device) are added for you. Blocking (a compiler process) -- between frames, not every frame.
    std::shared_ptr<IIr77Return const> CompileShader(std::string const& name, std::string const& text, std::vector<std::uint32_t>& spirv, std::string& errors) {
        std::shared_ptr<IIr77Return const> result{};

        std::shared_ptr<IIr77PVShader> shader{};
        if (Failed(m_init->GetShader(shader), result)) return result;

        return shader->CompileShader(name, text, spirv, errors);
    }

    // Puts SPIR-V into the registry under name -- replacing the shader of that name (same handle) or adding it -- then rebuilds every pipeline
    // that used it and reloads the manifest, so pipelines naming a shader that wasn't registered before resolve now. Waits for the device to
    // go idle first: the old pipelines may still be recording. A shader that breaks the contract fails and changes nothing.
    std::shared_ptr<IIr77Return const> ReloadShader(std::string const& name, std::vector<std::uint32_t> const& spirv) {
        return m_init->ReloadShader(name, spirv);
    }

    // The handle a draw item carries for a manifest pipeline ("water") -- look it up once, reuse it for every item. Needs CreatePipelines.
    std::shared_ptr<IIr77Return const> FindPipeline(std::string const& name, Ir77PVPipelineHandle& handle) { return m_init->FindPipeline(name, handle); }

    // The handle for an engine role -- follows the manifest's "roles"
    std::shared_ptr<IIr77Return const> FindPipeline(Ir77PVPipelineKind const& kind, Ir77PVPipelineHandle& handle) { return m_init->FindPipeline(kind, handle); }

    // size bytes, filled from data (zeros when data is null). Blocking upload. buffer_id for Upload / Destroy; slot for the shaders.
    std::shared_ptr<IIr77Return const> CreateStorageBuffer(VkDeviceSize const& size, void const* data, std::uint32_t& buffer_id, std::uint32_t& slot) {
        std::shared_ptr<IIr77Return const> result{};

        std::shared_ptr<IIr77PVTransfer> transfer{};
        if (Failed(m_init->GetTransfer(transfer), result)) return result;

        return transfer->CreateStorageBuffer(size, data, buffer_id, slot);
    }

    // CPU rewrite of [offset, offset + size) -- resets, spawn parameters. The copy runs in submission order ahead of later frames.
    std::shared_ptr<IIr77Return const> UploadStorageBuffer(std::uint32_t const& buffer_id, void const* data, VkDeviceSize const& size,
                                                           VkDeviceSize const& offset, bool const& wait) {
        std::shared_ptr<IIr77Return const> result{};

        std::shared_ptr<IIr77PVTransfer> transfer{};
        if (Failed(m_init->GetTransfer(transfer), result)) return result;

        return transfer->UploadStorageBuffer(buffer_id, data, size, offset, wait);
    }

    // Releases the slot and frees the buffer once no frame in flight can read it. Remove the draws / dispatches using it first.
    std::shared_ptr<IIr77Return const> DestroyStorageBuffer(std::uint32_t const& buffer_id) {
        std::shared_ptr<IIr77Return const> result{};

        std::shared_ptr<IIr77PVTransfer> transfer{};
        if (Failed(m_init->GetTransfer(transfer), result)) return result;

        return transfer->DestroyStorageBuffer(buffer_id);
    }

    // A canvas -- size x size RGBA8, cleared to transparent, that compute writes (storage image slot) and fragment shaders sample (texture
    // slot). Stays in VK_IMAGE_LAYOUT_GENERAL for both; the Compute pass's barriers order the writes against the reads.
    std::shared_ptr<IIr77Return const> CreateCanvas(std::uint32_t const& size, std::uint32_t& texture_slot, std::uint32_t& image_slot) {
        std::shared_ptr<IIr77Return const> result{};

        std::shared_ptr<IIr77PVMaterial> materials{};
        if (Failed(m_init->GetMaterials(materials), result)) return result;

        return materials->CreateCanvas(size, texture_slot, image_slot);
    }

    // Draw item with no vertex buffer: vertex_count vertices (times instance_count) that the vertex shader builds from gl_VertexIndex /
    // gl_InstanceIndex and storage buffers. The pipeline's vertex shader must read no inputs. It carries the grey default material. Caller
    // fills user / mode / light_index; for a GPU-driven count set item.indirect (a storage buffer holding VkDrawIndirectCommands) and
    // item.indirect_count. Goes into a frame window's lists (Ir77PVFrameWindow).
    std::shared_ptr<IIr77Return const> MakeProceduralDrawItem(std::size_t const& window, Ir77PVPipelineHandle const& pipeline,
                                                              std::uint32_t const& vertex_count, std::uint32_t const& instance_count, Ir77PVDrawItem& item) {
        std::shared_ptr<IIr77Return const> result{};

        std::shared_ptr<IIr77PVWorld> world{};
        if (Failed(m_init->GetWorld(world), result)) return result;

        std::shared_ptr<IIr77PVGeometry> geometry{};
        if (Failed(m_init->GetGeometry(geometry), result)) return result;

        std::uint32_t material_id{0};
        world->GetDefaultMaterial(material_id);

        return geometry->MakeProceduralDrawItem(window, pipeline, material_id, vertex_count, instance_count, item);
    }

    // A surface in the rays, rebuilt every frame from two storage buffers (CreateStorageBuffer ids): vertex_buffer (positions = first 3 floats
    // per vertex_stride bytes, vertex_count of them, world space) and index_buffer (triangle_count uint32 triangles). surface = its handle.
    // A device without ray queries succeeds with surface = IR77PV_NO_SURFACE -- the calls below take it and do nothing.
    std::shared_ptr<IIr77Return const> CreateSurfaceAcceleration(std::uint32_t const& vertex_buffer, std::uint32_t const& vertex_stride,
                                                                 std::uint32_t const& vertex_count, std::uint32_t const& index_buffer,
                                                                 std::uint32_t const& triangle_count, std::uint32_t& surface) {
        surface = IR77PV_NO_SURFACE;

        bool ray_query{false};
        m_init->GetRayQuery(ray_query);
        if (!ray_query) return Ir77RETURN<Ir77OperationSucceeded>();

        std::shared_ptr<IIr77Return const> result{};

        std::shared_ptr<IIr77PVAcceleration> acceleration{};
        if (Failed(m_init->GetAcceleration(acceleration), result)) return result;

        std::shared_ptr<IIr77PVBuffer> vertex{};
        std::shared_ptr<IIr77PVBuffer> index{};
        if (Failed(GetStorageBuffer(vertex_buffer, vertex), result)) return result;
        if (Failed(GetStorageBuffer(index_buffer, index), result)) return result;

        return acceleration->CreateSurface(vertex, vertex_stride, vertex_count, index, triangle_count, surface);
    }

    // In the rays or not, from the next frame
    std::shared_ptr<IIr77Return const> SetSurfaceAccelerationVisible(std::uint32_t const& surface, bool const& visible) {
        if (surface == IR77PV_NO_SURFACE) return Ir77RETURN<Ir77OperationSucceeded>();

        std::shared_ptr<IIr77Return const> result{};

        std::shared_ptr<IIr77PVAcceleration> acceleration{};
        if (Failed(m_init->GetAcceleration(acceleration), result)) return result;

        return acceleration->SetSurfaceVisible(surface, visible);
    }

    // Out of the rays for good (waits for the device) -- before destroying its storage buffers
    std::shared_ptr<IIr77Return const> ReleaseSurfaceAcceleration(std::uint32_t const& surface) {
        if (surface == IR77PV_NO_SURFACE) return Ir77RETURN<Ir77OperationSucceeded>();

        std::shared_ptr<IIr77Return const> result{};

        std::shared_ptr<IIr77PVAcceleration> acceleration{};
        if (Failed(m_init->GetAcceleration(acceleration), result)) return result;

        return acceleration->ReleaseSurface(surface);
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // frame loop -- direct, every frame
    // -------------------------------------------------------------------------------------------------------------------------------------

    // Uploads, indirect show/hide writes, graveyard -- before anything records
    std::shared_ptr<IIr77Return const> Tick() { return m_init->Tick(m_frame); }

    // Records and presents, then moves to the next frame in flight. Assumes Paint advances one frame in flight per Draw.
    std::shared_ptr<IIr77Return const> Draw() {
        std::shared_ptr<IIr77Return const> result{};

        std::shared_ptr<IIr77PVPaint> paint{};
        if (Failed(m_init->GetPaint(paint), result)) return result;

        paint->Draw();

        m_frame = (m_frame + 1) % MAX_FRAMES_IN_FLIGHT;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // pipe
    // -------------------------------------------------------------------------------------------------------------------------------------

    // Where this module's replies and events go -- the router's queue
    void SetPost(std::function<void(std::string)> post) { m_post = std::move(post); }

    // One message from the router. Runs on the main thread, between frames.
    void Sink(std::string json) {
        simdjson::dom::element root{};
        if (m_parser.parse(json).get(root) != simdjson::SUCCESS) {
            Ir77RETURN<Ir77OperationFailed>(this, "Ir77Server: not JSON.");
            return;
        }

        // a reply to something this module sent -- never answered, or two modules would answer each other forever
        if (root["ok"].error() == simdjson::SUCCESS) {
            bool reply_ok{true};
            if (root["ok"].get(reply_ok) == simdjson::SUCCESS && !reply_ok) Ir77RETURN<Ir77OperationFailed>(this, "Ir77Server: reply Failed: " + json);
            return;
        }

        std::int64_t id{0};
        if (root["id"].get(id) != simdjson::SUCCESS) id = 0;

        std::string_view from{};
        if (root["from"].get(from) != simdjson::SUCCESS) from = {};

        std::string_view op{};
        if (root["op"].get(op) != simdjson::SUCCESS) op = {};

        simdjson::dom::object args{};
        if (root["args"].get(args) != simdjson::SUCCESS) args = m_empty_args;

        std::string data{"{}"};

        auto const handler = m_operations.find(op);
        bool const known = handler != m_operations.end();

        bool const ok = known && handler->second(args, data)->ID() == GUIDIr77OperationSucceeded;
        char const* const error = ok ? "" : known ? "operation Failed" : "unknown op";

        if (!ok) Ir77RETURN<Ir77OperationFailed>(this, std::string{"Ir77Server: "} + error + ": " + json);

        if (from.empty() || !m_post) return;  // nobody to answer

        m_post("{\"id\":" + std::to_string(id) + ",\"from\":\"peregrinev\",\"to\":\"" + std::string{from} + "\",\"op\":\"" + std::string{op} +
               "\",\"ok\":" + (ok ? "true" : "false") + ",\"error\":\"" + error + "\",\"data\":" + data + "}");
    }

   private:
    using Ir77PVArgs = simdjson::dom::object;

    // a handler fills `data` (a JSON object) for the reply
    using Ir77PVOperation = std::function<std::shared_ptr<IIr77Return const>(Ir77PVArgs const&, std::string&)>;

    // heterogeneous lookup -- find by the string_view out of the parser, no std::string per message
    struct Ir77PVOpHash {
        using is_transparent = void;
        std::size_t operator()(std::string_view const& op) const { return std::hash<std::string_view>{}(op); }
    };

    // A storage buffer itself (CreateStorageBuffer id) -- what CreateSurfaceAcceleration hands Acceleration
    std::shared_ptr<IIr77Return const> GetStorageBuffer(std::uint32_t const& buffer_id, std::shared_ptr<IIr77PVBuffer>& buffer) {
        std::shared_ptr<IIr77Return const> result{};

        std::shared_ptr<IIr77PVTransfer> transfer{};
        if (Failed(m_init->GetTransfer(transfer), result)) return result;

        return transfer->GetStorageBuffer(buffer_id, buffer);
    }

    // Window `window`'s overlay, through Init -> CEF
    std::shared_ptr<IIr77Return const> FindOverlay(std::uint32_t const& window, std::shared_ptr<IIr77PVOverlay>& overlay) {
        std::shared_ptr<IIr77Return const> result{};

        std::shared_ptr<IIr77PVCEF> cef{};
        if (Failed(m_init->GetCEF(cef), result)) return result;

        return cef->GetOverlay(window, overlay);
    }

    void RegisterPipeOperations() {
        RegisterLifetimeOperations();
        RegisterDeviceQueries();
        RegisterReloadOperations();
    }

    // Init's lifetime sequence -- Iridium sends these at startup / shutdown, in order
    void RegisterLifetimeOperations() {
        m_operations["renderer.create"] = [this](Ir77PVArgs const& args, std::string& data) -> std::shared_ptr<IIr77Return const> {
            std::string_view shader_root{};
            if (args["shader_root"].get(shader_root) != simdjson::SUCCESS)
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Server: renderer.create needs \"shader_root\".");

            std::uint64_t device_id{0};
            auto const result = m_init->CreateRenderer(std::string{shader_root}, device_id);

            data = "{\"device_id\":" + std::to_string(device_id) + "}";

            return result;
        };

        m_operations["renderer.command_buffers"] = [this](Ir77PVArgs const&, std::string&) { return m_init->CreateCommandBuffers(); };

        m_operations["renderer.destroy"] = [this](Ir77PVArgs const&, std::string&) { return m_init->DestroyRenderer(); };
    }

    void RegisterDeviceQueries() {
        m_operations["device.info"] = [this](Ir77PVArgs const&, std::string& data) {
            std::string gpu{};
            auto const result = m_init->GetDeviceName(gpu);

            bool ray_query{false};
            bool mesh_shading{false};
            m_init->GetRayQuery(ray_query);
            m_init->GetMeshShading(mesh_shading);

            data = "{\"gpu\":\"" + Escape(gpu) + "\",\"ray_query\":" + (ray_query ? "true" : "false") +
                   ",\"mesh_shading\":" + (mesh_shading ? "true" : "false") + "}";

            return result;
        };
    }

    // Reloads -- the manifest now; shaders reload through CompileShader / ReloadShader
    void RegisterReloadOperations() {
        m_operations["pipelines.manifest"] = [this](Ir77PVArgs const& args, std::string&) -> std::shared_ptr<IIr77Return const> {
            std::string_view path{};
            if (args["path"].get(path) != simdjson::SUCCESS) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Server: pipelines.manifest needs \"path\".");

            return m_init->SetPipelineManifest(std::string{path});
        };
    }

    // JSON string escaping for text that goes into a reply
    std::string Escape(std::string const& text) const {
        std::string out{};
        out.reserve(text.size());

        for (char const c : text) {
            if (c == '"' || c == '\\') out += '\\';
            if (static_cast<unsigned char>(c) < 0x20) continue;  // control characters dropped
            out += c;
        }

        return out;
    }

   private:
    std::shared_ptr<IIr77PVInit> m_init{};  // the lifetime controller -- owns and wires the peers and everything under them

    std::uint32_t m_frame{0};  // frame in flight, for Init::Tick

    std::function<void(std::string)> m_post{};

    simdjson::dom::parser m_parser{};

    simdjson::dom::parser m_empty_parser{};  // owns m_empty_args

    Ir77PVArgs m_empty_args{};

    std::unordered_map<std::string, Ir77PVOperation, Ir77PVOpHash, std::equal_to<>> m_operations{};
};

}  // namespace NSIr77PeregrineV