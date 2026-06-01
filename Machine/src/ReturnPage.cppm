#pragma once

#include "winrt/Ir77X.h"

#include "ReturnPage.g.h"

namespace winrt::Machine::m_implementation {
struct ReturnPage : ReturnPageT<ReturnPage> {
   public:
    ReturnPage();
};
}  // namespace winrt::Machine::m_implementation

namespace winrt::Machine::factory_implementation {
struct ReturnPage : ReturnPageT<ReturnPage, m_implementation::ReturnPage> {};

}  // namespace winrt::Machine::factory_implementation
