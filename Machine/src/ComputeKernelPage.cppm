#pragma once

#include "winrt/Ir77X.h"

#include "ComputeKernelPage.g.h"

namespace winrt::Machine::m_implementation {
struct ComputeKernelPage : ComputeKernelPageT<ComputeKernelPage> {
   public:
    ComputeKernelPage();
};
}  // namespace winrt::Machine::m_implementation

namespace winrt::Machine::factory_implementation {
struct ComputeKernelPage : ComputeKernelPageT<ComputeKernelPage, m_implementation::ComputeKernelPage> {};

}  // namespace winrt::Machine::factory_implementation
