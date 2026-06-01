#pragma once

#include "winrt/Ir77X.h"

#include "PipePage.g.h"

namespace winrt::Machine::m_implementation {
struct PipePage : PipePageT<PipePage> {
   public:
    PipePage();
};
}  // namespace winrt::Machine::m_implementation

namespace winrt::Machine::factory_implementation {
struct PipePage : PipePageT<PipePage, m_implementation::PipePage> {};

}  // namespace winrt::Machine::factory_implementation
