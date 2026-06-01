#pragma once

#include "winrt/IIr77BASE.h"
#include "winrt/IIr77HEALTH.h"

#include "IDOIr77POKE.g.h"

namespace winrt::IIr77HEALTH::m_implementation {
struct IDOIr77POKE {
   public:
    IDOIr77POKE() = default;

   public:
    static winrt::guid HVIDOIr77POKE();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{winrt::single_threaded_vector<winrt::guid>(std::vector<winrt::guid>{HVIDOIr77POKE()})};
    }
};
}  // namespace winrt::IIr77HEALTH::m_implementation

namespace winrt::IIr77HEALTH::factory_implementation {
struct IDOIr77POKE : IDOIr77POKET<IDOIr77POKE, m_implementation::IDOIr77POKE> {};
}  // namespace winrt::IIr77HEALTH::factory_implementation
