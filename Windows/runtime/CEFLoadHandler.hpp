#pragma once

#include <iostream>

#include "include/cef_load_handler.h"

namespace CEF {

class CEFLoadHandler : public CefLoadHandler {
   public:
    CEFLoadHandler(){};

   public:
    void OnLoadingStateChange(CefRefPtr<CefBrowser> browser, bool isLoading, bool canGoBack, bool canGoForward) override {}

    void OnLoadStart(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, TransitionType transition_type) override { std::cout << "CEF: load start\n"; }

    void OnLoadEnd(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int httpStatusCode) override {
        std::cout << "CEF: load end — HTTP " << httpStatusCode << "\n";
    }

    void OnLoadError(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, ErrorCode errorCode, const CefString& errorText,
                     const CefString& failedUrl) override {
        std::cout << "CEF: load error " << errorCode << " — " << errorText.ToString() << " — " << failedUrl.ToString() << "\n";
    }

   private:
    IMPLEMENT_REFCOUNTING(CEFLoadHandler);
};

}  // namespace CEF