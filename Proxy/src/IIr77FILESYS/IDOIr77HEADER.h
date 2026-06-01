#pragma once

#include "winrt/IIr77FILESYS.h"

#include "IDOIr77HEADER.g.h"

namespace winrt::IIr77FILESYS::m_implementation {
struct IDOIr77HEADER {
   public:
    IDOIr77HEADER() = default;

   public:
    static winrt::guid HVIDOIr77HEADER();
    static winrt::guid HVIDOIr77UPDATE();
    static winrt::guid HVIDOIr77READBUF();
    static winrt::guid HVIDOIr77WRITEBUF();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{winrt::single_threaded_vector<winrt::guid>(
            std::vector<winrt::guid>{HVIDOIr77HEADER(), HVIDOIr77UPDATE(), HVIDOIr77READBUF(), HVIDOIr77WRITEBUF()})};
    }
};
}  // namespace winrt::IIr77FILESYS::m_implementation

namespace winrt::IIr77FILESYS::factory_implementation {
struct IDOIr77HEADER : IDOIr77HEADERT<IDOIr77HEADER, m_implementation::IDOIr77HEADER> {};
}  // namespace winrt::IIr77FILESYS::factory_implementation
