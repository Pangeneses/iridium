#pragma once

#include <cstddef>
#include <cstdio>
#include <string>
#include <vector>

#include "include/wrapper/cef_message_router.h"

namespace CEF {

// Browser process end of window.cefQuery. Every request is queued as-is (the app decides what it means) and answered at once with an empty
// success, so the page's promise resolves without waiting for a frame. The message loop is single-threaded (CefDoMessageLoopWork in
// ChromeStep), so OnQuery runs on the app's own thread -- no lock.
class CEFQueryHandler : public CefMessageRouterBrowserSide::Handler {
   public:
    CEFQueryHandler() = default;

   public:
    bool OnQuery(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int64_t query_id, CefString const& request, bool persistent,
                 CefRefPtr<Callback> callback) override {
        if (m_pending.size() >= MAX_PENDING) {
            callback->Failure(1, "iridium: query queue full");
            return true;
        }

        m_pending.push_back(request.ToString());
        callback->Success("");

        return true;
    }

    // Everything queued since the last call, oldest first; the queue is left empty
    void TakeQueries(std::vector<std::string>& queries) {
        queries.clear();
        queries.swap(m_pending);
    }

   private:
    // a page sending faster than the app drains (app stalled) -- refuse rather than grow without bound
    static constexpr std::size_t MAX_PENDING = 4096;

    std::vector<std::string> m_pending{};
};

}  // namespace CEF
