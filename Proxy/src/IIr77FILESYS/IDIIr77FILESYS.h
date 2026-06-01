#pragma once

#include "winrt/IIr77FILESYS.h"

#include "IDIIr77FILESYS.g.h"

namespace winrt::IIr77FILESYS::m_implementation {
struct IDIIr77FILESYS {
   public:
    IDIIr77FILESYS() = default;

   public:
    static winrt::guid HVIDIIr77FILESYS();
    static winrt::guid HVIDIIr77DIRECTORY();
    static winrt::guid HVIDIIr77FOLDER();
    static winrt::guid HVIDIIr77HEADER();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{winrt::single_threaded_vector<winrt::guid>(
            std::vector<winrt::guid>{HVIDIIr77FILESYS(), HVIDIIr77DIRECTORY(), HVIDIIr77FOLDER(), HVIDIIr77HEADER()})};
    }
};
}  // namespace winrt::IIr77FILESYS::m_implementation

namespace winrt::IIr77FILESYS::factory_implementation {
struct IDIIr77FILESYS : IDIIr77FILESYST<IDIIr77FILESYS, m_implementation::IDIIr77FILESYS> {};
}  // namespace winrt::IIr77FILESYS::factory_implementation
