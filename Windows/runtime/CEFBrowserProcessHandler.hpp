#pragma once

#include "include/cef_browser_process_handler.h"

namespace CEF {

class CEFBrowserProcessHandler : public CefBrowserProcessHandler {
   public:
    CEFBrowserProcessHandler(){};

   public:
    void OnRegisterCustomPreferences(cef_preferences_type_t type, CefRawPtr<CefPreferenceRegistrar> registrar) override {}

    void OnContextInitialized() override {}

    void OnBeforeChildProcessLaunch(CefRefPtr<CefCommandLine> command_line) override {}

    bool OnAlreadyRunningAppRelaunch(CefRefPtr<CefCommandLine> command_line, const CefString& current_directory) override { return false; }

    void OnScheduleMessagePumpWork(int64_t delay_ms) override {}

    CefRefPtr<CefClient> GetDefaultClient() override { return nullptr; }

    CefRefPtr<CefRequestContextHandler> GetDefaultRequestContextHandler() override { return nullptr; }

   private:
    IMPLEMENT_REFCOUNTING(CEFBrowserProcessHandler);
};

}  // namespace CEF