#pragma once

#include "winrt/IIr77BASE.h"
#include "winrt/IIr77HEALTH.h"
#include "winrt/IIr77APP.h"

#include "Ir77LANDING.g.h"

namespace WF = winrt::Windows::Foundation;
namespace WFC = winrt::Windows::Foundation::Collections;
namespace MUX = winrt::Microsoft::UI::Xaml;
namespace MUXC = winrt::Microsoft::UI::Xaml::Controls;

namespace winrt::Landing::m_implementation {
struct Ir77LANDING : Ir77LANDINGT<Ir77LANDING> {
   public:
    Ir77LANDING();

   public:
    IIr77BASE::IIr77RETURN EnlistedAs(winrt::guid& uid);

    IIr77BASE::IIr77RETURN EnlistedUuid(winrt::guid& uid);

    IIr77BASE::IIr77RETURN EnlistedChrono(uint64_t& moment);

    IIr77BASE::IIr77RETURN DelistedChrono(uint64_t& moment);

    IIr77BASE::IIr77RETURN MemberOfUuid(winrt::guid& uid);

    IIr77BASE::IIr77RETURN CollectionUuid(winrt::guid& uid);

   public:
    winrt::guid ID();

    winrt::guid GID();

    IIr77BASE::IIr77RETURN Dispatch(winrt::guid const& uid, IIr77BASE::IIr77STACK const& stack);

    IIr77BASE::IIr77RETURN SwapWorkload(winrt::guid const& uid);

   public:
    IIr77BASE::IIr77RETURN Load(IIr77BASE::IIr77Opcode const& cmd);

    IIr77BASE::IIr77RETURN Unload(IIr77BASE::IIr77Opcode const& cmd);

    IIr77BASE::IIr77RETURN Park(IIr77BASE::IIr77Opcode const& cmd);

    IIr77BASE::IIr77RETURN SetState(IIr77BASE::IIr77Opcode const& cmd);

    IIr77BASE::IIr77RETURN GetState(IIr77BASE::IIr77Opcode const& cmd);

   private:
    winrt::guid Instance{};

    std::chrono::system_clock::time_point Enlisted{};

    std::chrono::system_clock::time_point Delisted{};

    IIr77BASE::IIr77PATCH Patch{IIr77BASE::Ir77PATCH{}};

    IIr77BASE::IIr77SERVICE Health{nullptr};

    IIr77BASE::Ir77NAVIGATE Navigate{nullptr};

    MUXC::UserControl UXPage{nullptr};

    IIr77BASE::IIr77SERVICE Project{nullptr};
};
}  // namespace winrt::Landing::m_implementation

namespace winrt::Landing::factory_implementation {
struct Ir77LANDING : Ir77LANDINGT<Ir77LANDING, m_implementation::Ir77LANDING> {};
}  // namespace winrt::Landing::factory_implementation
