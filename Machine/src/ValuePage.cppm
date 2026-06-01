#pragma once

#include "winrt/Ir77X.h"

#include "ValuePage.g.h"

namespace winrt::Machine::m_implementation {
struct ValuePage : ValuePageT<ValuePage> {
   public:
    ValuePage();
};
}  // namespace winrt::Machine::m_implementation

namespace winrt::Machine::factory_implementation {
struct ValuePage : ValuePageT<ValuePage, m_implementation::ValuePage> {};

}  // namespace winrt::Machine::factory_implementation
