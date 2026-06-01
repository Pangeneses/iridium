#pragma once

#include "winrt/Ir77X.h"

#include "AcceleratorPage.g.h"

namespace winrt::Machine::m_implementation {
struct AcceleratorPage : AcceleratorPageT<AcceleratorPage> {
   public:
    AcceleratorPage();
};
}  // namespace winrt::Machine::m_implementation

namespace winrt::Machine::factory_implementation {
struct AcceleratorPage : AcceleratorPageT<AcceleratorPage, m_implementation::AcceleratorPage> {};

}  // namespace winrt::Machine::factory_implementation
