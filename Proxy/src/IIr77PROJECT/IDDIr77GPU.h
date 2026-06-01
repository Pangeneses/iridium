#pragma once

#include "winrt/IIr77PROJECT.h"

#include "IDDIr77GPU.g.h"

namespace winrt::IIr77PROJECT::m_implementation {
struct IDDIr77GPU {
   public:
    IDDIr77GPU() = default;

   public:
    static winrt::guid HVIDDIr77GPU();
    static winrt::guid HVIDDIr77PIPELINE();
    static winrt::guid HVIDDIr77CONTEXT();
    static winrt::guid HVIDDIr77NILOBJ();
    static winrt::guid HVIDDIr77TEXTURE();
    static winrt::guid HVIDDIr77SHADER();
    static winrt::guid HVIDDIr77BUFFER();
    static winrt::guid HVIDDIr77SAMPLER();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{
            winrt::single_threaded_vector<winrt::guid>(std::vector<winrt::guid>{HVIDDIr77GPU(), HVIDDIr77PIPELINE(), HVIDDIr77CONTEXT(), HVIDDIr77NILOBJ(),
                                                                                HVIDDIr77TEXTURE(), HVIDDIr77SHADER(), HVIDDIr77BUFFER(), HVIDDIr77SAMPLER()})};
    }
};
}  // namespace winrt::IIr77PROJECT::m_implementation

namespace winrt::IIr77PROJECT::factory_implementation {
struct IDDIr77GPU : IDDIr77GPUT<IDDIr77GPU, m_implementation::IDDIr77GPU> {};
}  // namespace winrt::IIr77PROJECT::factory_implementation
