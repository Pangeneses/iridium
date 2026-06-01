#pragma once

#include "winrt/IIr77PROJECT.h"

#include "IDOIr77DATABASE.g.h"

namespace winrt::IIr77PROJECT::m_implementation {
struct IDOIr77DATABASE {
   public:
    IDOIr77DATABASE() = default;

   public:
    static winrt::guid HVIDOIr77DATABASE();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{
            winrt::single_threaded_vector<winrt::guid>(std::vector<winrt::guid>{HVIDOIr77DATABASE()})};
    }
};
}  // namespace winrt::IIr77PROJECT::m_implementation

namespace winrt::IIr77PROJECT::factory_implementation {
struct IDOIr77DATABASE : IDOIr77DATABASET<IDOIr77DATABASE, m_implementation::IDOIr77DATABASE> {};
}  // namespace winrt::IIr77PROJECT::factory_implementation
