#pragma once

#include "winrt/IIr77BASE.h"
#include "winrt/IIr77GRAPHICS.h"

#include "IDXIr77FILTERING.g.h"

namespace winrt::IIr77GRAPHICS::m_implementation {
struct IDXIr77FILTERING {
   public:
    IDXIr77FILTERING() = default;

   public:
    static winrt::guid HVIDXIr77FILTERING();
    static winrt::guid HVIDXIr77FILTERINGBI();
    static winrt::guid HVIDXIr77FILTERINGTRI();
    static winrt::guid HVIDXIr77FILTERINGANISO021();
    static winrt::guid HVIDXIr77FILTERINGANISO041();
    static winrt::guid HVIDXIr77FILTERINGANISO081();
    static winrt::guid HVIDXIr77FILTERINGANISO161();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{winrt::single_threaded_vector<winrt::guid>(
            std::vector<winrt::guid>{HVIDXIr77FILTERING(), HVIDXIr77FILTERINGBI(), HVIDXIr77FILTERINGTRI(), HVIDXIr77FILTERINGANISO021(),
                                     HVIDXIr77FILTERINGANISO041(), HVIDXIr77FILTERINGANISO081(), HVIDXIr77FILTERINGANISO161()})};
    }
};
}  // namespace winrt::IIr77GRAPHICS::m_implementation

namespace winrt::IIr77GRAPHICS::factory_implementation {
struct IDXIr77FILTERING : IDXIr77FILTERINGT<IDXIr77FILTERING, m_implementation::IDXIr77FILTERING> {};
}  // namespace winrt::IIr77GRAPHICS::factory_implementation
