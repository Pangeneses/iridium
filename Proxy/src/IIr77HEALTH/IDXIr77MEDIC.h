#pragma once

#include "winrt/IIr77BASE.h"
#include "winrt/IIr77HEALTH.h"

#include "IDXIr77MEDIC.g.h"

namespace winrt::IIr77HEALTH::m_implementation {
struct IDXIr77MEDIC {
   public:
    IDXIr77MEDIC() = default;

   public:
    static winrt::guid HVIDXIr77MEDIC();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{winrt::single_threaded_vector<winrt::guid>(std::vector<winrt::guid>{HVIDXIr77MEDIC()})};
    }
};
}  // namespace winrt::IIr77HEALTH::m_implementation

namespace winrt::IIr77HEALTH::factory_implementation {
struct IDXIr77MEDIC : IDXIr77MEDICT<IDXIr77MEDIC, m_implementation::IDXIr77MEDIC> {};
}  // namespace winrt::IIr77HEALTH::factory_implementation
