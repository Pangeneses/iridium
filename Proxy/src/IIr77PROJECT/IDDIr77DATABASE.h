#pragma once

#include "winrt/IIr77PROJECT.h"

#include "IDDIr77DATABASE.g.h"

namespace winrt::IIr77PROJECT::m_implementation {
struct IDDIr77DATABASE {
   public:
    IDDIr77DATABASE() = default;

   public:
    static winrt::guid HVIDDIr77DATABASE();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{
            winrt::single_threaded_vector<winrt::guid>(std::vector<winrt::guid>{HVIDDIr77DATABASE()})};
    }
};
}  // namespace winrt::IIr77PROJECT::m_implementation

namespace winrt::IIr77PROJECT::factory_implementation {
struct IDDIr77DATABASE : IDDIr77DATABASET<IDDIr77DATABASE, m_implementation::IDDIr77DATABASE> {};
}  // namespace winrt::IIr77PROJECT::factory_implementation
