#pragma once

#include "winrt/IIr77PROJECT.h"

#include "IDXIr77VERSIONING.g.h"

namespace winrt::IIr77PROJECT::m_implementation {
struct IDXIr77VERSIONING {
   public:
    IDXIr77VERSIONING() = default;

   public:
    static winrt::guid HVIDXIr77VERSIONING();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{
            winrt::single_threaded_vector<winrt::guid>(std::vector<winrt::guid>{HVIDXIr77VERSIONING()})};
    }
};
}  // namespace winrt::IIr77PROJECT::m_implementation

namespace winrt::IIr77PROJECT::factory_implementation {
struct IDXIr77VERSIONING : IDXIr77VERSIONINGT<IDXIr77VERSIONING, m_implementation::IDXIr77VERSIONING> {};
}  // namespace winrt::IIr77PROJECT::factory_implementation
