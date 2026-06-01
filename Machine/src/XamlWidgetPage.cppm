#pragma once

#include "winrt/Ir77X.h"

#include "XamlWidgetPage.g.h"

namespace winrt::Machine::m_implementation {
struct XamlWidgetPage : XamlWidgetPageT<XamlWidgetPage> {
   public:
    XamlWidgetPage();
};
}  // namespace winrt::Machine::m_implementation

namespace winrt::Machine::factory_implementation {
struct XamlWidgetPage : XamlWidgetPageT<XamlWidgetPage, m_implementation::XamlWidgetPage> {};

}  // namespace winrt::Machine::factory_implementation
