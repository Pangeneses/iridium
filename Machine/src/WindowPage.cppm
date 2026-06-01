#pragma once

#include "winrt/Ir77X.h"

#include "WindowPage.g.h"

namespace winrt::Machine::m_implementation {
struct WindowPage : WindowPageT<WindowPage> {
   public:
    WindowPage();
};
}  // namespace winrt::Machine::m_implementation

namespace winrt::Machine::factory_implementation {
struct WindowPage : WindowPageT<WindowPage, m_implementation::WindowPage> {};

}  // namespace winrt::Machine::factory_implementation
