#pragma once

#include <algorithm>
#include <cctype>
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
#include <SDL3/SDL_dialog.h>

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

#include "./services/Ir77Services.hpp"

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

        // MongoDB and Sapphire -- nothing runs until a project opens (project.open); the database is the project's own
        m_services.SetConfig(m_services_config);

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

        // projects -- the folders under "projects"; the landing page lists them and opens one (project.open), which points the scene at it and
        // starts its database
        m_sinks["project"] = [this](std::string json) { ProjectSink(json); };

        Require(DispatchOperation("scene", "scene.view_size", ViewSizeArgs()), "Scene view size.");

        // the static runs' pipeline -- startup.pipeline from the manifest; unset -> the "static" role
        Require(DispatchOperation("scene", "scene.pipeline", "{\"name\":\"" + Escape(m_config.startup_pipeline) + "\"}"), "Scene pipeline.");

        // content -- an empty scene. What goes in it is MongoDB's: startup.scene names a stored scene, and the UI opens it (Iridium never talks
        // to the database -- the dump hands the UI the name).
        Require(DispatchOperation("scene", "scene.create", "{\"path\":\"\",\"use_mesh\":" + std::string{m_config.mesh_path ? "true" : "false"} + "}"),
                "Create scene.");

        // startup.project set -> straight into it, as if the landing page had picked it; '' -> none until the user picks one
        if (!m_config.startup_project.empty())
            Require(DispatchOperation("project", "project.open", "{\"name\":\"" + Escape(m_config.startup_project) + "\"}"), "Open startup project.");

        // after the open -- its empty scene.create would clear it. The UI opens this stored scene of the startup project.
        Require(DispatchOperation("scene", "scene.startup", "{\"name\":\"" + Escape(m_config.startup_scene) + "\"}"), "Scene startup.");

        // command buffers wire the overlay and the pipelines -- after the content
        Require(DispatchOperation("peregrinev", "renderer.command_buffers"), "Create command buffers.");

        Ir77RETURN<Ir77OperationSucceeded>(nullptr, "Initialize adapter.");
    }

    bool IsRunning() { return running; }
    void WindowStep() {
        m_services.Check();  // ready / unexpected exits -- never blocks

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

        // last -- Sapphire, then mongod, each given a clean shutdown
        m_services.Stop();
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

            Ir77ReadServices(root, base, m_services_config);

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

        // startup.scene is a stored scene's name (MongoDB, the project's scenes) -- the UI opens it
        m_config_report += "\n[CONFIG] ui " + m_config.url + ", shaders " + m_config.shader_root + ", project " + m_config.startup_project + ", scene " +
                           (m_config.startup_scene.empty() ? std::string{"(none)"} : m_config.startup_scene);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // projects -- the "project" sink. A project is a folder under "projects": Models/ (its .glb files), mongodb/ (its database, made on first
    // open), fields.json.
    //   project.list                 -> data { projects: [{name}], open }
    //   project.create  name         -> a new folder with Models/ -- data { name }
    //   project.open    name | path  -> the scene points at it (empty scene), its mongod starts (the last project's stops), Sapphire too if it
    //                                   isn't running -- data { name, path }. name: a folder under "projects"; path: any folder
    //   project.pick    kind         -> the desktop's picker -- kind "folder" (default; starts in "projects") or "glb" (a .glb file; starts in
    //                                   the open project's Models/) -- data { path } ('' = cancelled). Answered when the user closes it, not on
    //                                   this frame
    //   project.close                -> its mongod stops, the scene empties
    //   project.current              -> data { name, path } ('' = none)
    // -------------------------------------------------------------------------------------------------------------------------------------
    void ProjectSink(std::string const& json) {
        simdjson::dom::element root{};
        if (m_project_parser.parse(json).get(root) != simdjson::SUCCESS) {
            Ir77RETURN<Ir77OperationFailed>(nullptr, "Iridium: project: not JSON: " + json);
            return;
        }

        std::int64_t id{0};
        bool const has_id = root["id"].get(id) == simdjson::SUCCESS;

        std::string_view from_view{};
        std::string_view op_view{};
        std::string_view name_view{};
        std::string_view path_view{};
        std::string_view kind_view{};
        if (root["from"].get(from_view) != simdjson::SUCCESS) from_view = {};          // no "from" -> no reply
        if (root["op"].get(op_view) != simdjson::SUCCESS) op_view = {};                // no "op" -> "no op ''"
        if (root.at_pointer("/args/name").get(name_view) != simdjson::SUCCESS) name_view = {};  // ops without a name
        if (root.at_pointer("/args/path").get(path_view) != simdjson::SUCCESS) path_view = {};  // project.open by folder
        if (root.at_pointer("/args/kind").get(kind_view) != simdjson::SUCCESS) kind_view = {};  // project.pick: folder / glb

        std::string const from{from_view};
        std::string const op{op_view};
        std::string const name{name_view};
        std::string const path{path_view};

        // the picker answers later, from its own callback
        if (op == "project.pick") {
            Pick(has_id ? id : 0, from, std::string{kind_view});
            return;
        }

        std::string data{"{}"};
        std::string error{};
        bool const ok = ProjectOperation(op, name, path, data, error);

        if (!ok) Ir77RETURN<Ir77OperationFailed>(nullptr, "Iridium: " + op + ": " + error);

        // a reply only when the message names who asked
        if (!has_id || from.empty()) return;

        PostProjectReply(id, from, op, ok, error, data);
    }

    void PostProjectReply(std::int64_t const id, std::string const& from, std::string const& op, bool const ok, std::string const& error,
                          std::string const& data) {
        Post("{\"id\":" + std::to_string(id) + ",\"from\":\"project\",\"to\":\"" + Escape(from) + "\",\"op\":\"" + Escape(op) + "\",\"ok\":" +
             (ok ? "true" : "false") + ",\"error\":\"" + Escape(error) + "\",\"data\":" + (ok ? data : std::string{"{}"}) + "}");
    }

    // The desktop's picker (SDL: the XDG portal, or zenity) over Iridium's window -- a folder (a project, starting in "projects") or a .glb
    // (a model, starting in the open project's Models/). SDL calls back -- maybe on another thread -- when the user picks or cancels; the reply
    // is posted from there (Post is thread-safe) and delivered next frame.
    struct Ir77Pick {
        iridium* owner{nullptr};
        std::int64_t id{0};
        std::string from{};
        std::string start{};  // where the picker opens -- kept alive until the callback
    };

    void Pick(std::int64_t const id, std::string const& from, std::string const& kind) {
        bool const model = kind == "glb";

        std::string start = m_config.projects;
        if (model && !m_project_path.empty()) start = (std::filesystem::path{m_project_path} / "Models").string();

        auto* const pick = new Ir77Pick{this, id, from, start};  // the callback owns it

        SDL_DialogFileCallback const answer = [](void* userdata, char const* const* filelist, int) {
            std::unique_ptr<Ir77Pick> const pick{static_cast<Ir77Pick*>(userdata)};
            if (pick->from.empty()) return;  // nobody asked for an answer

            if (!filelist) {
                pick->owner->PostProjectReply(pick->id, pick->from, "project.pick", false, std::string{"picker: "} + SDL_GetError(), "{}");
                return;
            }

            std::string const path = filelist[0] ? std::string{filelist[0]} : std::string{};  // nothing picked = cancelled
            pick->owner->PostProjectReply(pick->id, pick->from, "project.pick", true, "", "{\"path\":\"" + pick->owner->Escape(path) + "\"}");
        };

        char const* const location = pick->start.empty() ? nullptr : pick->start.c_str();

        if (model) {
            static SDL_DialogFileFilter const filters[] = {{"glTF binary (.glb)", "glb"}};  // static -- SDL reads it until the callback
            SDL_ShowOpenFileDialog(answer, pick, m_window, filters, 1, location, false);
        } else {
            SDL_ShowOpenFolderDialog(answer, pick, m_window, location, false);
        }
    }

    bool ProjectOperation(std::string const& op, std::string const& name, std::string const& path, std::string& data, std::string& error) {
        std::error_code ec{};
        std::filesystem::path const projects{m_config.projects};

        if (op == "project.list") {
            std::vector<std::string> names{};
            for (auto const& entry : std::filesystem::directory_iterator(projects, ec)) {
                std::string const folder = entry.path().filename().string();
                if (entry.is_directory(ec) && !folder.empty() && folder[0] != '.') names.push_back(folder);
            }
            std::sort(names.begin(), names.end());

            std::string list{};
            for (std::string const& n : names) list += (list.empty() ? "" : ",") + std::string{"{\"name\":\""} + Escape(n) + "\"}";

            data = "{\"projects\":[" + list + "],\"open\":\"" + Escape(m_project) + "\"}";
            return true;
        }

        if (op == "project.current") {
            data = "{\"name\":\"" + Escape(m_project) + "\",\"path\":\"" + Escape(m_project_path) + "\"}";
            return true;
        }

        if (op == "project.create") {
            if (!ValidProjectName(name, error)) return false;

            std::filesystem::path const folder = projects / name;
            if (std::filesystem::exists(folder, ec)) {
                error = "a project named '" + name + "' already exists";
                return false;
            }

            if (!std::filesystem::create_directories(folder / "Models", ec)) {
                error = "can't make " + folder.string() + " (" + ec.message() + ")";
                return false;
            }

            data = "{\"name\":\"" + Escape(name) + "\"}";
            return true;
        }

        if (op == "project.open") {
            // by path -- any folder (the picker's answer); by name -- a folder under "projects"
            std::filesystem::path folder{};

            if (!path.empty()) {
                folder = std::filesystem::absolute(path, ec).lexically_normal();
                if (folder.has_filename() == false) folder = folder.parent_path();  // a trailing '/'
            } else {
                if (!ValidProjectName(name, error)) return false;
                folder = (projects / name).lexically_normal();
            }

            if (!std::filesystem::is_directory(folder, ec)) {
                error = "no project folder " + folder.string();
                return false;
            }

            if (!m_services.OpenProject(folder, error)) return false;

            m_project = folder.filename().string();
            m_project_path = folder.string();

            // delivered before this reply -- the page's next message finds the scene on the new project, empty
            Post("{\"to\":\"scene\",\"op\":\"scene.project\",\"args\":{\"path\":\"" + Escape(folder.string()) + "\"}}");
            Post("{\"to\":\"scene\",\"op\":\"scene.create\",\"args\":{\"path\":\"\",\"lights\":false,\"cameras\":false}}");

            data = "{\"name\":\"" + Escape(m_project) + "\",\"path\":\"" + Escape(m_project_path) + "\"}";
            return true;
        }

        if (op == "project.close") {
            m_services.CloseProject();
            m_project.clear();
            m_project_path.clear();

            Post("{\"to\":\"scene\",\"op\":\"scene.create\",\"args\":{\"path\":\"\",\"lights\":false,\"cameras\":false}}");
            Post("{\"to\":\"scene\",\"op\":\"scene.project\",\"args\":{\"path\":\"\"}}");
            return true;
        }

        error = "no op '" + op + "'";
        return false;
    }

    // a folder name under "projects" -- letters, digits, space, _ . - ; never a path
    bool ValidProjectName(std::string const& name, std::string& error) const {
        bool const ok = !name.empty() && name != "." && name != ".." && name.size() <= 64 &&
                        std::all_of(name.begin(), name.end(), [](char const c) { return std::isalnum(static_cast<unsigned char>(c)) || c == ' ' || c == '_' || c == '.' || c == '-'; });

        if (!ok) error = "a project name is letters, digits, spaces, _ . - (1 to 64)";
        return ok;
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

    // MongoDB and Sapphire -- iridium.json "services"; started after CEF's split, stopped last in Shutdown
    Ir77ServicesConfig m_services_config{};

    Ir77Services m_services{};

    // the open project -- its folder's name and the folder; '' = none
    std::string m_project{};

    std::string m_project_path{};

    simdjson::dom::parser m_project_parser{};

    std::string m_config_report{};  // what ConfigFromFile found -- printed by the browser process only

    static constexpr std::int64_t CONFIG_VERSION{1};

    // scene
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