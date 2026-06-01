#pragma once

#include "winrt/IIr77BASE.h"
#include "winrt/IIr77HEALTH.h"

#include "IDDIr77POKE.g.h"

namespace winrt::IIr77HEALTH::m_implementation {
struct IDDIr77POKE {
   public:
    IDDIr77POKE() = default;

   public:
    static winrt::guid HVIDDIr77POKE();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{winrt::single_threaded_vector<winrt::guid>(std::vector<winrt::guid>{HVIDDIr77POKE()})};
    }
};
}  // namespace winrt::IIr77HEALTH::m_implementation

namespace winrt::IIr77HEALTH::factory_implementation {
struct IDDIr77POKE : IDDIr77POKET<IDDIr77POKE, m_implementation::IDDIr77POKE> {};
}  // namespace winrt::IIr77HEALTH::factory_implementation
