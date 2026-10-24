#pragma once

#include <limits.h>
#include <chrono>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <filesystem>
#include <iostream>
#include <thread>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include "include/cef_base.h"
#include "include/cef_browser.h"
#include "include/cef_app.h"

#include "include/cef_render_handler.h"
#include "include/internal/cef_ptr.h"
#include "include/internal/cef_string.h"

#include "./CEFApp.hpp"
#include "./CEFClient.hpp"
#include "./CEFRenderHandler.hpp"
#include "./CEFMessageLoop.hpp"

#include "../../PeregrineV/server/Ir77Server.hpp"

#include "../../Ir77RT/runtime/Ir77Return.hpp"

using namespace NSIr77RT;
using namespace NSIr77PeregrineV;

namespace NSIr77CEF {

// CEF process state. Configure with SetStartUrl / SetDebugPort / SetViewSize (defaults below), then Initialize -- which returns only in the
// browser process; a CEF subprocess throws "__cef_subprocess__:<exit code>" out of it.
//
// Direct link to PeregrineV: SetOverlay hands CEF the window's overlay, and every paint goes straight into it -- no router in between.
// CEFStep, once per loop, pumps CEF and runs the resize handshake.
class CEFRTState {
   public:
    CEFRTState() = default;

    // the page the browser opens
    void SetStartUrl(std::string const& url) { m_start_url = url; }

    // DevTools (chrome://inspect -> localhost:<port>); 0 turns it off
    void SetDebugPort(std::uint16_t const port) { m_debug_port = port; }

    // Before Initialize: the size CEF lays the first page out at. After: a resize request -- CEFStep sends it to CEF once CEF has painted
    // the last size it was given (or that paint timed out).
    void SetViewSize(int const width, int const height) {
        m_pending_width = width;
        m_pending_height = height;
        m_resize_pending = true;
    }

    // PeregrineV's server -- every paint goes into this browser's overlay through it. Held weakly: a renderer that's gone just drops paints.
    void SetRenderer(std::shared_ptr<NSIr77PeregrineV::Ir77Server> const& renderer) {
        m_renderer = renderer;
        m_cef_repaint = true;  // anything painted before the renderer existed was dropped
    }

    // Once per loop: the resize handshake, a repaint if one was dropped, then ~2 ms of CEF's message loop (paints arrive in here)
    void CEFStep() {
        DefineResize();

        auto const until = std::chrono::steady_clock::now() + std::chrono::milliseconds(2);

        do m_message_loop->CEFDoMessageLoop(m_browser);
        while (std::chrono::steady_clock::now() < until);
    }

