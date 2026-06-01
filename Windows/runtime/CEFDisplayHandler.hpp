#pragma once

#include "include/cef_display_handler.h"

namespace CEF {

class CEFDisplayHandler : public CefDisplayHandler {
   public:
    CEFDisplayHandler(){};

   public:
    void OnAddressChange(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefString& url) override {}

    void OnTitleChange(CefRefPtr<CefBrowser> browser, const CefString& title) override {}

    void OnFaviconURLChange(CefRefPtr<CefBrowser> browser, const std::vector<CefString>& icon_urls) override {}

    void OnFullscreenModeChange(CefRefPtr<CefBrowser> browser, bool fullscreen) override {}

    bool OnTooltip(CefRefPtr<CefBrowser> browser, CefString& text) override { return false; }

    void OnStatusMessage(CefRefPtr<CefBrowser> browser, const CefString& value) override {}

    bool OnConsoleMessage(CefRefPtr<CefBrowser> browser, cef_log_severity_t level, const CefString& message, const CefString& source, int line) override {
        return false;
    }

    bool OnAutoResize(CefRefPtr<CefBrowser> browser, const CefSize& new_size) override { return false; }

    void OnLoadingProgressChange(CefRefPtr<CefBrowser> browser, double progress) override {}

    bool OnCursorChange(CefRefPtr<CefBrowser> browser, CefCursorHandle cursor, cef_cursor_type_t type, const CefCursorInfo& custom_cursor_info) override {
        return false;
    }

    void OnMediaAccessChange(CefRefPtr<CefBrowser> browser, bool has_video_access, bool has_audio_access) override {}

#if CEF_API_ADDED(13700)
    bool OnContentsBoundsChange(CefRefPtr<CefBrowser> browser, const CefRect& new_bounds) override { return false; }

    bool GetRootWindowScreenRect(CefRefPtr<CefBrowser> browser, CefRect& rect) override { return false; }
#endif

   private:
    IMPLEMENT_REFCOUNTING(CEFDisplayHandler);
};

}  // namespace CEF