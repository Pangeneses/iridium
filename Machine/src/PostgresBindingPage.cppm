#pragma once

#include "winrt/Ir77X.h"

#include "PostgresBindingPage.g.h"

namespace winrt::Machine::m_implementation {
struct PostgresBindingPage : PostgresBindingPageT<PostgresBindingPage> {
   public:
    PostgresBindingPage();
};
}  // namespace winrt::Machine::m_implementation

namespace winrt::Machine::factory_implementation {
struct PostgresBindingPage : PostgresBindingPageT<PostgresBindingPage, m_implementation::PostgresBindingPage> {};

}  // namespace winrt::Machine::factory_implementation
