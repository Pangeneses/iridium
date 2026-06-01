#pragma once

#include "winrt/IIr77LOADER.h"

#include "IDDIr77LOADER.g.h"

namespace winrt::IIr77LOADER::m_implementation {
struct IDDIr77LOADER {
   public:
    IDDIr77LOADER() = default;

   public:
    static winrt::guid HVIDDIr77LOADER();
    static winrt::guid HVIDDIr77STATE();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{
            winrt::single_threaded_vector<winrt::guid>(std::vector<winrt::guid>{HVIDDIr77LOADER(), HVIDDIr77STATE()})};
    }
};
}  // namespace winrt::IIr77LOADER::m_implementation

namespace winrt::IIr77LOADER::factory_implementation {
struct IDDIr77LOADER : IDDIr77LOADERT<IDDIr77LOADER, m_implementation::IDDIr77LOADER> {};
}  // namespace winrt::IIr77LOADER::factory_implementation
