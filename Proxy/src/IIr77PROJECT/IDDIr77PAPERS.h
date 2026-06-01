#pragma once

#include "winrt/IIr77PROJECT.h"

#include "IDDIr77PAPERS.g.h"

namespace winrt::IIr77PROJECT::m_implementation {
struct IDDIr77PAPERS {
   public:
    IDDIr77PAPERS() = default;

   public:
    static winrt::guid HVIDDIr77PAPERS();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{winrt::single_threaded_vector<winrt::guid>(std::vector<winrt::guid>{HVIDDIr77PAPERS()})};
    }
};
}  // namespace winrt::IIr77PROJECT::m_implementation

namespace winrt::IIr77PROJECT::factory_implementation {
struct IDDIr77PAPERS : IDDIr77PAPERST<IDDIr77PAPERS, m_implementation::IDDIr77PAPERS> {};
}  // namespace winrt::IIr77PROJECT::factory_implementation
