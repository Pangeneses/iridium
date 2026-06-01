#pragma once

#include "winrt/Ir77X.h"

#include "LibraryPage.g.h"

namespace winrt::Machine::m_implementation {
struct LibraryPage : LibraryPageT<LibraryPage> {
   public:
    LibraryPage();
};

}  // namespace winrt::Machine::m_implementation

namespace winrt::Machine::factory_implementation {
struct LibraryPage : LibraryPageT<LibraryPage, m_implementation::LibraryPage> {};
}  // namespace winrt::Machine::factory_implementation
