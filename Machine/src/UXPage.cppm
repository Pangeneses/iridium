#pragma once

#include "winrt/Ir77X.h"

#include "UXPage.g.h"

namespace winrt::Machine::m_implementation {
struct UXPage : UXPageT<UXPage> {
   public:
    UXPage();
};

}  // namespace winrt::Machine::m_implementation

namespace winrt::Machine::factory_implementation {
struct UXPage : UXPageT<UXPage, m_implementation::UXPage> {};
}  // namespace winrt::Machine::factory_implementation
