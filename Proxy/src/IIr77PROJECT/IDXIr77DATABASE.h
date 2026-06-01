#pragma once

#include "winrt/IIr77PROJECT.h"

#include "IDXIr77DATABASE.g.h"

namespace winrt::IIr77PROJECT::m_implementation {
struct IDXIr77DATABASE {
   public:
    IDXIr77DATABASE() = default;

   public:
    static winrt::guid HVIDXIr77DATABASE();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{
            winrt::single_threaded_vector<winrt::guid>(std::vector<winrt::guid>{HVIDXIr77DATABASE()})};
    }
};
}  // namespace winrt::IIr77PROJECT::m_implementation

namespace winrt::IIr77PROJECT::factory_implementation {
struct IDXIr77DATABASE : IDXIr77DATABASET<IDXIr77DATABASE, m_implementation::IDXIr77DATABASE> {};
}  // namespace winrt::IIr77PROJECT::factory_implementation
