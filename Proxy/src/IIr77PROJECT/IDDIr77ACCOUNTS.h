#pragma once

#include "winrt/IIr77PROJECT.h"

#include "IDDIr77ACCOUNTS.g.h"

namespace winrt::IIr77PROJECT::m_implementation {
struct IDDIr77ACCOUNTS {
   public:
    IDDIr77ACCOUNTS() = default;

   public:
    static winrt::guid HVIDDIr77ACCOUNTS();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{
            winrt::single_threaded_vector<winrt::guid>(std::vector<winrt::guid>{HVIDDIr77ACCOUNTS()})};
    }
};
}  // namespace winrt::IIr77PROJECT::m_implementation

namespace winrt::IIr77PROJECT::factory_implementation {
struct IDDIr77ACCOUNTS : IDDIr77ACCOUNTST<IDDIr77ACCOUNTS, m_implementation::IDDIr77ACCOUNTS> {};
}  // namespace winrt::IIr77PROJECT::factory_implementation
