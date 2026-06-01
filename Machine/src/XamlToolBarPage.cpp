#include "pch.h"
#include "XamlToolBarPage.xaml.h"
#if __has_include("XamlToolBarPage.g.cpp")
#include "XamlToolBarPage.g.cpp"
#endif

namespace winrt::Machine::m_implementation {
XamlToolBarPage::XamlToolBarPage() {
    InitializeComponent();

    return;
}

}  // namespace winrt::Machine::m_implementation
