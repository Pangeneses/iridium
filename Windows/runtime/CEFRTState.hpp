#pragma once

#include <limits.h>
#include <stdexcept>

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

class CEFRTState {
   public:
    CEFRTState() = default;

    void Initialize(int argc, char* argv[]) {
        CefMainArgs cef_args(argc, argv);

        int exit_code = CefExecuteProcess(cef_args, nullptr, nullptr);
        if (exit_code >= 0) throw std::runtime_error{"__cef_subprocess__:" + std::to_string(exit_code)};

        CefSettings settings{};
        settings.multi_threaded_message_loop = false;
        settings.no_sandbox = true;
        settings.windowless_rendering_enabled = true;
        CefString(&settings.root_cache_path).FromASCII("/tmp/iridium_cef_cache");
        CefString(&settings.resources_dir_path).FromASCII("/home/alpha/workspace/cef/Release");
        CefString(&settings.locales_dir_path).FromASCII("/home/alpha/workspace/cef/Release/locales");

        m_app = new CEFApp();

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
        m_render_handler->SetSize(1280, 720);

        if (!m_render_handler) {
            Ir77RETURN<Ir77OperationFailed>(nullptr, "CEF: CEFRenderHandler alloc failed");
            throw std::runtime_error{"CEF: CEFRenderHandler alloc failed"};
        }
        Ir77RETURN<Ir77OperationSucceeded>(nullptr, "CEF: CEFRenderHandler created");

        CefWindowInfo window_info{};
        window_info.SetAsWindowless(0);

        CefBrowserSettings browser_settings{};
        browser_settings.windowless_frame_rate = 60;

        m_browser = CefBrowserHost::CreateBrowserSync(window_info, new CEFClient(m_render_handler), "https://cobalt.pangeneses.com/landing", browser_settings,
                                                      nullptr, nullptr);

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

    void DestroyRTState() {
        m_window = nullptr;

        m_app = nullptr;

        m_render_handler = nullptr;

        m_browser = nullptr;
    }

   private:
    SDL_Window* m_window = nullptr;

    CefRefPtr<CEFApp> m_app{nullptr};

    CefRefPtr<CEFRenderHandler> m_render_handler{nullptr};

    CefRefPtr<CefBrowser> m_browser{nullptr};
};

}  // namespace CEF
