#pragma once

#include "winrt/IIr77PROJECT.h"

#include "IDXIr77GENERATOR.g.h"

namespace winrt::IIr77PROJECT::m_implementation {
struct IDXIr77GENERATOR {
   public:
    IDXIr77GENERATOR() = default;

   public:
    static winrt::guid HVIDXIr77GENERATOR();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{
            winrt::single_threaded_vector<winrt::guid>(std::vector<winrt::guid>{HVIDXIr77GENERATOR()})};
    }
};
}  // namespace winrt::IIr77PROJECT::m_implementation

namespace winrt::IIr77PROJECT::factory_implementation {
struct IDXIr77GENERATOR : IDXIr77GENERATORT<IDXIr77GENERATOR, m_implementation::IDXIr77GENERATOR> {};
}  // namespace winrt::IIr77PROJECT::factory_implementation
