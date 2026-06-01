#pragma once

#include "winrt/IIr77BASE.h"
#include "winrt/IIr77GRAPHICS.h"

#include "IDXIr77CODEC.g.h"

namespace winrt::IIr77GRAPHICS::m_implementation {
struct IDXIr77CODEC {
   public:
    IDXIr77CODEC() = default;

   public:
    static winrt::guid HVIDXIr77CODEC();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{winrt::single_threaded_vector<winrt::guid>(std::vector<winrt::guid>{HVIDXIr77CODEC()})};
    }
};
}  // namespace winrt::IIr77GRAPHICS::m_implementation

namespace winrt::IIr77GRAPHICS::factory_implementation {
struct IDXIr77CODEC : IDXIr77CODECT<IDXIr77CODEC, m_implementation::IDXIr77CODEC> {};
}  // namespace winrt::IIr77GRAPHICS::factory_implementation
