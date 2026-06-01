#pragma once

#include "winrt/IIr77GRAPHICS.h"

#include "IDOIr77GRAPHICS.g.h"

namespace winrt::IIr77GRAPHICS::m_implementation {
struct IDOIr77GRAPHICS {
   public:
    IDOIr77GRAPHICS() = default;

   public:
    static winrt::guid HVIDOIr77GRAPHICS();
    static winrt::guid HVIDOIr77GETSTATE();
    static winrt::guid HVIDOIr77SETSTATE();
    static winrt::guid HVIDOIr77SEATGPU();
    static winrt::guid HVIDOIr77RESETGPU();
    static winrt::guid HVIDOIr77RUNGPU();
    static winrt::guid HVIDOIr77STOPGPU();
    static winrt::guid HVIDOIr77KILLGPU();
    static winrt::guid HVIDOIr77BINDIA();
    static winrt::guid HVIDOIr77UNBINDIA();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{winrt::single_threaded_vector<winrt::guid>(
            std::vector<winrt::guid>{HVIDOIr77GRAPHICS(), HVIDOIr77GETSTATE(), HVIDOIr77SETSTATE(), HVIDOIr77SEATGPU(), HVIDOIr77RESETGPU(), HVIDOIr77RUNGPU(),
                                     HVIDOIr77STOPGPU(), HVIDOIr77KILLGPU(), HVIDOIr77BINDIA(), HVIDOIr77UNBINDIA()})};
    }
};
}  // namespace winrt::IIr77GRAPHICS::m_implementation

namespace winrt::IIr77GRAPHICS::factory_implementation {
struct IDOIr77GRAPHICS : IDOIr77GRAPHICST<IDOIr77GRAPHICS, m_implementation::IDOIr77GRAPHICS> {};
}  // namespace winrt::IIr77GRAPHICS::factory_implementation
