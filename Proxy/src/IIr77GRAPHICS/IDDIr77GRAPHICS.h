#pragma once

#include "winrt/IIr77GRAPHICS.h"

#include "IDDIr77GRAPHICS.g.h"

namespace winrt::IIr77GRAPHICS::m_implementation {
struct IDDIr77GRAPHICS {
   public:
    IDDIr77GRAPHICS() = default;

   public:
    static winrt::guid HVIDDIr77GRAPHICS();
    static winrt::guid HVIDDIr77GFXSTATE();
    static winrt::guid HVIDDIr77GFXOP();
    static winrt::guid HVIDDIr77GFXIA();
    static winrt::guid HVIDDIr77GFXSLOT();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{winrt::single_threaded_vector<winrt::guid>(
            std::vector<winrt::guid>{HVIDDIr77GRAPHICS(), HVIDDIr77GFXSTATE(), HVIDDIr77GFXOP(), HVIDDIr77GFXIA(), HVIDDIr77GFXSLOT()})};
    }
};
}  // namespace winrt::IIr77GRAPHICS::m_implementation

namespace winrt::IIr77GRAPHICS::factory_implementation {
struct IDDIr77GRAPHICS : IDDIr77GRAPHICST<IDDIr77GRAPHICS, m_implementation::IDDIr77GRAPHICS> {};
}  // namespace winrt::IIr77GRAPHICS::factory_implementation
