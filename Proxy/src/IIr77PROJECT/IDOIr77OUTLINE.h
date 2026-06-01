#pragma once

#include "winrt/IIr77PROJECT.h"

#include "IDOIr77OUTLINE.g.h"

namespace winrt::IIr77PROJECT::m_implementation {
struct IDOIr77OUTLINE {
   public:
    IDOIr77OUTLINE() = default;

   public:
    static winrt::guid HVIDOIr77OUTLINE();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{winrt::single_threaded_vector<winrt::guid>(std::vector<winrt::guid>{HVIDOIr77OUTLINE()})};
    }
};
}  // namespace winrt::IIr77PROJECT::m_implementation

namespace winrt::IIr77PROJECT::factory_implementation {
struct IDOIr77OUTLINE : IDOIr77OUTLINET<IDOIr77OUTLINE, m_implementation::IDOIr77OUTLINE> {};
}  // namespace winrt::IIr77PROJECT::factory_implementation
