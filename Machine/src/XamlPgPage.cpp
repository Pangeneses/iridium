#include "pch.h"
#include "XamlPgPage.xaml.h"
#if __has_include("XamlPgPage.g.cpp")
#include "XamlPgPage.g.cpp"
#endif

namespace winrt::Machine::m_implementation {
XamlPgPage::XamlPgPage() {
    InitializeComponent();

    return;
}

}  // namespace winrt::Machine::m_implementation
