#pragma once

#include "winrt/IIr77BASE.h"
#include "winrt/IIr77WIDGET.h"

#include "IDDIr77WIDGET.g.h"

namespace winrt::IIr77WIDGET::m_implementation {
struct IDDIr77WIDGET {
   public:
    IDDIr77WIDGET() = default;

   public:
    static winrt::guid HVIDDIr77WIDGET();

    static Windows::Foundation::Collections::IVector<winrt::guid> HVIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{winrt::single_threaded_vector<winrt::guid>(std::vector<winrt::guid>{HVIDDIr77WIDGET()})};
    }
};
}  // namespace winrt::IIr77WIDGET::m_implementation

namespace winrt::IIr77WIDGET::factory_implementation {
struct IDDIr77WIDGET : IDDIr77WIDGETT<IDDIr77WIDGET, m_implementation::IDDIr77WIDGET> {};
}  // namespace winrt::IIr77WIDGET::factory_implementation
