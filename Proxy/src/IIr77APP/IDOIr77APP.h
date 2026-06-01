#pragma once

#include "winrt/IIr77BASE.h"
#include "winrt/IIr77APP.h"

#include "IDOIr77APP.g.h"

namespace winrt::IIr77APP::m_implementation {
struct IDOIr77APP {
   public:
    IDOIr77APP() = default;

   public:
    static winrt::guid HVIDOIr77APP();
    static winrt::guid HVIDOIr77LOAD();
    static winrt::guid HVIDOIr77UNLOAD();
    static winrt::guid HVIDOIr77PARK();
    static winrt::guid HVIDOIr77SETSTATE();
    static winrt::guid HVIDOIr77GETSTATE();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{winrt::single_threaded_vector<winrt::guid>(
            std::vector<winrt::guid>{HVIDOIr77APP(), HVIDOIr77LOAD(), HVIDOIr77UNLOAD(), HVIDOIr77PARK(), HVIDOIr77SETSTATE(), HVIDOIr77GETSTATE()})};
    }
};
}  // namespace winrt::IIr77APP::m_implementation

namespace winrt::IIr77APP::factory_implementation {
struct IDOIr77APP : IDOIr77APPT<IDOIr77APP, m_implementation::IDOIr77APP> {};
}  // namespace winrt::IIr77APP::factory_implementation
