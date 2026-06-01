#include "pch.h"
#include "JSONBindingPage.xaml.h"
#if __has_include("JSONBindingPage.g.cpp")
#include "JSONBindingPage.g.cpp"
#endif

namespace winrt::Machine::m_implementation {
JSONBindingPage::JSONBindingPage() {
    InitializeComponent();

    return;
}

}  // namespace winrt::Machine::m_implementation
