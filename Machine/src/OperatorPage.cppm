#pragma once

#include "winrt/Ir77X.h"

#include "OperatorPage.g.h"

namespace winrt::Machine::m_implementation {
struct OperatorPage : OperatorPageT<OperatorPage> {
   public:
    OperatorPage();
};
}  // namespace winrt::Machine::m_implementation

namespace winrt::Machine::factory_implementation {
struct OperatorPage : OperatorPageT<OperatorPage, m_implementation::OperatorPage> {};

}  // namespace winrt::Machine::factory_implementation
