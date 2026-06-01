#pragma once

#include "winrt/IIr77PROJECT.h"

#include "IDOIr77ACCOUNTS.g.h"

namespace winrt::IIr77PROJECT::m_implementation {
struct IDOIr77ACCOUNTS {
   public:
    IDOIr77ACCOUNTS() = default;

   public:
    static winrt::guid HVIDOIr77ACCOUNTS();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{
            winrt::single_threaded_vector<winrt::guid>(std::vector<winrt::guid>{HVIDOIr77ACCOUNTS()})};
    }
};
}  // namespace winrt::IIr77PROJECT::m_implementation

namespace winrt::IIr77PROJECT::factory_implementation {
struct IDOIr77ACCOUNTS : IDOIr77ACCOUNTST<IDOIr77ACCOUNTS, m_implementation::IDOIr77ACCOUNTS> {};
}  // namespace winrt::IIr77PROJECT::factory_implementation
