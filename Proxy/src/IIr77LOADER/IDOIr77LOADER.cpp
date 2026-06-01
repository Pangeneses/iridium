#include "pch.h"
#include "IDOIr77LOADER.h"
#if __has_include("IDOIr77LOADER.g.cpp")
#include "IDOIr77LOADER.g.cpp"
#endif

namespace winrt::IIr77LOADER::m_implementation {
winrt::guid IDOIr77LOADER::HVIDOIr77LOADER() { return winrt::guid{"{5965D9E9-0558-458F-8515-6D2774E4E280}"}; }
winrt::guid IDOIr77LOADER::HVIDOIr77LOAD() { return winrt::guid{"{8576926F-BD80-4F4E-811B-312D3E20E9F1}"}; }
winrt::guid IDOIr77LOADER::HVIDOIr77UNLOAD() { return winrt::guid{"{52FEEDC8-E010-4F40-8DE3-58981F0A947D}"}; }
winrt::guid IDOIr77LOADER::HVIDOIr77SETSTATE() { return winrt::guid{"{11F64698-9416-47E0-864B-CEA83E286788}"}; }
winrt::guid IDOIr77LOADER::HVIDOIr77GETSTATE() { return winrt::guid{"{7B8A0F9C-C20A-4DD0-870A-04803534C386}"}; }
}  // namespace winrt::IIr77LOADER::m_implementation