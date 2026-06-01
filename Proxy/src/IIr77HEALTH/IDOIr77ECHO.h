#pragma once

#include "winrt/IIr77BASE.h"
#include "winrt/IIr77HEALTH.h"

#include "IDOIr77ECHO.g.h"

namespace winrt::IIr77HEALTH::m_implementation {
struct IDOIr77ECHO {
   public:
    IDOIr77ECHO() = default;

   public:
    static winrt::guid HVIDOIr77ECHO();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{winrt::single_threaded_vector<winrt::guid>(std::vector<winrt::guid>{HVIDOIr77ECHO()})};
    }
};
}  // namespace winrt::IIr77HEALTH::m_implementation

namespace winrt::IIr77HEALTH::factory_implementation {
struct IDOIr77ECHO : IDOIr77ECHOT<IDOIr77ECHO, m_implementation::IDOIr77ECHO> {};
}  // namespace winrt::IIr77HEALTH::factory_implementation
