#pragma once

#include "winrt/Ir77X.h"

#include "BitmapPage.g.h"

namespace winrt::Machine::m_implementation {
struct BitmapPage : BitmapPageT<BitmapPage> {
   public:
    BitmapPage();
};
}  // namespace winrt::Machine::m_implementation

namespace winrt::Machine::factory_implementation {
struct BitmapPage : BitmapPageT<BitmapPage, m_implementation::BitmapPage> {};

}  // namespace winrt::Machine::factory_implementation
