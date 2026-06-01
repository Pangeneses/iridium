#pragma once

#include "winrt/IIr77PROJECT.h"

#include "IDOIr77GENERATOR.g.h"

namespace winrt::IIr77PROJECT::m_implementation {
struct IDOIr77GENERATOR {
   public:
    IDOIr77GENERATOR() = default;

   public:
    static winrt::guid HVIDOIr77GENERATOR();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{
            winrt::single_threaded_vector<winrt::guid>(std::vector<winrt::guid>{HVIDOIr77GENERATOR()})};
    }
};
}  // namespace winrt::IIr77PROJECT::m_implementation

namespace winrt::IIr77PROJECT::factory_implementation {
struct IDOIr77GENERATOR : IDOIr77GENERATORT<IDOIr77GENERATOR, m_implementation::IDOIr77GENERATOR> {};
}  // namespace winrt::IIr77PROJECT::factory_implementation
