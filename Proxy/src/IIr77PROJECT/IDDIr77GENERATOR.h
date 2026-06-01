#pragma once

#include "winrt/IIr77PROJECT.h"

#include "IDDIr77GENERATOR.g.h"

namespace winrt::IIr77PROJECT::m_implementation {
struct IDDIr77GENERATOR {
   public:
    IDDIr77GENERATOR() = default;

   public:
    static winrt::guid HVIDDIr77GENERATOR();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{
            winrt::single_threaded_vector<winrt::guid>(std::vector<winrt::guid>{HVIDDIr77GENERATOR()})};
    }
};
}  // namespace winrt::IIr77PROJECT::m_implementation

namespace winrt::IIr77PROJECT::factory_implementation {
struct IDDIr77GENERATOR : IDDIr77GENERATORT<IDDIr77GENERATOR, m_implementation::IDDIr77GENERATOR> {};
}  // namespace winrt::IIr77PROJECT::factory_implementation
