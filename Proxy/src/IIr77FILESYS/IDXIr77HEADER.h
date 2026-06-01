#pragma once

#include "winrt/IIr77FILESYS.h"

#include "IDXIr77HEADER.g.h"

namespace winrt::IIr77FILESYS::m_implementation {
struct IDXIr77HEADER {
   public:
    IDXIr77HEADER() = default;

   public:
    static winrt::guid HVIDXIr77HEADER();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{winrt::single_threaded_vector<winrt::guid>(std::vector<winrt::guid>{HVIDXIr77HEADER()})};
    }
};
}  // namespace winrt::IIr77FILESYS::m_implementation

namespace winrt::IIr77FILESYS::factory_implementation {
struct IDXIr77HEADER : IDXIr77HEADERT<IDXIr77HEADER, m_implementation::IDXIr77HEADER> {};
}  // namespace winrt::IIr77FILESYS::factory_implementation
