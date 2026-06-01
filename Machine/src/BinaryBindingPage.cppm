#pragma once

#include "winrt/Ir77X.h"

#include "BinaryBindingPage.g.h"

namespace winrt::Machine::m_implementation {
struct BinaryBindingPage : BinaryBindingPageT<BinaryBindingPage> {
   public:
    BinaryBindingPage();
};
}  // namespace winrt::Machine::m_implementation

namespace winrt::Machine::factory_implementation {
struct BinaryBindingPage : BinaryBindingPageT<BinaryBindingPage, m_implementation::BinaryBindingPage> {};

}  // namespace winrt::Machine::factory_implementation
