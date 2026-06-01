#pragma once

#include "winrt/Ir77X.h"

#include "Level3AccountPage.g.h"

namespace winrt::Machine::m_implementation {
struct Level3AccountPage : Level3AccountPageT<Level3AccountPage> {
   public:
    Level3AccountPage();
};
}  // namespace winrt::Machine::m_implementation

namespace winrt::Machine::factory_implementation {
struct Level3AccountPage : Level3AccountPageT<Level3AccountPage, m_implementation::Level3AccountPage> {};

}  // namespace winrt::Machine::factory_implementation
