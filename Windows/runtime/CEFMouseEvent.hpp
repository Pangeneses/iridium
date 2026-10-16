#pragma once

#include <SDL3/SDL.h>

#include <cstdint>
#include <string>

#include "include/cef_browser.h"

// SDL3 input -> CEF (windowless). Keys: CEF wants Windows virtual-key codes in windows_key_code and the X11 keycode in native_key_code --
// SDL keycodes are neither (letters are lowercase ASCII, arrows / F-keys are 0x40000000 | scancode), so they are translated, never passed
// through. Typed text comes from SDL_EVENT_TEXT_INPUT as KEYEVENT_CHAR, which needs SDL_StartTextInput on the window.

inline CefBrowserHost::MouseButtonType SDLButtonToCEF(uint8_t btn) {
    switch (btn) {
        case SDL_BUTTON_RIGHT:  return MBT_RIGHT;
        case SDL_BUTTON_MIDDLE: return MBT_MIDDLE;
        default:                return MBT_LEFT;
    }
}

// Held keyboard modifiers and mouse buttons -> CEF event flags
inline uint32_t SDLModifiersToCEF(SDL_Keymod const mod, SDL_MouseButtonFlags const buttons) {
    uint32_t flags{0};

    if (mod & SDL_KMOD_SHIFT) flags |= EVENTFLAG_SHIFT_DOWN;
    if (mod & SDL_KMOD_CTRL) flags |= EVENTFLAG_CONTROL_DOWN;
    if (mod & SDL_KMOD_ALT) flags |= EVENTFLAG_ALT_DOWN;
    if (mod & SDL_KMOD_GUI) flags |= EVENTFLAG_COMMAND_DOWN;
    if (mod & SDL_KMOD_CAPS) flags |= EVENTFLAG_CAPS_LOCK_ON;
    if (mod & SDL_KMOD_NUM) flags |= EVENTFLAG_NUM_LOCK_ON;

    if (buttons & SDL_BUTTON_LMASK) flags |= EVENTFLAG_LEFT_MOUSE_BUTTON;
    if (buttons & SDL_BUTTON_MMASK) flags |= EVENTFLAG_MIDDLE_MOUSE_BUTTON;
    if (buttons & SDL_BUTTON_RMASK) flags |= EVENTFLAG_RIGHT_MOUSE_BUTTON;

    return flags;
}

// SDL keycode -> Windows virtual-key code (what Chromium builds the DOM key / code from). 0 = no mapping -- the key isn't sent.
inline int SDLKeyToWindows(SDL_Keycode const key) {
    if (key >= SDLK_A && key <= SDLK_Z) return static_cast<int>(key - SDLK_A) + 0x41;  // VK 'A'..'Z' are uppercase
    if (key >= SDLK_0 && key <= SDLK_9) return static_cast<int>(key - SDLK_0) + 0x30;
    if (key >= SDLK_F1 && key <= SDLK_F12) return static_cast<int>(key - SDLK_F1) + 0x70;
    if (key >= SDLK_KP_1 && key <= SDLK_KP_9) return static_cast<int>(key - SDLK_KP_1) + 0x61;

    switch (key) {
        case SDLK_BACKSPACE:    return 0x08;
        case SDLK_TAB:          return 0x09;
        case SDLK_RETURN:       return 0x0D;
        case SDLK_KP_ENTER:     return 0x0D;
        case SDLK_LSHIFT:
        case SDLK_RSHIFT:       return 0x10;
        case SDLK_LCTRL:
        case SDLK_RCTRL:        return 0x11;
        case SDLK_LALT:
        case SDLK_RALT:         return 0x12;
        case SDLK_PAUSE:        return 0x13;
        case SDLK_CAPSLOCK:     return 0x14;
        case SDLK_ESCAPE:       return 0x1B;
        case SDLK_SPACE:        return 0x20;
        case SDLK_PAGEUP:       return 0x21;
        case SDLK_PAGEDOWN:     return 0x22;
        case SDLK_END:          return 0x23;
        case SDLK_HOME:         return 0x24;
        case SDLK_LEFT:         return 0x25;
        case SDLK_UP:           return 0x26;
        case SDLK_RIGHT:        return 0x27;
        case SDLK_DOWN:         return 0x28;
        case SDLK_INSERT:       return 0x2D;
        case SDLK_DELETE:       return 0x2E;
        case SDLK_LGUI:         return 0x5B;
        case SDLK_RGUI:         return 0x5C;
        case SDLK_KP_0:         return 0x60;
        case SDLK_KP_MULTIPLY:  return 0x6A;
        case SDLK_KP_PLUS:      return 0x6B;
        case SDLK_KP_MINUS:     return 0x6D;
        case SDLK_KP_PERIOD:    return 0x6E;
        case SDLK_KP_DIVIDE:    return 0x6F;
        case SDLK_NUMLOCKCLEAR: return 0x90;
        case SDLK_SCROLLLOCK:   return 0x91;
        case SDLK_SEMICOLON:    return 0xBA;
        case SDLK_EQUALS:       return 0xBB;
        case SDLK_COMMA:        return 0xBC;
        case SDLK_MINUS:        return 0xBD;
        case SDLK_PERIOD:       return 0xBE;
        case SDLK_SLASH:        return 0xBF;
        case SDLK_GRAVE:        return 0xC0;
        case SDLK_LEFTBRACKET:  return 0xDB;
        case SDLK_BACKSLASH:    return 0xDC;
        case SDLK_RIGHTBRACKET: return 0xDD;
        case SDLK_APOSTROPHE:   return 0xDE;
        default:                return 0;
    }
}

