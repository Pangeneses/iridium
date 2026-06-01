#pragma once

#include "winrt/IIr77PROJECT.h"

#include "IDXIr77PAPERS.g.h"

namespace winrt::IIr77PROJECT::m_implementation {
struct IDXIr77PAPERS {
   public:
    IDXIr77PAPERS() = default;

   public:
    static winrt::guid HVIDXIr77PAPERS();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{winrt::single_threaded_vector<winrt::guid>(std::vector<winrt::guid>{HVIDXIr77PAPERS()})};
    }
};
}  // namespace winrt::IIr77PROJECT::m_implementation

namespace winrt::IIr77PROJECT::factory_implementation {
struct IDXIr77PAPERS : IDXIr77PAPERST<IDXIr77PAPERS, m_implementation::IDXIr77PAPERS> {};
}  // namespace winrt::IIr77PROJECT::factory_implementation
