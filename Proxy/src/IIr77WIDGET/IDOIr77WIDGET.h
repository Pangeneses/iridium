#pragma once

#include "winrt/IIr77BASE.h"
#include "winrt/IIr77WIDGET.h"

#include "IDOIr77WIDGET.g.h"

namespace winrt::IIr77WIDGET::m_implementation {
struct IDOIr77WIDGET {
   public:
    IDOIr77WIDGET() = default;

   public:
    static winrt::guid HVIDOIr77WIDGET();
    static winrt::guid HVIDOIr77SETVISSTATE();
    static winrt::guid HVIDOIr77GETVISSTATE();
    static winrt::guid HVIDOIr77SETSTATE();
    static winrt::guid HVIDOIr77GETSTATE();
    static winrt::guid HVIDOIr77INHOOK();
    static winrt::guid HVIDOIr77OUTHOOK();

    static Windows::Foundation::Collections::IVector<winrt::guid> HVIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{winrt::single_threaded_vector<winrt::guid>(std::vector<winrt::guid>{
            HVIDOIr77WIDGET(),
            HVIDOIr77SETVISSTATE(),
            HVIDOIr77GETVISSTATE(),
            HVIDOIr77SETSTATE(),
            HVIDOIr77GETSTATE(),
            HVIDOIr77INHOOK(),
            HVIDOIr77OUTHOOK(),
        })};
    }
};
}  // namespace winrt::IIr77WIDGET::m_implementation

namespace winrt::IIr77WIDGET::factory_implementation {
struct IDOIr77WIDGET : IDOIr77WIDGETT<IDOIr77WIDGET, m_implementation::IDOIr77WIDGET> {};
}  // namespace winrt::IIr77WIDGET::factory_implementation
