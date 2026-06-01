#pragma once

#include "winrt/IIr77FILESYS.h"

#include "IDXIr77DIRECTORY.g.h"

namespace winrt::IIr77FILESYS::m_implementation {
struct IDXIr77DIRECTORY {
   public:
    IDXIr77DIRECTORY() = default;

   public:
    static winrt::guid HVIDXIr77DIRECTORY();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{
            winrt::single_threaded_vector<winrt::guid>(std::vector<winrt::guid>{HVIDXIr77DIRECTORY()})};
    }
};
}  // namespace winrt::IIr77FILESYS::m_implementation

namespace winrt::IIr77FILESYS::factory_implementation {
struct IDXIr77DIRECTORY : IDXIr77DIRECTORYT<IDXIr77DIRECTORY, m_implementation::IDXIr77DIRECTORY> {};
}  // namespace winrt::IIr77FILESYS::factory_implementation
