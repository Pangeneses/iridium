#pragma once

#include <functional>
#include <iostream>
#include <thread>
#include <execinfo.h>

#include "include/cef_render_handler.h"

#include "../../Ir77RT/runtime/Ir77Return.hpp"

using namespace NSIr77RT;

namespace CEF {

class CEFRenderHandler : public CefRenderHandler {
   public:
    using PaintCallback = std::function<void(const void* buffer, int width, int height)>;

    CEFRenderHandler() = default;

    CEFRenderHandler(int width, int height, PaintCallback cb) : m_width{width}, m_height{height}, m_callback{std::move(cb)} {}

   public:
       void SetViewSize(int w, int h) {
        m_width = w;
        m_height = h;
    }
    
    void GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect) override { rect = CefRect(0, 0, m_width, m_height); }

    void OnPaint(CefRefPtr<CefBrowser> browser, PaintElementType type, const RectList& dirtyRects, const void* buffer, int width, int height) override {
        if (m_callback) {
            //std::cerr << "Paint: " << width << "x" << height << "\n";
            m_callback(buffer, width, height);
        }
    }

    // Same as cefclient's off-screen client: 1:1 pixels, and the "screen" is the view itself, so CEF never
    // lays out against a guessed screen size or scale factor during a resize
    bool GetScreenInfo(CefRefPtr<CefBrowser> browser, CefScreenInfo& screen_info) override {
        CefRect view{};
        GetViewRect(browser, view);

        screen_info.device_scale_factor = 1.0f;
        screen_info.rect = view;
        screen_info.available_rect = view;

        return true;
    }

    CefRefPtr<CefAccessibilityHandler> GetAccessibilityHandler() override { return nullptr; }

    bool GetRootScreenRect(CefRefPtr<CefBrowser> browser, CefRect& rect) override { return false; }

    bool GetScreenPoint(CefRefPtr<CefBrowser> browser, int viewX, int viewY, int& screenX, int& screenY) override { return false; }

   public:
    void SetPaintCallback(std::function<void(const void*, int, int)> fn) { m_callback = std::move(fn); }


   private:
    PaintCallback m_callback;

    std::atomic<int> m_width{1280};

    std::atomic<int> m_height{720};

   private:
    IMPLEMENT_REFCOUNTING(CEFRenderHandler);
};

}  // namespace CEF
