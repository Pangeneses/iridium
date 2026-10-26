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

// every module's switchboard is <module>/server/Ir77Server.hpp, class Ir77Server in the module's namespace -- always named qualified here,
// the using-directives below would make a bare Ir77Server ambiguous
#include "../Windows/server/Ir77Server.hpp"
#include "../PeregrineV/server/Ir77Server.hpp"
#include "../Scene/server/Ir77Server.hpp"

#include "./types/Ir77Config.hpp"

#include "../Scene/types/Ir77Stats.hpp"

using namespace NSIr77CEF;
using namespace NSIr77PeregrineV;
using namespace NSIr77Scene;

namespace NSIr77 {

class iridium {
   public:
    iridium() { m_ui = std::make_shared<NSIr77CEF::Ir77Server>(); }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // init
    // -------------------------------------------------------------------------------------------------------------------------------------
    // The command line -- before InitializePipeline. --config picks iridium.json; --url / --debug-port override it.
    void SetArgs(Ir77Args const& args) { m_args = args; }

    void InitializePipeline(int argc, char* argv[]) {
        Require(ConfigFromFile(), "Read iridium.json.");

        // Iridium's own sink first -- every setup command's reply comes back through it
        m_sinks["iridium"] = [this](std::string json) { DefineReply(json); };

        // the windows module's pipe -- page queries are posted from the moment CEF is up
        m_ui->SetPost([this](std::string json) { Post(std::move(json)); });
        m_sinks["windows"] = [this](std::string json) { m_ui->Sink(std::move(json)); };

        Require(DispatchOperation("windows", "page.configure",
                                  "{\"url\":\"" + Escape(m_config.url) + "\",\"debug_port\":" + std::to_string(m_config.debug_port) +
                                      ",\"width\":" + std::to_string(m_config.width) + ",\"height\":" + std::to_string(m_config.height) + "}"),
                "Configure pages.");

        m_ui->Initialize(argc, argv);  // a CEF subprocess leaves here -- everything below is the browser process only

        std::printf("%s\n", m_config_report.c_str());

        SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "x11");

        if (!SDL_Init(SDL_INIT_VIDEO)) {
            Ir77RETURN<Ir77OperationFailed>(nullptr, "SDL_Init failed: " + std::string{SDL_GetError()});
            throw std::runtime_error{"SDL_Init failed: " + std::string{SDL_GetError()}};
        }

        m_window_width = m_config.width;
        m_window_height = m_config.height;

        Require(m_ui->CreateDisplayWindow(m_config.title, m_config.width, m_config.height, m_window), "Create window.");

        std::vector<SDL_Window*> windows{m_window};

        // the renderer's pipe first -- it is set up through it
        m_peregrine = std::make_shared<NSIr77PeregrineV::Ir77Server>();
        m_peregrine->SetPost([this](std::string json) { Post(std::move(json)); });

        m_sinks["peregrinev"] = [this](std::string json) { m_peregrine->Sink(std::move(json)); };

        // renderer -- instance through pipelines. The windows are pointers: direct link, not args.
        Require(m_peregrine->SetWindows(windows), "Set windows.");
        Require(DispatchOperation("peregrinev", "renderer.create", "{\"shader_root\":\"" + Escape(m_config.shader_root) + "\"}"), "Create renderer.");

        simdjson::dom::element created{};
        if (m_reply_parser.parse(m_reply_data).get(created) != simdjson::SUCCESS || created["device_id"].get(m_device_id) != simdjson::SUCCESS)
            Require(Ir77RETURN<Ir77OperationFailed>(nullptr, "Iridium: renderer.create replied without device_id."), "Create renderer.");

        m_windows.emplace(m_device_id, windows);

        // direct links -- pointers, the only thing that can't go over the pipe: every other module reaches the renderer through its server
        m_ui->SetRenderer(m_peregrine);

