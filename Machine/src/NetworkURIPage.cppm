#pragma once

#include "NetworkURIPage.g.h"

namespace winrt::Machine::m_implementation {
struct NetworkURIPage : NetworkURIPageT<NetworkURIPage> {
   public:
    NetworkURIPage();
};

}  // namespace winrt::Machine::m_implementation

namespace winrt::Machine::factory_implementation {
struct NetworkURIPage : NetworkURIPageT<NetworkURIPage, m_implementation::NetworkURIPage> {};
}  // namespace winrt::Machine::factory_implementation
