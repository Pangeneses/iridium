#pragma once

#include "winrt/IIr77PROJECT.h"

#include "IDOIr77PAPERS.g.h"

namespace winrt::IIr77PROJECT::m_implementation {
struct IDOIr77PAPERS {
   public:
    IDOIr77PAPERS() = default;

   public:
    static winrt::guid HVIDOIr77PAPERS();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{winrt::single_threaded_vector<winrt::guid>(std::vector<winrt::guid>{HVIDOIr77PAPERS()})};
    }
};
}  // namespace winrt::IIr77PROJECT::m_implementation

namespace winrt::IIr77PROJECT::factory_implementation {
struct IDOIr77PAPERS : IDOIr77PAPERST<IDOIr77PAPERS, m_implementation::IDOIr77PAPERS> {};
}  // namespace winrt::IIr77PROJECT::factory_implementation
