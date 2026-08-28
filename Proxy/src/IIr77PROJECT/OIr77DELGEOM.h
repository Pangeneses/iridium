#pragma once

#include <memory>

#include "OIr77DELGEOM.g.h"
#include "winrt/IIr77BASE.h"
#include "winrt/IIr77PROJECT.h"

namespace WF = winrt::Windows::Foundation;

namespace winrt::IIr77PROJECT::m_implementation {
struct OIr77DELGEOM : OIr77DELGEOMT<OIr77DELGEOM> {
   public:
    OIr77DELGEOM();

   public:
    IIr77BASE::IIr77RETURN EnlistedAs(winrt::guid& uid);

    IIr77BASE::IIr77RETURN EnlistedUuid(winrt::guid& uid);

    IIr77BASE::IIr77RETURN EnlistedChrono(uint64_t& time);

    IIr77BASE::IIr77RETURN DelistedChrono(uint64_t& time);

    IIr77BASE::IIr77RETURN MemberUuid(winrt::guid& uid);

    IIr77BASE::IIr77RETURN CollectionUuid(winrt::guid& uid);

   public:
    winrt::guid ID();

    winrt::guid GID();

    IIr77BASE::IIr77RETURN SetHeap(IIr77BASE::IIr77BALE const& input);

    IIr77BASE::IIr77RETURN GetHeap(IIr77BASE::IIr77BALE& input);

    IIr77BASE::IIr77RETURN Result(IIr77BASE::IIr77BALE const& outcome, IIr77BASE::IIr77RETURN const& ret);

    IIr77BASE::IIr77RETURN Result(IIr77BASE::IIr77BALE& outcome);

    IIr77BASE::IIr77RETURN InvalidateOperation(IIr77BASE::IIr77RETURN const& conditions);

    IIr77BASE::IIr77RETURN IsInvalid();

    IIr77BASE::IIr77RETURN IsInvalid(IIr77BASE::IIr77RETURN& conditions);

   private:
    winrt::guid Instance{};

    std::chrono::system_clock::time_point Enlisted{};

    std::chrono::system_clock::time_point Delisted{};

    bool IsValid{true};

    IIr77BASE::IIr77RETURN InvalidationCondition{nullptr};

    IIr77BASE::IIr77BALEOperand{nullptr};

    bool Complete{false};

    IIr77BASE::IIr77BALE Outcome{nullptr};

    IIr77BASE::IIr77RETURN Return = IIr77BASE::Ir77_UNKNOWN{};
};
}  // namespace winrt::IIr77PROJECT::m_implementation

namespace winrt::IIr77PROJECT::factory_implementation {
struct OIr77DELGEOM : OIr77DELGEOMT<OIr77DELGEOM, m_implementation::OIr77DELGEOM> {};
}  // namespace winrt::IIr77PROJECT::factory_implementation
