#pragma once

#include "winrt/Ir77X.h"

#include "RUNTIMEPage.g.h"

namespace winrt::Machine::m_implementation {
struct RUNTIMEPage : RUNTIMEPageT<RUNTIMEPage> {
   public:
    RUNTIMEPage();
};

}  // namespace winrt::Machine::m_implementation

namespace winrt::Machine::factory_implementation {
struct RUNTIMEPage : RUNTIMEPageT<RUNTIMEPage, m_implementation::RUNTIMEPage> {};
}  // namespace winrt::Machine::factory_implementation
