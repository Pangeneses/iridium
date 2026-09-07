#pragma once

#include "include/cef_app.h"

namespace CEF {

class CEFApp : public CefApp {
   public:
    CEFApp() {};

   public:
    void OnBeforeCommandLineProcessing(const CefString& process_type, CefRefPtr<CefCommandLine> command_line) override {
        // command_line->AppendSwitch("single-process");
        // command_line->AppendSwitch("disable-gpu");
        // command_line->AppendSwitch("disable-gpu-compositing");
        // command_line->AppendSwitch("disable-software-rasterizer");
        // command_line->AppendSwitch("disable-zero-copy");
        // command_line->AppendSwitch("enable-begin-frame-scheduling");
        // command_line->AppendSwitch("no-zygote");
        // command_line->AppendSwitch("disable-dev-shm-usage");
        // command_line->AppendSwitchWithValue("disable-features", "Vulkan");
        command_line->AppendSwitchWithValue("ozone-platform", "headless");
        command_line->AppendSwitch("disable-crash-reporter");
    }

    void OnRegisterCustomSchemes(CefRawPtr<CefSchemeRegistrar> registrar) override {}

    CefRefPtr<CefResourceBundleHandler> GetResourceBundleHandler() override { return nullptr; }

    CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() override { return nullptr; }

    CefRefPtr<CefRenderProcessHandler> GetRenderProcessHandler() override { return nullptr; }

   private:
    IMPLEMENT_REFCOUNTING(CEFApp);
};

}  // namespace CEF
