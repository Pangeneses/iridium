#pragma once

#include "winrt/Ir77X.h"

#include "Level4AccountPage.g.h"

namespace winrt::Machine::m_implementation {
struct Level4AccountPage : Level4AccountPageT<Level4AccountPage> {
   public:
    Level4AccountPage();
};
}  // namespace winrt::Machine::m_implementation

namespace winrt::Machine::factory_implementation {
struct Level4AccountPage : Level4AccountPageT<Level4AccountPage, m_implementation::Level4AccountPage> {};

}  // namespace winrt::Machine::factory_implementation
