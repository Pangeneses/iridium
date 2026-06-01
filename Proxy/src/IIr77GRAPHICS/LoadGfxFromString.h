#pragma once

#include <winstring.h>

#include "winrt/IIr77BASE.h"
#include "winrt/IIr77GRAPHICS.h"

#include "LoadGfxFromString.g.h"

namespace WF = winrt::Windows::Foundation;
namespace MUX = winrt::Microsoft::UI::Xaml;
namespace MUXC = winrt::Microsoft::UI::Xaml::Controls;

namespace winrt::IIr77GRAPHICS::m_implementation {
struct LoadGfxFromString : LoadGfxFromStringT<LoadGfxFromString> {
   public:
    LoadGfxFromString(winrt::hstring file);

    // IIr77GRAPHICS::IIr77DEVICE Get();

   private:
    std::wstring gfxPipelineFile;

    std::wstring gfxPipelineName;
};
}  // namespace winrt::IIr77GRAPHICS::m_implementation

namespace winrt::IIr77GRAPHICS::factory_implementation {
struct LoadGfxFromString : LoadGfxFromStringT<LoadGfxFromString, m_implementation::LoadGfxFromString> {};
}  // namespace winrt::IIr77GRAPHICS::factory_implementation
