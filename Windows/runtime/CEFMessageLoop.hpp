#pragma once

#include <chrono>

#include "include/cef_app.h"

namespace CEF {

class CEFMessageLoop {
   public:
    CEFMessageLoop() = default;

   public:
    void CEFDoMessageLoop(CefRefPtr<CefBrowser> browser) {
        CefDoMessageLoopWork();

        if (!browser) return;

        auto const now = std::chrono::steady_clock::now();
        //if (now - m_last_invalidate < m_debounce_interval) return;

        browser->GetHost()->Invalidate(PET_VIEW);
        m_last_invalidate = now;
    }

   private:
    static constexpr std::chrono::milliseconds m_debounce_interval{1000/60};  // ~N/sec ceiling

    std::chrono::steady_clock::time_point m_last_invalidate{};
};
}  // namespace CEF