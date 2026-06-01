#pragma once

#include "winrt/IIr77FILESYS.h"

#include "IDOIr77FOLDER.g.h"

namespace winrt::IIr77FILESYS::m_implementation {
struct IDOIr77FOLDER {
   public:
    IDOIr77FOLDER() = default;

   public:
    static winrt::guid HVIDOIr77FOLDER();
    static winrt::guid HVIDOIr77OPEN();
    static winrt::guid HVIDOIr77CLOSE();
    static winrt::guid HVIDOIr77ADD();
    static winrt::guid HVIDOIr77RENAME();
    static winrt::guid HVIDOIr77DELETE();
    static winrt::guid HVIDOIr77NEWSEG();
    static winrt::guid HVIDOIr77DELSEG();
    static winrt::guid HVIDOIr77LOCKSEG();
    static winrt::guid HVIDOIr77PUTSEG();
    static winrt::guid HVIDOIr77FREESEG();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{winrt::single_threaded_vector<winrt::guid>(
            std::vector<winrt::guid>{HVIDOIr77FOLDER(), HVIDOIr77OPEN(), HVIDOIr77CLOSE(), HVIDOIr77ADD(), HVIDOIr77RENAME(), HVIDOIr77DELETE(),
                                     HVIDOIr77NEWSEG(), HVIDOIr77DELSEG(), HVIDOIr77LOCKSEG(), HVIDOIr77PUTSEG(), HVIDOIr77FREESEG()})};
    }
};
}  // namespace winrt::IIr77FILESYS::m_implementation

namespace winrt::IIr77FILESYS::factory_implementation {
struct IDOIr77FOLDER : IDOIr77FOLDERT<IDOIr77FOLDER, m_implementation::IDOIr77FOLDER> {};
}  // namespace winrt::IIr77FILESYS::factory_implementation