        // the scene -- its door first, then set up through it like the renderer
        m_scene = std::make_shared<NSIr77Scene::Ir77Server>();
        m_scene->SetPost([this](std::string json) { Post(std::move(json)); });
        m_sinks["scene"] = [this](std::string json) { m_scene->Sink(std::move(json)); };

        m_scene->SetRenderer(m_peregrine);

        // the project the scene reads and writes (fields.json) -- <projects>/<startup.project>
        std::string const project = (std::filesystem::path{m_config.projects} / m_config.startup_project).lexically_normal().string();
        Require(DispatchOperation("scene", "scene.project", "{\"path\":\"" + Escape(project) + "\"}"), "Scene project.");
        Require(DispatchOperation("scene", "scene.view_size", ViewSizeArgs()), "Scene view size.");

        // the static runs' pipeline -- startup.pipeline from the manifest; unset -> the "static" role
        Require(DispatchOperation("scene", "scene.pipeline", "{\"name\":\"" + Escape(m_config.startup_pipeline) + "\"}"), "Scene pipeline.");

        // content -- the startup scene from iridium.json, until the Angular app opens projects itself
        Require(DispatchOperation("scene", "scene.create",
                                  "{\"path\":\"" + Escape(m_scene_path) + "\",\"use_mesh\":" + (m_config.mesh_path ? "true" : "false") + "}"),
                "Create scene.");

        // command buffers wire the overlay and the pipelines -- after the content
        Require(DispatchOperation("peregrinev", "renderer.command_buffers"), "Create command buffers.");

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

                    // no replies wanted -- delivered on the next IridiumStep, before the frame is built
                    Post("{\"to\":\"windows\",\"op\":\"page.view_size\",\"args\":{\"width\":" + std::to_string(m_window_width) +
                         ",\"height\":" + std::to_string(m_window_height) + "}}");

                    if (m_scene) Post("{\"to\":\"scene\",\"op\":\"scene.view_size\",\"args\":" + ViewSizeArgs() + "}");

                    break;
                }

