#pragma once

#include "winrt/IIr77PROJECT.h"

#include "IDXIr77BUFFER.g.h"

namespace winrt::IIr77PROJECT::m_implementation {
struct IDXIr77BUFFER {
   public:
    IDXIr77BUFFER() = default;

   public:
    static winrt::guid HVIDXIr77BUFFER();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{winrt::single_threaded_vector<winrt::guid>(std::vector<winrt::guid>{HVIDXIr77BUFFER()})};
    }
};
}  // namespace winrt::IIr77PROJECT::m_implementation

namespace winrt::IIr77PROJECT::factory_implementation {
struct IDXIr77BUFFER : IDXIr77BUFFERT<IDXIr77BUFFER, m_implementation::IDXIr77BUFFER> {};
}  // namespace winrt::IIr77PROJECT::factory_implementation
