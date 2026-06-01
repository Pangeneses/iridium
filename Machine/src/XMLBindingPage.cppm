#pragma once

#include "winrt/Ir77X.h"

#include "XMLBindingPage.g.h"

namespace winrt::Machine::m_implementation {
struct XMLBindingPage : XMLBindingPageT<XMLBindingPage> {
   public:
    XMLBindingPage();
};
}  // namespace winrt::Machine::m_implementation

namespace winrt::Machine::factory_implementation {
struct XMLBindingPage : XMLBindingPageT<XMLBindingPage, m_implementation::XMLBindingPage> {};

}  // namespace winrt::Machine::factory_implementation
