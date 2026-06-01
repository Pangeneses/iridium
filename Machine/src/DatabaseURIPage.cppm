#pragma once

#include "winrt/Ir77X.h"

#include "DatabaseURIPage.g.h"

namespace winrt::Machine::m_implementation {
struct DatabaseURIPage : DatabaseURIPageT<DatabaseURIPage> {
   public:
    DatabaseURIPage();
};

}  // namespace winrt::Machine::m_implementation

namespace winrt::Machine::factory_implementation {
struct DatabaseURIPage : DatabaseURIPageT<DatabaseURIPage, m_implementation::DatabaseURIPage> {};
}  // namespace winrt::Machine::factory_implementation
