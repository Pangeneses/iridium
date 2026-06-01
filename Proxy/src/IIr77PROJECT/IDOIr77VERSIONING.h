#pragma once

#include "winrt/IIr77PROJECT.h"

#include "IDOIr77VERSIONING.g.h"

namespace winrt::IIr77PROJECT::m_implementation {
struct IDOIr77VERSIONING {
   public:
    IDOIr77VERSIONING() = default;

   public:
    static winrt::guid HVIDOIr77VERSIONING();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{
            winrt::single_threaded_vector<winrt::guid>(std::vector<winrt::guid>{HVIDOIr77VERSIONING()})};
    }
};
}  // namespace winrt::IIr77PROJECT::m_implementation

namespace winrt::IIr77PROJECT::factory_implementation {
struct IDOIr77VERSIONING : IDOIr77VERSIONINGT<IDOIr77VERSIONING, m_implementation::IDOIr77VERSIONING> {};
}  // namespace winrt::IIr77PROJECT::factory_implementation
