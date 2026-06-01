#pragma once

#include "winrt/Ir77X.h"

#include "ProfilePage.g.h"

namespace WF = winrt::Windows::Foundation;
namespace WFITT = winrt::Windows::Foundation::Collections;
namespace MUX = winrt::Microsoft::UI::Xaml;
namespace MUXC = winrt::Microsoft::UI::Xaml::Controls;
namespace MUXN = winrt::Microsoft::UI::Xaml::Navigation;

namespace winrt::Machine::m_implementation {
struct ProfilePage : ProfilePageT<ProfilePage> {
   public:
    ProfilePage();

   public:
    void Loaded(WF::IInspectable const&, MUX::RoutedEventArgs const&);
};
}  // namespace winrt::Machine::m_implementation

namespace winrt::Machine::factory_implementation {
struct ProfilePage : ProfilePageT<ProfilePage, m_implementation::ProfilePage> {};

}  // namespace winrt::Machine::factory_implementation
