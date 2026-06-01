#pragma once

#include "winrt/Ir77X.h"

#include "TopologyPage.g.h"

namespace winrt::Machine::m_implementation {
struct TopologyPage : TopologyPageT<TopologyPage> {
   public:
    TopologyPage();
};
}  // namespace winrt::Machine::m_implementation

namespace winrt::Machine::factory_implementation {
struct TopologyPage : TopologyPageT<TopologyPage, m_implementation::TopologyPage> {};

}  // namespace winrt::Machine::factory_implementation
