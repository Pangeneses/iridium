#pragma once

#include "winrt/IIr77PROJECT.h"

#include "IDXIr77SCRIPT.g.h"

namespace winrt::IIr77PROJECT::m_implementation {
struct IDXIr77SCRIPT {
   public:
    IDXIr77SCRIPT() = default;

   public:
    static winrt::guid HVIDXIr77SCRIPT();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{winrt::single_threaded_vector<winrt::guid>(std::vector<winrt::guid>{HVIDXIr77SCRIPT()})};
    }
};
}  // namespace winrt::IIr77PROJECT::m_implementation

namespace winrt::IIr77PROJECT::factory_implementation {
struct IDXIr77SCRIPT : IDXIr77SCRIPTT<IDXIr77SCRIPT, m_implementation::IDXIr77SCRIPT> {};
}  // namespace winrt::IIr77PROJECT::factory_implementation
