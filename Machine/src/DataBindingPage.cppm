#pragma once

#include "winrt/Ir77X.h"

#include "DataBindingPage.g.h"

namespace winrt::Machine::m_implementation {
struct DataBindingPage : DataBindingPageT<DataBindingPage> {
   public:
    DataBindingPage();
};

}  // namespace winrt::Machine::m_implementation

namespace winrt::Machine::factory_implementation {
struct DataBindingPage : DataBindingPageT<DataBindingPage, m_implementation::DataBindingPage> {};
}  // namespace winrt::Machine::factory_implementation
