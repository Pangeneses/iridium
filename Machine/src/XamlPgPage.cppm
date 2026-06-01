#pragma once

#include "winrt/Ir77X.h"

#include "XamlPgPage.g.h"

namespace winrt::Machine::m_implementation {
struct XamlPgPage : XamlPgPageT<XamlPgPage> {
   public:
    XamlPgPage();
};
}  // namespace winrt::Machine::m_implementation

namespace winrt::Machine::factory_implementation {
struct XamlPgPage : XamlPgPageT<XamlPgPage, m_implementation::XamlPgPage> {};

}  // namespace winrt::Machine::factory_implementation
