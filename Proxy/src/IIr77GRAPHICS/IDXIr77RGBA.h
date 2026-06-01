#pragma once

#include "winrt/IIr77GRAPHICS.h"

#include "IDXIr77RGBA.g.h"

namespace winrt::IIr77GRAPHICS::m_implementation {
struct IDXIr77RGBA {
   public:
    IDXIr77RGBA() = default;

   public:
    static winrt::guid HVIDX_DXGI_RGBA();
    static winrt::guid HVIDX_DXGI_RGBA_UNKNOWN();
    static winrt::guid HVIDX_DXGI_RGBA_R32G32B32A32_TYPELESS();
    static winrt::guid HVIDX_DXGI_RGBA_R32G32B32A32_FLOAT();
    static winrt::guid HVIDX_DXGI_RGBA_R32G32B32A32_UINT();
    static winrt::guid HVIDX_DXGI_RGBA_R32G32B32A32_SINT();
    static winrt::guid HVIDX_DXGI_RGBA_R32G32B32_TYPELESS();
    static winrt::guid HVIDX_DXGI_RGBA_R32G32B32_FLOAT();
    static winrt::guid HVIDX_DXGI_RGBA_R32G32B32_UINT();
    static winrt::guid HVIDX_DXGI_RGBA_R32G32B32_SINT();
    static winrt::guid HVIDX_DXGI_RGBA_R16G16B16A16_TYPELESS();
    static winrt::guid HVIDX_DXGI_RGBA_R16G16B16A16_FLOAT();
    static winrt::guid HVIDX_DXGI_RGBA_R16G16B16A16_UNORM();
    static winrt::guid HVIDX_DXGI_RGBA_R16G16B16A16_UINT();
    static winrt::guid HVIDX_DXGI_RGBA_R16G16B16A16_SNORM();
    static winrt::guid HVIDX_DXGI_RGBA_R16G16B16A16_SINT();
    static winrt::guid HVIDX_DXGI_RGBA_R32G32_TYPELESS();
    static winrt::guid HVIDX_DXGI_RGBA_R32G32_FLOAT();
    static winrt::guid HVIDX_DXGI_RGBA_R32G32_UINT();
    static winrt::guid HVIDX_DXGI_RGBA_R32G32_SINT();
    static winrt::guid HVIDX_DXGI_RGBA_R32G8X24_TYPELESS();
    static winrt::guid HVIDX_DXGI_RGBA_D32_FLOAT_S8X24_UINT();
    static winrt::guid HVIDX_DXGI_RGBA_R32_FLOAT_X8X24_TYPELESS();
    static winrt::guid HVIDX_DXGI_RGBA_X32_TYPELESS_G8X24_UINT();
    static winrt::guid HVIDX_DXGI_RGBA_R10G10B10A2_TYPELESS();
    static winrt::guid HVIDX_DXGI_RGBA_R10G10B10A2_UNORM();
    static winrt::guid HVIDX_DXGI_RGBA_R10G10B10A2_UINT();
    static winrt::guid HVIDX_DXGI_RGBA_R11G11B10_FLOAT();
    static winrt::guid HVIDX_DXGI_RGBA_R8G8B8A8_TYPELESS();
    static winrt::guid HVIDX_DXGI_RGBA_R8G8B8A8_UNORM();
    static winrt::guid HVIDX_DXGI_RGBA_R8G8B8A8_UNORM_SRGB();
    static winrt::guid HVIDX_DXGI_RGBA_R8G8B8A8_UINT();
    static winrt::guid HVIDX_DXGI_RGBA_R8G8B8A8_SNORM();
    static winrt::guid HVIDX_DXGI_RGBA_R8G8B8A8_SINT();
    static winrt::guid HVIDX_DXGI_RGBA_R16G16_TYPELESS();
    static winrt::guid HVIDX_DXGI_RGBA_R16G16_FLOAT();
    static winrt::guid HVIDX_DXGI_RGBA_R16G16_UNORM();
    static winrt::guid HVIDX_DXGI_RGBA_R16G16_UINT();
    static winrt::guid HVIDX_DXGI_RGBA_R16G16_SNORM();
    static winrt::guid HVIDX_DXGI_RGBA_R16G16_SINT();
    static winrt::guid HVIDX_DXGI_RGBA_R32_TYPELESS();
    static winrt::guid HVIDX_DXGI_RGBA_D32_FLOAT();
    static winrt::guid HVIDX_DXGI_RGBA_R32_FLOAT();
    static winrt::guid HVIDX_DXGI_RGBA_R32_UINT();
    static winrt::guid HVIDX_DXGI_RGBA_R32_SINT();
    static winrt::guid HVIDX_DXGI_RGBA_R24G8_TYPELESS();
    static winrt::guid HVIDX_DXGI_RGBA_D24_UNORM_S8_UINT();
    static winrt::guid HVIDX_DXGI_RGBA_R24_UNORM_X8_TYPELESS();
    static winrt::guid HVIDX_DXGI_RGBA_X24_TYPELESS_G8_UINT();
    static winrt::guid HVIDX_DXGI_RGBA_R8G8_TYPELESS();
    static winrt::guid HVIDX_DXGI_RGBA_R8G8_UNORM();
    static winrt::guid HVIDX_DXGI_RGBA_R8G8_UINT();
    static winrt::guid HVIDX_DXGI_RGBA_R8G8_SNORM();
    static winrt::guid HVIDX_DXGI_RGBA_R8G8_SINT();
    static winrt::guid HVIDX_DXGI_RGBA_R16_TYPELESS();
    static winrt::guid HVIDX_DXGI_RGBA_R16_FLOAT();
    static winrt::guid HVIDX_DXGI_RGBA_D16_UNORM();
    static winrt::guid HVIDX_DXGI_RGBA_R16_UNORM();
    static winrt::guid HVIDX_DXGI_RGBA_R16_UINT();
    static winrt::guid HVIDX_DXGI_RGBA_R16_SNORM();
    static winrt::guid HVIDX_DXGI_RGBA_R16_SINT();
    static winrt::guid HVIDX_DXGI_RGBA_R8_TYPELESS();
    static winrt::guid HVIDX_DXGI_RGBA_R8_UNORM();
    static winrt::guid HVIDX_DXGI_RGBA_R8_UINT();
    static winrt::guid HVIDX_DXGI_RGBA_R8_SNORM();
    static winrt::guid HVIDX_DXGI_RGBA_R8_SINT();
    static winrt::guid HVIDX_DXGI_RGBA_A8_UNORM();
    static winrt::guid HVIDX_DXGI_RGBA_R1_UNORM();
    static winrt::guid HVIDX_DXGI_RGBA_R9G9B9E5_SHAREDEXP();
    static winrt::guid HVIDX_DXGI_RGBA_R8G8_B8G8_UNORM();
    static winrt::guid HVIDX_DXGI_RGBA_G8R8_G8B8_UNORM();
    static winrt::guid HVIDX_DXGI_RGBA_BC1_TYPELESS();
    static winrt::guid HVIDX_DXGI_RGBA_BC1_UNORM();
    static winrt::guid HVIDX_DXGI_RGBA_BC1_UNORM_SRGB();
    static winrt::guid HVIDX_DXGI_RGBA_BC2_TYPELESS();
    static winrt::guid HVIDX_DXGI_RGBA_BC2_UNORM();
    static winrt::guid HVIDX_DXGI_RGBA_BC2_UNORM_SRGB();
    static winrt::guid HVIDX_DXGI_RGBA_BC3_TYPELESS();
    static winrt::guid HVIDX_DXGI_RGBA_BC3_UNORM();
    static winrt::guid HVIDX_DXGI_RGBA_BC3_UNORM_SRGB();
    static winrt::guid HVIDX_DXGI_RGBA_BC4_TYPELESS();
    static winrt::guid HVIDX_DXGI_RGBA_BC4_UNORM();
    static winrt::guid HVIDX_DXGI_RGBA_BC4_SNORM();
    static winrt::guid HVIDX_DXGI_RGBA_BC5_TYPELESS();
    static winrt::guid HVIDX_DXGI_RGBA_BC5_UNORM();
    static winrt::guid HVIDX_DXGI_RGBA_BC5_SNORM();
    static winrt::guid HVIDX_DXGI_RGBA_B5G6R5_UNORM();
    static winrt::guid HVIDX_DXGI_RGBA_B5G5R5A1_UNORM();
    static winrt::guid HVIDX_DXGI_RGBA_B8G8R8A8_UNORM();
    static winrt::guid HVIDX_DXGI_RGBA_B8G8R8X8_UNORM();
    static winrt::guid HVIDX_DXGI_RGBA_R10G10B10_XR_BIAS_A2_UNORM();
    static winrt::guid HVIDX_DXGI_RGBA_B8G8R8A8_TYPELESS();
    static winrt::guid HVIDX_DXGI_RGBA_B8G8R8A8_UNORM_SRGB();
    static winrt::guid HVIDX_DXGI_RGBA_B8G8R8X8_TYPELESS();
    static winrt::guid HVIDX_DXGI_RGBA_B8G8R8X8_UNORM_SRGB();
    static winrt::guid HVIDX_DXGI_RGBA_BC6H_TYPELESS();
    static winrt::guid HVIDX_DXGI_RGBA_BC6H_UF16();
    static winrt::guid HVIDX_DXGI_RGBA_BC6H_SF16();
    static winrt::guid HVIDX_DXGI_RGBA_BC7_TYPELESS();
    static winrt::guid HVIDX_DXGI_RGBA_BC7_UNORM();
    static winrt::guid HVIDX_DXGI_RGBA_BC7_UNORM_SRGB();
    static winrt::guid HVIDX_DXGI_RGBA_AYUV();
    static winrt::guid HVIDX_DXGI_RGBA_Y410();
    static winrt::guid HVIDX_DXGI_RGBA_Y416();
    static winrt::guid HVIDX_DXGI_RGBA_NV12();
    static winrt::guid HVIDX_DXGI_RGBA_P010();
    static winrt::guid HVIDX_DXGI_RGBA_P016();
    static winrt::guid HVIDX_DXGI_RGBA_420_OPAQUE();
    static winrt::guid HVIDX_DXGI_RGBA_YUY2();
    static winrt::guid HVIDX_DXGI_RGBA_Y210();
    static winrt::guid HVIDX_DXGI_RGBA_Y216();
    static winrt::guid HVIDX_DXGI_RGBA_NV11();
    static winrt::guid HVIDX_DXGI_RGBA_AI44();
    static winrt::guid HVIDX_DXGI_RGBA_IA44();
    static winrt::guid HVIDX_DXGI_RGBA_P8();
    static winrt::guid HVIDX_DXGI_RGBA_A8P8();
    static winrt::guid HVIDX_DXGI_RGBA_B4G4R4A4_UNORM();
    static winrt::guid HVIDX_DXGI_RGBA_P208();
    static winrt::guid HVIDX_DXGI_RGBA_V208();
    static winrt::guid HVIDX_DXGI_RGBA_V408();
    static winrt::guid HVIDX_DXGI_RGBA_SAMPLER_FEEDBACK_MIN_MIP_OPAQUE();
    static winrt::guid HVIDX_DXGI_RGBA_SAMPLER_FEEDBACK_MIP_REGION_USED_OPAQUE();
    static winrt::guid HVIDX_DXGI_RGBA_FORCE_UINT();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{
            winrt::single_threaded_vector<winrt::guid>(std::vector<winrt::guid>{HVIDX_DXGI_RGBA(),
                                                                                HVIDX_DXGI_RGBA_UNKNOWN(),
                                                                                HVIDX_DXGI_RGBA_R32G32B32A32_TYPELESS(),
                                                                                HVIDX_DXGI_RGBA_R32G32B32A32_FLOAT(),
                                                                                HVIDX_DXGI_RGBA_R32G32B32A32_UINT(),
                                                                                HVIDX_DXGI_RGBA_R32G32B32A32_SINT(),
                                                                                HVIDX_DXGI_RGBA_R32G32B32_TYPELESS(),
                                                                                HVIDX_DXGI_RGBA_R32G32B32_FLOAT(),
                                                                                HVIDX_DXGI_RGBA_R32G32B32_UINT(),
                                                                                HVIDX_DXGI_RGBA_R32G32B32_SINT(),
                                                                                HVIDX_DXGI_RGBA_R16G16B16A16_TYPELESS(),
                                                                                HVIDX_DXGI_RGBA_R16G16B16A16_FLOAT(),
                                                                                HVIDX_DXGI_RGBA_R16G16B16A16_UNORM(),
                                                                                HVIDX_DXGI_RGBA_R16G16B16A16_UINT(),
                                                                                HVIDX_DXGI_RGBA_R16G16B16A16_SNORM(),
                                                                                HVIDX_DXGI_RGBA_R16G16B16A16_SINT(),
                                                                                HVIDX_DXGI_RGBA_R32G32_TYPELESS(),
                                                                                HVIDX_DXGI_RGBA_R32G32_FLOAT(),
                                                                                HVIDX_DXGI_RGBA_R32G32_UINT(),
                                                                                HVIDX_DXGI_RGBA_R32G32_SINT(),
                                                                                HVIDX_DXGI_RGBA_R32G8X24_TYPELESS(),
                                                                                HVIDX_DXGI_RGBA_D32_FLOAT_S8X24_UINT(),
                                                                                HVIDX_DXGI_RGBA_R32_FLOAT_X8X24_TYPELESS(),
                                                                                HVIDX_DXGI_RGBA_X32_TYPELESS_G8X24_UINT(),
                                                                                HVIDX_DXGI_RGBA_R10G10B10A2_TYPELESS(),
                                                                                HVIDX_DXGI_RGBA_R10G10B10A2_UNORM(),
                                                                                HVIDX_DXGI_RGBA_R10G10B10A2_UINT(),
                                                                                HVIDX_DXGI_RGBA_R11G11B10_FLOAT(),
                                                                                HVIDX_DXGI_RGBA_R8G8B8A8_TYPELESS(),
                                                                                HVIDX_DXGI_RGBA_R8G8B8A8_UNORM(),
                                                                                HVIDX_DXGI_RGBA_R8G8B8A8_UNORM_SRGB(),
                                                                                HVIDX_DXGI_RGBA_R8G8B8A8_UINT(),
                                                                                HVIDX_DXGI_RGBA_R8G8B8A8_SNORM(),
                                                                                HVIDX_DXGI_RGBA_R8G8B8A8_SINT(),
                                                                                HVIDX_DXGI_RGBA_R16G16_TYPELESS(),
                                                                                HVIDX_DXGI_RGBA_R16G16_FLOAT(),
                                                                                HVIDX_DXGI_RGBA_R16G16_UNORM(),
                                                                                HVIDX_DXGI_RGBA_R16G16_UINT(),
                                                                                HVIDX_DXGI_RGBA_R16G16_SNORM(),
                                                                                HVIDX_DXGI_RGBA_R16G16_SINT(),
                                                                                HVIDX_DXGI_RGBA_R32_TYPELESS(),
                                                                                HVIDX_DXGI_RGBA_D32_FLOAT(),
                                                                                HVIDX_DXGI_RGBA_R32_FLOAT(),
                                                                                HVIDX_DXGI_RGBA_R32_UINT(),
                                                                                HVIDX_DXGI_RGBA_R32_SINT(),
                                                                                HVIDX_DXGI_RGBA_R24G8_TYPELESS(),
                                                                                HVIDX_DXGI_RGBA_D24_UNORM_S8_UINT(),
                                                                                HVIDX_DXGI_RGBA_R24_UNORM_X8_TYPELESS(),
                                                                                HVIDX_DXGI_RGBA_X24_TYPELESS_G8_UINT(),
                                                                                HVIDX_DXGI_RGBA_R8G8_TYPELESS(),
                                                                                HVIDX_DXGI_RGBA_R8G8_UNORM(),
                                                                                HVIDX_DXGI_RGBA_R8G8_UINT(),
                                                                                HVIDX_DXGI_RGBA_R8G8_SNORM(),
                                                                                HVIDX_DXGI_RGBA_R8G8_SINT(),
                                                                                HVIDX_DXGI_RGBA_R16_TYPELESS(),
                                                                                HVIDX_DXGI_RGBA_R16_FLOAT(),
                                                                                HVIDX_DXGI_RGBA_D16_UNORM(),
                                                                                HVIDX_DXGI_RGBA_R16_UNORM(),
                                                                                HVIDX_DXGI_RGBA_R16_UINT(),
                                                                                HVIDX_DXGI_RGBA_R16_SNORM(),
                                                                                HVIDX_DXGI_RGBA_R16_SINT(),
                                                                                HVIDX_DXGI_RGBA_R8_TYPELESS(),
                                                                                HVIDX_DXGI_RGBA_R8_UNORM(),
                                                                                HVIDX_DXGI_RGBA_R8_UINT(),
                                                                                HVIDX_DXGI_RGBA_R8_SNORM(),
                                                                                HVIDX_DXGI_RGBA_R8_SINT(),
                                                                                HVIDX_DXGI_RGBA_A8_UNORM(),
                                                                                HVIDX_DXGI_RGBA_R1_UNORM(),
                                                                                HVIDX_DXGI_RGBA_R9G9B9E5_SHAREDEXP(),
                                                                                HVIDX_DXGI_RGBA_R8G8_B8G8_UNORM(),
                                                                                HVIDX_DXGI_RGBA_G8R8_G8B8_UNORM(),
                                                                                HVIDX_DXGI_RGBA_BC1_TYPELESS(),
                                                                                HVIDX_DXGI_RGBA_BC1_UNORM(),
                                                                                HVIDX_DXGI_RGBA_BC1_UNORM_SRGB(),
                                                                                HVIDX_DXGI_RGBA_BC2_TYPELESS(),
                                                                                HVIDX_DXGI_RGBA_BC2_UNORM(),
                                                                                HVIDX_DXGI_RGBA_BC2_UNORM_SRGB(),
                                                                                HVIDX_DXGI_RGBA_BC3_TYPELESS(),
                                                                                HVIDX_DXGI_RGBA_BC3_UNORM(),
                                                                                HVIDX_DXGI_RGBA_BC3_UNORM_SRGB(),
                                                                                HVIDX_DXGI_RGBA_BC4_TYPELESS(),
                                                                                HVIDX_DXGI_RGBA_BC4_UNORM(),
                                                                                HVIDX_DXGI_RGBA_BC4_SNORM(),
                                                                                HVIDX_DXGI_RGBA_BC5_TYPELESS(),
                                                                                HVIDX_DXGI_RGBA_BC5_UNORM(),
                                                                                HVIDX_DXGI_RGBA_BC5_SNORM(),
                                                                                HVIDX_DXGI_RGBA_B5G6R5_UNORM(),
                                                                                HVIDX_DXGI_RGBA_B5G5R5A1_UNORM(),
                                                                                HVIDX_DXGI_RGBA_B8G8R8A8_UNORM(),
                                                                                HVIDX_DXGI_RGBA_B8G8R8X8_UNORM(),
                                                                                HVIDX_DXGI_RGBA_R10G10B10_XR_BIAS_A2_UNORM(),
                                                                                HVIDX_DXGI_RGBA_B8G8R8A8_TYPELESS(),
                                                                                HVIDX_DXGI_RGBA_B8G8R8A8_UNORM_SRGB(),
                                                                                HVIDX_DXGI_RGBA_B8G8R8X8_TYPELESS(),
                                                                                HVIDX_DXGI_RGBA_B8G8R8X8_UNORM_SRGB(),
                                                                                HVIDX_DXGI_RGBA_BC6H_TYPELESS(),
                                                                                HVIDX_DXGI_RGBA_BC6H_UF16(),
                                                                                HVIDX_DXGI_RGBA_BC6H_SF16(),
                                                                                HVIDX_DXGI_RGBA_BC7_TYPELESS(),
                                                                                HVIDX_DXGI_RGBA_BC7_UNORM(),
                                                                                HVIDX_DXGI_RGBA_BC7_UNORM_SRGB(),
                                                                                HVIDX_DXGI_RGBA_AYUV(),
                                                                                HVIDX_DXGI_RGBA_Y410(),
                                                                                HVIDX_DXGI_RGBA_Y416(),
                                                                                HVIDX_DXGI_RGBA_NV12(),
                                                                                HVIDX_DXGI_RGBA_P010(),
                                                                                HVIDX_DXGI_RGBA_P016(),
                                                                                HVIDX_DXGI_RGBA_420_OPAQUE(),
                                                                                HVIDX_DXGI_RGBA_YUY2(),
                                                                                HVIDX_DXGI_RGBA_Y210(),
                                                                                HVIDX_DXGI_RGBA_Y216(),
                                                                                HVIDX_DXGI_RGBA_NV11(),
                                                                                HVIDX_DXGI_RGBA_AI44(),
                                                                                HVIDX_DXGI_RGBA_IA44(),
                                                                                HVIDX_DXGI_RGBA_P8(),
                                                                                HVIDX_DXGI_RGBA_A8P8(),
                                                                                HVIDX_DXGI_RGBA_B4G4R4A4_UNORM(),
                                                                                HVIDX_DXGI_RGBA_P208(),
                                                                                HVIDX_DXGI_RGBA_V208(),
                                                                                HVIDX_DXGI_RGBA_V408(),
                                                                                HVIDX_DXGI_RGBA_SAMPLER_FEEDBACK_MIN_MIP_OPAQUE(),
                                                                                HVIDX_DXGI_RGBA_SAMPLER_FEEDBACK_MIP_REGION_USED_OPAQUE(),
                                                                                HVIDX_DXGI_RGBA_FORCE_UINT()})};
    }
};
}  // namespace winrt::IIr77GRAPHICS::m_implementation

namespace winrt::IIr77GRAPHICS::factory_implementation {
struct IDXIr77RGBA : IDXIr77RGBAT<IDXIr77RGBA, m_implementation::IDXIr77RGBA> {};
}  // namespace winrt::IIr77GRAPHICS::factory_implementation
