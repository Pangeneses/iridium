#include "OIr77DELGEOM.h"

#include "pch.h"

#if __has_include("OIr77DELGEOM.g.cpp")
#include "OIr77DELGEOM.g.cpp"
#endif

namespace winrt::IIr77PROJECT::m_implementation {
OIr77DELGEOM::OIr77DELGEOM() {
    GUID uid;

    HRESULT hr = CoCreateGuid(&uid);

    if (SUCCEEDED(hr)) {
        Instance = uid;
    } else {
        throw std::runtime_error{"Failed to generate Uuid."};
    }

    Enlisted = std::chrono::system_clock::now();
}

IIr77BASE::IIr77RETURN OIr77DELGEOM::EnlistedAs(winrt::guid& uid) {
    uid = IIr77BASE::IDIIr77ENLISTED::HVIDIIr77Opcode();

    if (!IsValid)
        return IIr77BASE::Ir77_INVALIDATED{*this, "Enlisted has been invalidated."};

    else
        return IIr77BASE::Ir77_OPERATION_SUCCEEDED{};
}

IIr77BASE::IIr77RETURN OIr77DELGEOM::EnlistedUuid(winrt::guid& uid) {
    uid = Instance;

    if (!IsValid)
        return IIr77BASE::Ir77_INVALIDATED{*this, "Enlisted has been invalidated."};

    else
        return IIr77BASE::Ir77_OPERATION_SUCCEEDED{};
}

IIr77BASE::IIr77RETURN OIr77DELGEOM::EnlistedChrono(uint64_t& moment) {
    moment = (uint64_t)std::chrono::system_clock::to_time_t(Enlisted);

    if (!IsValid)
        return IIr77BASE::Ir77_INVALIDATED{*this, "Enlisted has been invalidated."};

    else
        return IIr77BASE::Ir77_OPERATION_SUCCEEDED{};
}

IIr77BASE::IIr77RETURN OIr77DELGEOM::DelistedChrono(uint64_t& moment) {
    moment = (uint64_t)std::chrono::system_clock::to_time_t(Delisted);

    if (!IsValid)
        return IIr77BASE::Ir77_INVALIDATED{*this, "Enlisted has been invalidated."};

    else
        return IIr77BASE::Ir77_OPERATION_SUCCEEDED{};
}

IIr77BASE::IIr77RETURN OIr77DELGEOM::MemberUuid(winrt::guid& uid) {
    uid = IIr77PROJECT::IDOIr77CAD::HVIDOIr77DELGEOM();

    if (!IsValid)
        return IIr77BASE::Ir77_INVALIDATED{*this, "Enlisted has been invalidated."};

    else
        return IIr77BASE::Ir77_OPERATION_SUCCEEDED{};
}

IIr77BASE::IIr77RETURN OIr77DELGEOM::CollectionUuid(winrt::guid& uid) {
    uid = IIr77PROJECT::IDOIr77CAD::HVIDOIr77CAD();

    if (!IsValid)
        return IIr77BASE::Ir77_INVALIDATED{*this, "Enlisted has been invalidated."};

    else
        return IIr77BASE::Ir77_OPERATION_SUCCEEDED{};
}

winrt::guid OIr77DELGEOM::ID() { return IIr77PROJECT::IDOIr77CAD::HVIDOIr77DELGEOM(); }

winrt::guid OIr77DELGEOM::GID() { return IIr77PROJECT::IDOIr77CAD::HVIDOIr77CAD(); }

IIr77BASE::IIr77RETURN OIr77DELGEOM::SetHeap(IIr77BASE::IIr77BALE const& input) {
    if (!IsValid) return IIr77BASE::Ir77_INVALIDATED{*this, "Invalidated."};

    if (input == nullptr) return IIr77BASE::Ir77_INVALID_ARGUMENT{*this, "InvalidOperand; nullptr."};

    std::optional<IIr77BASE::Ir77GUID> geometry = input.try_as<IIr77BASE::Ir77GUID>();

    if (!geometry.has_value()) return IIr77BASE::Ir77_INVALID_ARGUMENT{*this, "InvalidOperand."};

    Heap = input;

    return IIr77BASE::Ir77_OPERATION_SUCCEEDED{};
}

IIr77BASE::IIr77RETURN OIr77DELGEOM::GetHeap(IIr77BASE::IIr77BALE& input) {
    if (!IsValid) return IIr77BASE::Ir77_INVALIDATED{*this, "Invalidated."};

    input = Operand;

    return IIr77BASE::Ir77_OPERATION_SUCCEEDED{};
}

IIr77BASE::IIr77RETURN OIr77DELGEOM::Result(IIr77BASE::IIr77BALE const& outcome, IIr77BASE::IIr77RETURN const& ret) {
    if (!IsValid) return IIr77BASE::Ir77_INVALIDATED{*this, "Invalidated."};

    if (Complete) return IIr77BASE::Ir77_PROCESS_COMPLETE{};

    if (ret == nullptr) return IIr77BASE::Ir77_INVALID_ARGUMENT{*this, "Invalid return."};

    winrt::guid uid;

    if (outcome.MemberUuid(uid).ID() == IIr77BASE::Ir77RETURN::HVID_Ir77_INVALIDATED()) {
        return IIr77BASE::Ir77_INVALIDATED{*this, "Outcome is invalid."};
    }

    if (uid != IIr77BASE::IDIr77MIDL::HVIDIr77RETVAR()) throw std::runtime_error{"InvalidOperand Type."};

    Complete = true;

    Outcome = outcome;

    Return = ret;

    return IIr77BASE::Ir77_OPERATION_SUCCEEDED{};
}

IIr77BASE::IIr77RETURN OIr77DELGEOM::Result(IIr77BASE::IIr77BALE& outcome) {
    if (!IsValid) return IIr77BASE::Ir77_INVALIDATED{*this, "Invalidated."};

    if (!Complete) return IIr77BASE::Ir77_PROCESS_NOT_COMPLETE{};

    outcome = Outcome;

    return Return;
}

IIr77BASE::IIr77RETURN OIr77DELGEOM::InvalidateOperation(IIr77BASE::IIr77RETURN const& conditions) {
    if (!IsValid) return IIr77BASE::Ir77_INVALIDATED{*this, "Invalidated."};

    IsValid = false;

    InvalidationCondition = conditions;

    Delisted = std::chrono::system_clock::now();

    return IIr77BASE::Ir77_OPERATION_SUCCEEDED{};
}

IIr77BASE::IIr77RETURN OIr77DELGEOM::IsInvalid() {
    if (!IsValid)
        return IIr77BASE::Ir77_INVALIDATED{*this, "Invalidated."};

    else
        return IIr77BASE::Ir77_TRUE{};
}

IIr77BASE::IIr77RETURN OIr77DELGEOM::IsInvalid(IIr77BASE::IIr77RETURN& conditions) {
    if (!IsValid)
        return IIr77BASE::Ir77_INVALIDATED{*this, "Invalidated."};

    else
        conditions = InvalidationCondition;
    return IIr77BASE::Ir77_TRUE{};
}
}  // namespace winrt::IIr77PROJECT::m_implementation