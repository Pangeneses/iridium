#pragma once

#include "winrt/Ir77X.h"

#include "QIFBindingPage.g.h"

namespace winrt::Machine::m_implementation {
struct QIFBindingPage : QIFBindingPageT<QIFBindingPage> {
   public:
    QIFBindingPage();
};
}  // namespace winrt::Machine::m_implementation

namespace winrt::Machine::factory_implementation {
struct QIFBindingPage : QIFBindingPageT<QIFBindingPage, m_implementation::QIFBindingPage> {};

}  // namespace winrt::Machine::factory_implementation
