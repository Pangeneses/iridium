#pragma once

#include "winrt/Ir77X.h"

#include "XAMLBindingPage.g.h"

namespace winrt::Machine::m_implementation {
struct XAMLBindingPage : XAMLBindingPageT<XAMLBindingPage> {
   public:
    XAMLBindingPage();
};
}  // namespace winrt::Machine::m_implementation

namespace winrt::Machine::factory_implementation {
struct XAMLBindingPage : XAMLBindingPageT<XAMLBindingPage, m_implementation::XAMLBindingPage> {};

}  // namespace winrt::Machine::factory_implementation