// The character a key types, for the key-down event's character fields (US layout, Shift for letters). 0 for non-printing keys.
// Real typed text -- other layouts, shifted symbols -- arrives separately through SDL_EVENT_TEXT_INPUT.
inline char16_t SDLKeyCharacter(SDL_Keycode const key, SDL_Keymod const mod) {
    if (key < 0x20 || key >= 0x7F) return 0;

    if (key >= SDLK_A && key <= SDLK_Z && (mod & SDL_KMOD_SHIFT)) return static_cast<char16_t>(key - SDLK_A + 'A');

    return static_cast<char16_t>(key);
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
            me.modifiers = SDLModifiersToCEF(SDL_GetModState(), e->motion.state);  // held buttons -- drags
            host->SendMouseMoveEvent(me, false);
            break;
        }

        // ── Mouse button ──────────────────────────────────────────────────
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP: {
            CefMouseEvent me{};
            me.x = static_cast<int>(e->button.x);
            me.y = static_cast<int>(e->button.y);
            me.modifiers = SDLModifiersToCEF(SDL_GetModState(), SDL_GetMouseState(nullptr, nullptr));
            host->SendMouseClickEvent(me, SDLButtonToCEF(e->button.button), e->type == SDL_EVENT_MOUSE_BUTTON_UP, e->button.clicks);
            break;
        }

        // ── Scroll ────────────────────────────────────────────────────────
        case SDL_EVENT_MOUSE_WHEEL: {
            CefMouseEvent me{};
            float mx, my;
            SDL_MouseButtonFlags const buttons = SDL_GetMouseState(&mx, &my);
            me.x = static_cast<int>(mx);
            me.y = static_cast<int>(my);
            me.modifiers = SDLModifiersToCEF(SDL_GetModState(), buttons);
            host->SendMouseWheelEvent(me,
                static_cast<int>(e->wheel.x * 20),
                static_cast<int>(e->wheel.y * 20));
            break;
        }

        // ── Key down / up -- keydown / keyup in the page (event.key, event.code) ─
        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP: {
            int const windows_key = SDLKeyToWindows(e->key.key);
            if (windows_key == 0) break;

            char16_t const character = SDLKeyCharacter(e->key.key, e->key.mod);

            CefKeyEvent ke{};
            ke.type                 = e->type == SDL_EVENT_KEY_DOWN ? KEYEVENT_RAWKEYDOWN : KEYEVENT_KEYUP;
            ke.windows_key_code     = windows_key;
            ke.native_key_code      = e->key.raw;  // X11 keycode on Linux -- the physical key (event.code)
            ke.modifiers            = SDLModifiersToCEF(e->key.mod, SDL_GetMouseState(nullptr, nullptr));
            ke.character            = character;
            ke.unmodified_character = character;
            ke.is_system_key        = false;
            if (e->key.repeat) ke.modifiers |= EVENTFLAG_IS_REPEAT;
            host->SendKeyEvent(ke);

            // Enter / Tab / Backspace / Escape type nothing through SDL_EVENT_TEXT_INPUT, but the page expects a char event for them
            if (e->type == SDL_EVENT_KEY_DOWN && (windows_key == 0x0D || windows_key == 0x09 || windows_key == 0x08 || windows_key == 0x1B)) {
                ke.type = KEYEVENT_CHAR;
                ke.character = ke.unmodified_character = static_cast<char16_t>(windows_key);
                ke.windows_key_code = windows_key;
                host->SendKeyEvent(ke);
            }
            break;
        }

        // ── Typed text -- keypress / input in the page. One char event per UTF-16 unit ─
        case SDL_EVENT_TEXT_INPUT: {
            std::u16string text{};
            for (char const* p = e->text.text; p && *p;) {
                Uint32 const codepoint = SDL_StepUTF8(&p, nullptr);
                if (codepoint == 0) break;

                if (codepoint >= 0x10000) {
                    Uint32 const v = codepoint - 0x10000;
                    text.push_back(static_cast<char16_t>(0xD800 + (v >> 10)));
                    text.push_back(static_cast<char16_t>(0xDC00 + (v & 0x3FF)));
                } else {
                    text.push_back(static_cast<char16_t>(codepoint));
                }
            }

            for (char16_t const unit : text) {
                CefKeyEvent ke{};
                ke.type                 = KEYEVENT_CHAR;
                ke.windows_key_code     = unit;
                ke.character            = unit;
                ke.unmodified_character = unit;
                ke.modifiers            = SDLModifiersToCEF(SDL_GetModState(), SDL_GetMouseState(nullptr, nullptr));
                host->SendKeyEvent(ke);
            }
            break;
        }

        default:
            break;
    }
}