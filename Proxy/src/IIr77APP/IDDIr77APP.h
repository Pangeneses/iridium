#pragma once

#include "winrt/IIr77BASE.h"
#include "winrt/IIr77APP.h"

#include "IDDIr77APP.g.h"

namespace winrt::IIr77APP::m_implementation {
struct IDDIr77APP {
   public:
    IDDIr77APP() = default;

   public:
    static winrt::guid HVIDDIr77APP();
    static winrt::guid HVIDDIr77STATE();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{
            winrt::single_threaded_vector<winrt::guid>(std::vector<winrt::guid>{HVIDDIr77APP(), HVIDDIr77STATE()})};
    }
};
}  // namespace winrt::IIr77APP::m_implementation

namespace winrt::IIr77APP::factory_implementation {
struct IDDIr77APP : IDDIr77APPT<IDDIr77APP, m_implementation::IDDIr77APP> {};
}  // namespace winrt::IIr77APP::factory_implementation
