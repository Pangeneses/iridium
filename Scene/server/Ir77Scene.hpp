#pragma once

#include <cstdint>
#include <cstdio>
#include <functional>
#include <stdexcept>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

#include <simdjson.h>

#include "../../Ir77RT/runtime/Ir77Return.hpp"

#include "./Ir77Animate.hpp"

using namespace NSIr77RT;

namespace NSIr77Scene {

// =============================================================================================================================================
// Ir77Scene -- the scene module's switchboard. Owns Ir77Animate (the work) and is the module's one door:
//
//   pipe:        Sink(json) in, the post function out (SetPost). Every command arrives here as JSON and every reply leaves the same way.
//   direct link: setup and the per-frame calls -- the Asset / Paint pointers and Update / CreateFrame can't travel as JSON.
//
// Message in:  {"id":7, "from":"ui", "to":"scene", "op":"camera.move", "args":{"right":1, "up":0, "forward":0}}
// Reply out:   {"id":7, "from":"scene", "to":"ui", "op":"camera.move", "ok":true, "error":""}   (only when the message names a "from")
//
// ops:
//   camera.move     right / up / forward in -1..1 -- held keys, applied every frame until set back to 0
//   camera.look     dx / dy in pixels             -- turns the fly camera / swings the orbit camera
//   camera.zoom     delta in wheel steps          -- orbit distance, 10% per step
//   camera.speed    value                         -- multiplies move speed
//   camera.mode     value "orbit" | "fly" | "file"
//   camera.select   index                         -- == camera count -> the controlled camera
//   camera.next                                   -- next file camera, then the controlled camera
//   camera.reset                                  -- orbit framed on the scene
//   animation.play  name, loop (true), speed (1)
//   animation.stop  name
//   animation.stop_all
//
// Ops live in a table (op -> handler), filled once by DefineOperations -- one lookup per message however many there are. A new op is one
// line in the group it belongs to; a new group is one more Define*Operations.
// =============================================================================================================================================
class Ir77Scene {
   public:
    Ir77Scene() {
        m_animate = std::make_shared<Ir77Animate>();

        if (m_empty_parser.parse(std::string_view{"{}"}).get(m_empty_args) != simdjson::SUCCESS)  // what a handler sees when a message has no "args"
            throw std::runtime_error{"Ir77Scene: empty args"};

        DefineOperations();
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
            std::printf("[WARN] scene: not JSON: %s\n", json.c_str());
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

        auto const handler = m_operations.find(op);
        bool const known = handler != m_operations.end();

        bool const ok = known && handler->second(args)->ID() == GUIDIr77OperationSucceeded;
        char const* const error = ok ? "" : known ? "operation failed" : "unknown op";

        if (!ok) std::printf("[WARN] scene: %s: %s\n", error, json.c_str());

        if (from.empty() || !m_post) return;  // nobody to answer

        m_post("{\"id\":" + std::to_string(id) + ",\"from\":\"scene\",\"to\":\"" + std::string{from} + "\",\"op\":\"" + std::string{op} +
               "\",\"ok\":" + (ok ? "true" : "false") + ",\"error\":\"" + error + "\"}");
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // direct link -- setup (Scene -> PeregrineV) and the frame loop
    // -------------------------------------------------------------------------------------------------------------------------------------
    std::shared_ptr<IIr77Return const> SetAsset(std::shared_ptr<Ir77PVAsset> const& asset) { return m_animate->SetAsset(asset); }

    std::shared_ptr<IIr77Return const> SetPaint(std::shared_ptr<Ir77PVPaint> const& paint) { return m_animate->SetPaint(paint); }

    std::shared_ptr<IIr77Return const> SetViewSize(std::size_t const& windows, int const& width, int const& height) {
        return m_animate->SetViewSize(windows, width, height);
    }

    std::shared_ptr<IIr77Return const> SetPipeline(Ir77PVPipelineHandle const& pipeline) { return m_animate->SetPipeline(pipeline); }

    std::shared_ptr<IIr77Return const> CreateScene(std::string const& path, bool const& use_mesh) { return m_animate->CreateScene(path, use_mesh); }

    std::shared_ptr<IIr77Return const> Update(float const& dt) { return m_animate->Update(dt); }

    std::shared_ptr<IIr77Return const> CreateFrame(float const& seconds) { return m_animate->CreateFrame(seconds); }

    std::shared_ptr<IIr77Return const> WriteFrameStats(Ir77FrameStats& stats) const { return m_animate->WriteFrameStats(stats); }

    std::shared_ptr<IIr77Return const> GetSceneStats(Ir77SceneStats& stats) const { return m_animate->GetSceneStats(stats); }

    // F2 -- window input, not a message
    std::shared_ptr<IIr77Return const> NextCamera() { return m_animate->NextCamera(); }

   private:
    using Ir77SceneArgs = simdjson::dom::object;

    using Ir77SceneOperation = std::function<std::shared_ptr<IIr77Return const>(Ir77SceneArgs const&)>;

    // heterogeneous lookup -- find by the string_view out of the parser, no std::string per message
    struct Ir77SceneOpHash {
        using is_transparent = void;
        std::size_t operator()(std::string_view const& op) const { return std::hash<std::string_view>{}(op); }
    };

    void DefineOperations() {
        DefineCameraOperations();
        DefineAnimationOperations();
    }

    void DefineCameraOperations() {
        m_operations["camera.move"] = [this](Ir77SceneArgs const& a) {
            return m_animate->SetCameraMove(glm::vec3(Number(a, "right", 0.0f), Number(a, "up", 0.0f), Number(a, "forward", 0.0f)));
        };
        m_operations["camera.look"] = [this](Ir77SceneArgs const& a) { return m_animate->SetCameraLook(Number(a, "dx", 0.0f), Number(a, "dy", 0.0f)); };
        m_operations["camera.zoom"] = [this](Ir77SceneArgs const& a) { return m_animate->SetCameraZoom(Number(a, "delta", 0.0f)); };
        m_operations["camera.speed"] = [this](Ir77SceneArgs const& a) { return m_animate->SetCameraSpeed(Number(a, "value", 0.0f)); };
        m_operations["camera.mode"] = [this](Ir77SceneArgs const& a) { return m_animate->SetCameraMode(Text(a, "value")); };
        m_operations["camera.select"] = [this](Ir77SceneArgs const& a) {
            return m_animate->SelectCamera(static_cast<std::uint32_t>(Number(a, "index", 0.0f)));
        };
        m_operations["camera.next"] = [this](Ir77SceneArgs const&) { return m_animate->NextCamera(); };
        m_operations["camera.reset"] = [this](Ir77SceneArgs const&) { return m_animate->ResetCamera(); };
    }

    void DefineAnimationOperations() {
        m_operations["animation.play"] = [this](Ir77SceneArgs const& a) {
            return m_animate->PlayAnimation(std::string{Text(a, "name")}, Flag(a, "loop", true), Number(a, "speed", 1.0f));
        };
        m_operations["animation.stop"] = [this](Ir77SceneArgs const& a) { return m_animate->StopAnimation(std::string{Text(a, "name")}); };
        m_operations["animation.stop_all"] = [this](Ir77SceneArgs const&) { return m_animate->StopAllAnimations(); };
    }

    // args readers -- a missing or mistyped key gives the fallback
    float Number(Ir77SceneArgs const& args, char const* key, float const fallback) const {
        double value{0.0};
        return args[key].get(value) == simdjson::SUCCESS ? static_cast<float>(value) : fallback;
    }

    bool Flag(Ir77SceneArgs const& args, char const* key, bool const fallback) const {
        bool value{false};
        return args[key].get(value) == simdjson::SUCCESS ? value : fallback;
    }

    std::string_view Text(Ir77SceneArgs const& args, char const* key) const {
        std::string_view value{};
        return args[key].get(value) == simdjson::SUCCESS ? value : std::string_view{};
    }

    std::shared_ptr<Ir77Animate> m_animate{};

    std::function<void(std::string)> m_post{};

    simdjson::dom::parser m_parser{};

    simdjson::dom::parser m_empty_parser{};  // owns m_empty_args

    Ir77SceneArgs m_empty_args{};

    std::unordered_map<std::string, Ir77SceneOperation, Ir77SceneOpHash, std::equal_to<>> m_operations{};
};

}  // namespace NSIr77Scene