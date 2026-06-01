#include "pch.h"
#include "DevicePage.xaml.h"
#if __has_include("DevicePage.g.cpp")
#include "DevicePage.g.cpp"
#endif

namespace winrt::Machine::m_implementation {
DevicePage::DevicePage() {
    InitializeComponent();

    return;
}

}  // namespace winrt::Machine::m_implementation
