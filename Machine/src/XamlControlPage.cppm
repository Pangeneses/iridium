#pragma once

#include "winrt/Ir77X.h"

#include "XamlControlPage.g.h"

namespace winrt::Machine::m_implementation {
struct XamlControlPage : XamlControlPageT<XamlControlPage> {
   public:
    XamlControlPage();
};
}  // namespace winrt::Machine::m_implementation

namespace winrt::Machine::factory_implementation {
struct XamlControlPage : XamlControlPageT<XamlControlPage, m_implementation::XamlControlPage> {};

}  // namespace winrt::Machine::factory_implementation
