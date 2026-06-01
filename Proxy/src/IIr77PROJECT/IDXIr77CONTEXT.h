#pragma once

#include "winrt/IIr77PROJECT.h"

#include "IDXIr77CONTEXT.g.h"

namespace winrt::IIr77PROJECT::m_implementation {
struct IDXIr77CONTEXT {
   public:
    IDXIr77CONTEXT() = default;

   public:
    static winrt::guid HVIDXIr77CONTEXT();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{winrt::single_threaded_vector<winrt::guid>(std::vector<winrt::guid>{HVIDXIr77CONTEXT()})};
    }
};
}  // namespace winrt::IIr77PROJECT::m_implementation

namespace winrt::IIr77PROJECT::factory_implementation {
struct IDXIr77CONTEXT : IDXIr77CONTEXTT<IDXIr77CONTEXT, m_implementation::IDXIr77CONTEXT> {};
}  // namespace winrt::IIr77PROJECT::factory_implementation
