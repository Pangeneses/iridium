#pragma once

#include "include/cef_app.h"

namespace CEF {

class CEFMessageLoop {
   public:
    CEFMessageLoop() = default;

   public:
    void CEFDoMessageLoop(CefRefPtr<CefBrowser> browser) {
        CefDoMessageLoopWork();
        if (browser) browser->GetHost()->Invalidate(PET_VIEW);
    }

   private:
};
}  // namespace CEF