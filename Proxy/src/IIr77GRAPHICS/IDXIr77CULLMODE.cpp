#include "pch.h"
#include "IDXIr77CULLMODE.h"
#if __has_include("IDXIr77CULLMODE.g.cpp")
#include "IDXIr77CULLMODE.g.cpp"
#endif

namespace winrt::IIr77GRAPHICS::m_implementation {
winrt::guid IDXIr77CULLMODE::HVIDX_D3D12_CULL_MODE() { return winrt::guid{"{5684692E-D38F-4AED-969A-536977EEC840}"}; }
winrt::guid IDXIr77CULLMODE::HVIDX_D3D12_CULL_MODE_NONE() { return winrt::guid{"{161938C7-0F6F-4B6E-9B63-C169E4154273}"}; }
winrt::guid IDXIr77CULLMODE::HVIDX_D3D12_CULL_MODE_FRONT() { return winrt::guid{"{47286DF3-9708-43C1-8CBD-E20976FA9AEF}"}; }
winrt::guid IDXIr77CULLMODE::HVIDX_D3D12_CULL_MODE_BACK() { return winrt::guid{"{B190052F-7D56-4503-9D2F-BB8807AE6D5A}"}; }
}  // namespace winrt::IIr77GRAPHICS::m_implementation