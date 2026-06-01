#include "pch.h"
#include "PostgresBindingPage.xaml.h"
#if __has_include("PostgresBindingPage.g.cpp")
#include "PostgresBindingPage.g.cpp"
#endif

namespace winrt::Machine::m_implementation {
PostgresBindingPage::PostgresBindingPage() {
    InitializeComponent();

    return;
}

}  // namespace winrt::Machine::m_implementation
