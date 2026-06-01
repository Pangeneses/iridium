#pragma once

#include "winrt/IIr77PROJECT.h"

#include "IDDIr77VERSIONING.g.h"

namespace winrt::IIr77PROJECT::m_implementation {
struct IDDIr77VERSIONING {
   public:
    IDDIr77VERSIONING() = default;

   public:
    static winrt::guid HVIDDIr77VERSIONING();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{
            winrt::single_threaded_vector<winrt::guid>(std::vector<winrt::guid>{HVIDDIr77VERSIONING()})};
    }
};
}  // namespace winrt::IIr77PROJECT::m_implementation

namespace winrt::IIr77PROJECT::factory_implementation {
struct IDDIr77VERSIONING : IDDIr77VERSIONINGT<IDDIr77VERSIONING, m_implementation::IDDIr77VERSIONING> {};
}  // namespace winrt::IIr77PROJECT::factory_implementation
