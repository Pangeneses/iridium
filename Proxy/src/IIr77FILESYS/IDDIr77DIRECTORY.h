#pragma once

#include "winrt/IIr77FILESYS.h"

#include "IDDIr77DIRECTORY.g.h"

namespace winrt::IIr77FILESYS::m_implementation {
struct IDDIr77DIRECTORY {
   public:
    IDDIr77DIRECTORY() = default;

   public:
    static winrt::guid HVIDDIr77DIRECTORY();
    static winrt::guid HVIDDIr77LOAD();
    static winrt::guid HVIDDIr77ROOT();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{
            winrt::single_threaded_vector<winrt::guid>(std::vector<winrt::guid>{HVIDDIr77DIRECTORY(), HVIDDIr77LOAD(), HVIDDIr77ROOT()})};
    }
};
}  // namespace winrt::IIr77FILESYS::m_implementation

namespace winrt::IIr77FILESYS::factory_implementation {
struct IDDIr77DIRECTORY : IDDIr77DIRECTORYT<IDDIr77DIRECTORY, m_implementation::IDDIr77DIRECTORY> {};
}  // namespace winrt::IIr77FILESYS::factory_implementation
