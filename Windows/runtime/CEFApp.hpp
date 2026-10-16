#pragma once

#include "include/cef_app.h"

#include "CEFRenderProcessHandler.hpp"

namespace CEF {

// Passed to CefExecuteProcess as well as CefInitialize, so the renderer subprocesses get GetRenderProcessHandler (window.cefQuery).
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
        command_line->AppendSwitch("disable-gpu-process-crash-limit");
        command_line->AppendSwitch("ignore-gpu-blocklist");
        command_line->AppendSwitch("enable-gpu-rasterization");
        // command_line->AppendSwitchWithValue("disable-features", "OptimizationGuideOnDeviceModel,OnDeviceModelService");
        // command_line->AppendSwitchWithValue("disable-features", "webgpu");
    }

    void OnRegisterCustomSchemes(CefRawPtr<CefSchemeRegistrar> registrar) override {}

    CefRefPtr<CefResourceBundleHandler> GetResourceBundleHandler() override { return nullptr; }

    CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() override { return nullptr; }

    // renderer processes only -- the browser process never asks
    CefRefPtr<CefRenderProcessHandler> GetRenderProcessHandler() override { return m_render_process_handler; }

   private:
    CefRefPtr<CefRenderProcessHandler> m_render_process_handler{new CEFRenderProcessHandler()};

    IMPLEMENT_REFCOUNTING(CEFApp);
};

}  // namespace CEF
