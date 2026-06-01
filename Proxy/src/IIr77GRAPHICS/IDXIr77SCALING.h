#pragma once

#include "winrt/IIr77GRAPHICS.h"

#include "IDXIr77SCALING.g.h"

namespace winrt::IIr77GRAPHICS::m_implementation {
struct IDXIr77SCALING {
   public:
    IDXIr77SCALING() = default;

   public:
    static winrt::guid HVIDX_DXGI_SCALING();
    static winrt::guid HVIDX_DXGI_SCALING_STRETCH();
    static winrt::guid HVIDX_DXGI_SCALING_NONE();
    static winrt::guid HVIDX_DXGI_SCALING_ASPECT_RATIO_STRETCH();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{winrt::single_threaded_vector<winrt::guid>(std::vector<winrt::guid>{
            HVIDX_DXGI_SCALING(), HVIDX_DXGI_SCALING_STRETCH(), HVIDX_DXGI_SCALING_NONE(), HVIDX_DXGI_SCALING_ASPECT_RATIO_STRETCH()})};
    }
};
}  // namespace winrt::IIr77GRAPHICS::m_implementation

namespace winrt::IIr77GRAPHICS::factory_implementation {
struct IDXIr77SCALING : IDXIr77SCALINGT<IDXIr77SCALING, m_implementation::IDXIr77SCALING> {};
}  // namespace winrt::IIr77GRAPHICS::factory_implementation
