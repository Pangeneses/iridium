#pragma once

#include "winrt/IIr77PROJECT.h"

#include "IDXIr77BOM.g.h"

namespace winrt::IIr77PROJECT::m_implementation {
struct IDXIr77BOM {
   public:
    IDXIr77BOM() = default;

   public:
    static winrt::guid HVIDXIr77BOM();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{winrt::single_threaded_vector<winrt::guid>(std::vector<winrt::guid>{HVIDXIr77BOM()})};
    }
};
}  // namespace winrt::IIr77PROJECT::m_implementation

namespace winrt::IIr77PROJECT::factory_implementation {
struct IDXIr77BOM : IDXIr77BOMT<IDXIr77BOM, m_implementation::IDXIr77BOM> {};
}  // namespace winrt::IIr77PROJECT::factory_implementation
