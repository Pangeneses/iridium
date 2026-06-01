#pragma once

#include "winrt/IIr77PROJECT.h"

#include "IDXIr77NILOBJ.g.h"

namespace winrt::IIr77PROJECT::m_implementation {
struct IDXIr77NILOBJ {
   public:
    IDXIr77NILOBJ() = default;

   public:
    static winrt::guid HVIDXIr77NILOBJ();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{winrt::single_threaded_vector<winrt::guid>(std::vector<winrt::guid>{HVIDXIr77NILOBJ()})};
    }
};
}  // namespace winrt::IIr77PROJECT::m_implementation

namespace winrt::IIr77PROJECT::factory_implementation {
struct IDXIr77NILOBJ : IDXIr77NILOBJT<IDXIr77NILOBJ, m_implementation::IDXIr77NILOBJ> {};
}  // namespace winrt::IIr77PROJECT::factory_implementation
