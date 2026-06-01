#include "pch.h"
#include "IDXIr77SWAPEFFECT.h"
#if __has_include("IDXIr77SWAPEFFECT.g.cpp")
#include "IDXIr77SWAPEFFECT.g.cpp"
#endif

namespace winrt::IIr77GRAPHICS::m_implementation {
winrt::guid IDXIr77SWAPEFFECT::DXGI_SWAP_EFFECT() { return winrt::guid{"{B437C87C-AA60-4ED6-9142-4D8BD67E962C}"}; }
winrt::guid IDXIr77SWAPEFFECT::DXGI_SWAP_EFFECT_DISCARD() { return winrt::guid{"{CE2F9210-CCA0-4571-9F43-AB738F273DA8}"}; }
winrt::guid IDXIr77SWAPEFFECT::DXGI_SWAP_EFFECT_SEQUENTIAL() { return winrt::guid{"{14D4766B-C9DB-463D-9D14-DA0AF9D447B3}"}; }
winrt::guid IDXIr77SWAPEFFECT::DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL() { return winrt::guid{"{6A1BB3AD-88BB-4C97-9E75-299DD3B32A71}"}; }
winrt::guid IDXIr77SWAPEFFECT::DXGI_SWAP_EFFECT_FLIP_DISCARD() { return winrt::guid{"{D12B92C8-AE22-46CC-9C81-BB1F9B94C477}"}; }
}  // namespace winrt::IIr77GRAPHICS::m_implementation