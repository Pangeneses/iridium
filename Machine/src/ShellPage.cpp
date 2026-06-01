#include "pch.h"
#include "ShellPage.xaml.h"
#if __has_include("ShellPage.g.cpp")
#include "ShellPage.g.cpp"
#endif

namespace winrt::Machine::m_implementation {
ShellPage::ShellPage() {
    InitializeComponent();

    return;
}

}  // namespace winrt::Machine::m_implementation
