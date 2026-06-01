#pragma once

#include "winrt/IIr77BASE.h"
#include "winrt/IIr77APP.h"

#include "IDLIr77APP.g.h"

namespace winrt::IIr77APP::m_implementation {
struct IDLIr77APP {
   public:
    IDLIr77APP() = default;

   public:
    static winrt::guid HVIDLIr77APP();
    static winrt::guid HVIDLIr77LANDING();
    static winrt::guid HVIDLIr77MACHINE();
    static winrt::guid HVIDLIr77PROJECT();
    static winrt::guid HVIDLIr77UTILITY();
    static winrt::guid HVIDLIr77SHELL();
    static winrt::guid HVIDLIr77DATABASE();
    static winrt::guid HVIDLIr77ARXIVER();
    static winrt::guid HVIDLIr77KERNELS();
    static winrt::guid HVIDLIr77MASSENTROPY();
    static winrt::guid HVIDLIr77CAD();
    static winrt::guid HVIDLIr77SMASHTOPOLOGY();
    static winrt::guid HVIDLIr77FABULOUS();
    static winrt::guid HVIDLIr77PLOTSFIELD();

    static Windows::Foundation::Collections::IVector<winrt::guid> UUIDLIST() {
        return Windows::Foundation::Collections::IVector<winrt::guid>{winrt::single_threaded_vector<winrt::guid>(
            std::vector<winrt::guid>{HVIDLIr77APP(), HVIDLIr77LANDING(), HVIDLIr77MACHINE(), HVIDLIr77PROJECT(), HVIDLIr77UTILITY(), HVIDLIr77SHELL(),
                                     HVIDLIr77DATABASE(), HVIDLIr77ARXIVER(), HVIDLIr77KERNELS(), HVIDLIr77MASSENTROPY(), HVIDLIr77CAD(),
                                     HVIDLIr77SMASHTOPOLOGY(), HVIDLIr77FABULOUS(), HVIDLIr77PLOTSFIELD()})};
    }
};
}  // namespace winrt::IIr77APP::m_implementation

namespace winrt::IIr77APP::factory_implementation {
struct IDLIr77APP : IDLIr77APPT<IDLIr77APP, m_implementation::IDLIr77APP> {};
}  // namespace winrt::IIr77APP::factory_implementation
