#include "pch.h"
#include "IDXIr77SCALING.h"
#if __has_include("IDXIr77SCALING.g.cpp")
#include "IDXIr77SCALING.g.cpp"
#endif

namespace winrt::IIr77GRAPHICS::m_implementation {
winrt::guid IDXIr77SCALING::HVIDX_DXGI_SCALING() { return winrt::guid{"{73EB0731-76C2-4CE9-A202-99EEDCE7ACAE}"}; }
winrt::guid IDXIr77SCALING::HVIDX_DXGI_SCALING_STRETCH() { return winrt::guid{"{E752B2FA-7533-4883-A783-91B85C79DAD8}"}; }
winrt::guid IDXIr77SCALING::HVIDX_DXGI_SCALING_NONE() { return winrt::guid{"{67B2E0C0-776B-4052-B948-5609BBF03D07}"}; }
winrt::guid IDXIr77SCALING::HVIDX_DXGI_SCALING_ASPECT_RATIO_STRETCH() { return winrt::guid{"{F68A3927-83C2-4E0E-B7F8-E1C3A8AC3BD6}"}; }
}  // namespace winrt::IIr77GRAPHICS::m_implementation