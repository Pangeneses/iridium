
#pragma once

#include <SDL3/SDL.h>

#include "include/cef_browser.h"

inline CefBrowserHost::MouseButtonType SDLButtonToCEF(uint8_t btn) {
    switch (btn) {
        case SDL_BUTTON_RIGHT:  return MBT_RIGHT;
        case SDL_BUTTON_MIDDLE: return MBT_MIDDLE;
        default:                return MBT_LEFT;
    }
}

inline void CEFInputEvent(const SDL_Event* e, CefRefPtr<CefBrowser> browser) {
    if (!browser) return;
    auto host = browser->GetHost();

    switch (e->type) {

        // ── Mouse move ────────────────────────────────────────────────────
        case SDL_EVENT_MOUSE_MOTION: {
            CefMouseEvent me{};
            me.x = static_cast<int>(e->motion.x);
            me.y = static_cast<int>(e->motion.y);
            host->SendMouseMoveEvent(me, false);
            break;
        }

        // ── Mouse button ──────────────────────────────────────────────────
        case SDL_EVENT_MOUSE_BUTTON_DOWN: {
            CefMouseEvent me{};
            me.x = static_cast<int>(e->button.x);
            me.y = static_cast<int>(e->button.y);
            host->SendMouseClickEvent(me, SDLButtonToCEF(e->button.button), false, 1);
            break;
        }
        case SDL_EVENT_MOUSE_BUTTON_UP: {
            CefMouseEvent me{};
            me.x = static_cast<int>(e->button.x);
            me.y = static_cast<int>(e->button.y);
            host->SendMouseClickEvent(me, SDLButtonToCEF(e->button.button), true, 1);
            break;
        }

        // ── Scroll ────────────────────────────────────────────────────────
        case SDL_EVENT_MOUSE_WHEEL: {
            CefMouseEvent me{};
            float mx, my;
            SDL_GetMouseState(&mx, &my);
            me.x = static_cast<int>(mx);
            me.y = static_cast<int>(my);
            host->SendMouseWheelEvent(me,
                static_cast<int>(e->wheel.x * 20),
                static_cast<int>(e->wheel.y * 20));
            break;
        }

        // ── Key down ──────────────────────────────────────────────────────
        case SDL_EVENT_KEY_DOWN: {
            CefKeyEvent ke{};
            ke.type         = KEYEVENT_RAWKEYDOWN;
            ke.windows_key_code  = e->key.key;
            ke.native_key_code   = e->key.scancode;
            ke.modifiers         = 0;
            host->SendKeyEvent(ke);

            // Also send character event for printable keys
            if (e->key.key >= 32 && e->key.key < 127) {
                ke.type            = KEYEVENT_CHAR;
                ke.character       = static_cast<char16_t>(e->key.key);
                host->SendKeyEvent(ke);
            }
            break;
        }

        // ── Key up ────────────────────────────────────────────────────────
        case SDL_EVENT_KEY_UP: {
            CefKeyEvent ke{};
            ke.type              = KEYEVENT_KEYUP;
            ke.windows_key_code  = e->key.key;
            ke.native_key_code   = e->key.scancode;
            host->SendKeyEvent(ke);
            break;
        }

        default:
            break;
    }
}
