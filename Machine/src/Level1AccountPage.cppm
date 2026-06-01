#pragma once

#include "winrt/Ir77X.h"

#include "Level1AccountPage.g.h"

namespace winrt::Machine::m_implementation {
struct Level1AccountPage : Level1AccountPageT<Level1AccountPage> {
   public:
    Level1AccountPage();
};

}  // namespace winrt::Machine::m_implementation

namespace winrt::Machine::factory_implementation {
struct Level1AccountPage : Level1AccountPageT<Level1AccountPage, m_implementation::Level1AccountPage> {};

}  // namespace winrt::Machine::factory_implementation
