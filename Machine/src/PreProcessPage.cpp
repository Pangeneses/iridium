#include "pch.h"
#include "PreProcessPage.xaml.h"
#if __has_include("PreProcessPage.g.cpp")
#include "PreProcessPage.g.cpp"
#endif

namespace winrt::Machine::m_implementation {
PreProcessPage::PreProcessPage() {
    InitializeComponent();

    return;
}

}  // namespace winrt::Machine::m_implementation
