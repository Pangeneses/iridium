#pragma once

#include "winrt/IIr77BASE.h"
#include "winrt/IIr77HEALTH.h"

#include "IDXIr77POKE.g.h"

namespace winrt::IIr77HEALTH::m_implementation {
struct IDXIr77POKE {
   public:
    IDXIr77POKE() = default;

   public:
    static winrt::guid HVIDXIr77POKE();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{winrt::single_threaded_vector<winrt::guid>(std::vector<winrt::guid>{HVIDXIr77POKE()})};
    }
};
}  // namespace winrt::IIr77HEALTH::m_implementation

namespace winrt::IIr77HEALTH::factory_implementation {
struct IDXIr77POKE : IDXIr77POKET<IDXIr77POKE, m_implementation::IDXIr77POKE> {};
}  // namespace winrt::IIr77HEALTH::factory_implementation
