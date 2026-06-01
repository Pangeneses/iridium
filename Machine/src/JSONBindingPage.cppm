#pragma once

#include "winrt/Ir77X.h"

#include "JSONBindingPage.g.h"

namespace winrt::Machine::m_implementation {
struct JSONBindingPage : JSONBindingPageT<JSONBindingPage> {
   public:
    JSONBindingPage();
};
}  // namespace winrt::Machine::m_implementation

namespace winrt::Machine::factory_implementation {
struct JSONBindingPage : JSONBindingPageT<JSONBindingPage, m_implementation::JSONBindingPage> {};

}  // namespace winrt::Machine::factory_implementation
