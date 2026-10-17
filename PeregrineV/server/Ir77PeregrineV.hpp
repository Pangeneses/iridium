#pragma once

#include <cstdint>
#include <cstdio>
#include <functional>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <SDL3/SDL_video.h>

#include <simdjson.h>

#include "../../Ir77RT/runtime/Ir77Return.hpp"

#include "./Ir77PVInit.hpp"
#include "./Ir77PVCEF.hpp"
#include "./Ir77PVPaint.hpp"
#include "./Ir77PVAsset.hpp"

#include "../types/Ir77PVTypes.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

// =============================================================================================================================================
// Ir77PeregrineV -- the renderer module's switchboard. Owns Ir77PVInit and the servers built on it (CEF overlay, Paint, Asset); they hold
// Init, nothing below holds this. The module's one door:
//
//   pipe:        Sink(json) in, the post function out (SetPost)
//   direct link: setup, the pointers other modules hold (overlay -> CEF, Asset / Paint -> Scene) and the frame loop (Tick / Draw)
//
//   setup:     CreateRenderer(windows, shader_root, device_id) -> [other modules wire GetOverlay / GetAsset / GetPaint, load content]
//              -> CreateCommandBuffers
//   per frame: Tick -> [scene hands its frame to Paint] -> Draw
//
// Message in:  {"id":3, "from":"ui", "to":"peregrinev", "op":"device.info"}
// Reply out:   {"id":3, "from":"peregrinev", "to":"ui", "op":"device.info", "ok":true, "error":"", "data":{"gpu":"..."}}
//
// ops:
//   device.info     -> data { gpu }
// =============================================================================================================================================
class Ir77PeregrineV {
   public:
    Ir77PeregrineV() {
        m_init = std::make_shared<Ir77PVInit>();
        m_cef = std::make_shared<Ir77PVCEF>();
        m_paint = std::make_shared<Ir77PVPaint>();
        m_asset = std::make_shared<Ir77PVAsset>();

        m_cef->SetPVInit(m_init);
        m_paint->SetPVInit(m_init);
        m_asset->SetPVInit(m_init);

        if (m_empty_parser.parse(std::string_view{"{}"}).get(m_empty_args) != simdjson::SUCCESS)  // what a handler sees when a message has no "args"
            throw std::runtime_error{"Ir77PeregrineV: empty args"};

        DefineOperations();
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // setup
    // -------------------------------------------------------------------------------------------------------------------------------------

    // Instance through pipelines, in the only order that works: device and frame resources (Init), the CEF overlays, the shaders (Asset),
    // then the pipelines that need them (Init). device_id names the display device -- the key for the caller's window map.
    std::shared_ptr<IIr77Return const> CreateRenderer(std::vector<SDL_Window*> const& windows, std::string const& shader_root, std::uint64_t& device_id) {
        std::shared_ptr<IIr77Return const> result{};

        auto const failed = [&result](std::shared_ptr<IIr77Return const> const& step) {
            result = step;
            return step->ID() != GUIDIr77OperationSucceeded;
        };

        // instance / device -- instance, enumeration and allocator report through the return log; the steps after them fail if they did
        m_init->CreateInstance();
        m_init->EnumeratePhysicalDevices();
        if (failed(m_init->CreateDisplayDevice(windows, device_id))) return result;

        m_windows.clear();
        m_windows.emplace(device_id, windows);

        if (failed(m_init->CreateSurfaces(m_windows))) return result;
        m_init->CreateAllocator();

        // layouts + passes (Main only; Shadow pipelines are skipped until CreateRenderPass(Shadow) is called)
        if (failed(m_init->CreateLayouts())) return result;
        if (failed(m_init->CreateRenderPasses())) return result;
        if (failed(m_init->CreateSwapchains())) return result;

        // frame resources -- the placeholder texture must exist before the descriptor sets and materials bind it
        if (failed(m_init->CreatePlaceholderTexture())) return result;
        if (failed(m_init->CreateFrameBuffers())) return result;
        if (failed(m_init->CreateDescriptorSets())) return result;

        // CEF overlay -- one per window, uses the CEF layout from CreateLayouts and each window's swapchain
        if (failed(m_cef->CreateOverlays())) return result;

        // pipelines need shaders, layouts and the Main pass
        m_asset->SetShaderRoot(shader_root);
        if (failed(m_asset->CreateShaders())) return result;

        std::vector<Ir77PVPipelineKind> kinds{};
        if (failed(m_asset->GetPipelineKinds(kinds))) return result;
        if (failed(m_init->CreatePipelines(kinds))) return result;

        return Ir77RETURN<Ir77OperationSucceeded>(nullptr, "Ir77PeregrineV: renderer created.");
    }

    // After content is loaded -- the command buffers wire the overlay and the pipelines
    std::shared_ptr<IIr77Return const> CreateCommandBuffers() { return m_paint->CreateCommandBuffers(); }

    // Reverse of creation -- before SDL destroys the windows
    std::shared_ptr<IIr77Return const> DestroyRenderer() {
        m_paint.reset();
        m_asset.reset();
        m_cef.reset();
        m_init.reset();

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // direct links -- pointers other modules hold, set once at init
    // -------------------------------------------------------------------------------------------------------------------------------------

    // Window `index`'s overlay -- CEF paints straight into it
    std::shared_ptr<IIr77Return const> GetOverlay(std::size_t const& index, std::shared_ptr<IIr77PVOverlay>& overlay) {
        return m_cef->GetOverlay(index, overlay);
    }

    // Uploads, materials, pipelines -- Scene's link for content
    std::shared_ptr<IIr77Return const> GetAsset(std::shared_ptr<Ir77PVAsset>& asset) {
        asset = m_asset;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Frame data and draw lists -- Scene's link for frames
    std::shared_ptr<IIr77Return const> GetPaint(std::shared_ptr<Ir77PVPaint>& paint) {
        paint = m_paint;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // frame loop -- direct, every frame
    // -------------------------------------------------------------------------------------------------------------------------------------

    // Uploads, indirect show/hide writes, graveyard -- before anything records
    std::shared_ptr<IIr77Return const> Tick() {
        m_asset->Tick(m_frame);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Records and presents, then moves to the next frame in flight. Assumes Paint advances one frame in flight per Draw.
    std::shared_ptr<IIr77Return const> Draw() {
        m_paint->Draw();

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
            std::printf("[WARN] peregrinev: not JSON: %s\n", json.c_str());
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
        char const* const error = ok ? "" : known ? "operation failed" : "unknown op";

        if (!ok) std::printf("[WARN] peregrinev: %s: %s\n", error, json.c_str());

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

    void DefineOperations() { DefineDeviceOperations(); }

    void DefineDeviceOperations() {
        m_operations["device.info"] = [this](Ir77PVArgs const&, std::string& data) {
            std::string gpu{};
            auto const result = m_asset->GetDeviceName(gpu);

            data = "{\"gpu\":\"" + Escape(gpu) + "\"}";

            return result;
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

    std::shared_ptr<Ir77PVInit> m_init{};

    std::shared_ptr<Ir77PVCEF> m_cef{};

    std::shared_ptr<Ir77PVPaint> m_paint{};

    std::shared_ptr<Ir77PVAsset> m_asset{};

    std::map<std::uint64_t, std::vector<SDL_Window*>> m_windows{};  // the display device's windows -- CreateSurfaces reads it

    std::uint32_t m_frame{0};  // frame in flight, for Asset::Tick

    std::function<void(std::string)> m_post{};

    simdjson::dom::parser m_parser{};

    simdjson::dom::parser m_empty_parser{};  // owns m_empty_args

    Ir77PVArgs m_empty_args{};

    std::unordered_map<std::string, Ir77PVOperation, Ir77PVOpHash, std::equal_to<>> m_operations{};
};

}  // namespace NSIr77PeregrineV