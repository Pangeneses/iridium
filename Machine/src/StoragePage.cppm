#pragma once

#include "winrt/Ir77X.h"

#include "StoragePage.g.h"

namespace winrt::Machine::m_implementation {
struct StoragePage : StoragePageT<StoragePage> {
   public:
    StoragePage();
};
}  // namespace winrt::Machine::m_implementation

namespace winrt::Machine::factory_implementation {
struct StoragePage : StoragePageT<StoragePage, m_implementation::StoragePage> {};

}  // namespace winrt::Machine::factory_implementation
