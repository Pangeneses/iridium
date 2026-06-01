#pragma once

#include "DirectoryURIPage.g.h"

namespace winrt::Machine::m_implementation {
struct DirectoryURIPage : DirectoryURIPageT<DirectoryURIPage> {
   public:
    DirectoryURIPage();
};
}  // namespace winrt::Machine::m_implementation

namespace winrt::Machine::factory_implementation {
struct DirectoryURIPage : DirectoryURIPageT<DirectoryURIPage, m_implementation::DirectoryURIPage> {};
}  // namespace winrt::Machine::factory_implementation
