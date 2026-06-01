#pragma once

#include "include/cef_app.h"

namespace CEF {

class CEFApp : public CefApp {
   public:
    CEFApp(){};

   public:
    void OnBeforeCommandLineProcessing(const CefString& process_type, CefRefPtr<CefCommandLine> command_line) override {
        command_line->AppendSwitch("disable-gpu");
        command_line->AppendSwitch("disable-gpu-compositing");
        command_line->AppendSwitch("disable-software-rasterizer");
        command_line->AppendSwitch("enable-begin-frame-scheduling");
    }

    void OnRegisterCustomSchemes(CefRawPtr<CefSchemeRegistrar> registrar) override {}

    CefRefPtr<CefResourceBundleHandler> GetResourceBundleHandler() override { return nullptr; }

    CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() override { return nullptr; }

    CefRefPtr<CefRenderProcessHandler> GetRenderProcessHandler() override { return nullptr; }

   private:
    IMPLEMENT_REFCOUNTING(CEFApp);
};

}  // namespace CEF
