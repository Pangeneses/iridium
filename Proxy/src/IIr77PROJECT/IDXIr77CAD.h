#pragma once

#include "winrt/IIr77PROJECT.h"

#include "IDXIr77CAD.g.h"

namespace winrt::IIr77PROJECT::m_implementation {
struct IDXIr77CAD {
   public:
    IDXIr77CAD() = default;

   public:
    static winrt::guid HVIDXIr77CAD();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{winrt::single_threaded_vector<winrt::guid>(std::vector<winrt::guid>{HVIDXIr77CAD()})};
    }
};
}  // namespace winrt::IIr77PROJECT::m_implementation

namespace winrt::IIr77PROJECT::factory_implementation {
struct IDXIr77CAD : IDXIr77CADT<IDXIr77CAD, m_implementation::IDXIr77CAD> {};
}  // namespace winrt::IIr77PROJECT::factory_implementation
