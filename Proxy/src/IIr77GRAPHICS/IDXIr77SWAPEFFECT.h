#pragma once

#include "winrt/IIr77GRAPHICS.h"

#include "IDXIr77SWAPEFFECT.g.h"

namespace winrt::IIr77GRAPHICS::m_implementation {
struct IDXIr77SWAPEFFECT {
   public:
    IDXIr77SWAPEFFECT() = default;

   public:
    static winrt::guid DXGI_SWAP_EFFECT();
    static winrt::guid DXGI_SWAP_EFFECT_DISCARD();
    static winrt::guid DXGI_SWAP_EFFECT_SEQUENTIAL();
    static winrt::guid DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL();
    static winrt::guid DXGI_SWAP_EFFECT_FLIP_DISCARD();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{
            winrt::single_threaded_vector<winrt::guid>(std::vector<winrt::guid>{DXGI_SWAP_EFFECT(), DXGI_SWAP_EFFECT_DISCARD(), DXGI_SWAP_EFFECT_SEQUENTIAL(),
                                                                                DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL(), DXGI_SWAP_EFFECT_FLIP_DISCARD()})};
    }
};
}  // namespace winrt::IIr77GRAPHICS::m_implementation

namespace winrt::IIr77GRAPHICS::factory_implementation {
struct IDXIr77SWAPEFFECT : IDXIr77SWAPEFFECTT<IDXIr77SWAPEFFECT, m_implementation::IDXIr77SWAPEFFECT> {};
}  // namespace winrt::IIr77GRAPHICS::factory_implementation
