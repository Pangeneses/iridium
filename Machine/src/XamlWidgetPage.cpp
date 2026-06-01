#include "pch.h"
#include "XamlWidgetPage.xaml.h"
#if __has_include("XamlWidgetPage.g.cpp")
#include "XamlWidgetPage.g.cpp"
#endif

namespace winrt::Machine::m_implementation {
XamlWidgetPage::XamlWidgetPage() {
    InitializeComponent();

    return;
}

}  // namespace winrt::Machine::m_implementation
