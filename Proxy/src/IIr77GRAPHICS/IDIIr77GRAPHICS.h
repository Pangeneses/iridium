#pragma once

#include "winrt/IIr77BASE.h"
#include "winrt/IIr77GRAPHICS.h"

#include "IDIIr77GRAPHICS.g.h"

namespace winrt::IIr77GRAPHICS::m_implementation {
struct IDIIr77GRAPHICS {
   public:
    IDIIr77GRAPHICS() = default;

   public:
    static winrt::guid HVIDIIr77GRAPHICS();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{
            winrt::single_threaded_vector<winrt::guid>(std::vector<winrt::guid>{HVIDIIr77GRAPHICS()})};
    }
};
}  // namespace winrt::IIr77GRAPHICS::m_implementation

namespace winrt::IIr77GRAPHICS::factory_implementation {
struct IDIIr77GRAPHICS : IDIIr77GRAPHICST<IDIIr77GRAPHICS, m_implementation::IDIIr77GRAPHICS> {};
}  // namespace winrt::IIr77GRAPHICS::factory_implementation
