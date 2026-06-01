#include "pch.h"
#include "XamlResourcePage.xaml.h"
#if __has_include("XamlResourcePage.g.cpp")
#include "XamlResourcePage.g.cpp"
#endif

namespace winrt::Machine::m_implementation {
XamlResourcePage::XamlResourcePage() {
    InitializeComponent();

    return;
}

}  // namespace winrt::Machine::m_implementation
