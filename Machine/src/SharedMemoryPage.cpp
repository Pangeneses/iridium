#include "pch.h"
#include "SharedMemoryPage.xaml.h"
#if __has_include("SharedMemoryPage.g.cpp")
#include "SharedMemoryPage.g.cpp"
#endif

namespace winrt::Machine::m_implementation {
SharedMemoryPage::SharedMemoryPage() {
    InitializeComponent();

    return;
}

}  // namespace winrt::Machine::m_implementation
