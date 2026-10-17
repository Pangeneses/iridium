#pragma once

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <map>
#include <memory>
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

#include "../PeregrineV/server/Ir77PVInit.hpp"
#include "../PeregrineV/server/Ir77PVCEF.hpp"
#include "../PeregrineV/server/Ir77PVPaint.hpp"
#include "../PeregrineV/server/Ir77PVAsset.hpp"

#include "../PeregrineV/types/Ir77PVTypes.hpp"

#include "../Scene/server/Ir77Animate.hpp"

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

        m_vulkan = std::make_shared<Ir77PVInit>();
        m_cef = std::make_shared<Ir77PVCEF>();
        m_paint = std::make_shared<Ir77PVPaint>();
        m_asset = std::make_shared<Ir77PVAsset>();

        m_cef->SetPVInit(m_vulkan);
        m_paint->SetPVInit(m_vulkan);
        m_asset->SetPVInit(m_vulkan);

        // instance / device
        m_vulkan->CreateInstance();

        m_vulkan->EnumeratePhysicalDevices();
        Require(m_vulkan->CreateDisplayDevice(windows, m_device_id), "Create display device.");
        m_windows.emplace(m_device_id, windows);
        Require(m_vulkan->CreateSurfaces(m_windows), "Create surfaces.");
        m_vulkan->CreateAllocator();

        // layouts + passes (Main only; Shadow pipelines are skipped until CreateRenderPass(Shadow) is called)
        Require(m_vulkan->CreateLayouts());
        Require(m_vulkan->CreateRenderPasses());
        Require(m_vulkan->CreateSwapchains());

        // frame resources -- the placeholder texture must exist before the descriptor sets and materials bind it
        Require(m_vulkan->CreatePlaceholderTexture());
        Require(m_vulkan->CreateFrameBuffers());
        Require(m_vulkan->CreateDescriptorSets());

        // CEF overlay -- one per window, uses the CEF layout from CreateLayouts and each window's swapchain
        Require(m_cef->CreateOverlays());

        std::shared_ptr<IIr77PVOverlay> overlay{};
        Require(m_cef->GetOverlay(0, overlay), "Get overlay.");

        m_rt_state->SetOverlay(overlay);  // CEF paints go straight into the overlay

        // pipelines need shaders, layouts and the Main pass
        m_asset->SetShaderRoot(m_config.shader_root);
        Require(m_asset->CreateShaders(), "Create shaders.");

        std::vector<Ir77PVPipelineKind> kinds;
        Require(m_asset->GetPipelineKinds(kinds));

        Require(m_vulkan->CreatePipelines(kinds));

        // scene -- direct link to PeregrineV (Asset for uploads, Paint for frames)
        m_scene = std::make_shared<Ir77Animate>();
        m_scene->SetAsset(m_asset);
        m_scene->SetPaint(m_paint);
        m_scene->SetViewSize(m_windows.at(m_device_id).size(), m_window_width, m_window_height);

        // the static runs' pipeline -- startup.pipeline from the manifest; unset -> the scene uses the "static" role
        if (!m_config.startup_pipeline.empty()) {
            Ir77PVPipelineHandle pipeline{IR77PV_NO_PIPELINE};
            Require(m_asset->FindPipeline(m_config.startup_pipeline, pipeline), "Find startup.pipeline.");
            m_scene->SetPipeline(pipeline);
        }

        // content -- the startup scene from iridium.json, until the Angular app opens projects itself
        if (m_scene->CreateScene(m_scene_path, m_config.mesh_path)->ID() != GUIDIr77OperationSucceeded) {
            std::cerr << "Iridium: scene '" << m_scene_path << "' failed to load \n";
            throw std::runtime_error{"CreateScene failed"};
        }

        // command buffers wire the overlay, so they come after CreateBufferCEF and CreatePipelines
        Require(m_paint->CreateCommandBuffers());

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

    // Angular -> Iridium: every window.cefQuery request since the last frame, applied in arrival order before the frame is built
    void IridiumStep() {
        m_rt_state->GetClient()->TakeQueries(m_queries);

        for (std::string const& query : m_queries) ApplyQuery(query);
    }

    void VulkanStep() {
        auto const frame_start = std::chrono::steady_clock::now();
        float const seconds = std::chrono::duration<float>(frame_start - m_start_time).count();

        // uploads, indirect show/hide writes, graveyard -- before anything records
        m_asset->Tick(m_frame);

        float const dt = m_vstep_seconds > 0.0f ? seconds - m_vstep_seconds : 0.0f;
        m_vstep_seconds = seconds;

        m_scene->Update(dt);                               // animation, world transforms, bones, the controlled camera
        auto const framed = m_scene->CreateFrame(seconds);  // the frame's results -> Paint, direct
        if (framed->ID() != GUIDIr77OperationSucceeded && !m_upload_error_logged) {
            std::printf("[ERROR] Scene CreateFrame failed -- no draws this frame\n");
            m_upload_error_logged = true;
        }

        auto const draw_start = std::chrono::steady_clock::now();
        m_paint->Draw();
        auto const draw_end = std::chrono::steady_clock::now();

        UpdateFrameStats(dt, frame_start, draw_start, draw_end);

        // assumes Paint advances one frame-in-flight per Draw -- swap for Paint's own index if it exposes one
        m_frame = (m_frame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    void Shutdown() {
        m_scene.reset();
        m_paint.reset();
        m_asset.reset();
        m_cef.reset();
        m_vulkan.reset();

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
    // UI commands -- window.cefQuery requests from Angular, JSON: {"camera": "<command>", ...}
    //   move  forward / right / up in -1..1 -- held-key state, applied every frame (0 on key up)
    //   look  dx / dy in pixels             -- mouse drag; turns the fly camera / swings the orbit camera
    //   zoom  delta in wheel steps          -- orbit distance, 10% per step
    //   speed value                         -- multiplies move speed
    //   mode  value "orbit" | "fly" | "file" -- orbit / fly start from the current view, so switching never jumps
    //   next                                -- next file camera, then the controlled camera (F2)
    //   reset                               -- orbit framed on the scene
    // -------------------------------------------------------------------------------------------------------------------------------------
    void ApplyQuery(std::string const& query) {
        simdjson::dom::element root{};
        std::string_view command{};

        if (m_query_parser.parse(query).get(root) != simdjson::SUCCESS || root["camera"].get(command) != simdjson::SUCCESS) {
            std::printf("[WARN] UI query ignored: %s\n", query.c_str());
            return;
        }

        if (!m_query_seen) {
            std::printf("[UI] first query: %s\n", query.c_str());
            m_query_seen = true;
        }

        auto const number = [&root](char const* key) {
            double value{0.0};
            if (root[key].get(value) != simdjson::SUCCESS) value = 0.0;  // missing / not a number -> 0
            return static_cast<float>(value);
        };

        // camera commands go to the scene -- it owns the cameras (and takes over from a file camera on move / look / zoom)
        if (command == "move") {
            m_scene->SetCameraMove(glm::vec3(number("right"), number("up"), number("forward")));
        } else if (command == "look") {
            m_scene->SetCameraLook(number("dx"), number("dy"));
        } else if (command == "zoom") {
            m_scene->SetCameraZoom(number("delta"));
        } else if (command == "speed") {
            m_scene->SetCameraSpeed(number("value"));
        } else if (command == "mode") {
            std::string_view mode{};
            if (root["value"].get(mode) != simdjson::SUCCESS) mode = {};

            m_scene->SetCameraMode(mode);
        } else if (command == "next") {
            m_scene->NextCamera();
        } else if (command == "reset") {
            m_scene->ResetCamera();
        } else {
            std::printf("[WARN] UI camera command unknown: %s\n", query.c_str());
        }
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

    std::shared_ptr<Ir77PVInit> m_vulkan;

    std::shared_ptr<Ir77PVCEF> m_cef;

    std::shared_ptr<Ir77PVPaint> m_paint;

    std::shared_ptr<Ir77PVAsset> m_asset;

    // one id for both the window map and SetCurrentDevice -- they must match
    std::uint64_t m_device_id{0};

    std::uint32_t m_frame{0};  // frame in flight, for Asset::Tick

    float m_vstep_seconds{0.0f};  // previous frame's clock, for the scene's dt

    // config -- iridium.json + the command line, read once by DefineConfig
    Ir77Args m_args{};

    Ir77Config m_config{};

    std::string m_config_report{};  // what DefineConfig found -- printed by the browser process only

    static constexpr std::int64_t CONFIG_VERSION{1};

    // scene
    std::string m_scene_path{};  // <projects>/<startup.project>/Models/<startup.scene>

    std::shared_ptr<Ir77Animate> m_scene{};

    bool m_upload_error_logged{false};

    Ir77FrameStats m_frame_stats{};  // refreshed every frame

    // UI queries -- drained each frame by IridiumStep
    std::vector<std::string> m_queries{};

    simdjson::dom::parser m_query_parser{};

    bool m_query_seen{false};  // [UI] first query logged

    SDL_Window* m_window{nullptr};

    std::map<std::uint64_t, std::vector<SDL_Window*>> m_windows;

    bool running = true;

    int m_window_width{1280};

    int m_window_height{720};

    std::chrono::steady_clock::time_point m_start_time{std::chrono::steady_clock::now()};
};

}  // namespace Ir77