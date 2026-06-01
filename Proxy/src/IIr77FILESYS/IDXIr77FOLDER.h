#pragma once

#include "winrt/IIr77FILESYS.h"

#include "IDXIr77FOLDER.g.h"

namespace winrt::IIr77FILESYS::m_implementation {
struct IDXIr77FOLDER {
   public:
    IDXIr77FOLDER() = default;

   public:
    static winrt::guid HVIDXIr77FOLDER();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{winrt::single_threaded_vector<winrt::guid>(std::vector<winrt::guid>{HVIDXIr77FOLDER()})};
    }
};
}  // namespace winrt::IIr77FILESYS::m_implementation

namespace winrt::IIr77FILESYS::factory_implementation {
struct IDXIr77FOLDER : IDXIr77FOLDERT<IDXIr77FOLDER, m_implementation::IDXIr77FOLDER> {};
}  // namespace winrt::IIr77FILESYS::factory_implementation
