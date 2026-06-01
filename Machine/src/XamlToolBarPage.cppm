#pragma once

#include "winrt/Ir77X.h"

#include "XamlToolBarPage.g.h"

namespace winrt::Machine::m_implementation {
struct XamlToolBarPage : XamlToolBarPageT<XamlToolBarPage> {
   public:
    XamlToolBarPage();
};
}  // namespace winrt::Machine::m_implementation

namespace winrt::Machine::factory_implementation {
struct XamlToolBarPage : XamlToolBarPageT<XamlToolBarPage, m_implementation::XamlToolBarPage> {};

}  // namespace winrt::Machine::factory_implementation
