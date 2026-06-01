#pragma once

#include "winrt/IIr77GRAPHICS.h"

#include "IDXIr77CULLMODE.g.h"

namespace winrt::IIr77GRAPHICS::m_implementation {
struct IDXIr77CULLMODE {
   public:
    IDXIr77CULLMODE() = default;

   public:
    static winrt::guid HVIDX_D3D12_CULL_MODE();
    static winrt::guid HVIDX_D3D12_CULL_MODE_NONE();
    static winrt::guid HVIDX_D3D12_CULL_MODE_FRONT();
    static winrt::guid HVIDX_D3D12_CULL_MODE_BACK();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{winrt::single_threaded_vector<winrt::guid>(
            std::vector<winrt::guid>{HVIDX_D3D12_CULL_MODE(), HVIDX_D3D12_CULL_MODE_NONE(), HVIDX_D3D12_CULL_MODE_FRONT(), HVIDX_D3D12_CULL_MODE_BACK()})};
    }
};
}  // namespace winrt::IIr77GRAPHICS::m_implementation

namespace winrt::IIr77GRAPHICS::factory_implementation {
struct IDXIr77CULLMODE : IDXIr77CULLMODET<IDXIr77CULLMODE, m_implementation::IDXIr77CULLMODE> {};
}  // namespace winrt::IIr77GRAPHICS::factory_implementation
