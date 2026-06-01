#pragma once

#include "winrt/Ir77X.h"

#include "Level2AccountPage.g.h"

namespace winrt::Machine::m_implementation {
struct Level2AccountPage : Level2AccountPageT<Level2AccountPage> {
   public:
    Level2AccountPage();
};
}  // namespace winrt::Machine::m_implementation

namespace winrt::Machine::factory_implementation {
struct Level2AccountPage : Level2AccountPageT<Level2AccountPage, m_implementation::Level2AccountPage> {};

}  // namespace winrt::Machine::factory_implementation
