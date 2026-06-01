#pragma once

#include "winrt/IIr77GRAPHICS.h"

#include "IDXIr77FILLMODE.g.h"

namespace winrt::IIr77GRAPHICS::m_implementation {
struct IDXIr77FILLMODE {
   public:
    IDXIr77FILLMODE() = default;

   public:
    static winrt::guid HVIDX_D3D12_FILL_MODE();
    static winrt::guid HVIDX_D3D12_FILL_MODE_WIREFRAME();
    static winrt::guid HVIDX_D3D12_FILL_MODE_SOLID();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{winrt::single_threaded_vector<winrt::guid>(
            std::vector<winrt::guid>{HVIDX_D3D12_FILL_MODE(), HVIDX_D3D12_FILL_MODE_WIREFRAME(), HVIDX_D3D12_FILL_MODE_SOLID()})};
    }
};
}  // namespace winrt::IIr77GRAPHICS::m_implementation

namespace winrt::IIr77GRAPHICS::factory_implementation {
struct IDXIr77FILLMODE : IDXIr77FILLMODET<IDXIr77FILLMODE, m_implementation::IDXIr77FILLMODE> {};
}  // namespace winrt::IIr77GRAPHICS::factory_implementation
