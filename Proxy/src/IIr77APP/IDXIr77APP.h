#pragma once

#include "winrt/IIr77BASE.h"
#include "winrt/IIr77APP.h"

#include "IDXIr77APP.g.h"

namespace winrt::IIr77APP::m_implementation {
struct IDXIr77APP {
   public:
    IDXIr77APP() = default;

   public:
    static winrt::guid HVIDXIr77APP();
    static winrt::guid HVIDXIr77HEALTH();
    static winrt::guid HVIDXIr77NAV();
    static winrt::guid HVIDXIr77PAGE();
    static winrt::guid HVIDXIr77PROJECT();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{winrt::single_threaded_vector<winrt::guid>(
            std::vector<winrt::guid>{HVIDXIr77APP(), HVIDXIr77HEALTH(), HVIDXIr77NAV(), HVIDXIr77PAGE(), HVIDXIr77PROJECT()})};
    }
};
}  // namespace winrt::IIr77APP::m_implementation

namespace winrt::IIr77APP::factory_implementation {
struct IDXIr77APP : IDXIr77APPT<IDXIr77APP, m_implementation::IDXIr77APP> {};
}  // namespace winrt::IIr77APP::factory_implementation
