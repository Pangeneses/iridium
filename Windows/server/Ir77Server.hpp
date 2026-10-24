#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <SDL3/SDL.h>
#include <SDL3/SDL_video.h>

#include <simdjson.h>

#include "../../Ir77RT/dictionary/IDIIr77MPVM.hpp"

#include "../dictionary/IDIIr77Windows.hpp"

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"
#include "../../Ir77RT/runtime/Ir77Return.hpp"

#include "../runtime/CEFRTState.hpp"
#include "../runtime/CEFClient.hpp"
#include "../runtime/CEFMouseEvent.hpp"

#include "../../PeregrineV/server/Ir77Server.hpp"

using namespace NSIr77RT;

namespace NSIr77CEF {

// =============================================================================================================================================
// Ir77Server -- the windows module's switchboard. Owns the SDL windows and the CEF state (browser, client, the pages' query handler).
// The module's one door:
//
//   pipe:        Sink(json) in, the post function out (SetPost). Two kinds of traffic arrive at the one sink:
//                  "to":"windows"            -> an op for this module
//                  "to":"ui" / "ui.<page>"   -> a reply or event for a page -- handed to the client, which finds the page
//                and every page query goes out through the post function, stamped "from":"ui.<page>".
//   direct link: only what can't travel as JSON -- Initialize (argc / argv), CreateDisplayWindow (hands back the SDL window), the renderer's
//                server (SetRenderer) and the loop (Step, InputEvent). Everything else is an op, setup included.
//
// A page is a CEF browser; its id is the browser id.
//
// ops:
//   page.configure { url?, debug_port?, width?, height? } -- before Initialize: the first page's URL, DevTools port (0 = off), layout size
//   page.view_size { width, height }           -- after Initialize: a resize, sent to CEF once it has painted the last size
//   window.info   { window? }                  -> data { width, height, title, page }
//   window.size   { window?, width, height }   -- the SDL resize event follows, as if the user had dragged it
//   window.title  { window?, title }
//   page.url      { page?, url }               -- loads a URL in the page (default: the first)
// =============================================================================================================================================
class Ir77Server : public Ir77Enlisted, public std::enable_shared_from_this<Ir77Server> {
   public:
    Ir77Server() {
        m_rt_state = std::make_shared<CEFRTState>();

        if (m_empty_parser.parse(std::string_view{"{}"}).get(m_empty_args) != simdjson::SUCCESS)  // what a handler sees when a message has no "args"
            throw std::runtime_error{"Ir77Server: empty args"};

        DefineOperations();
    }

