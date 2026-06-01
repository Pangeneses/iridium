#pragma once

#include "winrt/IIr77GRAPHICS.h"

#include "IDXIr77ALPHAMODE.g.h"

namespace winrt::IIr77GRAPHICS::m_implementation {
struct IDXIr77ALPHAMODE {
   public:
    IDXIr77ALPHAMODE() = default;

   public:
    static winrt::guid Ir77X_DXGI_ALPHA_MODE();
    static winrt::guid Ir77X_DXGI_ALPHA_MODE_UNSPECIFIED();
    static winrt::guid Ir77X_DXGI_ALPHA_MODE_PREMULTIPLIED();
    static winrt::guid Ir77X_DXGI_ALPHA_MODE_STRAIGHT();
    static winrt::guid Ir77X_DXGI_ALPHA_MODE_IGNORE();
    static winrt::guid Ir77X_DXGI_ALPHA_MODE_FORCE_DWORD();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{winrt::single_threaded_vector<winrt::guid>(
            std::vector<winrt::guid>{Ir77X_DXGI_ALPHA_MODE(), Ir77X_DXGI_ALPHA_MODE_UNSPECIFIED(), Ir77X_DXGI_ALPHA_MODE_PREMULTIPLIED(),
                                     Ir77X_DXGI_ALPHA_MODE_STRAIGHT(), Ir77X_DXGI_ALPHA_MODE_IGNORE(), Ir77X_DXGI_ALPHA_MODE_FORCE_DWORD()})};
    }
};
}  // namespace winrt::IIr77GRAPHICS::m_implementation

namespace winrt::IIr77GRAPHICS::factory_implementation {
struct IDXIr77ALPHAMODE : IDXIr77ALPHAMODET<IDXIr77ALPHAMODE, m_implementation::IDXIr77ALPHAMODE> {};
}  // namespace winrt::IIr77GRAPHICS::factory_implementation
