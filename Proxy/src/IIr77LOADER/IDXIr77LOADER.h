#pragma once

#include "winrt/IIr77LOADER.h"

#include "IDXIr77LOADER.g.h"

namespace winrt::IIr77LOADER::m_implementation {
struct IDXIr77LOADER {
   public:
    IDXIr77LOADER() = default;

   public:
    static winrt::guid HVIDXIr77LOADER();
    static winrt::guid HVIDXIr77PATH();
    static winrt::guid HVIDXIr77PROJECT();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{
            winrt::single_threaded_vector<winrt::guid>(std::vector<winrt::guid>{HVIDXIr77LOADER(), HVIDXIr77PATH(), HVIDXIr77PROJECT()})};
    }
};
}  // namespace winrt::IIr77LOADER::m_implementation

namespace winrt::IIr77LOADER::factory_implementation {
struct IDXIr77LOADER : IDXIr77LOADERT<IDXIr77LOADER, m_implementation::IDXIr77LOADER> {};
}  // namespace winrt::IIr77LOADER::factory_implementation
