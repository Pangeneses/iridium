#pragma once

#include "winrt/IIr77BASE.h"
#include "winrt/IIr77GRAPHICS.h"

#include "IDXIr77DISPLAY.g.h"

namespace winrt::IIr77GRAPHICS::m_implementation {
struct IDXIr77DISPLAY {
   public:
    IDXIr77DISPLAY() = default;

   public:
    static winrt::guid HVIDXIr77DISPLAY();
    static winrt::guid HVIDXIr77DISPLAYSCALED();
    static winrt::guid HVIDXIr77DISPLAY480P();
    static winrt::guid HVIDXIr77DISPLAY720P();
    static winrt::guid HVIDXIr77DISPLAY1024P();
    static winrt::guid HVIDXIr77DISPLAY1080P();
    static winrt::guid HVIDXIr77DISPLAY1080P3D();
    static winrt::guid HVIDXIr77DISPLAY1080PF3D();
    static winrt::guid HVIDXIr77DISPLAY1200P();
    static winrt::guid HVIDXIr77DISPLAY1440P();
    static winrt::guid HVIDXIr77DISPLAY1600P();
    static winrt::guid HVIDXIr77DISPLAY2160P();
    static winrt::guid HVIDXIr77DISPLAY2160P3D();
    static winrt::guid HVIDXIr77DISPLAY2160PF3D();
    static winrt::guid HVIDXIr77DISPLAY2540P();
    static winrt::guid HVIDXIr77DISPLAY4000P();
    static winrt::guid HVIDXIr77DISPLAY4320P();
    static winrt::guid HVIDXIr77DISPLAYVR();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{winrt::single_threaded_vector<winrt::guid>(std::vector<winrt::guid>{
            HVIDXIr77DISPLAY(), HVIDXIr77DISPLAYSCALED(), HVIDXIr77DISPLAY480P(), HVIDXIr77DISPLAY720P(), HVIDXIr77DISPLAY1024P(), HVIDXIr77DISPLAY1080P(),
            HVIDXIr77DISPLAY1080P3D(), HVIDXIr77DISPLAY1080PF3D(), HVIDXIr77DISPLAY1200P(), HVIDXIr77DISPLAY1440P(), HVIDXIr77DISPLAY1600P(),
            HVIDXIr77DISPLAY2160P(), HVIDXIr77DISPLAY2160P3D(), HVIDXIr77DISPLAY2160PF3D(), HVIDXIr77DISPLAY2540P(), HVIDXIr77DISPLAY4000P(),
            HVIDXIr77DISPLAY4320P(), HVIDXIr77DISPLAYVR()})};
    }
};
}  // namespace winrt::IIr77GRAPHICS::m_implementation

namespace winrt::IIr77GRAPHICS::factory_implementation {
struct IDXIr77DISPLAY : IDXIr77DISPLAYT<IDXIr77DISPLAY, m_implementation::IDXIr77DISPLAY> {};
}  // namespace winrt::IIr77GRAPHICS::factory_implementation
