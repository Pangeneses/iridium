#pragma once

#include "winrt/Ir77X.h"

#include "XamlResourcePage.g.h"

namespace winrt::Machine::m_implementation {
struct XamlResourcePage : XamlResourcePageT<XamlResourcePage> {
   public:
    XamlResourcePage();
};
}  // namespace winrt::Machine::m_implementation

namespace winrt::Machine::factory_implementation {
struct XamlResourcePage : XamlResourcePageT<XamlResourcePage, m_implementation::XamlResourcePage> {};

}  // namespace winrt::Machine::factory_implementation
