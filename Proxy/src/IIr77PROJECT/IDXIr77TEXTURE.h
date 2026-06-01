#pragma once

#include "winrt/IIr77PROJECT.h"

#include "IDXIr77TEXTURE.g.h"

namespace winrt::IIr77PROJECT::m_implementation {
struct IDXIr77TEXTURE {
   public:
    IDXIr77TEXTURE() = default;

   public:
    static winrt::guid HVIDXIr77TEXTURE();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{winrt::single_threaded_vector<winrt::guid>(std::vector<winrt::guid>{HVIDXIr77TEXTURE()})};
    }
};
}  // namespace winrt::IIr77PROJECT::m_implementation

namespace winrt::IIr77PROJECT::factory_implementation {
struct IDXIr77TEXTURE : IDXIr77TEXTURET<IDXIr77TEXTURE, m_implementation::IDXIr77TEXTURE> {};
}  // namespace winrt::IIr77PROJECT::factory_implementation
