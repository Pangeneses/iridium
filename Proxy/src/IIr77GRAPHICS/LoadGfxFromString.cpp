#include "pch.h"
#include "LoadGfxFromString.h"
#if __has_include("LoadGfxFromString.g.cpp")
#include "LoadGfxFromString.g.cpp"
#endif

namespace winrt::IIr77GRAPHICS::m_implementation {
LoadGfxFromString::LoadGfxFromString(winrt::hstring file) { /*
                                                                   gfxPipelineFile = "C:\\Users\\rebek\\source\\Ir77\\x64\\Debug\\";

                                                                   gfxPipelineFile += std::wstring{ file } + std::wstring{ "\\" };

                                                                   gfxPipelineFile += std::wstring{ file.c_str() } + std::wstring{ ".dll" };

                                                                   gfxPipelineName = file;

                                                                   gfxPipelineName += ".GraphicsDevice";
                                                                   */
}
/*
IIr77GRAPHICS::IIr77DEVICE LoadGfxFromString::Get()
{
        HMODULE dllHandle = LoadLibraryW(gfxPipelineFile.c_str());

        if (!dllHandle) { throw std::invalid_argument{ "Invalid application specified." }; }

        void* pProc = GetProcAddress(dllHandle, "DllGetActivationFactory");

        auto DllGetActivationFactory = reinterpret_cast<int32_t(__stdcall*)(void* classId, void** factory)>(pProc);

        HSTRING hcname = NULL;
        HSTRING_HEADER project;
        HRESULT hr = WindowsCreateStringReference(gfxPipelineName.data(), (UINT32)gfxPipelineName.length(), &project, &hcname);

        if (FAILED(hr)) throw std::runtime_error{ "Invalid path." };

        WF::IActivationFactory activationFactory{ nullptr };
        auto ret = (*DllGetActivationFactory)((void*)&project, winrt::put_abi(activationFactory));

        if (ret) throw std::runtime_error{ "Could not acquire activation factory." };

        IIr77GRAPHICS::IIr77DEVICE gfxDevice{ nullptr };
        gfxDevice = activationFactory.ActivateInstance<IIr77GRAPHICS::IIr77DEVICE>();

        if (!gfxDevice) throw std::runtime_error{ "Could not acquire runtime class." };

        return gfxDevice;
                }
*/
}  // namespace winrt::IIr77GRAPHICS::m_implementation
