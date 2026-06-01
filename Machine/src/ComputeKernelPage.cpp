#include "pch.h"
#include "ComputeKernelPage.xaml.h"
#if __has_include("ComputeKernelPage.g.cpp")
#include "ComputeKernelPage.g.cpp"
#endif

namespace winrt::Machine::m_implementation {
ComputeKernelPage::ComputeKernelPage() {
    InitializeComponent();

    return;
}

}  // namespace winrt::Machine::m_implementation
