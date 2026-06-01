#pragma once

#include "winrt/Ir77X.h"

#include "ArchiveURIPage.g.h"

namespace winrt::Machine::m_implementation {
struct ArchiveURIPage : ArchiveURIPageT<ArchiveURIPage> {
   public:
    ArchiveURIPage();
};

}  // namespace winrt::Machine::m_implementation

namespace winrt::Machine::factory_implementation {
struct ArchiveURIPage : ArchiveURIPageT<ArchiveURIPage, m_implementation::ArchiveURIPage> {};
}  // namespace winrt::Machine::factory_implementation
