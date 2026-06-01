#include "pch.h"
#include "UXPage.xaml.h"
#if __has_include("UXPage.g.cpp")
#include "UXPage.g.cpp"
#endif

namespace winrt::Machine::m_implementation {
UXPage::UXPage() {
    InitializeComponent();

    return;
}

}  // namespace winrt::Machine::m_implementation
