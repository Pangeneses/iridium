#include "pch.h"
#include "XamlControlPage.xaml.h"
#if __has_include("XamlControlPage.g.cpp")
#include "XamlControlPage.g.cpp"
#endif

namespace winrt::Machine::m_implementation {
XamlControlPage::XamlControlPage() {
    InitializeComponent();

    return;
}

}  // namespace winrt::Machine::m_implementation
