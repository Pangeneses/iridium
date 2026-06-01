#pragma once

#include "winrt/IIr77BASE.h"
#include "winrt/IIr77HEALTH.h"

#include "IDDIr77MEDIC.g.h"

namespace winrt::IIr77HEALTH::m_implementation {
struct IDDIr77MEDIC {
   public:
    IDDIr77MEDIC() = default;

   public:
    static winrt::guid HVIDDIr77MEDIC();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{winrt::single_threaded_vector<winrt::guid>(std::vector<winrt::guid>{HVIDDIr77MEDIC()})};
    }
};
}  // namespace winrt::IIr77HEALTH::m_implementation

namespace winrt::IIr77HEALTH::factory_implementation {
struct IDDIr77MEDIC : IDDIr77MEDICT<IDDIr77MEDIC, m_implementation::IDDIr77MEDIC> {};
}  // namespace winrt::IIr77HEALTH::factory_implementation
