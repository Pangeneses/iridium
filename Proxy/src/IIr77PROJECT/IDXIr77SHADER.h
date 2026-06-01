#pragma once

#include "winrt/IIr77PROJECT.h"

#include "IDXIr77SHADER.g.h"

namespace winrt::IIr77PROJECT::m_implementation {
struct IDXIr77SHADER {
   public:
    IDXIr77SHADER() = default;

   public:
    static winrt::guid HVIDXIr77SHADER();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{winrt::single_threaded_vector<winrt::guid>(std::vector<winrt::guid>{HVIDXIr77SHADER()})};
    }
};
}  // namespace winrt::IIr77PROJECT::m_implementation

namespace winrt::IIr77PROJECT::factory_implementation {
struct IDXIr77SHADER : IDXIr77SHADERT<IDXIr77SHADER, m_implementation::IDXIr77SHADER> {};
}  // namespace winrt::IIr77PROJECT::factory_implementation
