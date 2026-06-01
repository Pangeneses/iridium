#pragma once

#include "winrt/IIr77BASE.h"
#include "winrt/IIr77GRAPHICS.h"

#include "IDXIr77STATE.g.h"

namespace winrt::IIr77GRAPHICS::m_implementation {
struct IDXIr77STATE {
   public:
    IDXIr77STATE() = default;

   public:
    static winrt::guid HVIDXIr77STATE();
    static winrt::guid HVIDXIr77API();
    static winrt::guid HVIDXIr77SURFACE();
    static winrt::guid HVIDXIr77TIMEOUT();
    static winrt::guid HVIDXIr77REFRESH();
    static winrt::guid HVIDXIr77DISPLAY();
    static winrt::guid HVIDXIr77RGBA();
    static winrt::guid HVIDXIr77DELTA();
    static winrt::guid HVIDXIr77EVENT();
    static winrt::guid HVIDIr77TOP();
    static winrt::guid HVIDIr77LEFT();
    static winrt::guid HVIDIr77RIGHT();
    static winrt::guid HVIDIr77BOTTOM();
    static winrt::guid HVIDXIr77TOPLEFTX();
    static winrt::guid HVIDXIr77TOPLEFTY();
    static winrt::guid HVIDXIr77WIDTH();
    static winrt::guid HVIDXIr77HEIGHT();
    static winrt::guid HVIDXIr77MINDEPTH();
    static winrt::guid HVIDXIr77MAXDEPTH();
    static winrt::guid HVIDXIr77FRAMES();
    static winrt::guid HVIDXIr77STEREO();
    static winrt::guid HVIDXIr77COUNT();
    static winrt::guid HVIDXIr77QUALITY();
    static winrt::guid HVIDXIr77BUFFERUSAGE();
    static winrt::guid HVIDXIr77BUFFERCOUNT();
    static winrt::guid HVIDXIr77SCALING();
    static winrt::guid HVIDXIr77SWAPEFFECT();
    static winrt::guid HVIDXIr77ALPHAMODE();
    static winrt::guid HVIDXIr77FLAGS();
    static winrt::guid HVIDXIr77FILLMODE();
    static winrt::guid HVIDXIr77CULLMODE();
    static winrt::guid HVIDXIr77FRONTCOUNTERCLOCKWISE();
    static winrt::guid HVIDXIr77DEPTHBIAS();
    static winrt::guid HVIDXIr77DEPTHBIASCLAMP();
    static winrt::guid HVIDXIr77SLOPESCALEDDEPTHBIAS();
    static winrt::guid HVIDXIr77DEPTHCLIPENABLE();
    static winrt::guid HVIDXIr77MULTISAMPLEENABLE();
    static winrt::guid HVIDXIr77ANTIALIASEDLINEENABLE();
    static winrt::guid HVIDXIr77FORCEDSAMPLECOUNT();
    static winrt::guid HVIDXIr77CONSERVATIVERASTER();
    static winrt::guid HVIDXIr77VSYNC();
    static winrt::guid HVIDXIr77EXPOSURE();
    static winrt::guid HVIDXIr77GAMMA();
    static winrt::guid HVIDXIr77CHROMA();
    static winrt::guid HVIDXIr77AA();
    static winrt::guid HVIDXIr77AAX();
    static winrt::guid HVIDXIr77FXAA();
    static winrt::guid HVIDXIr77FILTERING();
    static winrt::guid HVIDXIr77HDR();
    static winrt::guid HVIDXIr77BLOOM();
    static winrt::guid HVIDXIr77OCCLUSION();
    static winrt::guid HVIDXIr77CAUSTIC();
    static winrt::guid HVIDXIr77VOLUMETRIC();
    static winrt::guid HVIDXIr77REFLECTION();
    static winrt::guid HVIDXIr77RADIOSITY();
    static winrt::guid HVIDXIr77RAYTRACING();
    static winrt::guid HVIDXIr77LOD();
    static winrt::guid HVIDXIr77SL();
    static winrt::guid HVIDXIr77TESSELLATION();
    static winrt::guid HVIDXIr77CODEC();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{
            winrt::single_threaded_vector<winrt::guid>(std::vector<winrt::guid>{HVIDXIr77STATE(),
                                                                                HVIDXIr77API(),
                                                                                HVIDXIr77SURFACE(),
                                                                                HVIDXIr77TIMEOUT(),
                                                                                HVIDXIr77REFRESH(),
                                                                                HVIDXIr77DISPLAY(),
                                                                                HVIDXIr77RGBA(),
                                                                                HVIDXIr77DELTA(),
                                                                                HVIDXIr77EVENT(),
                                                                                HVIDIr77TOP(),
                                                                                HVIDIr77LEFT(),
                                                                                HVIDIr77RIGHT(),
                                                                                HVIDIr77BOTTOM(),
                                                                                HVIDXIr77TOPLEFTX(),
                                                                                HVIDXIr77TOPLEFTY(),
                                                                                HVIDXIr77WIDTH(),
                                                                                HVIDXIr77HEIGHT(),
                                                                                HVIDXIr77MINDEPTH(),
                                                                                HVIDXIr77MAXDEPTH(),
                                                                                HVIDXIr77FRAMES(),
                                                                                HVIDXIr77STEREO(),
                                                                                HVIDXIr77COUNT(),
                                                                                HVIDXIr77QUALITY(),
                                                                                HVIDXIr77BUFFERUSAGE(),
                                                                                HVIDXIr77BUFFERCOUNT(),
                                                                                HVIDXIr77SCALING(),
                                                                                HVIDXIr77SWAPEFFECT(),
                                                                                HVIDXIr77ALPHAMODE(),
                                                                                HVIDXIr77FLAGS(),
                                                                                HVIDXIr77FILLMODE(),
                                                                                HVIDXIr77CULLMODE(),
                                                                                HVIDXIr77FRONTCOUNTERCLOCKWISE(),
                                                                                HVIDXIr77DEPTHBIAS(),
                                                                                HVIDXIr77DEPTHBIASCLAMP(),
                                                                                HVIDXIr77SLOPESCALEDDEPTHBIAS(),
                                                                                HVIDXIr77DEPTHCLIPENABLE(),
                                                                                HVIDXIr77MULTISAMPLEENABLE(),
                                                                                HVIDXIr77ANTIALIASEDLINEENABLE(),
                                                                                HVIDXIr77FORCEDSAMPLECOUNT(),
                                                                                HVIDXIr77CONSERVATIVERASTER(),
                                                                                HVIDXIr77VSYNC(),
                                                                                HVIDXIr77EXPOSURE(),
                                                                                HVIDXIr77GAMMA(),
                                                                                HVIDXIr77CHROMA(),
                                                                                HVIDXIr77AA(),
                                                                                HVIDXIr77AAX(),
                                                                                HVIDXIr77FXAA(),
                                                                                HVIDXIr77FILTERING(),
                                                                                HVIDXIr77HDR(),
                                                                                HVIDXIr77BLOOM(),
                                                                                HVIDXIr77OCCLUSION(),
                                                                                HVIDXIr77CAUSTIC(),
                                                                                HVIDXIr77VOLUMETRIC(),
                                                                                HVIDXIr77REFLECTION(),
                                                                                HVIDXIr77RADIOSITY(),
                                                                                HVIDXIr77RAYTRACING(),
                                                                                HVIDXIr77LOD(),
                                                                                HVIDXIr77SL(),
                                                                                HVIDXIr77TESSELLATION(),
                                                                                HVIDXIr77CODEC()})};
    }
};
}  // namespace winrt::IIr77GRAPHICS::m_implementation

namespace winrt::IIr77GRAPHICS::factory_implementation {
struct IDXIr77STATE : IDXIr77STATET<IDXIr77STATE, m_implementation::IDXIr77STATE> {};
}  // namespace winrt::IIr77GRAPHICS::factory_implementation
