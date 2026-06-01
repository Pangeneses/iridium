#include "pch.h"
#include "IDDIr77CAD.h"
#if __has_include("IDDIr77CAD.g.cpp")
#include "IDDIr77CAD.g.cpp"
#endif

namespace winrt::IIr77PROJECT::m_implementation {
winrt::guid IDDIr77CAD::HVIDDIr77CAD() { return winrt::guid{"{0AA91E68-E1CE-4A7B-B938-B404D00753A7}"}; }
winrt::guid IDDIr77CAD::HVIDDIr77GEOMETRY() { return winrt::guid{"{D9E472F6-A8B5-4BC7-B708-D488A7CEEA2A}"}; }
winrt::guid IDDIr77CAD::HVIDDIr77OPERATION() { return winrt::guid{"{EF5AB5B0-54DD-47BD-BDCE-F73D39874F55}"}; }
}  // namespace winrt::IIr77PROJECT::m_implementation