#include "pch.h"
#include "XamlContextMenuPage.xaml.h"
#if __has_include("XamlContextMenuPage.g.cpp")
#include "XamlContextMenuPage.g.cpp"
#endif

namespace winrt::Machine::m_implementation {
XamlContextMenuPage::XamlContextMenuPage() {
    InitializeComponent();

    return;
}

}  // namespace winrt::Machine::m_implementation
