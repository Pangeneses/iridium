#pragma once

#include "winrt/IIr77BASE.h"
#include "winrt/IIr77GRAPHICS.h"

#include "IDXIr77ALIASING.g.h"

namespace winrt::IIr77GRAPHICS::m_implementation {
struct IDXIr77ALIASING {
   public:
    IDXIr77ALIASING() = default;

   public:
    static winrt::guid HVIDXIr77ALIASING();
    static winrt::guid HVIDXIr77ALIASINGSS();
    static winrt::guid HVIDXIr77ALIASINGMS();
    static winrt::guid HVIDXIr77ALIASINGTAA();
    static winrt::guid HVIDXIr77ALIASINGSPM();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{winrt::single_threaded_vector<winrt::guid>(
            std::vector<winrt::guid>{HVIDXIr77ALIASING(), HVIDXIr77ALIASINGSS(), HVIDXIr77ALIASINGMS(), HVIDXIr77ALIASINGTAA(), HVIDXIr77ALIASINGSPM()})};
    }
};
}  // namespace winrt::IIr77GRAPHICS::m_implementation

namespace winrt::IIr77GRAPHICS::factory_implementation {
struct IDXIr77ALIASING : IDXIr77ALIASINGT<IDXIr77ALIASING, m_implementation::IDXIr77ALIASING> {};
}  // namespace winrt::IIr77GRAPHICS::factory_implementation
