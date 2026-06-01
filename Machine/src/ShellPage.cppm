#pragma once

#include "winrt/Ir77X.h"

#include "ShellPage.g.h"

namespace winrt::Machine::m_implementation {
struct ShellPage : ShellPageT<ShellPage> {
   public:
    ShellPage();
};

}  // namespace winrt::Machine::m_implementation

namespace winrt::Machine::factory_implementation {
struct ShellPage : ShellPageT<ShellPage, m_implementation::ShellPage> {};

}  // namespace winrt::Machine::factory_implementation
