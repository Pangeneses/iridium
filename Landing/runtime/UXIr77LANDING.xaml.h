#pragma once

#include <shobjidl.h>

#include <functional>

#include "winrt/Windows.UI.Popups.h"

#include "winrt/Ir77X.h"

#include "winrt/IIr77BASE.h"
#include "winrt/IIr77APP.h"

#include "UXIr77LANDING.g.h"

namespace WF = winrt::Windows::Foundation;
namespace WFC = winrt::Windows::Foundation::Collections;
namespace WUPOP = winrt::Windows::UI::Popups;
namespace MUX = winrt::Microsoft::UI::Xaml;

namespace Ir77X = winrt::Ir77X;

namespace winrt::Landing::m_implementation {
struct UXIr77LANDING : UXIr77LANDINGT<UXIr77LANDING> {
   public:
    UXIr77LANDING();

   public:
    void SetNavigate(IIr77BASE::Ir77NAVIGATE const& nav) { Navigate = nav; }

   private:
    IIr77BASE::Ir77NAVIGATE Navigate{nullptr};

   public:
    void Loading(MUX::FrameworkElement const&, WF::IInspectable const&);

    void Loaded(WF::IInspectable const&, MUX::RoutedEventArgs const&);

   private:
   public:
    void Navigate001(WF::IInspectable const&, MUX::RoutedEventArgs const&);

    void Navigate002(WF::IInspectable const&, MUX::RoutedEventArgs const&);

    void Navigate003(WF::IInspectable const&, MUX::RoutedEventArgs const&);

    void Navigate004(WF::IInspectable const&, MUX::RoutedEventArgs const&);

    void Navigate005(WF::IInspectable const&, MUX::RoutedEventArgs const&);

    void Navigate006(WF::IInspectable const&, MUX::RoutedEventArgs const&);

   private:
   private:
    void Ir77ThrowFailed();

    void Ir77ThrowInvalidProject();

    void Ir77ThrowCouldNotNavigate();
};
}  // namespace winrt::Landing::m_implementation

namespace winrt::Landing::factory_implementation {
struct UXIr77LANDING : UXIr77LANDINGT<UXIr77LANDING, m_implementation::UXIr77LANDING> {};
}  // namespace winrt::Landing::factory_implementation
