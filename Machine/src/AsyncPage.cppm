#pragma once

#include "winrt/Ir77X.h"

#include "AsyncPage.g.h"

namespace winrt::Machine::m_implementation {
struct AsyncPage : AsyncPageT<AsyncPage> {
   public:
    AsyncPage();
};
}  // namespace winrt::Machine::m_implementation

namespace winrt::Machine::factory_implementation {
struct AsyncPage : AsyncPageT<AsyncPage, m_implementation::AsyncPage> {};

}  // namespace winrt::Machine::factory_implementation
