#pragma once

#include "winrt/IIr77PROJECT.h"

#include "IDOIr77SCRIPT.g.h"

namespace winrt::IIr77PROJECT::m_implementation {
struct IDOIr77SCRIPT {
   public:
    IDOIr77SCRIPT() = default;

   public:
    static winrt::guid HVIDOIr77SCRIPT();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{winrt::single_threaded_vector<winrt::guid>(std::vector<winrt::guid>{HVIDOIr77SCRIPT()})};
    }
};
}  // namespace winrt::IIr77PROJECT::m_implementation

namespace winrt::IIr77PROJECT::factory_implementation {
struct IDOIr77SCRIPT : IDOIr77SCRIPTT<IDOIr77SCRIPT, m_implementation::IDOIr77SCRIPT> {};
}  // namespace winrt::IIr77PROJECT::factory_implementation
