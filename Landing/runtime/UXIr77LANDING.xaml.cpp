#include "pch.h"
#include "UXIr77LANDING.xaml.h"
#if __has_include("UXIr77LANDING.g.cpp")
#include "UXIr77LANDING.g.cpp"
#endif

namespace winrt::Landing::m_implementation {
UXIr77LANDING::UXIr77LANDING() {}

void UXIr77LANDING::Loading(MUX::FrameworkElement const& sender, WF::IInspectable const& args) { return; }

void UXIr77LANDING::Loaded(WF::IInspectable const& sender, MUX::RoutedEventArgs const& e) { return; }

void UXIr77LANDING::Navigate001(WF::IInspectable const& sender, MUX::RoutedEventArgs const& e) { return; }

void UXIr77LANDING::Navigate002(WF::IInspectable const& sender, MUX::RoutedEventArgs const& e) { return; }

void UXIr77LANDING::Navigate003(WF::IInspectable const& sender, MUX::RoutedEventArgs const& e) {
    Navigate(IIr77APP::IDLIr77APP::HVIDLIr77CAD());

    return;
}

void UXIr77LANDING::Navigate004(WF::IInspectable const& sender, MUX::RoutedEventArgs const& e) { return; }

void UXIr77LANDING::Navigate005(WF::IInspectable const& sender, MUX::RoutedEventArgs const& e) { return; }

void UXIr77LANDING::Navigate006(WF::IInspectable const& sender, MUX::RoutedEventArgs const& e) {
    MUX::Application().Exit();

    return;
}

void UXIr77LANDING::Ir77ThrowFailed() {
    WUPOP::MessageDialog Msg{"Error."};

    WUPOP::UICommand Failed{"Fail", [](WUPOP::IUICommand const& command) { return; }};

    Msg.Commands().Append(Failed);

    Msg.DefaultCommandIndex(0);

    auto bound{Msg.as<::IInitializeWithWindow>()};

    HWND hWnd = GetActiveWindow();

    bound->Initialize(hWnd);

    Msg.ShowAsync();

    return;
}

void UXIr77LANDING::Ir77ThrowInvalidProject() {
    WUPOP::MessageDialog Msg{"Error."};

    WUPOP::UICommand debProject{"Debug Project", [](WUPOP::IUICommand const& command) { return; }};

    Msg.Commands().Append(debProject);

    Msg.DefaultCommandIndex(0);

    auto bound{Msg.as<::IInitializeWithWindow>()};

    HWND hWnd = GetActiveWindow();

    bound->Initialize(hWnd);

    Msg.ShowAsync();

    return;
}

void UXIr77LANDING::Ir77ThrowCouldNotNavigate() {
    WUPOP::MessageDialog Msg{"Error."};

    WUPOP::UICommand CoreUI{"Pass", [](WUPOP::IUICommand const& command) { return; }};

    Msg.Commands().Append(CoreUI);

    Msg.DefaultCommandIndex(0);

    auto bound{Msg.as<::IInitializeWithWindow>()};

    HWND hWnd = GetActiveWindow();

    bound->Initialize(hWnd);

    Msg.ShowAsync();

    return;
}
}  // namespace winrt::Landing::m_implementation
