#include "pch.h"
#include "DebugPage.xaml.h"
#if __has_include("DebugPage.g.cpp")
#include "DebugPage.g.cpp"
#endif

namespace winrt::Machine::m_implementation {
DebugPage::DebugPage() {
    InitializeComponent();

    return;
}

}  // namespace winrt::Machine::m_implementation
