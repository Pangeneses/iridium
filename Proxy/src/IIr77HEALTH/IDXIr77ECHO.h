#pragma once

#include "winrt/IIr77BASE.h"
#include "winrt/IIr77HEALTH.h"

#include "IDXIr77ECHO.g.h"

namespace winrt::IIr77HEALTH::m_implementation {
struct IDXIr77ECHO {
   public:
    IDXIr77ECHO() = default;

   public:
    static winrt::guid HVIDXIr77ECHO();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{winrt::single_threaded_vector<winrt::guid>(std::vector<winrt::guid>{HVIDXIr77ECHO()})};
    }
};
}  // namespace winrt::IIr77HEALTH::m_implementation

namespace winrt::IIr77HEALTH::factory_implementation {
struct IDXIr77ECHO : IDXIr77ECHOT<IDXIr77ECHO, m_implementation::IDXIr77ECHO> {};
}  // namespace winrt::IIr77HEALTH::factory_implementation
