#pragma once

#include "winrt/IIr77BASE.h"
#include "winrt/IIr77HEALTH.h"

#include "IDOIr77MEDIC.g.h"

namespace winrt::IIr77HEALTH::m_implementation {
struct IDOIr77MEDIC {
   public:
    IDOIr77MEDIC() = default;

   public:
    static winrt::guid HVIDOIr77MEDIC();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{winrt::single_threaded_vector<winrt::guid>(std::vector<winrt::guid>{HVIDOIr77MEDIC()})};
    }
};
}  // namespace winrt::IIr77HEALTH::m_implementation

namespace winrt::IIr77HEALTH::factory_implementation {
struct IDOIr77MEDIC : IDOIr77MEDICT<IDOIr77MEDIC, m_implementation::IDOIr77MEDIC> {};
}  // namespace winrt::IIr77HEALTH::factory_implementation
