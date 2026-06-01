#include "pch.h"
#include "IDOIr77APP.h"
#if __has_include("IDOIr77APP.g.cpp")
#include "IDOIr77APP.g.cpp"
#endif

namespace winrt::IIr77APP::m_implementation {
winrt::guid IDOIr77APP::HVIDOIr77APP() { return winrt::guid{"{74C957E6-C2EF-439C-A919-F1F1D8868324}"}; }
winrt::guid IDOIr77APP::HVIDOIr77LOAD() { return winrt::guid{"{150D352F-EF40-441F-BBFD-60634163A92B}"}; }
winrt::guid IDOIr77APP::HVIDOIr77UNLOAD() { return winrt::guid{"{2C487F21-3C72-4EF3-B78B-5811DCE4C1E0}"}; }
winrt::guid IDOIr77APP::HVIDOIr77PARK() { return winrt::guid{"{902CE761-5ED9-4FF5-B837-599C7FEF2B66}"}; }
winrt::guid IDOIr77APP::HVIDOIr77SETSTATE() { return winrt::guid{"{9DB2C875-247C-4707-A609-185F05F8CC33}"}; }
winrt::guid IDOIr77APP::HVIDOIr77GETSTATE() { return winrt::guid{"{0BED2007-A8B6-410A-B7FA-E8141A19D055}"}; }
}  // namespace winrt::IIr77APP::m_implementation