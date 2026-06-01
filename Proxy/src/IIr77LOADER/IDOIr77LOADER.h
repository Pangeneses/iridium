#pragma once

#include "winrt/IIr77LOADER.h"

#include "IDOIr77LOADER.g.h"

namespace winrt::IIr77LOADER::m_implementation {
struct IDOIr77LOADER {
   public:
    IDOIr77LOADER() = default;

   public:
    static winrt::guid HVIDOIr77LOADER();
    static winrt::guid HVIDOIr77LOAD();
    static winrt::guid HVIDOIr77UNLOAD();
    static winrt::guid HVIDOIr77SETSTATE();
    static winrt::guid HVIDOIr77GETSTATE();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{winrt::single_threaded_vector<winrt::guid>(
            std::vector<winrt::guid>{HVIDOIr77LOADER(), HVIDOIr77LOAD(), HVIDOIr77UNLOAD(), HVIDOIr77SETSTATE(), HVIDOIr77GETSTATE()})};
    }
};
}  // namespace winrt::IIr77LOADER::m_implementation

namespace winrt::IIr77LOADER::factory_implementation {
struct IDOIr77LOADER : IDOIr77LOADERT<IDOIr77LOADER, m_implementation::IDOIr77LOADER> {};
}  // namespace winrt::IIr77LOADER::factory_implementation
