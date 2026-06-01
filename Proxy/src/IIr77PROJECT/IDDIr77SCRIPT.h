#pragma once

#include "winrt/IIr77PROJECT.h"

#include "IDDIr77SCRIPT.g.h"

namespace winrt::IIr77PROJECT::m_implementation {
struct IDDIr77SCRIPT {
   public:
    IDDIr77SCRIPT() = default;

   public:
    static winrt::guid HVIDDIr77SCRIPT();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{winrt::single_threaded_vector<winrt::guid>(std::vector<winrt::guid>{HVIDDIr77SCRIPT()})};
    }
};
}  // namespace winrt::IIr77PROJECT::m_implementation

namespace winrt::IIr77PROJECT::factory_implementation {
struct IDDIr77SCRIPT : IDDIr77SCRIPTT<IDDIr77SCRIPT, m_implementation::IDDIr77SCRIPT> {};
}  // namespace winrt::IIr77PROJECT::factory_implementation
