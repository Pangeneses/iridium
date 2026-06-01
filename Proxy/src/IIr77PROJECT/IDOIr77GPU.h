#pragma once

#include "winrt/IIr77PROJECT.h"

#include "IDOIr77GPU.g.h"

namespace winrt::IIr77PROJECT::m_implementation {
struct IDOIr77GPU {
   public:
    IDOIr77GPU() = default;

   public:
    static winrt::guid HVIDOIr77GPU();
    static winrt::guid HVIDOIr77ATTPIPELINE();
    static winrt::guid HVIDOIr77DETPIPELINE();
    static winrt::guid HVIDOIr77ATTCONTEXT();
    static winrt::guid HVIDOIr77DETCONTEXT();
    static winrt::guid HVIDOIr77NEWCONTEXT();
    static winrt::guid HVIDOIr77DELCONTEXT();
    static winrt::guid HVIDOIr77SETCONTEXT();
    static winrt::guid HVIDOIr77NEWNILOBJ();
    static winrt::guid HVIDOIr77DELNILOBJ();
    static winrt::guid HVIDOIr77NEWTEXTURE();
    static winrt::guid HVIDOIr77DELTEXTURE();
    static winrt::guid HVIDOIr77NEWSHADER();
    static winrt::guid HVIDOIr77DELSHADER();
    static winrt::guid HVIDOIr77NEWBUFFER();
    static winrt::guid HVIDOIr77DELBUFFER();
    static winrt::guid HVIDOIr77NEWSAMPLER();
    static winrt::guid HVIDOIr77DELSAMPLER();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{winrt::single_threaded_vector<winrt::guid>(std::vector<winrt::guid>{
            HVIDOIr77GPU(), HVIDOIr77ATTPIPELINE(), HVIDOIr77DETPIPELINE(), HVIDOIr77ATTCONTEXT(), HVIDOIr77DETCONTEXT(), HVIDOIr77NEWCONTEXT(),
            HVIDOIr77DELCONTEXT(), HVIDOIr77SETCONTEXT(), HVIDOIr77NEWNILOBJ(), HVIDOIr77DELNILOBJ(), HVIDOIr77NEWTEXTURE(), HVIDOIr77DELTEXTURE(),
            HVIDOIr77NEWSHADER(), HVIDOIr77DELSHADER(), HVIDOIr77NEWBUFFER(), HVIDOIr77DELBUFFER(), HVIDOIr77NEWSAMPLER(), HVIDOIr77DELSAMPLER()})};
    }
};
}  // namespace winrt::IIr77PROJECT::m_implementation

namespace winrt::IIr77PROJECT::factory_implementation {
struct IDOIr77GPU : IDOIr77GPUT<IDOIr77GPU, m_implementation::IDOIr77GPU> {};
}  // namespace winrt::IIr77PROJECT::factory_implementation
