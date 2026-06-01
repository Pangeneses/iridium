#pragma once

#include "winrt/IIr77BASE.h"
#include "winrt/IIr77GRAPHICS.h"

#include "IDXIr77SL.g.h"

namespace winrt::IIr77GRAPHICS::m_implementation {
struct IDXIr77SL {
   public:
    IDXIr77SL() = default;

   public:
    static winrt::guid HVIDXIr77SL();
    static winrt::guid HVIDXIr77HLSL13();
    static winrt::guid HVIDXIr77HLSL14();
    static winrt::guid HVIDXIr77HLSL20();
    static winrt::guid HVIDXIr77HLSL2A();
    static winrt::guid HVIDXIr77HLSL2B();
    static winrt::guid HVIDXIr77HLSL30();
    static winrt::guid HVIDXIr77HLSL40();
    static winrt::guid HVIDXIr77HLSL41();
    static winrt::guid HVIDXIr77HLSL50();
    static winrt::guid HVIDXIr77HLSL60();
    static winrt::guid HVIDXIr77HLSL61();
    static winrt::guid HVIDXIr77HLSL62();
    static winrt::guid HVIDXIr77HLSL63();
    static winrt::guid HVIDXIr77HLSL64();
    static winrt::guid HVIDXIr77HLSL65();
    static winrt::guid HVIDXIr77HLSL66();
    static winrt::guid HVIDXIr77HLSL67();
    static winrt::guid HVIDXIr77GLSL33();
    static winrt::guid HVIDXIr77GLSL40();
    static winrt::guid HVIDXIr77GLSL41();
    static winrt::guid HVIDXIr77GLSL42();
    static winrt::guid HVIDXIr77GLSL43();
    static winrt::guid HVIDXIr77GLSL44();
    static winrt::guid HVIDXIr77GLSL45();
    static winrt::guid HVIDXIr77GLSL46();
    static winrt::guid HVIDXIr77SPIRV();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{winrt::single_threaded_vector<winrt::guid>(std::vector<winrt::guid>{
            HVIDXIr77SL(),     HVIDXIr77HLSL13(), HVIDXIr77HLSL14(), HVIDXIr77HLSL20(), HVIDXIr77HLSL2A(), HVIDXIr77HLSL2B(), HVIDXIr77HLSL30(),
            HVIDXIr77HLSL40(), HVIDXIr77HLSL41(), HVIDXIr77HLSL50(), HVIDXIr77HLSL60(), HVIDXIr77HLSL61(), HVIDXIr77HLSL62(), HVIDXIr77HLSL63(),
            HVIDXIr77HLSL64(), HVIDXIr77HLSL65(), HVIDXIr77HLSL66(), HVIDXIr77HLSL67(), HVIDXIr77GLSL33(), HVIDXIr77GLSL40(), HVIDXIr77GLSL41(),
            HVIDXIr77GLSL42(), HVIDXIr77GLSL43(), HVIDXIr77GLSL44(), HVIDXIr77GLSL45(), HVIDXIr77GLSL46(), HVIDXIr77SPIRV()})};
    }
};
}  // namespace winrt::IIr77GRAPHICS::m_implementation

namespace winrt::IIr77GRAPHICS::factory_implementation {
struct IDXIr77SL : IDXIr77SLT<IDXIr77SL, m_implementation::IDXIr77SL> {};
}  // namespace winrt::IIr77GRAPHICS::factory_implementation
