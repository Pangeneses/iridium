#include "pch.h"
#include "BitmapPage.xaml.h"
#if __has_include("BitmapPage.g.cpp")
#include "BitmapPage.g.cpp"
#endif

namespace winrt::Machine::m_implementation {
BitmapPage::BitmapPage() {
    InitializeComponent();

    return;
}

}  // namespace winrt::Machine::m_implementation
