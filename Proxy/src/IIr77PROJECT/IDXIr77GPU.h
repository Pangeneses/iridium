#pragma once

#include "winrt/IIr77PROJECT.h"

#include "IDXIr77GPU.g.h"

namespace winrt::IIr77PROJECT::m_implementation {
struct IDXIr77GPU {
   public:
    IDXIr77GPU() = default;

   public:
    static winrt::guid HVIDXIr77GPU();
    static winrt::guid HVIDXIr77CONTEXT();
    static winrt::guid HVIXIr77NILOBJ();
    static winrt::guid HVIDXIr77TEXTURE();
    static winrt::guid HVIDXIr77SHADER();
    static winrt::guid HVIDXIr77BUFFER();
    static winrt::guid HVIDXIr77SAMPLER();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{winrt::single_threaded_vector<winrt::guid>(std::vector<winrt::guid>{
            HVIDXIr77GPU(), HVIDXIr77CONTEXT(), HVIXIr77NILOBJ(), HVIDXIr77TEXTURE(), HVIDXIr77SHADER(), HVIDXIr77BUFFER(), HVIDXIr77SAMPLER()})};
    }
};
}  // namespace winrt::IIr77PROJECT::m_implementation

namespace winrt::IIr77PROJECT::factory_implementation {
struct IDXIr77GPU : IDXIr77GPUT<IDXIr77GPU, m_implementation::IDXIr77GPU> {};
}  // namespace winrt::IIr77PROJECT::factory_implementation
