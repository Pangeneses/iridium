#pragma once

#include "winrt/IIr77BASE.h"
#include "winrt/IIr77GRAPHICS.h"

#include "IDXIr77GFXAPI.g.h"

namespace winrt::IIr77GRAPHICS::m_implementation {
struct IDXIr77GFXAPI {
   public:
    IDXIr77GFXAPI() = default;

   public:
    static winrt::guid HVIDXIr77GFXAPI();
    static winrt::guid HVIDXIr77DIRECTX09_C();
    static winrt::guid HVIDXIr77DIRECTX10_1();
    static winrt::guid HVIDXIr77DIRECTX11_0();
    static winrt::guid HVIDXIr77DIRECTX11_1();
    static winrt::guid HVIDXIr77DIRECTX11_2();
    static winrt::guid HVIDXIr77DIRECTX11_X();
    static winrt::guid HVIDXIr77DIRECTX11_3();
    static winrt::guid HVIDXIr77DIRECTX12_U();
    static winrt::guid HVIDXIr77OPENGL03_0();
    static winrt::guid HVIDXIr77OPENGL03_1();
    static winrt::guid HVIDXIr77OPENGL03_2();
    static winrt::guid HVIDXIr77OPENGL03_3();
    static winrt::guid HVIDXIr77OPENGL04_0();
    static winrt::guid HVIDXIr77OPENGL04_1();
    static winrt::guid HVIDXIr77OPENGL04_2();
    static winrt::guid HVIDXIr77OPENGL04_3();
    static winrt::guid HVIDXIr77OPENGL04_4();
    static winrt::guid HVIDXIr77OPENGL04_5();
    static winrt::guid HVIDXIr77OPENGL04_6();
    static winrt::guid HVIDXIr77VULKAN01_3();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{winrt::single_threaded_vector<winrt::guid>(std::vector<winrt::guid>{
            HVIDXIr77GFXAPI(),      HVIDXIr77DIRECTX09_C(), HVIDXIr77DIRECTX10_1(), HVIDXIr77DIRECTX11_0(), HVIDXIr77DIRECTX11_1(), HVIDXIr77DIRECTX11_2(),
            HVIDXIr77DIRECTX11_X(), HVIDXIr77DIRECTX11_3(), HVIDXIr77DIRECTX12_U(), HVIDXIr77OPENGL03_0(),  HVIDXIr77OPENGL03_1(),  HVIDXIr77OPENGL03_2(),
            HVIDXIr77OPENGL03_3(),  HVIDXIr77OPENGL04_0(),  HVIDXIr77OPENGL04_1(),  HVIDXIr77OPENGL04_2(),  HVIDXIr77OPENGL04_3(),  HVIDXIr77OPENGL04_4(),
            HVIDXIr77OPENGL04_5(),  HVIDXIr77OPENGL04_6(),  HVIDXIr77VULKAN01_3()})};
    }
};
}  // namespace winrt::IIr77GRAPHICS::m_implementation

namespace winrt::IIr77GRAPHICS::factory_implementation {
struct IDXIr77GFXAPI : IDXIr77GFXAPIT<IDXIr77GFXAPI, m_implementation::IDXIr77GFXAPI> {};
}  // namespace winrt::IIr77GRAPHICS::factory_implementation
