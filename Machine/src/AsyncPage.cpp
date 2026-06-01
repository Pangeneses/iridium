#include "pch.h"
#include "AsyncPage.xaml.h"
#if __has_include("AsyncPage.g.cpp")
#include "AsyncPage.g.cpp"
#endif

namespace winrt::Machine::m_implementation {
AsyncPage::AsyncPage() {
    InitializeComponent();

    return;
}

}  // namespace winrt::Machine::m_implementation
