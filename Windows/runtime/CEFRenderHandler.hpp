#pragma once

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <functional>
#include <vector>

#include "include/cef_render_handler.h"

#include "../../Ir77RT/runtime/Ir77Return.hpp"

using namespace NSIr77RT;

namespace NSIr77CEF {

// Windowless CEF paints two layers: the page (PET_VIEW) and, while a <select> list / colour picker / date field is open, that popup
// (PET_POPUP) at its own size. The overlay takes one full-size BGRA frame, so the popup is composited over the last page paint here.
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

    // A popup opened or closed -- closing repaints the page underneath it
    void OnPopupShow(CefRefPtr<CefBrowser> browser, bool show) override {
        m_popup_open = show;

        if (!show) {
            m_popup.clear();
            m_popup_rect = CefRect{};
            browser->GetHost()->Invalidate(PET_VIEW);
        }
    }

    // Where the popup sits, in view pixels
    void OnPopupSize(CefRefPtr<CefBrowser> browser, CefRect const& rect) override { m_popup_rect = rect; }

    void OnPaint(CefRefPtr<CefBrowser> browser, PaintElementType type, const RectList& dirtyRects, const void* buffer, int width, int height) override {
        if (!m_callback || !buffer || width <= 0 || height <= 0) return;

        std::size_t const bytes = static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4;
        std::uint8_t const* const pixels = static_cast<std::uint8_t const*>(buffer);

        if (type == PET_VIEW) {
            m_view.assign(pixels, pixels + bytes);  // the page, kept so a popup repaint has something to sit on
            m_view_width = width;
            m_view_height = height;

            if (!m_popup_open || m_popup.empty()) {
                m_callback(buffer, width, height);
                return;
            }
        } else {
            if (!m_popup_open) return;

            m_popup.assign(pixels, pixels + bytes);
            m_popup_width = width;
            m_popup_height = height;

            if (m_view.empty()) return;  // no page yet
        }

        DefineComposite();
        m_callback(m_frame.data(), m_view_width, m_view_height);
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
    // m_frame = the last page with the popup copied over it at m_popup_rect, clipped to the view
    void DefineComposite() {
        m_frame = m_view;

        int const x0 = std::max(m_popup_rect.x, 0);
        int const x1 = std::min(m_popup_rect.x + m_popup_width, m_view_width);
        if (x1 <= x0) return;

        std::size_t const row_bytes = static_cast<std::size_t>(x1 - x0) * 4;

        for (int row = 0; row < m_popup_height; row++) {
            int const y = m_popup_rect.y + row;
            if (y < 0 || y >= m_view_height) continue;

            std::uint8_t const* const from = m_popup.data() + (static_cast<std::size_t>(row) * m_popup_width + (x0 - m_popup_rect.x)) * 4;
            std::uint8_t* const to = m_frame.data() + (static_cast<std::size_t>(y) * m_view_width + x0) * 4;

            std::memcpy(to, from, row_bytes);
        }
    }

   private:
    PaintCallback m_callback{};

    std::atomic<int> m_width{1280};

    std::atomic<int> m_height{720};

    // the page's last paint
    std::vector<std::uint8_t> m_view{};

    int m_view_width{0};

    int m_view_height{0};

    // the open popup -- its pixels and where it sits
    bool m_popup_open{false};

    std::vector<std::uint8_t> m_popup{};

    int m_popup_width{0};

    int m_popup_height{0};

    CefRect m_popup_rect{};

    std::vector<std::uint8_t> m_frame{};  // page + popup, what the overlay gets while a popup is open

   private:
    IMPLEMENT_REFCOUNTING(CEFRenderHandler);
};

}  // namespace NSIr77CEF