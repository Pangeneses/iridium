#pragma once

#include "winrt/IIr77PROJECT.h"

#include "IDDIr77OUTLINE.g.h"

namespace winrt::IIr77PROJECT::m_implementation {
struct IDDIr77OUTLINE {
   public:
    IDDIr77OUTLINE() = default;

   public:
    static winrt::guid HVIDDIr77OUTLINE();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{winrt::single_threaded_vector<winrt::guid>(std::vector<winrt::guid>{HVIDDIr77OUTLINE()})};
    }
};
}  // namespace winrt::IIr77PROJECT::m_implementation

namespace winrt::IIr77PROJECT::factory_implementation {
struct IDDIr77OUTLINE : IDDIr77OUTLINET<IDDIr77OUTLINE, m_implementation::IDDIr77OUTLINE> {};
}  // namespace winrt::IIr77PROJECT::factory_implementation
