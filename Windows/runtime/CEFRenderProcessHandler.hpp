#pragma once

#include "include/cef_render_process_handler.h"
#include "include/wrapper/cef_message_router.h"

namespace NSIr77CEF {

// Renderer process side of the JavaScript bridge. Gives every page window.cefQuery / window.cefQueryCancel and carries their requests to
// the browser process (CEFClient -> CEFQueryHandler). The router config must match CEFClient's -- both use the defaults.
class CEFRenderProcessHandler : public CefRenderProcessHandler {
   public:
    CEFRenderProcessHandler() = default;

   public:
    void OnWebKitInitialized() override {
        CefMessageRouterConfig config{};
        m_router = CefMessageRouterRendererSide::Create(config);
    }

    void OnContextCreated(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context) override {
        if (m_router) m_router->OnContextCreated(browser, frame, context);
    }

    void OnContextReleased(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context) override {
        if (m_router) m_router->OnContextReleased(browser, frame, context);
    }

    bool OnProcessMessageReceived(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefProcessId source_process,
                                  CefRefPtr<CefProcessMessage> message) override {
        return m_router && m_router->OnProcessMessageReceived(browser, frame, source_process, message);
    }

   private:
    CefRefPtr<CefMessageRouterRendererSide> m_router{};

    IMPLEMENT_REFCOUNTING(CEFRenderProcessHandler);
};

}  // namespace NSIr77CEF
