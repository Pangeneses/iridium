#pragma once

#include <functional>
#include <string>
#include <vector>

#include "include/cef_client.h"
#include "include/cef_life_span_handler.h"
#include "include/cef_render_handler.h"
#include "include/cef_request_handler.h"
#include "include/wrapper/cef_message_router.h"

#include "CEFLoadHandler.hpp"
#include "CEFQueryHandler.hpp"
#include "include/cef_context_menu_handler.h"

namespace NSIr77CEF {

// Browser-side handlers. Also the life span and request handler, only to keep the message router told about closes, navigations and
// renderer crashes (pending window.cefQuery calls are cancelled then). SetPost is where what the pages send goes.
class CEFClient : public CefClient, public CefContextMenuHandler, public CefDisplayHandler, public CefLifeSpanHandler, public CefRequestHandler {
   public:
    CEFClient() { DefineRouter(); }

    explicit CEFClient(CefRefPtr<CefRenderHandler> render_handler) : m_render_handler(render_handler) { DefineRouter(); }

    ~CEFClient() override {
        if (m_router) m_router->RemoveHandler(&m_query_handler);
    }

   public:
    CefRefPtr<CefAudioHandler> GetAudioHandler() override { return m_audio_handler; }

    CefRefPtr<CefCommandHandler> GetCommandHandler() override { return m_command_handler; }

    CefRefPtr<CefContextMenuHandler> GetContextMenuHandler() override { return this; }

    CefRefPtr<CefDialogHandler> GetDialogHandler() override { return m_dialog_handler; }

    // ---- CefContextMenuHandler ---------------------------------------------------------------------------------------------------------------
    // windowless: CEF has no window to show a native menu in -- an empty model means none is attempted. Menus belong to the Angular UI.
    void OnBeforeContextMenu(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefContextMenuParams> params,
                             CefRefPtr<CefMenuModel> model) override {
        model->Clear();
    }

    CefRefPtr<CefDisplayHandler> GetDisplayHandler() override { return m_display_handler; }

    CefRefPtr<CefDownloadHandler> GetDownloadHandler() override { return m_download_handler; }

    CefRefPtr<CefDragHandler> GetDragHandler() override { return m_drag_handler; }

    CefRefPtr<CefFindHandler> GetFindHandler() override { return m_find_handler; }

    CefRefPtr<CefFocusHandler> GetFocusHandler() override { return m_focus_handler; }

    CefRefPtr<CefFrameHandler> GetFrameHandler() override { return m_frame_handler; }

    CefRefPtr<CefPermissionHandler> GetPermissionHandler() override { return m_permission_handler; }

    CefRefPtr<CefJSDialogHandler> GetJSDialogHandler() override { return m_jsdialog_handler; }

    CefRefPtr<CefKeyboardHandler> GetKeyboardHandler() override { return m_keyboard_handler; }

    CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override { return this; }

    CefRefPtr<CefLoadHandler> GetLoadHandler() override { return m_load_handler; }

    CefRefPtr<CefPrintHandler> GetPrintHandler() override { return m_print_handler; }

    CefRefPtr<CefRenderHandler> GetRenderHandler() override { return m_render_handler; }

    CefRefPtr<CefRequestHandler> GetRequestHandler() override { return this; }

    // window.cefQuery traffic from the renderer
    bool OnProcessMessageReceived(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefProcessId source_process,
                                  CefRefPtr<CefProcessMessage> message) override {
        return m_router->OnProcessMessageReceived(browser, frame, source_process, message);
    }

    // ---- CefLifeSpanHandler ------------------------------------------------------------------------------------------------------------------
    void OnAfterCreated(CefRefPtr<CefBrowser> browser) override {
        m_open_browsers++;
        browser->GetHost()->SetFocus(true);
    }

    void OnBeforeClose(CefRefPtr<CefBrowser> browser) override {
        m_router->OnBeforeClose(browser);

        if (m_open_browsers > 0) m_open_browsers--;
    }

    // Pages not yet closed -- DestroyPages pumps CEF until this is 0
    int OpenBrowsers() const { return m_open_browsers; }

    // ---- CefRequestHandler -------------------------------------------------------------------------------------------------------------------
    // a navigation cancels the old page's pending queries; returns false -- the navigation goes ahead
    bool OnBeforeBrowse(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request, bool user_gesture, bool is_redirect) override {
        m_router->OnBeforeBrowse(browser, frame);

        return false;
    }

    void OnRenderProcessTerminated(CefRefPtr<CefBrowser> browser, TerminationStatus status, int error_code, CefString const& error_string) override {
        m_router->OnRenderProcessTerminated(browser);
    }

    // ---- app side ----------------------------------------------------------------------------------------------------------------------------
    // Where every window.cefQuery request goes -- the router's queue
    void SetPost(std::function<void(std::string)> post) { m_query_handler.SetPost(std::move(post)); }

    // A message for the page -- answers its waiting request by "id", or goes to the ui.listen channel
    void Reply(std::string const& json) { m_query_handler.Reply(json); }

   private:
    void DefineRouter() {
        CefMessageRouterConfig config{};  // window.cefQuery / window.cefQueryCancel -- must match CEFRenderProcessHandler

        m_router = CefMessageRouterBrowserSide::Create(config);
        m_router->AddHandler(&m_query_handler, false);
    }

   private:
    CefRefPtr<CefAudioHandler> m_audio_handler{};
    CefRefPtr<CefCommandHandler> m_command_handler{};
    CefRefPtr<CefContextMenuHandler> m_context_menu_handler{};
    CefRefPtr<CefDialogHandler> m_dialog_handler{};
    CefRefPtr<CefDisplayHandler> m_display_handler{};
    CefRefPtr<CefDownloadHandler> m_download_handler{};
    CefRefPtr<CefDragHandler> m_drag_handler{};
    CefRefPtr<CefFindHandler> m_find_handler{};
    CefRefPtr<CefFocusHandler> m_focus_handler{};
    CefRefPtr<CefFrameHandler> m_frame_handler{};
    CefRefPtr<CefPermissionHandler> m_permission_handler{};
    CefRefPtr<CefJSDialogHandler> m_jsdialog_handler{};
    CefRefPtr<CefKeyboardHandler> m_keyboard_handler{};
    CefRefPtr<CefLoadHandler> m_load_handler{new CEFLoadHandler()};  // main-frame load timing -> stderr
    CefRefPtr<CefPrintHandler> m_print_handler{};
    CefRefPtr<CefRenderHandler> m_render_handler{};

    // declared before the router -- the router holds a raw pointer to it and is released first
    CEFQueryHandler m_query_handler{};

    CefRefPtr<CefMessageRouterBrowserSide> m_router{};

    int m_open_browsers{0};

    IMPLEMENT_REFCOUNTING(CEFClient);
};

}  // namespace NSIr77CEF