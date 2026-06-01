#pragma once

#include "winrt/IIr77PROJECT.h"

#include "IDDIr77BOM.g.h"

namespace winrt::IIr77PROJECT::m_implementation {
struct IDDIr77BOM {
   public:
    IDDIr77BOM() = default;

   public:
    static winrt::guid HVIDDIr77BOM();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{winrt::single_threaded_vector<winrt::guid>(std::vector<winrt::guid>{HVIDDIr77BOM()})};
    }
};
}  // namespace winrt::IIr77PROJECT::m_implementation

namespace winrt::IIr77PROJECT::factory_implementation {
struct IDDIr77BOM : IDDIr77BOMT<IDDIr77BOM, m_implementation::IDDIr77BOM> {};
}  // namespace winrt::IIr77PROJECT::factory_implementation
