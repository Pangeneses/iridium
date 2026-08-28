#pragma once

#include "DIr77PIPELINE.g.h"
#include "winrt/IIr77BASE.h"

namespace WF = winrt::Windows::Foundation;
namespace MUXC = winrt::Microsoft::UI::Xaml::Controls;

namespace winrt::IIr77PROJECT::m_implementation {
struct DIr77PIPELINE : DIr77PIPELINET<DIr77PIPELINE> {
   public:
    DIr77PIPELINE();

   public:
    IIr77BASE::IIr77RETURN EnlistedAs(winrt::guid& uid);

    IIr77BASE::IIr77RETURN EnlistedUuid(winrt::guid& uid);

    IIr77BASE::IIr77RETURN EnlistedChrono(uint64_t& moment);

    IIr77BASE::IIr77RETURN DelistedChrono(uint64_t& moment);

    IIr77BASE::IIr77RETURN MemberUuid(winrt::guid& uid);

    IIr77BASE::IIr77RETURN CollectionUuid(winrt::guid& uid);

   public:
    IIr77BASE::IIr77RETURN Resize(size_t const& size);

    IIr77BASE::IIr77RETURN Size(uint32_t& size);

    IIr77BASE::IIr77RETURN IsEmpty();

    IIr77BASE::IIr77RETURN Set(size_t const& at, WF::IInspectable const& value);

    IIr77BASE::IIr77RETURN SealOperand();

    IIr77BASE::IIr77RETURN IsSealed();

    IIr77BASE::IIr77RETURN At(uint32_t const& at, WF::IInspectable& value);

    IIr77BASE::IIr77RETURN InvalidateHeap(IIr77BASE::IIr77RETURN const& conditions);

    IIr77BASE::IIr77RETURN IsInvalid();

    IIr77BASE::IIr77RETURN IsInvalid(IIr77BASE::IIr77RETURN& conditions);

   private:
    winrt::guid Instance{};

    std::chrono::system_clock::time_point Enlisted{};

    std::chrono::system_clock::time_point Delisted{};

    bool Sealed{false};

    bool IsValid{true};

    IIr77BASE::IIr77RETURN InvalidationCondition{nullptr};

    WF::IInspectableOperand[1];
};
}  // namespace winrt::IIr77PROJECT::m_implementation

namespace winrt::IIr77PROJECT::factory_implementation {
struct DIr77PIPELINE : DIr77PIPELINET<DIr77PIPELINE, m_implementation::DIr77PIPELINE> {};
}  // namespace winrt::IIr77PROJECT::factory_implementation
