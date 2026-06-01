#include "pch.h"
#include "WindowPage.xaml.h"
#if __has_include("WindowPage.g.cpp")
#include "WindowPage.g.cpp"
#endif

namespace winrt::Machine::m_implementation {
WindowPage::WindowPage() {
    InitializeComponent();

    return;
}

}  // namespace winrt::Machine::m_implementation
