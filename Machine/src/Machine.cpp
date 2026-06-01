#include "pch.h"
#include "Machine.xaml.h"
#if __has_include("Machine.g.cpp")
#include "Machine.g.cpp"
#endif

namespace winrt::Machine::m_implementation {
Machine::Machine() {
    InitializeComponent();

    InitializeIndex();

    return;
}

void Machine::Page_Loaded(WF::IInspectable const& sender, MUX::RoutedEventArgs const& e) { return; }

void Machine::MetadataPortalLoaded(WF::IInspectable const& sender, MUX::RoutedEventArgs const& e) { return; }

void Machine::MetadataPortalSelectionChanged(MUXC::NavigationView const& sender, MUXC::NavigationViewSelectionChangedEventArgs const& args) {
    IInspectable Obj = args.SelectedItem();

    if (!Obj) {
        throw IIr77BASE::Ir77_INVALID_ARGUMENT{};
    }

    auto NavViewItem = Obj.try_as<MUXC::NavigationViewItem>();

    if (!Obj) {
        throw IIr77BASE::Ir77_INVALID_ARGUMENT{};
    }

    hstring selection = winrt::unbox_value<hstring>(NavViewItem.Tag());

    hstring name = NavViewItem.Name();

    CIr77TBASIC::CIr77HASH Key{""};

    try {
        Key = CIr77TBASIC::CIr77HASH{selection.c_str(), name.c_str()};
    } catch (std::exception) {
        throw IIr77BASE::Ir77_INVALID_ARGUMENT{};
    }

    try {
        Pages->at(Key)();
    } catch (std::out_of_range) {
        throw IIr77BASE::Ir77_IMPROPERLY_CONFIGURED{};
    }

    return;
}

void Machine::MetadataPortalNavFailed(WF::IInspectable const& sender, MUXN::NavigationFailedEventArgs const& e) { return; }

void Machine::Cancel_Click(WF::IInspectable const& sender, MUX::RoutedEventArgs const& e) {}

void Machine::Apply_Click(WF::IInspectable const& sender, MUX::RoutedEventArgs const& e) {}

void Machine::Exit_Click(WF::IInspectable const& sender, MUX::RoutedEventArgs const& e) {
    if (Frame().CanGoBack()) Frame().GoBack();
}

void Machine::InitializeIndex() {
    Pages = new std::unordered_map<CIr77TBASIC::CIr77HASH, std::function<bool()>, CIr77TBASIC::CIr77HASHFN>{
        {{winrt::guid{0x2F385E17, 0x4F57, 0x4A6B, {0xA3, 0xD6, 0x9C, 0x63, 0xA9, 0x7D, 0x77, 0xE5}}, "ProfilePage"},
         std::function<bool()>{[this]() { return MachineFRAME().Navigate(winrt::xaml_typename<winrt::Machine::ProfilePage>()); }}},
        {{winrt::guid{0x859710B0, 0xBC7E, 0x47FB, {0x99, 0x07, 0x2D, 0xE9, 0xDF, 0x5E, 0x17, 0x98}}, "AccountPage"},
         std::function<bool()>{[this]() { return MachineFRAME().Navigate(winrt::xaml_typename<winrt::Machine::AccountPage>()); }}},
        {{winrt::guid{0x14EE711B, 0x158A, 0x4113, {0xAC, 0x93, 0xBE, 0xA9, 0xD9, 0x97, 0x00, 0xDF}}, "Level1AccountPage"},
         std::function<bool()>{[this]() { return MachineFRAME().Navigate(winrt::xaml_typename<winrt::Machine::Level1AccountPage>()); }}},
        {{winrt::guid{0xE0AF95F9, 0x8AA2, 0x41B2, {0x92, 0x38, 0xFA, 0x79, 0x84, 0xF2, 0x83, 0xA5}}, "Level2AccountPage"},
         std::function<bool()>{[this]() { return MachineFRAME().Navigate(winrt::xaml_typename<winrt::Machine::Level2AccountPage>()); }}},
        {{winrt::guid{0x06D3531C, 0xED9A, 0x4CA2, {0x9A, 0x13, 0x78, 0x08, 0x0A, 0x38, 0xBD, 0x02}}, "Level3AccountPage"},
         std::function<bool()>{[this]() { return MachineFRAME().Navigate(winrt::xaml_typename<winrt::Machine::Level3AccountPage>()); }}},
        {{winrt::guid{0xD382650E, 0x91C0, 0x4F7C, {0x84, 0x85, 0x1F, 0xF4, 0x6C, 0x9A, 0xF9, 0x41}}, "Level4AccountPage"},
         std::function<bool()>{[this]() { return MachineFRAME().Navigate(winrt::xaml_typename<winrt::Machine::Level4AccountPage>()); }}},
        {{winrt::guid{0x2724AAA2, 0x123B, 0x4185, {0xA9, 0xF1, 0x72, 0x05, 0xD7, 0x42, 0x26, 0x2C}}, "URIPage"},
         std::function<bool()>{[this]() { return true; }}},
        {{winrt::guid{0x217ED8EF, 0xD03A, 0x4737, {0xB2, 0x29, 0x94, 0xA0, 0x21, 0x96, 0xFD, 0xFC}}, "ArchiveURIPage"},
         std::function<bool()>{[this]() { return MachineFRAME().Navigate(winrt::xaml_typename<winrt::Machine::ArchiveURIPage>()); }}},
        {{winrt::guid{0x2904CE48, 0x78FA, 0x44E7, {0xA5, 0x1E, 0x5D, 0x0B, 0x59, 0x03, 0x3A, 0x79}}, "DatabaseURIPage"},
         std::function<bool()>{[this]() { return MachineFRAME().Navigate(winrt::xaml_typename<winrt::Machine::DatabaseURIPage>()); }}},
        {{winrt::guid{0x8F67DF86, 0x6F97, 0x4D75, {0x99, 0x1D, 0x9E, 0x3A, 0xED, 0x30, 0xF0, 0xCE}}, "DirectoryURIPage"},
         std::function<bool()>{[this]() { return MachineFRAME().Navigate(winrt::xaml_typename<winrt::Machine::DirectoryURIPage>()); }}},
        {{winrt::guid{0xC1567628, 0x618F, 0x4F52, {0x9E, 0x85, 0xD4, 0x98, 0x30, 0x83, 0xB1, 0x68}}, "NetworkURIPage"},
         std::function<bool()>{[this]() { return MachineFRAME().Navigate(winrt::xaml_typename<winrt::Machine::NetworkURIPage>()); }}},
        {{winrt::guid{0x02C3E566, 0x4174, 0x4D42, {0xBC, 0x66, 0xA0, 0x1C, 0x7D, 0xC8, 0xD5, 0x33}}, "RUNTIMEPage"},
         std::function<bool()>{[this]() { return MachineFRAME().Navigate(winrt::xaml_typename<winrt::Machine::RUNTIMEPage>()); }}},
        {{winrt::guid{0x9126C512, 0x05D3, 0x4CFE, {0xBF, 0xC0, 0x6E, 0xBC, 0x18, 0x97, 0xDE, 0x0F}}, "EnterprisePage"},
         std::function<bool()>{[this]() { return MachineFRAME().Navigate(winrt::xaml_typename<winrt::Machine::EnterprisePage>()); }}},
        {{winrt::guid{0xEB6EDAF6, 0x5FB5, 0x4F6F, {0x9E, 0xC7, 0x77, 0x02, 0xE1, 0xE9, 0xFF, 0xAD}}, "WindowPage"},
         std::function<bool()>{[this]() { return MachineFRAME().Navigate(winrt::xaml_typename<winrt::Machine::WindowPage>()); }}},
        {{winrt::guid{0x2612BFF3, 0xC034, 0x413E, {0xA0, 0x1E, 0x08, 0x48, 0xA4, 0x5F, 0x99, 0x17}}, "SoftDrivePage"},
         std::function<bool()>{[this]() { return MachineFRAME().Navigate(winrt::xaml_typename<winrt::Machine::SoftDrivePage>()); }}},
        {{winrt::guid{0xB7EB7843, 0x92A7, 0x4F79, {0xB6, 0x26, 0xCF, 0x20, 0x97, 0x8A, 0xB4, 0xA8}}, "SoftPartitionPage"},
         std::function<bool()>{[this]() { return MachineFRAME().Navigate(winrt::xaml_typename<winrt::Machine::SoftPartitionPage>()); }}},
        {{winrt::guid{0x31EABFEA, 0x8F60, 0x48FE, {0xA7, 0x47, 0x8C, 0x24, 0x36, 0x8E, 0x98, 0x97}}, "PromptPage"},
         std::function<bool()>{[this]() { return MachineFRAME().Navigate(winrt::xaml_typename<winrt::Machine::PromptPage>()); }}},
        {{winrt::guid{0x55AFB022, 0xE642, 0x4CC2, {0x82, 0x8E, 0xD3, 0x97, 0x9E, 0xC2, 0x72, 0xEC}}, "DebugPage"},
         std::function<bool()>{[this]() { return MachineFRAME().Navigate(winrt::xaml_typename<winrt::Machine::DebugPage>()); }}},
        {{winrt::guid{0x1B3D1454, 0x1FD9, 0x454A, {0x81, 0x2C, 0x7F, 0xC1, 0x6B, 0x2F, 0x9B, 0x51}}, "MetricPage"},
         std::function<bool()>{[this]() { return MachineFRAME().Navigate(winrt::xaml_typename<winrt::Machine::MetricPage>()); }}},
        {{winrt::guid{0xE1F8583C, 0x67B8, 0x4D9A, {0xBD, 0x15, 0x25, 0x6B, 0x3C, 0x0E, 0x06, 0x98}}, "DataBindingPage"},
         std::function<bool()>{[this]() { return MachineFRAME().Navigate(winrt::xaml_typename<winrt::Machine::DataBindingPage>()); }}},
        {{winrt::guid{0x5E8FCF0C, 0x62AD, 0x46FA, {0xBD, 0xFE, 0xA9, 0x4B, 0xFB, 0x18, 0xD5, 0xC9}}, "BinaryBindingPage"},
         std::function<bool()>{[this]() { return MachineFRAME().Navigate(winrt::xaml_typename<winrt::Machine::BinaryBindingPage>()); }}},
        {{winrt::guid{0x6515BE6A, 0x7349, 0x4DBC, {0x80, 0x3E, 0x8C, 0x19, 0x0B, 0x19, 0x87, 0x58}}, "JSONBindingPage"},
         std::function<bool()>{[this]() { return MachineFRAME().Navigate(winrt::xaml_typename<winrt::Machine::JSONBindingPage>()); }}},
        {{winrt::guid{0x3D0ED970, 0x7D01, 0x40D8, {0xA6, 0x79, 0x72, 0xD5, 0x6E, 0x41, 0x2E, 0xBB}}, "XMLBindingPage"},
         std::function<bool()>{[this]() { return MachineFRAME().Navigate(winrt::xaml_typename<winrt::Machine::XMLBindingPage>()); }}},
        {{winrt::guid{0xDC7DA50F, 0x8D48, 0x4EEC, {0x93, 0xF5, 0xF1, 0x2D, 0x4A, 0xEA, 0x28, 0x18}}, "XAMLBindingPage"},
         std::function<bool()>{[this]() { return MachineFRAME().Navigate(winrt::xaml_typename<winrt::Machine::XAMLBindingPage>()); }}},
        {{winrt::guid{0x2618883D, 0x37BA, 0x460B, {0xB3, 0xEF, 0x6D, 0x61, 0x77, 0x01, 0xCB, 0xA6}}, "QIFBindingPage"},
         std::function<bool()>{[this]() { return MachineFRAME().Navigate(winrt::xaml_typename<winrt::Machine::QIFBindingPage>()); }}},
        {{winrt::guid{0xEDC1BCCE, 0xD74E, 0x4C1C, {0x9D, 0xD7, 0x7C, 0x5E, 0xBD, 0xAF, 0x31, 0xB0}}, "PostgresBindingPage"},
         std::function<bool()>{[this]() { return MachineFRAME().Navigate(winrt::xaml_typename<winrt::Machine::PostgresBindingPage>()); }}},
        {{winrt::guid{0x2A0D7577, 0xA39A, 0x409F, {0xB4, 0xBB, 0x20, 0xE0, 0xD2, 0xC7, 0x1B, 0x96}}, "IOPage"},
         std::function<bool()>{[this]() { return true; }}},
        {{winrt::guid{0xAEB4D9F3, 0xE14E, 0x4F60, {0x93, 0x15, 0x93, 0x38, 0x6C, 0x60, 0x8B, 0x28}}, "AcceleratorPage"},
         std::function<bool()>{[this]() { return MachineFRAME().Navigate(winrt::xaml_typename<winrt::Machine::AcceleratorPage>()); }}},
        {{winrt::guid{0x1FB6C794, 0xC781, 0x436A, {0x8B, 0xD6, 0xC7, 0xE3, 0x04, 0x9C, 0x44, 0x91}}, "DevicePage"},
         std::function<bool()>{[this]() { return MachineFRAME().Navigate(winrt::xaml_typename<winrt::Machine::DevicePage>()); }}},
        {{winrt::guid{0xAFB7FB2D, 0x4C2D, 0x4A47, {0x85, 0x0A, 0x05, 0xCE, 0x3F, 0xB1, 0xCA, 0x1B}}, "StoragePage"},
         std::function<bool()>{[this]() { return MachineFRAME().Navigate(winrt::xaml_typename<winrt::Machine::StoragePage>()); }}},
        {{winrt::guid{0x19E262AB, 0xC56E, 0x47A7, {0xAA, 0xE9, 0xA9, 0x45, 0x9F, 0x47, 0x26, 0xBF}}, "MemoryPage"},
         std::function<bool()>{[this]() { return MachineFRAME().Navigate(winrt::xaml_typename<winrt::Machine::MemoryPage>()); }}},
        {{winrt::guid{0x36E17529, 0x39A7, 0x4AEC, {0x9E, 0xF9, 0x1C, 0xAF, 0x31, 0x88, 0xB5, 0x6A}}, "ProcessorPage"},
         std::function<bool()>{[this]() { return MachineFRAME().Navigate(winrt::xaml_typename<winrt::Machine::ProcessorPage>()); }}},
        {{winrt::guid{0x1106C674, 0xFBBE, 0x4342, {0x82, 0xBB, 0x18, 0xC0, 0xDD, 0x5C, 0xC2, 0x1D}}, "LibraryPage"},
         std::function<bool()>{[this]() { return MachineFRAME().Navigate(winrt::xaml_typename<winrt::Machine::LibraryPage>()); }}},
        {{winrt::guid{0x10F0039B, 0x38CC, 0x4E11, {0xAF, 0x68, 0xC6, 0xCD, 0x8B, 0x23, 0xF8, 0x96}}, "CObjectPage"},
         std::function<bool()>{[this]() { return MachineFRAME().Navigate(winrt::xaml_typename<winrt::Machine::CObjectPage>()); }}},
        {{winrt::guid{0x3EA478B5, 0xDA4C, 0x412E, {0x80, 0x93, 0xA6, 0x73, 0x30, 0x03, 0xBB, 0x7D}}, "LanePage"},
         std::function<bool()>{[this]() { return MachineFRAME().Navigate(winrt::xaml_typename<winrt::Machine::LanePage>()); }}},
        {{winrt::guid{0xB75B0EEF, 0x136B, 0x4BCE, {0x87, 0x31, 0x63, 0x88, 0x18, 0xE8, 0x46, 0x0D}}, "IteratorPage"},
         std::function<bool()>{[this]() { return MachineFRAME().Navigate(winrt::xaml_typename<winrt::Machine::IteratorPage>()); }}},
        {{winrt::guid{0x3DEFE389, 0x29BB, 0x4DE9, {0x9C, 0x51, 0x6D, 0x04, 0x8F, 0x12, 0x8F, 0x32}}, "OperatorPage"},
         std::function<bool()>{[this]() { return MachineFRAME().Navigate(winrt::xaml_typename<winrt::Machine::OperatorPage>()); }}},
        {{winrt::guid{0xF36421E9, 0x986D, 0x4BE8, {0x95, 0x13, 0x11, 0xBB, 0x24, 0x09, 0xD9, 0x4C}}, "ValuePage"},
         std::function<bool()>{[this]() { return MachineFRAME().Navigate(winrt::xaml_typename<winrt::Machine::ValuePage>()); }}},
        {{winrt::guid{0x30FAEF18, 0x2BF3, 0x4D67, {0x83, 0x7F, 0x45, 0xC9, 0xD2, 0x03, 0x6A, 0xC9}}, "ReturnPage"},
         std::function<bool()>{[this]() { return MachineFRAME().Navigate(winrt::xaml_typename<winrt::Machine::ReturnPage>()); }}},
        {{winrt::guid{0xF8A96E30, 0x710E, 0x4C6E, {0xA1, 0x58, 0xB8, 0x39, 0x0C, 0xDF, 0x61, 0xBF}}, "UXPage"},
         std::function<bool()>{[this]() { return MachineFRAME().Navigate(winrt::xaml_typename<winrt::Machine::UXPage>()); }}},
        {{winrt::guid{0x0DB7C610, 0x447E, 0x423F, {0x85, 0xC3, 0x44, 0xE8, 0x1D, 0x7F, 0x16, 0xB3}}, "XamlResourcePage"},
         std::function<bool()>{[this]() { return MachineFRAME().Navigate(winrt::xaml_typename<winrt::Machine::XamlResourcePage>()); }}},
        {{winrt::guid{0xECFE799D, 0x62A4, 0x498A, {0xA3, 0x98, 0x20, 0xE6, 0xE4, 0xAD, 0x2A, 0x9C}}, "XamlControlPage"},
         std::function<bool()>{[this]() { return MachineFRAME().Navigate(winrt::xaml_typename<winrt::Machine::XamlControlPage>()); }}},
        {{winrt::guid{0x853BC787, 0xD7FD, 0x4618, {0xA1, 0x8E, 0xDD, 0x5A, 0x53, 0xE4, 0xAF, 0x44}}, "XamlPgPage"},
         std::function<bool()>{[this]() { return MachineFRAME().Navigate(winrt::xaml_typename<winrt::Machine::XamlPgPage>()); }}},
        {{winrt::guid{0x71E2E256, 0x72DF, 0x4403, {0x9D, 0xDA, 0x09, 0xBB, 0xC1, 0x1A, 0xBD, 0x7D}}, "XamlToolBarPage"},
         std::function<bool()>{[this]() { return MachineFRAME().Navigate(winrt::xaml_typename<winrt::Machine::XamlToolBarPage>()); }}},
        {{winrt::guid{0x69840795, 0x22B9, 0x4E5D, {0xA3, 0x36, 0x8A, 0x65, 0x2E, 0x83, 0x68, 0x3E}}, "XamlWidgetPage"},
         std::function<bool()>{[this]() { return MachineFRAME().Navigate(winrt::xaml_typename<winrt::Machine::XamlWidgetPage>()); }}},
        {{winrt::guid{0x8304CF6F, 0x5ACD, 0x4B68, {0xA5, 0x6B, 0x5F, 0xF1, 0x89, 0x28, 0x73, 0x74}}, "XamlContextMenuPage"},
         std::function<bool()>{[this]() { return MachineFRAME().Navigate(winrt::xaml_typename<winrt::Machine::XamlContextMenuPage>()); }}},
        {{winrt::guid{0x727D1D5A, 0x6AE2, 0x422C, {0x99, 0xE6, 0x47, 0xCB, 0xB8, 0xC1, 0x0E, 0x2B}}, "PipelinePage"},
         std::function<bool()>{[this]() { return true; }}},
        {{winrt::guid{0x0BF841B8, 0x024E, 0x4A7F, {0x84, 0xC5, 0x54, 0x4C, 0x98, 0x9E, 0xA3, 0x00}}, "BitmapPage"},
         std::function<bool()>{[this]() { return MachineFRAME().Navigate(winrt::xaml_typename<winrt::Machine::BitmapPage>()); }}},
        {{winrt::guid{0x33AA5F70, 0x7FD1, 0x427E, {0xBB, 0xE4, 0xCA, 0x40, 0x0E, 0xEF, 0x3C, 0x92}}, "TopologyPage"},
         std::function<bool()>{[this]() { return MachineFRAME().Navigate(winrt::xaml_typename<winrt::Machine::TopologyPage>()); }}},
        {{winrt::guid{0x2EE34942, 0xEAA8, 0x4E19, {0x85, 0xAA, 0x21, 0x3D, 0xF5, 0xE0, 0xB0, 0xB2}}, "PreProcessPage"},
         std::function<bool()>{[this]() { return MachineFRAME().Navigate(winrt::xaml_typename<winrt::Machine::PreProcessPage>()); }}},
        {{winrt::guid{0x14BF219E, 0x9399, 0x4EFE, {0xA8, 0x9D, 0xEC, 0xEF, 0xB3, 0xA4, 0x9F, 0xE9}}, "PostProcessPage"},
         std::function<bool()>{[this]() { return MachineFRAME().Navigate(winrt::xaml_typename<winrt::Machine::PostProcessPage>()); }}},
        {{winrt::guid{0x64D2469E, 0xD50C, 0x465F, {0x99, 0xD2, 0x5D, 0x40, 0x9E, 0xFC, 0x1D, 0xBC}}, "ComputeKernelPage"},
         std::function<bool()>{[this]() { return MachineFRAME().Navigate(winrt::xaml_typename<winrt::Machine::ComputeKernelPage>()); }}},
        {{winrt::guid{0xB12D2C16, 0xF078, 0x4E6C, {0x96, 0xBD, 0x7D, 0x4C, 0x46, 0xAE, 0x14, 0x9E}}, "ProcessPage"},
         std::function<bool()>{[this]() { return true; }}},
        {{winrt::guid{0x82A08776, 0xC815, 0x4367, {0x89, 0x96, 0x17, 0xA8, 0xAA, 0x1C, 0xC5, 0x3A}}, "ApplicationPage"},
         std::function<bool()>{[this]() { return MachineFRAME().Navigate(winrt::xaml_typename<winrt::Machine::ApplicationPage>()); }}},
        {{winrt::guid{0x202AF229, 0x5997, 0x499D, {0x96, 0x3E, 0x6A, 0x02, 0x1C, 0x8E, 0xAA, 0xD1}}, "ShellPage"},
         std::function<bool()>{[this]() { return MachineFRAME().Navigate(winrt::xaml_typename<winrt::Machine::ShellPage>()); }}},
        {{winrt::guid{0x41BB9503, 0x037B, 0x404B, {0x86, 0xBF, 0xF5, 0x21, 0x77, 0x33, 0x43, 0xAA}}, "AsyncPage"},
         std::function<bool()>{[this]() { return MachineFRAME().Navigate(winrt::xaml_typename<winrt::Machine::AsyncPage>()); }}},
        {{winrt::guid{0xDF0AF07C, 0xF15B, 0x4574, {0xAF, 0x66, 0x8B, 0x0D, 0xD6, 0xC8, 0x89, 0x1A}}, "RPCPage"},
         std::function<bool()>{[this]() { return MachineFRAME().Navigate(winrt::xaml_typename<winrt::Machine::RPCPage>()); }}},
        {{winrt::guid{0xB34BD3DB, 0x6250, 0x487A, {0xA1, 0x1B, 0x16, 0x0F, 0xA5, 0xB0, 0x9F, 0xA3}}, "PipePage"},
         std::function<bool()>{[this]() { return MachineFRAME().Navigate(winrt::xaml_typename<winrt::Machine::PipePage>()); }}},
        {{winrt::guid{0xAD99D754, 0x4A6C, 0x422D, {0xB4, 0xFC, 0x71, 0x64, 0x91, 0x6C, 0xE3, 0xED}}, "SharedMemPage"},
         std::function<bool()>{[this]() { return MachineFRAME().Navigate(winrt::xaml_typename<winrt::Machine::SharedMemoryPage>()); }}},
        {{winrt::guid{0x34D7C2F1, 0xF74E, 0x47A7, {0xA9, 0xD4, 0x3E, 0x81, 0xF1, 0x84, 0xBD, 0xDE}}, "ProjectIndexPage"},
         std::function<bool()>{[this]() { return MachineFRAME().Navigate(winrt::xaml_typename<winrt::Machine::ProjectIndexPage>()); }}}};

    return;
}

}  // namespace winrt::Machine::m_implementation