#pragma once

#include "winrt/IIr77FILESYS.h"

#include "IDDIr77FOLDER.g.h"

namespace winrt::IIr77FILESYS::m_implementation {
struct IDDIr77FOLDER {
   public:
    IDDIr77FOLDER() = default;

   public:
    static winrt::guid HVIDDIr77FOLDER();
    static winrt::guid HVIDDIr77SEGMENT();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{
            winrt::single_threaded_vector<winrt::guid>(std::vector<winrt::guid>{HVIDDIr77FOLDER(), HVIDDIr77SEGMENT()})};
    }
};
}  // namespace winrt::IIr77FILESYS::m_implementation

namespace winrt::IIr77FILESYS::factory_implementation {
struct IDDIr77FOLDER : IDDIr77FOLDERT<IDDIr77FOLDER, m_implementation::IDDIr77FOLDER> {};
}  // namespace winrt::IIr77FILESYS::factory_implementation
