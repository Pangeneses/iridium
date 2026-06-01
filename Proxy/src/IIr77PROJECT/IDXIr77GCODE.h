#pragma once

#include "winrt/IIr77PROJECT.h"

#include "IDXIr77GCODE.g.h"

namespace winrt::IIr77PROJECT::m_implementation {
struct IDXIr77GCODE {
   public:
    IDXIr77GCODE() = default;

   public:
    static winrt::guid HVIDXIr77GCODE();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{winrt::single_threaded_vector<winrt::guid>(std::vector<winrt::guid>{HVIDXIr77GCODE()})};
    }
};
}  // namespace winrt::IIr77PROJECT::m_implementation

namespace winrt::IIr77PROJECT::factory_implementation {
struct IDXIr77GCODE : IDXIr77GCODET<IDXIr77GCODE, m_implementation::IDXIr77GCODE> {};
}  // namespace winrt::IIr77PROJECT::factory_implementation
