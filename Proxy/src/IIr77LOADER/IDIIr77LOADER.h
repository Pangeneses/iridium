#pragma once

#include "winrt/IIr77LOADER.h"

#include "IDIIr77LOADER.g.h"

namespace winrt::IIr77LOADER::m_implementation {
struct IDIIr77LOADER {
   public:
    IDIIr77LOADER() = default;

   public:
    static winrt::guid HVIDIIr77LOADER();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{winrt::single_threaded_vector<winrt::guid>(std::vector<winrt::guid>{HVIDIIr77LOADER()})};
    }
};
}  // namespace winrt::IIr77LOADER::m_implementation

namespace winrt::IIr77LOADER::factory_implementation {
struct IDIIr77LOADER : IDIIr77LOADERT<IDIIr77LOADER, m_implementation::IDIIr77LOADER> {};
}  // namespace winrt::IIr77LOADER::factory_implementation