    void Initialize(int argc, char* argv[]) {
        CefMainArgs cef_args(argc, argv);

        m_app = new CEFApp();  // the renderer subprocesses need it too -- CEFRenderProcessHandler installs window.cefQuery

        int exit_code = CefExecuteProcess(cef_args, m_app, nullptr);
        if (exit_code >= 0) throw std::runtime_error{"__cef_subprocess__:" + std::to_string(exit_code)};

        std::filesystem::path exe_dir = std::filesystem::canonical("/proc/self/exe").parent_path();
        std::cerr << "[DEBUG] exe_dir = " << exe_dir.string() << std::endl;
        std::cerr << "[DEBUG] resources_dir_path will be = " << exe_dir.string() << std::endl;

        CefSettings settings{};
        settings.remote_debugging_port = m_debug_port;
        settings.multi_threaded_message_loop = false;
        settings.no_sandbox = true;
        settings.windowless_rendering_enabled = true;
        CefString(&settings.root_cache_path).FromASCII("/tmp/iridium_cef_cache");
        CefString(&settings.resources_dir_path).FromString(exe_dir.string());
        CefString(&settings.locales_dir_path).FromString((exe_dir / "locales").string());

        settings.log_severity = LOGSEVERITY_WARNING;  // errors, crashes, GPU / renderer failures -- without the updater noise
        CefString(&settings.log_file).FromASCII("/tmp/cef_debug.log");

        std::filesystem::remove_all("/tmp/iridium_cef_cache");

        {
            int fd = ::open("/tmp/Ir77RetLog.txt", O_WRONLY | O_CREAT | O_APPEND, 0644);
            const char* msg = "[raw] main started\n";
            ::write(fd, msg, strlen(msg));
            ::close(fd);
        }

        if (!CefInitialize(cef_args, settings, m_app, nullptr)) {
            Ir77RETURN<Ir77OperationFailed>(nullptr, "CEF: CefInitialize failed");
            throw std::runtime_error{"CEF: CefInitialize failed"};
        }
        
        Ir77RETURN<Ir77OperationSucceeded>(nullptr, "CEF: CefInitialize succeeded");

        m_initialized = true;

        {
            int fd = ::open("/tmp/Ir77RetLog.txt", O_WRONLY | O_CREAT | O_APPEND, 0644);
            const char* msg = "[raw] main started\n";
            ::write(fd, msg, strlen(msg));
            ::close(fd);
        }
        // the first size is the one CEF starts at -- nothing to hand shake
        m_cef_width = m_pending_width;
        m_cef_height = m_pending_height;
        m_resize_pending = false;

        m_render_handler = new CEFRenderHandler();
        m_render_handler->SetViewSize(m_cef_width, m_cef_height);

        if (!m_render_handler) {
            Ir77RETURN<Ir77OperationFailed>(nullptr, "CEF: CEFRenderHandler alloc failed");
            throw std::runtime_error{"CEF: CEFRenderHandler alloc failed"};
        }
        Ir77RETURN<Ir77OperationSucceeded>(nullptr, "CEF: CEFRenderHandler created");

        // CEF paint -> overlay, direct. Paints arrive on this thread, inside CEFStep's message loop.
        m_render_handler->SetPaintCallback([this](void const* buffer, int const width, int const height) { DefinePaint(buffer, width, height); });

        m_message_loop = std::make_shared<CEFMessageLoop>();

        CefWindowInfo window_info{};
        window_info.SetAsWindowless(0);

        CefBrowserSettings browser_settings{};
        browser_settings.windowless_frame_rate = 60;

        m_client = new CEFClient(m_render_handler);

        m_browser = CefBrowserHost::CreateBrowserSync(window_info, m_client, m_start_url, browser_settings, nullptr, nullptr);

        if (!m_browser) {
            Ir77RETURN<Ir77OperationFailed>(nullptr, "CEF: CreateBrowserSync failed");
            throw std::runtime_error{"CEF: CreateBrowserSync failed"};
        }

        m_browser->GetHost()->WasResized();
        m_browser->GetHost()->Invalidate(PET_VIEW);

        Ir77RETURN<Ir77OperationSucceeded>(nullptr, "CEF: CreateBrowserSync succeeded");
    }

    CefRefPtr<CefBrowser> GetBrowser() { return m_browser; }

    CefRefPtr<CEFRenderHandler> GetRenderHandler() { return m_render_handler; }

    CefRefPtr<CEFClient> GetClient() { return m_client; }

