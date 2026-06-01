#pragma once

#include "winrt/Ir77X.h"

#include "SoftPartitionPage.g.h"

namespace winrt::Machine::m_implementation {
struct SoftPartitionPage : SoftPartitionPageT<SoftPartitionPage> {
   public:
    SoftPartitionPage();
};
}  // namespace winrt::Machine::m_implementation

namespace winrt::Machine::factory_implementation {
struct SoftPartitionPage : SoftPartitionPageT<SoftPartitionPage, m_implementation::SoftPartitionPage> {};

}  // namespace winrt::Machine::factory_implementation
