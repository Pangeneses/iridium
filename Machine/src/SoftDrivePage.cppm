#pragma once

#include "winrt/Ir77X.h"

#include "SoftDrivePage.g.h"

namespace winrt::Machine::m_implementation {
struct SoftDrivePage : SoftDrivePageT<SoftDrivePage> {
   public:
    SoftDrivePage();
};
}  // namespace winrt::Machine::m_implementation

namespace winrt::Machine::factory_implementation {
struct SoftDrivePage : SoftDrivePageT<SoftDrivePage, m_implementation::SoftDrivePage> {};

}  // namespace winrt::Machine::factory_implementation
