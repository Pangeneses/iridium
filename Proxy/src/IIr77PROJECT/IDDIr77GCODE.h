#pragma once

#include "winrt/IIr77PROJECT.h"

#include "IDDIr77GCODE.g.h"

namespace winrt::IIr77PROJECT::m_implementation {
struct IDDIr77GCODE {
   public:
    IDDIr77GCODE() = default;

   public:
    static winrt::guid HVIDDIr77GCODE();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{winrt::single_threaded_vector<winrt::guid>(std::vector<winrt::guid>{HVIDDIr77GCODE()})};
    }
};
}  // namespace winrt::IIr77PROJECT::m_implementation

namespace winrt::IIr77PROJECT::factory_implementation {
struct IDDIr77GCODE : IDDIr77GCODET<IDDIr77GCODE, m_implementation::IDDIr77GCODE> {};
}  // namespace winrt::IIr77PROJECT::factory_implementation
