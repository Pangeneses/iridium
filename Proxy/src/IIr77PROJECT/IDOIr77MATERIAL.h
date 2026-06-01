#pragma once

#include "winrt/IIr77PROJECT.h"

#include "IDOIr77MATERIAL.g.h"

namespace winrt::IIr77PROJECT::m_implementation {
struct IDOIr77MATERIAL {
   public:
    IDOIr77MATERIAL() = default;

   public:
    static winrt::guid HVIDOIr77MATERIAL();
    static winrt::guid HVIDOIr77CREATE();
    static winrt::guid HVIDOIr77DESTROY();
    static winrt::guid HVIDOIr77READ();
    static winrt::guid HVIDOIr77WRITE();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{winrt::single_threaded_vector<winrt::guid>(
            std::vector<winrt::guid>{HVIDOIr77MATERIAL(), HVIDOIr77CREATE(), HVIDOIr77DESTROY(), HVIDOIr77READ(), HVIDOIr77WRITE()})};
    }
};
}  // namespace winrt::IIr77PROJECT::m_implementation

namespace winrt::IIr77PROJECT::factory_implementation {
struct IDOIr77MATERIAL : IDOIr77MATERIALT<IDOIr77MATERIAL, m_implementation::IDOIr77MATERIAL> {};
}  // namespace winrt::IIr77PROJECT::factory_implementation
