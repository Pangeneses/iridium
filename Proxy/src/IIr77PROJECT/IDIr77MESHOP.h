#pragma once

#include "winrt/IIr77PROJECT.h"

#include "IDIr77MESHOP.g.h"

namespace winrt::IIr77PROJECT::m_implementation {
struct IDIr77MESHOP {
   public:
    IDIr77MESHOP() = default;

   public:
    static winrt::guid HVIDIr77MESHOP();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{winrt::single_threaded_vector<winrt::guid>(std::vector<winrt::guid>{HVIDIr77MESHOP()})};
    }
};
}  // namespace winrt::IIr77PROJECT::m_implementation

namespace winrt::IIr77PROJECT::factory_implementation {
struct IDIr77MESHOP : IDIr77MESHOPT<IDIr77MESHOP, m_implementation::IDIr77MESHOP> {};
}  // namespace winrt::IIr77PROJECT::factory_implementation
