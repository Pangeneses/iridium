#pragma once

#include "winrt/IIr77FILESYS.h"

#include "IDOIr77DIRECTORY.g.h"

namespace winrt::IIr77FILESYS::m_implementation {
struct IDOIr77DIRECTORY {
   public:
    IDOIr77DIRECTORY() = default;

   public:
    static winrt::guid HVIDOIr77DIRECTORY();
    static winrt::guid HVIDOIr77LOADIMG();
    static winrt::guid HVIDOIr77PARKIMG();
    static winrt::guid HVIDOIr77REFACTOR();
    static winrt::guid HVIDOIr77ROOT();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{winrt::single_threaded_vector<winrt::guid>(
            std::vector<winrt::guid>{HVIDOIr77DIRECTORY(), HVIDOIr77LOADIMG(), HVIDOIr77PARKIMG(), HVIDOIr77REFACTOR(), HVIDOIr77ROOT()})};
    }
};
}  // namespace winrt::IIr77FILESYS::m_implementation

namespace winrt::IIr77FILESYS::factory_implementation {
struct IDOIr77DIRECTORY : IDOIr77DIRECTORYT<IDOIr77DIRECTORY, m_implementation::IDOIr77DIRECTORY> {};
}  // namespace winrt::IIr77FILESYS::factory_implementation
