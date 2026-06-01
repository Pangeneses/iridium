#pragma once

#include "winrt/IIr77BASE.h"
#include "winrt/IIr77WIDGET.h"

#include "IDXIr77VISSTATE.g.h"

namespace winrt::IIr77WIDGET::m_implementation {
struct IDXIr77VISSTATE {
   public:
    IDXIr77VISSTATE() = default;

   public:
    static winrt::guid HVIDXVISSTATE();
    static winrt::guid HVIDXTERMINATE();
    static winrt::guid HVIDXVISIBLE();
    static winrt::guid HVIDXREFRESH();
    static winrt::guid HVIDXTHEME();

    static Windows::Foundation::Collections::IVector<winrt::guid> HVIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{winrt::single_threaded_vector<winrt::guid>(
            std::vector<winrt::guid>{HVIDXVISSTATE(), HVIDXTERMINATE(), HVIDXVISIBLE(), HVIDXREFRESH(), HVIDXTHEME()})};
    }
};
}  // namespace winrt::IIr77WIDGET::m_implementation

namespace winrt::IIr77WIDGET::factory_implementation {
struct IDXIr77VISSTATE : IDXIr77VISSTATET<IDXIr77VISSTATE, m_implementation::IDXIr77VISSTATE> {};
}  // namespace winrt::IIr77WIDGET::factory_implementation
