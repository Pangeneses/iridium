#pragma once

#include <functional>
#include <bitset>

#include "winrt/IIr77BASE.h"
#include "winrt/Ir77TBASIC.h"
#include "winrt/Ir77X.h"
#include "winrt/Machine.h"

#include "Machine.g.h"

namespace WF = winrt::Windows::Foundation;
namespace WFITT = winrt::Windows::Foundation::Collections;
namespace MUX = winrt::Microsoft::UI::Xaml;
namespace MUXC = winrt::Microsoft::UI::Xaml::Controls;
namespace MUXN = winrt::Microsoft::UI::Xaml::Navigation;

import CIr77TBASIC;

namespace winrt::Machine::m_implementation {
struct Machine : MachineT<Machine> {
   public:
    Machine();

   public:
    void Page_Loaded(WF::IInspectable const&, MUX::RoutedEventArgs const&);

   private:
    void InitializeIndex();

    std::unordered_map<CIr77TBASIC::CIr77HASH, std::function<bool()>, CIr77TBASIC::CIr77HASHFN>* Pages;

   public:
    void MetadataPortalLoaded(WF::IInspectable const&, MUX::RoutedEventArgs const&);

   public:
    void MetadataPortalSelectionChanged(MUXC::NavigationView const&, MUXC::NavigationViewSelectionChangedEventArgs const&);

   public:
    void MetadataPortalNavFailed(WF::IInspectable const&, MUXN::NavigationFailedEventArgs const&);

   public:
    void Cancel_Click(WF::IInspectable const&, MUX::RoutedEventArgs const&);
    void Apply_Click(WF::IInspectable const&, MUX::RoutedEventArgs const&);
    void Exit_Click(WF::IInspectable const&, MUX::RoutedEventArgs const&);

   private:
    int initialized{0};
};
}  // namespace winrt::Machine::m_implementation

namespace winrt::Machine::factory_implementation {
struct Machine : MachineT<Machine, m_implementation::Machine> {};
}  // namespace winrt::Machine::factory_implementation
