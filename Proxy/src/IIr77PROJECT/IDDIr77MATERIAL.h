#pragma once

#include "winrt/IIr77PROJECT.h"

#include "IDDIr77MATERIAL.g.h"

namespace winrt::IIr77PROJECT::m_implementation {
struct IDDIr77MATERIAL {
   public:
    IDDIr77MATERIAL() = default;

   public:
    static winrt::guid HVIDDIr77MATERIAL();
    static winrt::guid HVIDDIr77DDSFILE();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{
            winrt::single_threaded_vector<winrt::guid>(std::vector<winrt::guid>{HVIDDIr77MATERIAL(), HVIDDIr77DDSFILE()})};
    }
};
}  // namespace winrt::IIr77PROJECT::m_implementation

namespace winrt::IIr77PROJECT::factory_implementation {
struct IDDIr77MATERIAL : IDDIr77MATERIALT<IDDIr77MATERIAL, m_implementation::IDDIr77MATERIAL> {};
}  // namespace winrt::IIr77PROJECT::factory_implementation