    // Every page closed, then CEF shut down -- main thread, once the router is done with the pages. A page that won't close within
    // CEF_CLOSE_TIMEOUT_MS is reported and CEF is shut down anyway.
    std::shared_ptr<IIr77Return const> DestroyPages() {
        if (!m_initialized) return Ir77RETURN<Ir77OperationSucceeded>();

        if (m_client) m_client->SetPost({});  // a query sent while closing has nowhere to go -- the page gets "not connected"

        if (m_browser) m_browser->GetHost()->CloseBrowser(true);  // force: no beforeunload prompt

        auto const until = std::chrono::steady_clock::now() + std::chrono::milliseconds(CEF_CLOSE_TIMEOUT_MS);

        while (m_client && m_client->OpenBrowsers() > 0 && std::chrono::steady_clock::now() < until) {
            CefDoMessageLoopWork();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        bool const closed = !m_client || m_client->OpenBrowsers() == 0;

        DestroyRTState();  // no references into CEF may outlive CefShutdown

        CefShutdown();
        m_initialized = false;

        if (!closed) return Ir77RETURN<Ir77OperationFailed>(nullptr, "CEF: a page did not close in time -- shut down anyway");

        return Ir77RETURN<Ir77OperationSucceeded>(nullptr, "CEF: pages closed, CEF shut down");
    }

    void DestroyRTState() {
        m_window = nullptr;

        m_renderer.reset();

        m_message_loop.reset();

        m_app = nullptr;

        m_render_handler = nullptr;

        m_client = nullptr;

        m_browser = nullptr;
    }

   private:
    // One paint. While a resize is in flight, a paint at any other size is stale -- dropped, and the last good frame stays up.
    void DefinePaint(void const* buffer, int const width, int const height) {
        auto const renderer = m_renderer.lock();
        if (!renderer) {
            m_cef_repaint = true;
            return;
        }

        // no overlay yet, or mid-rebuild -- this paint is lost; CEFStep asks for another
        bool resizing{false};
        if (renderer->IsOverlayResizing(m_overlay_window, resizing)->ID() != GUIDIr77OperationSucceeded || resizing) {
            m_cef_repaint = true;
            return;
        }

        if (m_cef_in_flight) {
            auto const waited = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - m_last_cef_resize).count();

            // stale size -- drop it, unless CEF has had long enough and still hasn't painted the requested size
            if ((width != m_cef_width || height != m_cef_height) && waited < CEF_RESIZE_TIMEOUT_MS) {
                m_cef_repaint = true;
                return;
            }

            m_cef_in_flight = false;  // CEF has caught up -- the next size may go
        }

        renderer->UploadOverlay(m_overlay_window, buffer, width, height);

        m_cef_repaint = false;
    }

    // The handshake -- the next size goes to CEF only after CEF has painted the last one (or it timed out). A dropped paint on a static page
    // is never repeated, so once nothing is in flight CEF is asked to paint again.
    void DefineResize() {
        auto const now = std::chrono::steady_clock::now();
        auto const since_applied = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_last_cef_resize).count();

        if (m_resize_pending && (!m_cef_in_flight || since_applied >= CEF_RESIZE_TIMEOUT_MS)) {
            m_resize_pending = false;

            if (m_pending_width != m_cef_width || m_pending_height != m_cef_height) {
                m_last_cef_resize = now;

                m_cef_width = m_pending_width;
                m_cef_height = m_pending_height;
                m_cef_in_flight = true;

                m_render_handler->SetViewSize(m_cef_width, m_cef_height);
                m_browser->GetHost()->WasResized();
                m_browser->GetHost()->Invalidate(PET_VIEW);
            }
        }

        auto const waited = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - m_last_cef_resize).count();

        if (m_cef_in_flight && waited >= CEF_RESIZE_TIMEOUT_MS) {
            m_cef_in_flight = false;
            m_cef_repaint = true;
        }

        if (m_cef_repaint && !m_cef_in_flight) {
            m_cef_repaint = false;
            m_browser->GetHost()->Invalidate(PET_VIEW);
        }
    }

    static constexpr long long CEF_RESIZE_TIMEOUT_MS{100};  // give up waiting for CEF's paint at the requested size after this long

    static constexpr long long CEF_CLOSE_TIMEOUT_MS{2000};  // DestroyPages: how long the pages get to close

    bool m_initialized{false};  // CefInitialize succeeded and CefShutdown hasn't run

    std::string m_start_url{"http://localhost:50003/landing"};

    std::uint16_t m_debug_port{9222};

    int m_cef_width{1280};  // the size CEF was last given

    int m_cef_height{720};

    int m_pending_width{1280};  // the size asked for

    int m_pending_height{720};

    bool m_resize_pending{false};

    bool m_cef_in_flight{false};  // a size was sent to CEF and its paint at that size hasn't arrived yet

    bool m_cef_repaint{false};

    std::chrono::steady_clock::time_point m_last_cef_resize{};

    std::weak_ptr<NSIr77PeregrineV::Ir77Server> m_renderer{};

    std::uint32_t m_overlay_window{0};  // the window whose overlay this browser paints into

    std::shared_ptr<CEFMessageLoop> m_message_loop{};

    SDL_Window* m_window = nullptr;

    CefRefPtr<CEFApp> m_app{nullptr};

    CefRefPtr<CEFRenderHandler> m_render_handler{nullptr};

    CefRefPtr<CEFClient> m_client{nullptr};

    CefRefPtr<CefBrowser> m_browser{nullptr};
};

}  // namespace NSIr77CEF