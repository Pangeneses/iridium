#pragma once

#include "winrt/IIr77PROJECT.h"

#include "IDOIr77GCODE.g.h"

namespace winrt::IIr77PROJECT::m_implementation {
struct IDOIr77GCODE {
   public:
    IDOIr77GCODE() = default;

   public:
    static winrt::guid HVIDOIr77GCODE();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{winrt::single_threaded_vector<winrt::guid>(std::vector<winrt::guid>{HVIDOIr77GCODE()})};
    }
};
}  // namespace winrt::IIr77PROJECT::m_implementation

namespace winrt::IIr77PROJECT::factory_implementation {
struct IDOIr77GCODE : IDOIr77GCODET<IDOIr77GCODE, m_implementation::IDOIr77GCODE> {};
}  // namespace winrt::IIr77PROJECT::factory_implementation
