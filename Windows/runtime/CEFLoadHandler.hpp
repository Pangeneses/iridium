#pragma once

#include <chrono>
#include <cstdio>

#include "include/cef_load_handler.h"

namespace NSIr77CEF {

// Main-frame load events with milliseconds since startup, on stderr (unbuffered) so they line up with the [UI] status lines
class CEFLoadHandler : public CefLoadHandler {
   public:
    CEFLoadHandler() = default;

   public:
    void OnLoadingStateChange(CefRefPtr<CefBrowser> browser, bool isLoading, bool canGoBack, bool canGoForward) override {
        std::fprintf(stderr, "[LOAD %lld ms] loading %s\n", Elapsed(), isLoading ? "started" : "finished");
    }

    void OnLoadStart(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, TransitionType transition_type) override {
        if (!frame->IsMain()) return;

        std::fprintf(stderr, "[LOAD %lld ms] start %s\n", Elapsed(), frame->GetURL().ToString().c_str());
    }

    void OnLoadEnd(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int httpStatusCode) override {
        if (!frame->IsMain()) return;

        std::fprintf(stderr, "[LOAD %lld ms] end HTTP %d\n", Elapsed(), httpStatusCode);
    }

    void OnLoadError(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, ErrorCode errorCode, CefString const& errorText,
                     CefString const& failedUrl) override {
        std::fprintf(stderr, "[LOAD %lld ms] error %d (%s) %s -- %s\n", Elapsed(), static_cast<int>(errorCode), frame->IsMain() ? "main" : "sub",
                     errorText.ToString().c_str(), failedUrl.ToString().c_str());
    }

   private:
    long long Elapsed() const { return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - m_start).count(); }

   private:
    std::chrono::steady_clock::time_point m_start{std::chrono::steady_clock::now()};

    IMPLEMENT_REFCOUNTING(CEFLoadHandler);
};

}  // namespace NSIr77CEF