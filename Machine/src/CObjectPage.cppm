#pragma once

#include "winrt/Ir77X.h"

#include "CObjectPage.g.h"

namespace winrt::Machine::m_implementation {
struct CObjectPage : CObjectPageT<CObjectPage> {
   public:
    CObjectPage();
};
}  // namespace winrt::Machine::m_implementation

namespace winrt::Machine::factory_implementation {
struct CObjectPage : CObjectPageT<CObjectPage, m_implementation::CObjectPage> {};

}  // namespace winrt::Machine::factory_implementation
