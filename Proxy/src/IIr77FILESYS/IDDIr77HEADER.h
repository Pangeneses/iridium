#pragma once

#include "winrt/IIr77FILESYS.h"

#include "IDDIr77HEADER.g.h"

namespace winrt::IIr77FILESYS::m_implementation {
struct IDDIr77HEADER {
   public:
    IDDIr77HEADER() = default;

   public:
    static winrt::guid HVIDDIr77HEADER();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{winrt::single_threaded_vector<winrt::guid>(std::vector<winrt::guid>{HVIDDIr77HEADER()})};
    }
};
}  // namespace winrt::IIr77FILESYS::m_implementation

namespace winrt::IIr77FILESYS::factory_implementation {
struct IDDIr77HEADER : IDDIr77HEADERT<IDDIr77HEADER, m_implementation::IDDIr77HEADER> {};
}  // namespace winrt::IIr77FILESYS::factory_implementation
