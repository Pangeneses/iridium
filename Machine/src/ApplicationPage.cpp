#include "pch.h"
#include "ApplicationPage.xaml.h"
#if __has_include("ApplicationPage.g.cpp")
#include "ApplicationPage.g.cpp"
#endif

namespace winrt::Machine::m_implementation {
ApplicationPage::ApplicationPage() {
    InitializeComponent();

    return;
}

}  // namespace winrt::Machine::m_implementation
