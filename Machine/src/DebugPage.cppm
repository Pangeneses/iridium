#pragma once

#include "winrt/Ir77X.h"

#include "DebugPage.g.h"

namespace winrt::Machine::m_implementation {
struct DebugPage : DebugPageT<DebugPage> {
   public:
    DebugPage();
};
}  // namespace winrt::Machine::m_implementation

namespace winrt::Machine::factory_implementation {
struct DebugPage : DebugPageT<DebugPage, m_implementation::DebugPage> {};
}  // namespace winrt::Machine::factory_implementation
