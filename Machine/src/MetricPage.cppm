#pragma once

#include "winrt/Ir77X.h"

#include "MetricPage.g.h"

namespace winrt::Machine::m_implementation {
struct MetricPage : MetricPageT<MetricPage> {
   public:
    MetricPage();
};
}  // namespace winrt::Machine::m_implementation

namespace winrt::Machine::factory_implementation {
struct MetricPage : MetricPageT<MetricPage, m_implementation::MetricPage> {};

}  // namespace winrt::Machine::factory_implementation
