#include "pch.h"
#include "ProfilePage.xaml.h"
#if __has_include("ProfilePage.g.cpp")
#include "ProfilePage.g.cpp"
#endif

namespace winrt::Machine::m_implementation {
ProfilePage::ProfilePage() {
    InitializeComponent();

    return;
}

void ProfilePage::Loaded(WF::IInspectable const& sender, MUX::RoutedEventArgs const& e) { return; }

}  // namespace winrt::Machine::m_implementation
