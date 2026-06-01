#pragma once

#include "winrt/IIr77PROJECT.h"

#include "IDXIr77MATERIAL.g.h"

namespace winrt::IIr77PROJECT::m_implementation {
struct IDXIr77MATERIAL {
   public:
    IDXIr77MATERIAL() = default;

   public:
    static winrt::guid HVIDXIr77MATERIAL();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{
            winrt::single_threaded_vector<winrt::guid>(std::vector<winrt::guid>{HVIDXIr77MATERIAL()})};
    }
};
}  // namespace winrt::IIr77PROJECT::m_implementation

namespace winrt::IIr77PROJECT::factory_implementation {
struct IDXIr77MATERIAL : IDXIr77MATERIALT<IDXIr77MATERIAL, m_implementation::IDXIr77MATERIAL> {};
}  // namespace winrt::IIr77PROJECT::factory_implementation
