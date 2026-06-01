#pragma once

#include "include/cef_client.h"
#include "include/cef_render_handler.h"

namespace CEF {

class CEFClient : public CefClient {
   public:
    CEFClient() = default;

    explicit CEFClient(CefRefPtr<CefRenderHandler> render_handler) : m_render_handler(render_handler) {}

   public:
    CefRefPtr<CefAudioHandler> GetAudioHandler() override { return m_audio_handler; }

    CefRefPtr<CefCommandHandler> GetCommandHandler() override { return m_command_handler; }

    CefRefPtr<CefContextMenuHandler> GetContextMenuHandler() override { return m_context_menu_handler; }

    CefRefPtr<CefDialogHandler> GetDialogHandler() override { return m_dialog_handler; }

    CefRefPtr<CefDisplayHandler> GetDisplayHandler() override { return m_display_handler; }

    CefRefPtr<CefDownloadHandler> GetDownloadHandler() override { return m_download_handler; }

    CefRefPtr<CefDragHandler> GetDragHandler() override { return m_drag_handler; }

    CefRefPtr<CefFindHandler> GetFindHandler() override { return m_find_handler; }

    CefRefPtr<CefFocusHandler> GetFocusHandler() override { return m_focus_handler; }

    CefRefPtr<CefFrameHandler> GetFrameHandler() override { return m_frame_handler; }

    CefRefPtr<CefPermissionHandler> GetPermissionHandler() override { return m_permission_handler; }

    CefRefPtr<CefJSDialogHandler> GetJSDialogHandler() override { return m_jsdialog_handler; }

    CefRefPtr<CefKeyboardHandler> GetKeyboardHandler() override { return m_keyboard_handler; }

    CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override { return m_life_span_handler; }

    CefRefPtr<CefLoadHandler> GetLoadHandler() override { return m_load_handler; }

    CefRefPtr<CefPrintHandler> GetPrintHandler() override { return m_print_handler; }

    CefRefPtr<CefRenderHandler> GetRenderHandler() override { return m_render_handler; }

    CefRefPtr<CefRequestHandler> GetRequestHandler() override { return m_request_handler; }

    bool OnProcessMessageReceived(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefProcessId source_process,
                                  CefRefPtr<CefProcessMessage> message) override {
        return false;
    }

   private:
    CefRefPtr<CefAudioHandler> m_audio_handler;
    CefRefPtr<CefCommandHandler> m_command_handler;
    CefRefPtr<CefContextMenuHandler> m_context_menu_handler;
    CefRefPtr<CefDialogHandler> m_dialog_handler;
    CefRefPtr<CefDisplayHandler> m_display_handler;
    CefRefPtr<CefDownloadHandler> m_download_handler;
    CefRefPtr<CefDragHandler> m_drag_handler;
    CefRefPtr<CefFindHandler> m_find_handler;
    CefRefPtr<CefFocusHandler> m_focus_handler;
    CefRefPtr<CefFrameHandler> m_frame_handler;
    CefRefPtr<CefPermissionHandler> m_permission_handler;
    CefRefPtr<CefJSDialogHandler> m_jsdialog_handler;
    CefRefPtr<CefKeyboardHandler> m_keyboard_handler;
    CefRefPtr<CefLifeSpanHandler> m_life_span_handler;
    CefRefPtr<CefLoadHandler> m_load_handler;
    CefRefPtr<CefPrintHandler> m_print_handler;
    CefRefPtr<CefRenderHandler> m_render_handler;
    CefRefPtr<CefRequestHandler> m_request_handler;

    IMPLEMENT_REFCOUNTING(CEFClient);
};

}  // namespace CEF
