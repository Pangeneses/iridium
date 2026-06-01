#pragma once

#include "winrt/Ir77X.h"

#include "SharedMemoryPage.g.h"

namespace winrt::Machine::m_implementation {
struct SharedMemoryPage : SharedMemoryPageT<SharedMemoryPage> {
   public:
    SharedMemoryPage();
};
}  // namespace winrt::Machine::m_implementation

namespace winrt::Machine::factory_implementation {
struct SharedMemoryPage : SharedMemoryPageT<SharedMemoryPage, m_implementation::SharedMemoryPage> {};

}  // namespace winrt::Machine::factory_implementation
