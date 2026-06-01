#include "pch.h"
#include "IDDIr77APP.h"
#if __has_include("IDDIr77APP.g.cpp")
#include "IDDIr77APP.g.cpp"
#endif

namespace winrt::IIr77APP::m_implementation {
winrt::guid IDDIr77APP::HVIDDIr77APP() { return winrt::guid{"{5297DECB-688F-4D2A-B2CA-30C5B919602C}"}; }
winrt::guid IDDIr77APP::HVIDDIr77STATE() { return winrt::guid{"{B99FA1A4-2BB0-418B-8835-F6C12857422E}"}; }
}  // namespace winrt::IIr77APP::m_implementation