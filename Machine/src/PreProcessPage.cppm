#pragma once

#include "winrt/Ir77X.h"

#include "PreProcessPage.g.h"

namespace winrt::Machine::m_implementation {
struct PreProcessPage : PreProcessPageT<PreProcessPage> {
   public:
    PreProcessPage();
};
}  // namespace winrt::Machine::m_implementation

namespace winrt::Machine::factory_implementation {
struct PreProcessPage : PreProcessPageT<PreProcessPage, m_implementation::PreProcessPage> {};

}  // namespace winrt::Machine::factory_implementation
