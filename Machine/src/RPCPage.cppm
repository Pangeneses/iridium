#pragma once

#include "winrt/Ir77X.h"

#include "RPCPage.g.h"

namespace winrt::Machine::m_implementation {
struct RPCPage : RPCPageT<RPCPage> {
   public:
    RPCPage();
};
}  // namespace winrt::Machine::m_implementation

namespace winrt::Machine::factory_implementation {
struct RPCPage : RPCPageT<RPCPage, m_implementation::RPCPage> {};

}  // namespace winrt::Machine::factory_implementation
