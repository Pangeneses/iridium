#pragma once

#include "winrt/Ir77X.h"

#include "MemoryPage.g.h"

namespace winrt::Machine::m_implementation {
struct MemoryPage : MemoryPageT<MemoryPage> {
    MemoryPage();
};

}  // namespace winrt::Machine::m_implementation

namespace winrt::Machine::factory_implementation {
struct MemoryPage : MemoryPageT<MemoryPage, m_implementation::MemoryPage> {};
}  // namespace winrt::Machine::factory_implementation
