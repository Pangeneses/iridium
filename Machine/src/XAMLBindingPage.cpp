#include "pch.h"
#include "XAMLBindingPage.xaml.h"
#if __has_include("XAMLBindingPage.g.cpp")
#include "XAMLBindingPage.g.cpp"
#endif

namespace winrt::Machine::m_implementation {
XAMLBindingPage::XAMLBindingPage() {
    InitializeComponent();

    return;
}

}  // namespace winrt::Machine::m_implementation
