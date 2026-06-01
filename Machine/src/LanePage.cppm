#pragma once

#include "winrt/Ir77X.h"

#include "LanePage.g.h"

namespace winrt::Machine::m_implementation {
struct LanePage : LanePageT<LanePage> {
   public:
    LanePage();
};
}  // namespace winrt::Machine::m_implementation

namespace winrt::Machine::factory_implementation {
struct LanePage : LanePageT<LanePage, m_implementation::LanePage> {};

}  // namespace winrt::Machine::factory_implementation
