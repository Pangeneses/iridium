#pragma once

#include "winrt/IIr77PROJECT.h"

#include "IDXIr77SAMPLER.g.h"

namespace winrt::IIr77PROJECT::m_implementation {
struct IDXIr77SAMPLER {
   public:
    IDXIr77SAMPLER() = default;

   public:
    static winrt::guid HVIDXIr77SAMPLER();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{winrt::single_threaded_vector<winrt::guid>(std::vector<winrt::guid>{HVIDXIr77SAMPLER()})};
    }
};
}  // namespace winrt::IIr77PROJECT::m_implementation

namespace winrt::IIr77PROJECT::factory_implementation {
struct IDXIr77SAMPLER : IDXIr77SAMPLERT<IDXIr77SAMPLER, m_implementation::IDXIr77SAMPLER> {};
}  // namespace winrt::IIr77PROJECT::factory_implementation
