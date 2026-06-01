#pragma once

#include "winrt/IIr77BASE.h"
#include "winrt/IIr77HEALTH.h"

#include "IDDIr77ECHO.g.h"

namespace winrt::IIr77HEALTH::m_implementation {
struct IDDIr77ECHO {
   public:
    IDDIr77ECHO() = default;

   public:
    static winrt::guid HVIDDIr77ECHO();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{winrt::single_threaded_vector<winrt::guid>(std::vector<winrt::guid>{HVIDDIr77ECHO()})};
    }
};
}  // namespace winrt::IIr77HEALTH::m_implementation

namespace winrt::IIr77HEALTH::factory_implementation {
struct IDDIr77ECHO : IDDIr77ECHOT<IDDIr77ECHO, m_implementation::IDDIr77ECHO> {};
}  // namespace winrt::IIr77HEALTH::factory_implementation
