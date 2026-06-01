#pragma once

#include "winrt/IIr77BASE.h"
#include "winrt/IIr77PROJECT.h"

#include "IDIIr77PROJECT.g.h"

namespace winrt::IIr77PROJECT::m_implementation {
struct IDIIr77PROJECT {
   public:
    IDIIr77PROJECT() = default;

   public:
    static winrt::guid HVIDIIr77PROJECT();
    static winrt::guid HVIDIr77ACCOUNTS();
    static winrt::guid HVIDIr77VERSIONING();
    static winrt::guid HVIDIr77MESH();
    static winrt::guid HVIDIr77CAD();
    static winrt::guid HVIDIr77MATERIAL();
    static winrt::guid HVIDIr77GENERATOR();
    static winrt::guid HVIDIr77SCRIPT();
    static winrt::guid HVIDIr77BOM();
    static winrt::guid HVIDIr77GCODE();
    static winrt::guid HVIDIr77PAPERS();
    static winrt::guid HVIDIr77DATABASE();
    static winrt::guid HVIDIr77GPU();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{winrt::single_threaded_vector<winrt::guid>(std::vector<winrt::guid>{
            HVIDIIr77PROJECT(), HVIDIr77ACCOUNTS(), HVIDIr77VERSIONING(), HVIDIr77MESH(), HVIDIr77CAD(), HVIDIr77MATERIAL(), HVIDIr77GENERATOR(),
            HVIDIr77SCRIPT(), HVIDIr77BOM(), HVIDIr77GCODE(), HVIDIr77PAPERS(), HVIDIr77DATABASE(), HVIDIr77GPU()})};
    }
};
}  // namespace winrt::IIr77PROJECT::m_implementation

namespace winrt::IIr77PROJECT::factory_implementation {
struct IDIIr77PROJECT : IDIIr77PROJECTT<IDIIr77PROJECT, m_implementation::IDIIr77PROJECT> {};
}  // namespace winrt::IIr77PROJECT::factory_implementation
