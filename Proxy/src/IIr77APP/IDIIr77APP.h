#pragma once

#include "winrt/IIr77BASE.h"
#include "winrt/IIr77APP.h"

#include "IDIIr77APP.g.h"

namespace winrt::IIr77APP::m_implementation {
struct IDIIr77APP {
   public:
    IDIIr77APP() = default;

   public:
    static winrt::guid HVIDIIr77APP();

    static Windows::Foundation::Collections::IVector<winrt::guid> HVIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{winrt::single_threaded_vector<winrt::guid>(std::vector<winrt::guid>{HVIDIIr77APP()})};
    }
};
}  // namespace winrt::IIr77APP::m_implementation

namespace winrt::IIr77APP::factory_implementation {
struct IDIIr77APP : IDIIr77APPT<IDIIr77APP, m_implementation::IDIIr77APP> {};
}  // namespace winrt::IIr77APP::factory_implementation
