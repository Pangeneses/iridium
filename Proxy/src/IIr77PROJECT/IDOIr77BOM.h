#pragma once

#include "winrt/IIr77PROJECT.h"

#include "IDOIr77BOM.g.h"

namespace winrt::IIr77PROJECT::m_implementation {
struct IDOIr77BOM {
   public:
    IDOIr77BOM() = default;

   public:
    static winrt::guid HVIDOIr77BOM();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{winrt::single_threaded_vector<winrt::guid>(std::vector<winrt::guid>{HVIDOIr77BOM()})};
    }
};
}  // namespace winrt::IIr77PROJECT::m_implementation

namespace winrt::IIr77PROJECT::factory_implementation {
struct IDOIr77BOM : IDOIr77BOMT<IDOIr77BOM, m_implementation::IDOIr77BOM> {};
}  // namespace winrt::IIr77PROJECT::factory_implementation
