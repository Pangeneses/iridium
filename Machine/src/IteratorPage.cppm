#pragma once

#include "winrt/Ir77X.h"

#include "IteratorPage.g.h"

namespace winrt::Machine::m_implementation {
struct IteratorPage : IteratorPageT<IteratorPage> {
   public:
    IteratorPage();
};
}  // namespace winrt::Machine::m_implementation

namespace winrt::Machine::factory_implementation {
struct IteratorPage : IteratorPageT<IteratorPage, m_implementation::IteratorPage> {};

}  // namespace winrt::Machine::factory_implementation
