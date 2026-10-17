#pragma once

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <vector>

#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_vulkan.h>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <simdjson.h>

#include "../../Ir77RT/runtime/Ir77Return.hpp"

#include "../Windows/runtime/CEFRTState.hpp"
#include "../Windows/runtime/CEFClient.hpp"
#include "../Windows/runtime/CEFMouseEvent.hpp"

#include "../PeregrineV/server/Ir77PeregrineV.hpp"

#include "../PeregrineV/types/Ir77PVTypes.hpp"

#include "../Scene/server/Ir77Scene.hpp"

#include "./types/Ir77Config.hpp"

#include "../Scene/types/Ir77Stats.hpp"

using namespace CEF;
using namespace NSIr77PeregrineV;
using namespace NSIr77Scene;

namespace Ir77 {

class iridium {
   public:
    iridium() {
        m_rt_state = std::make_shared<CEFRTState>();
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // init
    // -------------------------------------------------------------------------------------------------------------------------------------
    // The command line -- before InitializePipeline. --config picks iridium.json; --url / --debug-port override it.
    void SetArgs(Ir77Args const& args) { m_args = args; }

    void InitializePipeline(int argc, char* argv[]) {
        Require(DefineConfig(), "Read iridium.json.");

        m_rt_state->SetStartUrl(m_config.url);
        m_rt_state->SetDebugPort(m_config.debug_port);
        m_rt_state->SetViewSize(m_config.width, m_config.height);

        m_rt_state->Initialize(argc, argv);  // a CEF subprocess leaves here -- everything below is the browser process only

        std::printf("%s\n", m_config_report.c_str());

        SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "x11");

        if (!SDL_Init(SDL_INIT_VIDEO)) {
            Ir77RETURN<Ir77OperationFailed>(nullptr, "SDL_Init failed: " + std::string{SDL_GetError()});
            throw std::runtime_error{"SDL_Init failed: " + std::string{SDL_GetError()}};
        }

        m_window_width = m_config.width;
        m_window_height = m_config.height;

        m_window = SDL_CreateWindow(m_config.title.c_str(), m_config.width, m_config.height, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);

        if (!m_window) {
            Ir77RETURN<Ir77OperationFailed>(nullptr, "SDL_CreateWindow failed: " + std::string{SDL_GetError()});
            throw std::runtime_error{"SDL_CreateWindow failed: " + std::string{SDL_GetError()}};
        }

        SDL_ShowWindow(m_window);

        SDL_StartTextInput(m_window);  // SDL_EVENT_TEXT_INPUT -> typed characters for the UI's inputs

        std::vector<SDL_Window*> windows{m_window};

        // renderer -- instance through pipelines
        m_peregrine = std::make_shared<Ir77PeregrineV>();
        Require(m_peregrine->CreateRenderer(windows, m_config.shader_root, m_device_id), "Create renderer.");
        m_windows.emplace(m_device_id, windows);

        // direct links -- CEF paints into the overlay; Scene uploads through Asset and hands frames to Paint
        std::shared_ptr<IIr77PVOverlay> overlay{};
        Require(m_peregrine->GetOverlay(0, overlay), "Get overlay.");
        m_rt_state->SetOverlay(overlay);

        std::shared_ptr<Ir77PVAsset> asset{};
        std::shared_ptr<Ir77PVPaint> paint{};
        Require(m_peregrine->GetAsset(asset), "Get asset.");
        Require(m_peregrine->GetPaint(paint), "Get paint.");

        // scene -- direct link to PeregrineV (Asset for uploads, Paint for frames)
        m_scene = std::make_shared<Ir77Scene>();
        m_scene->SetAsset(asset);
        m_scene->SetPaint(paint);
        m_scene->SetViewSize(m_windows.at(m_device_id).size(), m_window_width, m_window_height);

        // the static runs' pipeline -- startup.pipeline from the manifest; unset -> the scene uses the "static" role
        if (!m_config.startup_pipeline.empty()) {
            Ir77PVPipelineHandle pipeline{IR77PV_NO_PIPELINE};
            Require(asset->FindPipeline(m_config.startup_pipeline, pipeline), "Find startup.pipeline.");
            m_scene->SetPipeline(pipeline);
        }

        // content -- the startup scene from iridium.json, until the Angular app opens projects itself
        if (m_scene->CreateScene(m_scene_path, m_config.mesh_path)->ID() != GUIDIr77OperationSucceeded) {
            std::cerr << "Iridium: scene '" << m_scene_path << "' failed to load \n";
            throw std::runtime_error{"CreateScene failed"};
        }

        // command buffers wire the overlay and the pipelines -- after the content
        Require(m_peregrine->CreateCommandBuffers(), "Create command buffers.");

        // the pipe -- every module's door, and where each posts back to
        m_scene->SetPost([this](std::string json) { Post(std::move(json)); });
        m_peregrine->SetPost([this](std::string json) { Post(std::move(json)); });

        m_sinks["scene"] = [this](std::string json) { m_scene->Sink(std::move(json)); };
        m_sinks["peregrinev"] = [this](std::string json) { m_peregrine->Sink(std::move(json)); };
        m_sinks["ui"] = [this](std::string json) { DefineUIReply(json); };

        Ir77RETURN<Ir77OperationSucceeded>(nullptr, "Initialize adapter.");
    }

