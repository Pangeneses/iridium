#pragma once

#include "include/cef_focus_handler.h"

namespace CEF {

class CEFFocusHandler : public CefFocusHandler {
   public:
    CEFFocusHandler(){};

   public:
    void OnTakeFocus(CefRefPtr<CefBrowser> browser, bool next) override {}

    bool OnSetFocus(CefRefPtr<CefBrowser> browser, FocusSource source) override { return false; }

    void OnGotFocus(CefRefPtr<CefBrowser> browser) override {}

   private:
    IMPLEMENT_REFCOUNTING(CEFFocusHandler);
};

}  // namespace CEF