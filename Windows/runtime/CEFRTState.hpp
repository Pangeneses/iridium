#pragma once

#include <limits.h>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <filesystem>
#include <iostream>

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

#include "../../Ir77RT/runtime/Ir77Return.hpp"

using namespace NSIr77RT;

namespace CEF {

// CEF process state. Configure with SetStartUrl / SetDebugPort / SetViewSize (defaults below), then Initialize -- which returns only in the
// browser process; a CEF subprocess throws "__cef_subprocess__:<exit code>" out of it.
class CEFRTState {
   public:
    CEFRTState() = default;

    // the page the browser opens
    void SetStartUrl(std::string const& url) { m_start_url = url; }

    // DevTools (chrome://inspect -> localhost:<port>); 0 turns it off
    void SetDebugPort(std::uint16_t const port) { m_debug_port = port; }

    // the view size CEF lays the first page out at -- the window's size
    void SetViewSize(int const width, int const height) {
        m_view_width = width;
        m_view_height = height;
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

        {
            int fd = ::open("/tmp/Ir77RetLog.txt", O_WRONLY | O_CREAT | O_APPEND, 0644);
            const char* msg = "[raw] main started\n";
            ::write(fd, msg, strlen(msg));
            ::close(fd);
        }
        m_render_handler = new CEFRenderHandler();
        m_render_handler->SetViewSize(m_view_width, m_view_height);

        if (!m_render_handler) {
            Ir77RETURN<Ir77OperationFailed>(nullptr, "CEF: CEFRenderHandler alloc failed");
            throw std::runtime_error{"CEF: CEFRenderHandler alloc failed"};
        }
        Ir77RETURN<Ir77OperationSucceeded>(nullptr, "CEF: CEFRenderHandler created");

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

    void DestroyRTState() {
        m_window = nullptr;

        m_app = nullptr;

        m_render_handler = nullptr;

        m_client = nullptr;

        m_browser = nullptr;
    }

   private:
    std::string m_start_url{"http://localhost:50003/landing"};

    std::uint16_t m_debug_port{9222};

    int m_view_width{1280};

    int m_view_height{720};

    SDL_Window* m_window = nullptr;

    CefRefPtr<CEFApp> m_app{nullptr};

    CefRefPtr<CEFRenderHandler> m_render_handler{nullptr};

    CefRefPtr<CEFClient> m_client{nullptr};

    CefRefPtr<CefBrowser> m_browser{nullptr};
};

}  // namespace CEF