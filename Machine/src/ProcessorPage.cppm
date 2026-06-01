#pragma once

#include "winrt/Ir77X.h"

#include "ProcessorPage.g.h"

namespace winrt::Machine::m_implementation {
struct ProcessorPage : ProcessorPageT<ProcessorPage> {
   public:
    ProcessorPage();
};

}  // namespace winrt::Machine::m_implementation

namespace winrt::Machine::factory_implementation {
struct ProcessorPage : ProcessorPageT<ProcessorPage, m_implementation::ProcessorPage> {};
}  // namespace winrt::Machine::factory_implementation
