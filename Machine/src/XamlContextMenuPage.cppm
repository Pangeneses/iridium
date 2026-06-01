#pragma once

#include "winrt/Ir77X.h"

#include "XamlContextMenuPage.g.h"

namespace winrt::Machine::m_implementation {
struct XamlContextMenuPage : XamlContextMenuPageT<XamlContextMenuPage> {
   public:
    XamlContextMenuPage();
};
}  // namespace winrt::Machine::m_implementation

namespace winrt::Machine::factory_implementation {
struct XamlContextMenuPage : XamlContextMenuPageT<XamlContextMenuPage, m_implementation::XamlContextMenuPage> {};

}  // namespace winrt::Machine::factory_implementation