   public:
    std::shared_ptr<IIr77Return const> EnlistedAs(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIIr77Enlisted>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> MemberUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIIr77Server>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CollectionUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIIr77Windows>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    Ir77GUID QueryInterface(Ir77GUID iid, std::shared_ptr<void>& obj) {
        if (iid == GUIDIIr77Enlisted)
            obj = std::shared_ptr<IIr77Enlisted>(shared_from_this(), static_cast<IIr77Enlisted*>(this));

        else if (iid == GUIDIIr77Server)
            obj = std::shared_ptr<Ir77Server>(shared_from_this(), static_cast<Ir77Server*>(this));

        else
            return GUIDQueryFailed;

        return GUIDQuerySucceeded;
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // setup -- direct
    // -------------------------------------------------------------------------------------------------------------------------------------

    // CEF up and the first browser created, as page.configure set it. A CEF subprocess throws "__cef_subprocess__:<exit code>" out of here;
    // the browser process returns. The query handler posts every page query from here on -- SetPost first.
    void Initialize(int argc, char* argv[]) {
        m_rt_state->Initialize(argc, argv);

        m_rt_state->GetClient()->SetPost([this](std::string json) {
            if (m_post) m_post(std::move(json));
        });
    }

    // One SDL window, shown, taking text input -- its index is its place in the order created. SDL_Init(VIDEO) first.
    std::shared_ptr<IIr77Return const> CreateDisplayWindow(std::string const& title, int const width, int const height, SDL_Window*& window) {
        window = SDL_CreateWindow(title.c_str(), width, height, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
        if (!window) return Ir77RETURN<Ir77OperationFailed>(this, "Ir77Server: SDL_CreateWindow failed: " + std::string{SDL_GetError()});

        SDL_ShowWindow(window);
        SDL_StartTextInput(window);  // SDL_EVENT_TEXT_INPUT -> typed characters for the UI's inputs

        m_sdl_windows.push_back(window);

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Ir77Server: window created.");
    }

    // Every page closed, CEF shut down -- after the router has stopped routing to the pages, before DestroyWindows
    std::shared_ptr<IIr77Return const> DestroyPages() { return m_rt_state->DestroyPages(); }

    // After the renderer is destroyed (its surfaces are on these windows), before SDL_Quit
    std::shared_ptr<IIr77Return const> DestroyWindows() {
        for (SDL_Window* window : m_sdl_windows) SDL_DestroyWindow(window);
        m_sdl_windows.clear();

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // direct links
    // -------------------------------------------------------------------------------------------------------------------------------------

    // PeregrineV's server -- the first page's paints go into window 0's overlay through it, held weakly by CEF
    void SetRenderer(std::shared_ptr<NSIr77PeregrineV::Ir77Server> const& renderer) { m_rt_state->SetRenderer(renderer); }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // loop -- direct, every frame
    // -------------------------------------------------------------------------------------------------------------------------------------

    // CEF's message loop and the resize handshake -- page queries are posted from in here
    void Step() {
        if (m_rt_state->GetBrowser()) m_rt_state->CEFStep();  // nothing to pump once the pages are gone
    }

    // An SDL event the loop didn't handle -- mouse, keys, text -> the page
    void InputEvent(SDL_Event* event) {
        if (auto const browser = m_rt_state->GetBrowser()) CEFInputEvent(event, browser);
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // pipe
    // -------------------------------------------------------------------------------------------------------------------------------------

    // Where this module's replies and the pages' queries go -- the router's queue
    void SetPost(std::function<void(std::string)> post) { m_post = std::move(post); }

    // One message from the router. Runs on the main thread, between frames.
    void Sink(std::string json) {
        simdjson::dom::element root{};
        if (m_parser.parse(json).get(root) != simdjson::SUCCESS) {
            Ir77RETURN<Ir77OperationFailed>(this, "Ir77Server: not JSON.");
            return;
        }

        std::string_view to{};
        if (root["to"].get(to) != simdjson::SUCCESS) to = {};

        // for a page -- the client answers its waiting request or pushes to its listener
        if (to != "windows") {
            if (auto const client = m_rt_state->GetClient()) client->Reply(json);  // dropped once the pages are destroyed
            return;
        }

        // a reply to something this module sent -- never answered, or two modules would answer each other forever
        if (root["ok"].error() == simdjson::SUCCESS) {
            bool reply_ok{true};
            if (root["ok"].get(reply_ok) == simdjson::SUCCESS && !reply_ok) Ir77RETURN<Ir77OperationFailed>(this, "Ir77Server: reply failed: " + json);
            return;
        }

        std::int64_t id{0};
        if (root["id"].get(id) != simdjson::SUCCESS) id = 0;

        std::string_view from{};
        if (root["from"].get(from) != simdjson::SUCCESS) from = {};

        std::string_view op{};
        if (root["op"].get(op) != simdjson::SUCCESS) op = {};

        simdjson::dom::object args{};
        if (root["args"].get(args) != simdjson::SUCCESS) args = m_empty_args;

        std::string data{"{}"};

        auto const handler = m_operations.find(op);
        bool const known = handler != m_operations.end();

        bool const ok = known && handler->second(args, data)->ID() == GUIDIr77OperationSucceeded;
        char const* const error = ok ? "" : known ? "operation failed" : "unknown op";

        if (!ok) Ir77RETURN<Ir77OperationFailed>(this, std::string{"Ir77Server: "} + error + ": " + json);

        if (from.empty() || !m_post) return;  // nobody to answer

        m_post("{\"id\":" + std::to_string(id) + ",\"from\":\"windows\",\"to\":\"" + std::string{from} + "\",\"op\":\"" + std::string{op} +
               "\",\"ok\":" + (ok ? "true" : "false") + ",\"error\":\"" + error + "\",\"data\":" + data + "}");
    }

   private:
    using Ir77ServerArgs = simdjson::dom::object;

    // a handler fills `data` (a JSON object) for the reply
    using Ir77ServerOperation = std::function<std::shared_ptr<IIr77Return const>(Ir77ServerArgs const&, std::string&)>;

    // heterogeneous lookup -- find by the string_view out of the parser
    struct Ir77ServerOpHash {
        using is_transparent = void;
        std::size_t operator()(std::string_view const& op) const { return std::hash<std::string_view>{}(op); }
    };

    void DefineOperations() {
        DefineWindowOperations();
        DefinePageOperations();
    }

    void DefineWindowOperations() {
        m_operations["window.info"] = [this](Ir77ServerArgs const& args, std::string& data) -> std::shared_ptr<IIr77Return const> {
            SDL_Window* window{nullptr};
            if (!DefineWindow(args, window)) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Server: no such window.");

            int width{0};
            int height{0};
            SDL_GetWindowSize(window, &width, &height);

            char const* const title = SDL_GetWindowTitle(window);

            int page{0};
            if (m_rt_state->GetBrowser()) page = m_rt_state->GetBrowser()->GetIdentifier();

            data = "{\"width\":" + std::to_string(width) + ",\"height\":" + std::to_string(height) + ",\"title\":\"" + Escape(title ? title : "") +
                   "\",\"page\":" + std::to_string(page) + "}";

            return Ir77RETURN<Ir77OperationSucceeded>();
        };

        m_operations["window.size"] = [this](Ir77ServerArgs const& args, std::string&) -> std::shared_ptr<IIr77Return const> {
            SDL_Window* window{nullptr};
            if (!DefineWindow(args, window)) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Server: no such window.");

            std::int64_t width{0};
            std::int64_t height{0};
            if (args["width"].get(width) != simdjson::SUCCESS || args["height"].get(height) != simdjson::SUCCESS || width <= 0 || height <= 0)
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Server: window.size needs width and height above 0.");

            if (!SDL_SetWindowSize(window, static_cast<int>(width), static_cast<int>(height)))
                return Ir77RETURN<Ir77OperationFailed>(this, "Ir77Server: SDL_SetWindowSize failed: " + std::string{SDL_GetError()});

            return Ir77RETURN<Ir77OperationSucceeded>();
        };

        m_operations["window.title"] = [this](Ir77ServerArgs const& args, std::string&) -> std::shared_ptr<IIr77Return const> {
            SDL_Window* window{nullptr};
            if (!DefineWindow(args, window)) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Server: no such window.");

            std::string_view title{};
            if (args["title"].get(title) != simdjson::SUCCESS) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Server: window.title needs \"title\".");

            if (!SDL_SetWindowTitle(window, std::string{title}.c_str()))
                return Ir77RETURN<Ir77OperationFailed>(this, "Ir77Server: SDL_SetWindowTitle failed: " + std::string{SDL_GetError()});

            return Ir77RETURN<Ir77OperationSucceeded>();
        };

        // The page's copy / cut -- windowless CEF copies into its own clipboard, so the page hands the text here for the desktop's
        m_operations["clipboard.set"] = [this](Ir77ServerArgs const& args, std::string&) -> std::shared_ptr<IIr77Return const> {
            std::string_view text{};
            if (args["text"].get(text) != simdjson::SUCCESS) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Server: clipboard.set needs \"text\".");

            if (!SDL_SetClipboardText(std::string{text}.c_str()))
                return Ir77RETURN<Ir77OperationFailed>(this, "Ir77Server: SDL_SetClipboardText failed: " + std::string{SDL_GetError()});

            return Ir77RETURN<Ir77OperationSucceeded>();
        };
    }

    void DefinePageOperations() {
        m_operations["page.configure"] = [this](Ir77ServerArgs const& args, std::string&) -> std::shared_ptr<IIr77Return const> {
            if (m_rt_state->GetBrowser())
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Server: page.configure after Initialize -- use page.url / page.view_size.");

            std::string_view url{};
            if (args["url"].get(url) == simdjson::SUCCESS && !url.empty()) m_rt_state->SetStartUrl(std::string{url});

            std::int64_t port{0};
            if (args["debug_port"].get(port) == simdjson::SUCCESS) {
                if (port < 0 || port > 65535) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Server: page.configure debug_port must be 0..65535.");
                m_rt_state->SetDebugPort(static_cast<std::uint16_t>(port));
            }

            std::int64_t width{0};
            std::int64_t height{0};
            bool const has_width = args["width"].get(width) == simdjson::SUCCESS;
            bool const has_height = args["height"].get(height) == simdjson::SUCCESS;

            if (has_width || has_height) {
                if (!has_width || !has_height || width <= 0 || height <= 0)
                    return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Server: page.configure needs width and height together, above 0.");
                m_rt_state->SetViewSize(static_cast<int>(width), static_cast<int>(height));
            }

            return Ir77RETURN<Ir77OperationSucceeded>();
        };

        m_operations["page.view_size"] = [this](Ir77ServerArgs const& args, std::string&) -> std::shared_ptr<IIr77Return const> {
            std::int64_t width{0};
            std::int64_t height{0};
            if (args["width"].get(width) != simdjson::SUCCESS || args["height"].get(height) != simdjson::SUCCESS || width <= 0 || height <= 0)
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Server: page.view_size needs width and height above 0.");

            m_rt_state->SetViewSize(static_cast<int>(width), static_cast<int>(height));

            return Ir77RETURN<Ir77OperationSucceeded>();
        };

        m_operations["page.url"] = [this](Ir77ServerArgs const& args, std::string&) -> std::shared_ptr<IIr77Return const> {
            CefRefPtr<CefBrowser> browser{};
            if (!DefinePage(args, browser)) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Server: no such page.");

            std::string_view url{};
            if (args["url"].get(url) != simdjson::SUCCESS || url.empty()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Server: page.url needs \"url\".");

            browser->GetMainFrame()->LoadURL(CefString{std::string{url}});

            return Ir77RETURN<Ir77OperationSucceeded>();
        };
    }

    // args "window" (index, default 0) -> the SDL window
    bool DefineWindow(Ir77ServerArgs const& args, SDL_Window*& window) const {
        std::int64_t index{0};
        if (args["window"].get(index) != simdjson::SUCCESS) index = 0;

        if (index < 0 || static_cast<std::size_t>(index) >= m_sdl_windows.size()) return false;

        window = m_sdl_windows[static_cast<std::size_t>(index)];

        return true;
    }

    // args "page" (browser id, default the first page) -> the browser. One browser for now.
    bool DefinePage(Ir77ServerArgs const& args, CefRefPtr<CefBrowser>& browser) const {
        browser = m_rt_state->GetBrowser();
        if (!browser) return false;

        std::int64_t page{0};
        if (args["page"].get(page) == simdjson::SUCCESS && page != browser->GetIdentifier()) return false;

        return true;
    }

    // JSON string escaping for text that goes into a reply
    std::string Escape(std::string const& text) const {
        std::string out{};
        out.reserve(text.size());

        for (char const c : text) {
            if (c == '"' || c == '\\') out += '\\';
            if (static_cast<unsigned char>(c) < 0x20) continue;  // control characters dropped
            out += c;
        }

        return out;
    }

   private:
    std::shared_ptr<CEFRTState> m_rt_state{};

    std::vector<SDL_Window*> m_sdl_windows{};  // [window index] -- CreateDisplayWindow order

    std::function<void(std::string)> m_post{};

    simdjson::dom::parser m_parser{};

    simdjson::dom::parser m_empty_parser{};  // owns m_empty_args

    Ir77ServerArgs m_empty_args{};

    std::unordered_map<std::string, Ir77ServerOperation, Ir77ServerOpHash, std::equal_to<>> m_operations{};
};

}  // namespace NSIr77CEF
