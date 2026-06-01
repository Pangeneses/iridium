#pragma once

#include "winrt/Ir77X.h"

#include "DevicePage.g.h"

namespace winrt::Machine::m_implementation {
struct DevicePage : DevicePageT<DevicePage> {
   public:
    DevicePage();
};
}  // namespace winrt::Machine::m_implementation

namespace winrt::Machine::factory_implementation {
struct DevicePage : DevicePageT<DevicePage, m_implementation::DevicePage> {};

}  // namespace winrt::Machine::factory_implementation
