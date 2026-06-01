#pragma once

#include "winrt/IIr77PROJECT.h"

#include "IDXIr77DDSFILE.g.h"

namespace winrt::IIr77PROJECT::m_implementation {
struct IDXIr77DDSFILE {
   public:
    IDXIr77DDSFILE() = default;

   public:
    static winrt::guid HVIDXIr77DDSFILE();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{winrt::single_threaded_vector<winrt::guid>(std::vector<winrt::guid>{HVIDXIr77DDSFILE()})};
    }
};
}  // namespace winrt::IIr77PROJECT::m_implementation

namespace winrt::IIr77PROJECT::factory_implementation {
struct IDXIr77DDSFILE : IDXIr77DDSFILET<IDXIr77DDSFILE, m_implementation::IDXIr77DDSFILE> {};
}  // namespace winrt::IIr77PROJECT::factory_implementation
