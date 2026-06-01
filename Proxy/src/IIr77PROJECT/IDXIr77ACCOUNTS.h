#pragma once

#include "winrt/IIr77PROJECT.h"

#include "IDXIr77ACCOUNTS.g.h"

namespace winrt::IIr77PROJECT::m_implementation {
struct IDXIr77ACCOUNTS {
   public:
    IDXIr77ACCOUNTS() = default;

   public:
    static winrt::guid HVIDXIr77ACCOUNTS();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{
            winrt::single_threaded_vector<winrt::guid>(std::vector<winrt::guid>{HVIDXIr77ACCOUNTS()})};
    }
};
}  // namespace winrt::IIr77PROJECT::m_implementation

namespace winrt::IIr77PROJECT::factory_implementation {
struct IDXIr77ACCOUNTS : IDXIr77ACCOUNTST<IDXIr77ACCOUNTS, m_implementation::IDXIr77ACCOUNTS> {};
}  // namespace winrt::IIr77PROJECT::factory_implementation
