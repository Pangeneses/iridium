#include "pch.h"
#include "BinaryBindingPage.xaml.h"
#if __has_include("BinaryBindingPage.g.cpp")
#include "BinaryBindingPage.g.cpp"
#endif

namespace winrt::Machine::m_implementation {
BinaryBindingPage::BinaryBindingPage() {
    InitializeComponent();

    return;
}

}  // namespace winrt::Machine::m_implementation