    bool IsRunning() { return running; }
    void WindowStep() {
        SDL_Event event;

        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_EVENT_QUIT:
                    running = false;
                    break;

                case SDL_EVENT_WINDOW_RESIZED:
                case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED: {
                    SDL_GetWindowSizeInPixels(m_window, &m_window_width, &m_window_height);

                    m_rt_state->SetViewSize(m_window_width, m_window_height);
                    if (m_scene) m_scene->SetViewSize(m_windows.at(m_device_id).size(), m_window_width, m_window_height);

                    break;
                }

                // F2 cycles file cameras, then the controlled camera -- kept from CEF
                case SDL_EVENT_KEY_DOWN: {
                    if (event.key.key == SDLK_F2 && !event.key.repeat) {
                        m_scene->NextCamera();
                        break;
                    }

                    CEFInputEvent(&event, m_rt_state->GetBrowser());
                    break;
                }

                case SDL_EVENT_MOUSE_WHEEL: {
                    float mouseX, mouseY;
                    SDL_GetMouseState(&mouseX, &mouseY);

                    CefMouseEvent cef_event;
                    cef_event.x = mouseX;
                    cef_event.y = mouseY;

                    const int scroll_scale = 120;
                    int deltaX = event.wheel.x * scroll_scale;
                    int deltaY = event.wheel.y * scroll_scale;

                    m_rt_state->GetBrowser()->GetHost()->SendMouseWheelEvent(cef_event, deltaX, deltaY);
                    break;
                }

                default:
                    CEFInputEvent(&event, m_rt_state->GetBrowser());
                    break;
            }
        }
    }

    // CEF -- pump, resize handshake, paints straight into the overlay
    void ChromeStep() { m_rt_state->CEFStep(); }

    // The router. Angular's window.cefQuery requests join the queue, then everything queued since the last frame goes to the sink its "to"
    // names, in arrival order, before the frame is built.
    void IridiumStep() {
        m_rt_state->GetClient()->TakeQueries(m_queries);

        for (std::string const& query : m_queries) Post(DefineEnvelope(query));

        DefineRoutes();
    }

    // Any module, any thread -- a message for the router. Delivered on the next IridiumStep.
    void Post(std::string json) {
        std::lock_guard lock{m_post_mutex};
        m_posted.push_back(std::move(json));
    }

    void VulkanStep() {
        auto const frame_start = std::chrono::steady_clock::now();
        float const seconds = std::chrono::duration<float>(frame_start - m_start_time).count();

        // uploads, indirect show/hide writes, graveyard -- before anything records
        m_peregrine->Tick();

        float const dt = m_vstep_seconds > 0.0f ? seconds - m_vstep_seconds : 0.0f;
        m_vstep_seconds = seconds;

        m_scene->Update(dt);                               // animation, world transforms, bones, the controlled camera
        auto const framed = m_scene->CreateFrame(seconds);  // the frame's results -> Paint, direct
        if (framed->ID() != GUIDIr77OperationSucceeded && !m_upload_error_logged) {
            std::printf("[ERROR] Scene CreateFrame failed -- no draws this frame\n");
            m_upload_error_logged = true;
        }

        auto const draw_start = std::chrono::steady_clock::now();
        m_peregrine->Draw();
        auto const draw_end = std::chrono::steady_clock::now();

        UpdateFrameStats(dt, frame_start, draw_start, draw_end);
    }

    void Shutdown() {
        m_scene.reset();

        if (m_peregrine) m_peregrine->DestroyRenderer();
        m_peregrine.reset();

        if (m_window) {
            SDL_DestroyWindow(m_window);
            m_window = nullptr;
        }

        SDL_Quit();
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // stats -- the loop's timings plus the scene's part (camera, draw counts, playing clips). Scene stats: m_scene->GetSceneStats.
    // -------------------------------------------------------------------------------------------------------------------------------------
    std::shared_ptr<IIr77Return const> GetFrameStats(Ir77FrameStats& stats) const {
        stats = m_frame_stats;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   private:
    // Once per frame, after Draw -- timings, smoothed fps, active camera, playing clips
    void UpdateFrameStats(float const& dt, std::chrono::steady_clock::time_point const& frame_start, std::chrono::steady_clock::time_point const& draw_start,
                          std::chrono::steady_clock::time_point const& draw_end) {
        auto const ms = [](auto const& from, auto const& to) { return std::chrono::duration<float, std::milli>(to - from).count(); };

        Ir77FrameStats& f = m_frame_stats;
        f.frame++;
        f.dt_ms = dt * 1000.0f;
        if (dt > 0.0f) f.fps = f.fps > 0.0f ? f.fps * 0.9f + (1.0f / dt) * 0.1f : 1.0f / dt;
        f.scene_ms = ms(frame_start, draw_start);
        f.draw_ms = ms(draw_start, draw_end);

        m_scene->WriteFrameStats(f);
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // router
    // -------------------------------------------------------------------------------------------------------------------------------------

    // Everything posted so far, to the sink its "to" names. Swapped out under the lock, so a sink posting while it runs lands in the next pass.
    void DefineRoutes() {
        std::vector<std::string> batch{};
        {
            std::lock_guard lock{m_post_mutex};
            batch.swap(m_posted);
        }

        for (std::string& json : batch) {
            simdjson::dom::element root{};
            std::string_view to_view{};

            if (m_route_parser.parse(json).get(root) != simdjson::SUCCESS || root["to"].get(to_view) != simdjson::SUCCESS) {
                std::printf("[WARN] router: no \"to\": %s\n", json.c_str());
                continue;
            }

            std::string const to{to_view};  // the parser is reused by the sink below

            auto const sink = m_sinks.find(to);
            if (sink == m_sinks.end()) {
                std::printf("[WARN] router: no sink '%s': %s\n", to.c_str(), json.c_str());
                continue;
            }

            sink->second(std::move(json));
        }
    }

    // Until the Angular side sends envelopes: the old {"camera":"move", ...} becomes {"from":"ui","to":"scene","op":"camera.move","args":{...}}.
    // A message that already has a "to" passes untouched.
    std::string DefineEnvelope(std::string const& query) {
        simdjson::dom::element root{};
        if (m_route_parser.parse(query).get(root) != simdjson::SUCCESS) return query;  // the router reports it

        std::string_view to{};
        if (root["to"].get(to) == simdjson::SUCCESS) return query;

        std::string_view camera{};
        if (root["camera"].get(camera) != simdjson::SUCCESS) return query;

        if (!m_query_seen) {
            std::printf("[UI] first query: %s\n", query.c_str());
            m_query_seen = true;
        }

        return "{\"from\":\"ui\",\"to\":\"scene\",\"op\":\"camera." + std::string{camera} + "\",\"args\":" + query + "}";
    }

    // Replies for the UI -- logged when they fail, until CEF answers cefQuery callbacks by id
    void DefineUIReply(std::string const& json) {
        simdjson::dom::element root{};
        bool ok{true};

        if (m_route_parser.parse(json).get(root) != simdjson::SUCCESS || root["ok"].get(ok) != simdjson::SUCCESS) return;
        if (!ok) std::printf("[WARN] ui reply: %s\n", json.c_str());
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // config
    // -------------------------------------------------------------------------------------------------------------------------------------
    // iridium.json -> m_config. --config=<path> names the file, otherwise <executable folder>/iridium.json. A missing file keeps the
    // built-in defaults; a malformed one stops startup. Relative paths resolve against the file's folder. --url / --debug-port override.
    // Prints nothing -- CEF subprocesses run this too; m_config_report is printed once, by the browser process.
    std::shared_ptr<IIr77Return const> DefineConfig() {
        std::error_code error{};

        std::filesystem::path const exe_dir = std::filesystem::canonical("/proc/self/exe", error).parent_path();
        std::filesystem::path const file = std::filesystem::absolute(m_args.config.empty() ? exe_dir / "iridium.json" : std::filesystem::path{m_args.config});
        std::filesystem::path const base = file.parent_path();

        if (std::filesystem::is_regular_file(file, error)) {
            simdjson::dom::parser parser{};
            simdjson::dom::element root{};

            if (parser.load(file.string()).get(root) != simdjson::SUCCESS)
                return Ir77RETURN<Ir77NotConfigured>(nullptr, "Iridium: " + file.string() + " is not valid JSON.");

            // a key that is missing (or the wrong type) keeps its default
            auto const read_string = [&root](char const* pointer, std::string& value) {
                std::string_view found{};
                if (root.at_pointer(pointer).get(found) == simdjson::SUCCESS) value = std::string{found};
            };
            auto const read_integer = [&root](char const* pointer, auto& value) {
                std::int64_t found{0};
                if (root.at_pointer(pointer).get(found) == simdjson::SUCCESS) value = static_cast<std::remove_reference_t<decltype(value)>>(found);
            };
            auto const read_bool = [&root](char const* pointer, bool& value) {
                bool found{false};
                if (root.at_pointer(pointer).get(found) == simdjson::SUCCESS) value = found;
            };

            std::int64_t version{CONFIG_VERSION};
            read_integer("/version", version);
            if (version != CONFIG_VERSION)
                return Ir77RETURN<Ir77NotConfigured>(nullptr, "Iridium: " + file.string() + " -- \"version\" must be " + std::to_string(CONFIG_VERSION) + ".");

            read_string("/window/title", m_config.title);
            read_integer("/window/width", m_config.width);
            read_integer("/window/height", m_config.height);

            read_string("/ui/url", m_config.url);
            read_integer("/ui/debug_port", m_config.debug_port);

            read_string("/render/shader_root", m_config.shader_root);
            read_bool("/render/mesh_path", m_config.mesh_path);

            std::string mode{"selfserve"};
            read_string("/iridium/mode", mode);

            if (mode == "selfserve") m_config.mode = Ir77ServerMode::SelfServe;
            else if (mode == "server") m_config.mode = Ir77ServerMode::Server;
            else if (mode == "client") m_config.mode = Ir77ServerMode::Client;
            else return Ir77RETURN<Ir77NotConfigured>(nullptr, "Iridium: " + file.string() + " -- \"iridium.mode\" must be selfserve, server or client.");

            read_string("/iridium/server/host", m_config.server_host);
            read_integer("/iridium/server/port", m_config.server_port);

            read_string("/projects", m_config.projects);

            read_string("/startup/project", m_config.startup_project);
            read_string("/startup/scene", m_config.startup_scene);
            read_string("/startup/pipeline", m_config.startup_pipeline);

            if (m_config.width <= 0 || m_config.height <= 0)
                return Ir77RETURN<Ir77NotConfigured>(nullptr, "Iridium: " + file.string() + " -- window width / height must be above 0.");

            m_config.path = file.string();
            m_config_report = "[CONFIG] " + file.string();
        } else {
            m_config_report = "[CONFIG] no " + file.string() + " -- built-in defaults";
        }

        // relative -> absolute, against the config file's folder
        auto const resolve = [&base](std::string& path) {
            if (!path.empty() && std::filesystem::path{path}.is_relative()) path = (base / path).lexically_normal().string();
        };

        resolve(m_config.shader_root);
        resolve(m_config.projects);

        // the command line wins
        if (!m_args.url.empty()) m_config.url = m_args.url;
        if (m_args.debug_port != 0) m_config.debug_port = m_args.debug_port;

        // <projects>/<startup.project>/Models/<startup.scene>
        if (!m_config.startup_scene.empty())
            m_scene_path = (std::filesystem::path{m_config.projects} / m_config.startup_project / "Models" / m_config.startup_scene).lexically_normal().string();

        m_config_report += "\n[CONFIG] ui " + m_config.url + ", shaders " + m_config.shader_root + ", scene " +
                           (m_scene_path.empty() ? std::string{"(none)"} : m_scene_path);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    void Require(std::shared_ptr<IIr77Return const> const& result, char const* step = "") {
        if (result->ID() != GUIDIr77OperationSucceeded) {
            std::cerr << "Iridium: init step failed: " << step << "\n";
            throw std::runtime_error{step};
        }
    }

   private:
    std::shared_ptr<CEFRTState> m_rt_state;

    std::shared_ptr<Ir77PeregrineV> m_peregrine{};

    // one id for both the window map and SetCurrentDevice -- they must match
    std::uint64_t m_device_id{0};

    float m_vstep_seconds{0.0f};  // previous frame's clock, for the scene's dt

    // config -- iridium.json + the command line, read once by DefineConfig
    Ir77Args m_args{};

    Ir77Config m_config{};

    std::string m_config_report{};  // what DefineConfig found -- printed by the browser process only

    static constexpr std::int64_t CONFIG_VERSION{1};

    // scene
    std::string m_scene_path{};  // <projects>/<startup.project>/Models/<startup.scene>

    std::shared_ptr<Ir77Scene> m_scene{};

    bool m_upload_error_logged{false};

    Ir77FrameStats m_frame_stats{};  // refreshed every frame

    // router -- every module posts here; IridiumStep delivers by "to"
    std::vector<std::string> m_queries{};  // cefQuery requests, taken from CEF each frame

    std::mutex m_post_mutex{};

    std::vector<std::string> m_posted{};

    std::map<std::string, std::function<void(std::string)>> m_sinks{};

    simdjson::dom::parser m_route_parser{};

    bool m_query_seen{false};  // [UI] first query logged

    SDL_Window* m_window{nullptr};

    std::map<std::uint64_t, std::vector<SDL_Window*>> m_windows;

    bool running = true;

    int m_window_width{1280};

    int m_window_height{720};

    std::chrono::steady_clock::time_point m_start_time{std::chrono::steady_clock::now()};
};

}  // namespace Ir77