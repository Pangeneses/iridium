#pragma once

#include "winrt/IIr77PROJECT.h"

#include "IDDIr77CAD.g.h"

namespace winrt::IIr77PROJECT::m_implementation {
struct IDDIr77CAD {
   public:
    IDDIr77CAD() = default;

   public:
    static winrt::guid HVIDDIr77CAD();
    static winrt::guid HVIDDIr77GEOMETRY();
    static winrt::guid HVIDDIr77OPERATION();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{
            winrt::single_threaded_vector<winrt::guid>(std::vector<winrt::guid>{HVIDDIr77CAD(), HVIDDIr77GEOMETRY(), HVIDDIr77OPERATION()})};
    }
};
}  // namespace winrt::IIr77PROJECT::m_implementation

namespace winrt::IIr77PROJECT::factory_implementation {
struct IDDIr77CAD : IDDIr77CADT<IDDIr77CAD, m_implementation::IDDIr77CAD> {};
}  // namespace winrt::IIr77PROJECT::factory_implementation
