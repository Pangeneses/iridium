#pragma once

#include "include/cef_keyboard_handler.h"

namespace CEF {

class CEFKeyboardHandler : public CefKeyboardHandler {
   public:
    CEFKeyboardHandler(){};

   public:
    bool OnPreKeyEvent(CefRefPtr<CefBrowser> browser, const CefKeyEvent& event, CefEventHandle os_event, bool* is_keyboard_shortcut) override { return false; }

    bool OnKeyEvent(CefRefPtr<CefBrowser> browser, const CefKeyEvent& event, CefEventHandle os_event) override { return false; }

   private:
    IMPLEMENT_REFCOUNTING(CEFKeyboardHandler);
};

}  // namespace CEF