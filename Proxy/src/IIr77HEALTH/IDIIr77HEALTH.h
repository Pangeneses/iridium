#pragma once

#include "winrt/IIr77BASE.h"
#include "winrt/IIr77HEALTH.h"

#include "IDIIr77HEALTH.g.h"

namespace winrt::IIr77HEALTH::m_implementation {
struct IDIIr77HEALTH {
   public:
    IDIIr77HEALTH() = default;

   public:
    static winrt::guid HVIDIr77HEALTH();
    static winrt::guid HVIDIr77MEDIC();
    static winrt::guid HVIDIr77POKE();
    static winrt::guid HVIDIr77ECHO();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{
            winrt::single_threaded_vector<winrt::guid>(std::vector<winrt::guid>{HVIDIr77HEALTH(), HVIDIr77MEDIC(), HVIDIr77POKE(), HVIDIr77ECHO()})};
    }
};
}  // namespace winrt::IIr77HEALTH::m_implementation

namespace winrt::IIr77HEALTH::factory_implementation {
struct IDIIr77HEALTH : IDIIr77HEALTHT<IDIIr77HEALTH, m_implementation::IDIIr77HEALTH> {};
}  // namespace winrt::IIr77HEALTH::factory_implementation