                default:
                    m_ui->InputEvent(&event);
                    break;
            }
        }
    }

    // CEF -- pump, resize handshake, paints straight into the overlay; page queries are posted from in here
    void ChromeStep() { m_ui->Step(); }

    // The router. Everything posted since the last frame (page queries included) goes to the sink its "to" names, in arrival order, before
    // the frame is built.
    void IridiumStep() { EmptyQueue(); }

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

        m_scene->Update(dt);                                // animation, world transforms, bones, the controlled camera
        auto const framed = m_scene->CreateFrame(seconds);  // the frame's results -> Paint, direct
        if (framed->ID() != GUIDIr77OperationSucceeded && !m_upload_error_logged) {
            Ir77RETURN<Ir77OperationFailed>(nullptr, "[ERROR] Scene CreateFrame failed -- no draws this frame\n");
            m_upload_error_logged = true;
        }

        auto const draw_start = std::chrono::steady_clock::now();
        m_peregrine->Draw();
        auto const draw_end = std::chrono::steady_clock::now();

        UpdateFrameStats(dt, frame_start, draw_start, draw_end);
    }

    void Shutdown() {
        // the scene first -- it holds PeregrineV's server and must not call into it once the renderer is destroyed
        m_sinks.erase("scene");
        m_scene.reset();

        if (m_peregrine) DispatchOperation("peregrinev", "renderer.destroy");  // a failure is in the return log; shutdown carries on
        m_peregrine.reset();
        m_sinks.clear();

        // nothing routes to the pages any more -- close them and shut CEF down
        if (m_ui) m_ui->DestroyPages();

        // after the renderer -- its surfaces were on these windows
        if (m_ui) m_ui->DestroyWindows();
        m_window = nullptr;

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

    // Everything posted, to the sink its "to" names -- then whatever those sinks posted, until the queue is empty or ROUTE_PASSES runs out
    // (left over waits for the next frame). Each pass is swapped out under the lock, so worker threads can keep posting.
    void EmptyQueue() {
        for (std::uint32_t pass = 0; pass < ROUTE_PASSES; pass++) {
            std::vector<std::string> batch{};
            {
                std::lock_guard lock{m_post_mutex};
                batch.swap(m_posted);
            }

            if (batch.empty()) return;

            for (std::string& json : batch) {
                simdjson::dom::element root{};
                std::string_view to_view{};

                if (m_route_parser.parse(json).get(root) != simdjson::SUCCESS || root["to"].get(to_view) != simdjson::SUCCESS) {
                    Ir77RETURN<Ir77OperationFailed>(nullptr, "Iridium: router: no \"to\": " + json);
                    continue;
                }

                std::string const to{to_view};  // the parser is reused by the sink below

                // pages ("ui" and every "ui.<browser id>") belong to the windows module -- its sink picks the page
                auto const sink = m_sinks.find(to == "ui" || to.rfind("ui.", 0) == 0 ? std::string{"windows"} : to);
                if (sink == m_sinks.end()) {
                    Ir77RETURN<Ir77OperationFailed>(nullptr, "Iridium: router: no sink '" + to + "': " + json);
                    continue;
                }

                sink->second(std::move(json));
            }
        }

        std::lock_guard lock{m_post_mutex};
        if (!m_posted.empty())
            Ir77RETURN<Ir77OperationFailed>(nullptr, "Iridium: router still busy after " + std::to_string(ROUTE_PASSES) + " passes -- " +
                                                         std::to_string(m_posted.size()) + " wait a frame");
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // config
    // -------------------------------------------------------------------------------------------------------------------------------------
    // iridium.json -> m_config. --config=<path> names the file, otherwise <executable folder>/iridium.json. A missing file keeps the
    // built-in defaults; a malformed one stops startup. Relative paths resolve against the file's folder. --url / --debug-port override.
    // Prints nothing -- CEF subprocesses run this too; m_config_report is printed once, by the browser process.
    std::shared_ptr<IIr77Return const> ConfigFromFile() {
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

            std::string mode{"local"};
            read_string("/iridium/mode", mode);

            if (mode == "local")
                m_config.mode = Ir77ServerMode::Local;
            else if (mode == "render_host")
                m_config.mode = Ir77ServerMode::RenderHost;
            else if (mode == "render_client")
                m_config.mode = Ir77ServerMode::RenderClient;
            else
                return Ir77RETURN<Ir77NotConfigured>(nullptr,
                                                     "Iridium: " + file.string() + " -- \"iridium.mode\" must be local, render_host or render_client.");

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
            m_scene_path =
                (std::filesystem::path{m_config.projects} / m_config.startup_project / "Models" / m_config.startup_scene).lexically_normal().string();

        m_config_report +=
            "\n[CONFIG] ui " + m_config.url + ", shaders " + m_config.shader_root + ", scene " + (m_scene_path.empty() ? std::string{"(none)"} : m_scene_path);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // commands -- Iridium's own messages over the pipe
    // -------------------------------------------------------------------------------------------------------------------------------------

    // One command, answered before this returns -- startup and shutdown, where order matters. Posted, routed now (anything else queued
    // goes too), the reply's data left in m_reply_data.
    std::shared_ptr<IIr77Return const> DispatchOperation(std::string const& to, std::string const& op, std::string const& args = "{}") {
        std::int64_t const id = ++m_command_id;

        m_reply_id = 0;
        m_reply_ok = false;
        m_reply_error.clear();
        m_reply_data = "{}";

        Post("{\"id\":" + std::to_string(id) + ",\"from\":\"iridium\",\"to\":\"" + to + "\",\"op\":\"" + op + "\",\"args\":" + args + "}");

        EmptyQueue();

        if (m_reply_id != id) return Ir77RETURN<Ir77OperationFailed>(nullptr, "Iridium: no reply to " + op + ".");
        if (!m_reply_ok) return Ir77RETURN<Ir77OperationFailed>(nullptr, "Iridium: " + op + " failed: " + m_reply_error);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // The "iridium" sink -- a reply to one of DispatchOperation's commands
    void DefineReply(std::string const& json) {
        simdjson::dom::element root{};
        if (m_reply_parser.parse(json).get(root) != simdjson::SUCCESS) {
            Ir77RETURN<Ir77OperationFailed>(nullptr, "Iridium: reply not JSON: " + json);
            return;
        }

        std::int64_t id{0};
        if (root["id"].get(id) != simdjson::SUCCESS) return;  // an event -- nothing waits for it

        bool ok{false};
        if (root["ok"].get(ok) != simdjson::SUCCESS) ok = false;

        std::string_view error{};
        if (root["error"].get(error) != simdjson::SUCCESS) error = {};

        simdjson::dom::element data{};
        m_reply_data = root["data"].get(data) == simdjson::SUCCESS ? simdjson::minify(data) : std::string{"{}"};

        m_reply_id = id;
        m_reply_ok = ok;
        m_reply_error = std::string{error};
    }

    // scene.view_size's args -- the display device's windows and their size now
    std::string ViewSizeArgs() const {
        auto const windows = m_windows.find(m_device_id);
        std::size_t const count = windows == m_windows.end() ? 1 : windows->second.size();

        return "{\"windows\":" + std::to_string(count) + ",\"width\":" + std::to_string(m_window_width) + ",\"height\":" + std::to_string(m_window_height) +
               "}";
    }

    // JSON string escaping for args
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

    void Require(std::shared_ptr<IIr77Return const> const& result, char const* step = "") {
        if (result->ID() != GUIDIr77OperationSucceeded) {
            std::cerr << "Iridium: init step failed: " << step << "\n";
            throw std::runtime_error{step};
        }
    }

   private:
    std::shared_ptr<NSIr77CEF::Ir77Server> m_ui{};  // the windows module -- SDL windows, CEF, the pages

    std::shared_ptr<NSIr77PeregrineV::Ir77Server> m_peregrine{};  // the renderer module

    // one id for both the window map and SetCurrentDevice -- they must match
    std::uint64_t m_device_id{0};

    float m_vstep_seconds{0.0f};  // previous frame's clock, for the scene's dt

    // config -- iridium.json + the command line, read once by ConfigFromFile
    Ir77Args m_args{};

    Ir77Config m_config{};

    std::string m_config_report{};  // what ConfigFromFile found -- printed by the browser process only

    static constexpr std::int64_t CONFIG_VERSION{1};

    // scene
    std::string m_scene_path{};  // <projects>/<startup.project>/Models/<startup.scene>

    std::shared_ptr<NSIr77Scene::Ir77Server> m_scene{};  // the scene module

    bool m_upload_error_logged{false};

    Ir77FrameStats m_frame_stats{};  // refreshed every frame

    // router -- every module posts here; IridiumStep delivers by "to"
    std::mutex m_post_mutex{};

    std::vector<std::string> m_posted{};

    std::map<std::string, std::function<void(std::string)>> m_sinks{};

    static constexpr std::uint32_t ROUTE_PASSES{8};  // drain passes per frame -- a request / reply chain this deep finishes in one frame

    simdjson::dom::parser m_route_parser{};

    // DispatchOperation -- Iridium's own commands and the reply each waits for
    std::int64_t m_command_id{0};

    std::int64_t m_reply_id{0};

    bool m_reply_ok{false};

    std::string m_reply_error{};

    std::string m_reply_data{"{}"};

    simdjson::dom::parser m_reply_parser{};

    SDL_Window* m_window{nullptr};

    std::map<std::uint64_t, std::vector<SDL_Window*>> m_windows;

    bool running = true;

    int m_window_width{1280};

    int m_window_height{720};

    std::chrono::steady_clock::time_point m_start_time{std::chrono::steady_clock::now()};
};

}  // namespace NSIr77