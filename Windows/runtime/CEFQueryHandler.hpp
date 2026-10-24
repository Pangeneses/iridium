#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <simdjson.h>

#include "include/wrapper/cef_message_router.h"

namespace NSIr77CEF {

// Browser process end of window.cefQuery -- both directions of the UI pipe, for any number of pages (one per CEF browser).
//
//   in:    every request is posted to the router (SetPost) with its "from" set to "ui.<browser id>" -- the page that sent it, whatever it
//          claimed. One with an "id" keeps its callback until Reply answers it; id 0 is fire-and-forget and is answered at once.
//   out:   Reply(json) reads "to": "ui.<browser id>" answers that page's waiting request with the same "id", otherwise goes to that page's
//          listener; plain "ui" goes to every page's listener (a broadcast event).
//   listen: each page's one persistent query, op "ui.listen" -- never queued.
//
// Pages number their requests independently, so two pages may both send id 1000: waiting requests are keyed by (browser, id).
// The message loop is single-threaded (CefDoMessageLoopWork in CEFStep), and Reply is called from the router on the same thread -- no lock.
class CEFQueryHandler : public CefMessageRouterBrowserSide::Handler {
   private:
    struct CEFWaitingQuery {
        std::int64_t query_id{0};  // CEF's id -- what OnQueryCanceled names
        CefRefPtr<Callback> callback{};
    };

    struct CEFListener {
        std::int64_t query_id{0};
        CefRefPtr<Callback> callback{};
    };

    using CEFWaitingKey = std::pair<int, std::int64_t>;  // (browser id, message id)

   public:
    CEFQueryHandler() = default;

   public:
    bool OnQuery(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int64_t query_id, CefString const& request, bool persistent,
                 CefRefPtr<Callback> callback) override {
        int const browser_id{browser->GetIdentifier()};
        std::string json{request.ToString()};

        simdjson::dom::object root{};
        if (m_parser.parse(json).get(root) != simdjson::SUCCESS) {
            callback->Failure(2, "iridium: request is not a JSON object");
            return true;
        }

        std::string_view op{};
        if (root["op"].get(op) != simdjson::SUCCESS) op = {};

        // this page's event channel -- kept, answered every time something is pushed to it, never queued
        if (op == "ui.listen") {
            if (!persistent) {
                callback->Failure(3, "iridium: ui.listen must be persistent");
                return true;
            }

            m_listeners[browser_id] = CEFListener{query_id, callback};
            return true;
        }

        if (!m_post) {
            callback->Failure(1, "iridium: not connected to the router");
            return true;
        }

        std::int64_t id{0};
        if (root["id"].get(id) != simdjson::SUCCESS) id = 0;

        // the sender is the browser it came from -- a module answers to "from", so the answer finds this page and no other
        std::string stamped{};
        DefineStamped(root, browser_id, stamped);

        if (id == 0)
            callback->Success("");  // nobody waits for a reply
        else
            m_waiting[{browser_id, id}] = CEFWaitingQuery{query_id, callback};

        m_post(std::move(stamped));

        return true;
    }

    // Navigation, reload or a closed browser -- the page that was waiting is gone
    void OnQueryCanceled(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int64_t query_id) override {
        int const browser_id{browser->GetIdentifier()};

        auto const listener = m_listeners.find(browser_id);
        if (listener != m_listeners.end() && listener->second.query_id == query_id) {
            m_listeners.erase(listener);
            return;
        }

        std::erase_if(m_waiting, [browser_id, query_id](auto const& entry) { return entry.first.first == browser_id && entry.second.query_id == query_id; });
    }

    // Where page queries go -- the router's queue (through the windows module)
    void SetPost(std::function<void(std::string)> post) { m_post = std::move(post); }

    // A message for a page. "to": "ui.<browser id>" -> that page: the waiting request with the same "id", else its listener.
    // "to": "ui" -> every page's listener.
    void Reply(std::string const& json) {
        simdjson::dom::element root{};
        if (m_parser.parse(json).get(root) != simdjson::SUCCESS) return;

        std::string_view to{};
        if (root["to"].get(to) != simdjson::SUCCESS) to = {};

        int browser_id{0};
        if (!BrowserOf(to, browser_id)) {
            PushAll(json);
            return;
        }

        std::int64_t id{0};
        if (root["id"].get(id) == simdjson::SUCCESS && id != 0) {
            auto const waiting = m_waiting.find({browser_id, id});

            if (waiting != m_waiting.end()) {
                waiting->second.callback->Success(json);
                m_waiting.erase(waiting);
                return;
            }
        }

        Push(browser_id, json);
    }

    // To one page's listener -- dropped if that page hasn't sent ui.listen (yet)
    void Push(int const browser_id, std::string const& json) {
        auto const listener = m_listeners.find(browser_id);
        if (listener != m_listeners.end()) listener->second.callback->Success(json);
    }

    // To every page's listener
    void PushAll(std::string const& json) {
        for (auto const& [browser_id, listener] : m_listeners) listener.callback->Success(json);
    }

   private:
    // "ui.<n>" -> n; false for plain "ui" (and anything else)
    bool BrowserOf(std::string_view const to, int& browser_id) const {
        if (to.size() <= 3 || to.substr(0, 3) != "ui.") return false;

        int value{0};
        for (char const c : to.substr(3)) {
            if (c < '0' || c > '9') return false;
            value = value * 10 + (c - '0');
        }

        browser_id = value;
        return true;
    }

    // The request again, with "from" set to "ui.<browser id>" (added when it had none). Every other member is copied as it came.
    void DefineStamped(simdjson::dom::object const& root, int const browser_id, std::string& out) const {
        out = "{\"from\":\"ui." + std::to_string(browser_id) + "\"";

        for (auto const field : root) {
            if (field.key == "from") continue;

            out += ",\"";
            AppendEscaped(field.key, out);
            out += "\":";
            out += simdjson::minify(field.value);
        }

        out += "}";
    }

    void AppendEscaped(std::string_view const text, std::string& out) const {
        for (char const c : text) {
            switch (c) {
                case '"': out += "\\\""; break;
                case '\\': out += "\\\\"; break;
                case '\n': out += "\\n"; break;
                case '\r': out += "\\r"; break;
                case '\t': out += "\\t"; break;
                default:
                    if (static_cast<unsigned char>(c) < 0x20) {
                        char code[8]{};
                        std::snprintf(code, sizeof(code), "\\u%04x", static_cast<unsigned>(static_cast<unsigned char>(c)));
                        out += code;
                    } else {
                        out += c;
                    }
            }
        }
    }

   private:
    std::function<void(std::string)> m_post{};

    std::map<CEFWaitingKey, CEFWaitingQuery> m_waiting{};  // [(browser, message id)] -> the query waiting for its reply

    std::map<int, CEFListener> m_listeners{};  // [browser] -> its ui.listen -- persistent

    simdjson::dom::parser m_parser{};
};

}  // namespace NSIr77CEF
