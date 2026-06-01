#pragma once

#include "winrt/IIr77BASE.h"
#include "winrt/IIr77WIDGET.h"

#include "IDIIr77WIDGET.g.h"

namespace winrt::IIr77WIDGET::m_implementation {
struct IDIIr77WIDGET {
   public:
    IDIIr77WIDGET() = default;

   public:
    static winrt::guid HVIDIIr77WIDGET();

    static Windows::Foundation::Collections::IVector<winrt::guid> HVIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{winrt::single_threaded_vector<winrt::guid>(std::vector<winrt::guid>{HVIDIIr77WIDGET()})};
    }
};
}  // namespace winrt::IIr77WIDGET::m_implementation

namespace winrt::IIr77WIDGET::factory_implementation {
struct IDIIr77WIDGET : IDIIr77WIDGETT<IDIIr77WIDGET, m_implementation::IDIIr77WIDGET> {};
}  // namespace winrt::IIr77WIDGET::factory_implementation
