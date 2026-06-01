#pragma once

#include "winrt/IIr77PROJECT.h"

#include "IDOIr77CAD.g.h"

namespace winrt::IIr77PROJECT::m_implementation {
struct IDOIr77CAD {
   public:
    IDOIr77CAD() = default;

   public:
    static winrt::guid HVIDOIr77CAD();
    static winrt::guid HVIDOIr77ADDGEOM();
    static winrt::guid HVIDOIr77DELGEOM();
    static winrt::guid HVIDOIr77EDITGEOM();
    static winrt::guid HVIDOIr77GEOMOP();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{winrt::single_threaded_vector<winrt::guid>(
            std::vector<winrt::guid>{HVIDOIr77CAD(), HVIDOIr77ADDGEOM(), HVIDOIr77DELGEOM(), HVIDOIr77EDITGEOM(), HVIDOIr77GEOMOP()})};
    }
};
}  // namespace winrt::IIr77PROJECT::m_implementation

namespace winrt::IIr77PROJECT::factory_implementation {
struct IDOIr77CAD : IDOIr77CADT<IDOIr77CAD, m_implementation::IDOIr77CAD> {};
}  // namespace winrt::IIr77PROJECT::factory_implementation
