#include "OIr77ADD.h"

#include "pch.h"

#if __has_include("OIr77ADD.g.cpp")
#include "OIr77ADD.g.cpp"
#endif

namespace winrt::IIr77FILESYS::m_implementation {
OIr77ADD::OIr77ADD() {
    GUID uid;

    HRESULT hr = CoCreateGuid(&uid);

    if (SUCCEEDED(hr)) {
        Instance = uid;
    } else {
        throw std::runtime_error{"Failed to generate Uuid."};
    }

    Enlisted = std::chrono::system_clock::now();
}

IIr77BASE::IIr77RETURN OIr77ADD::EnlistedAs(winrt::guid& uid) {
    uid = IIr77BASE::IDIIr77ENLISTED::HVIDIIr77Opcode();

    if (!IsValid)
        return IIr77BASE::Ir77_INVALIDATED{*this, "Enlisted has been invalidated."};

    else
        return IIr77BASE::Ir77_OPERATION_SUCCEEDED{};
}

IIr77BASE::IIr77RETURN OIr77ADD::EnlistedUuid(winrt::guid& uid) {
    uid = Instance;

    if (!IsValid)
        return IIr77BASE::Ir77_INVALIDATED{*this, "Enlisted has been invalidated."};

    else
        return IIr77BASE::Ir77_OPERATION_SUCCEEDED{};
}

IIr77BASE::IIr77RETURN OIr77ADD::EnlistedChrono(uint64_t& moment) {
    moment = (uint64_t)std::chrono::system_clock::to_time_t(Enlisted);

    if (!IsValid)
        return IIr77BASE::Ir77_INVALIDATED{*this, "Enlisted has been invalidated."};

    else
        return IIr77BASE::Ir77_OPERATION_SUCCEEDED{};
}

IIr77BASE::IIr77RETURN OIr77ADD::DelistedChrono(uint64_t& moment) {
    moment = (uint64_t)std::chrono::system_clock::to_time_t(Delisted);

    if (!IsValid)
        return IIr77BASE::Ir77_INVALIDATED{*this, "Enlisted has been invalidated."};

    else
        return IIr77BASE::Ir77_OPERATION_SUCCEEDED{};
}

IIr77BASE::IIr77RETURN OIr77ADD::MemberUuid(winrt::guid& uid) {
    uid = IIr77FILESYS::IDOIr77FOLDER::HVIDOIr77ADD();

    if (!IsValid)
        return IIr77BASE::Ir77_INVALIDATED{*this, "Enlisted has been invalidated."};

    else
        return IIr77BASE::Ir77_OPERATION_SUCCEEDED{};
}

IIr77BASE::IIr77RETURN OIr77ADD::CollectionUuid(winrt::guid& uid) {
    uid = IIr77FILESYS::IDOIr77FOLDER::HVIDOIr77FOLDER();

    if (!IsValid)
        return IIr77BASE::Ir77_INVALIDATED{*this, "Enlisted has been invalidated."};

    else
        return IIr77BASE::Ir77_OPERATION_SUCCEEDED{};
}

winrt::guid OIr77ADD::ID() { return IIr77FILESYS::IDOIr77FOLDER::HVIDOIr77ADD(); }

winrt::guid OIr77ADD::GID() { return IIr77FILESYS::IDOIr77FOLDER::HVIDOIr77FOLDER(); }

IIr77BASE::IIr77RETURN OIr77ADD::SetHeap(IIr77BASE::IIr77BALE const& input) {
    if (!IsValid) return IIr77BASE::Ir77_INVALIDATED{*this, "Invalidated."};

    if (input == nullptr) return IIr77BASE::Ir77_INVALID_ARGUMENT{*this, "InvalidOperand; nullptr."};

    std::optional<IIr77FILESYS::DIr77FOLDER> folder = input.try_as<IIr77FILESYS::DIr77FOLDER>();

    if (!folder.has_value()) return IIr77BASE::Ir77_INVALID_ARGUMENT{*this, "InvalidOperand."};

    Heap = input;

    return IIr77BASE::Ir77_OPERATION_SUCCEEDED{};
}

IIr77BASE::IIr77RETURN OIr77ADD::GetHeap(IIr77BASE::IIr77BALE& input) {
    if (!IsValid) return IIr77BASE::Ir77_INVALIDATED{*this, "Invalidated."};

    input = Operand;

    return IIr77BASE::Ir77_OPERATION_SUCCEEDED{};
}

IIr77BASE::IIr77RETURN OIr77ADD::Result(IIr77BASE::IIr77BALE const& outcome, IIr77BASE::IIr77RETURN const& ret) {
    if (!IsValid) return IIr77BASE::Ir77_INVALIDATED{*this, "Invalidated."};

    if (Complete) return IIr77BASE::Ir77_PROCESS_COMPLETE{};

    if (outcome == nullptr) return IIr77BASE::Ir77_INVALID_ARGUMENT{*this, "InvalidOperand."};

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

IIr77BASE::IIr77RETURN OIr77ADD::Result(IIr77BASE::IIr77BALE& outcome) {
    if (!IsValid) return IIr77BASE::Ir77_INVALIDATED{*this, "Invalidated."};

    if (!Complete) return IIr77BASE::Ir77_PROCESS_NOT_COMPLETE{};

    outcome = Outcome;

    return Return;
}

IIr77BASE::IIr77RETURN OIr77ADD::InvalidateOperation(IIr77BASE::IIr77RETURN const& conditions) {
    if (!IsValid) return IIr77BASE::Ir77_INVALIDATED{*this, "Invalidated."};

    IsValid = false;

    InvalidationCondition = conditions;

    Delisted = std::chrono::system_clock::now();

    return IIr77BASE::Ir77_OPERATION_SUCCEEDED{};
}

IIr77BASE::IIr77RETURN OIr77ADD::IsInvalid() {
    if (!IsValid)
        return IIr77BASE::Ir77_INVALIDATED{*this, "Invalidated."};

    else
        return IIr77BASE::Ir77_TRUE{};
}

IIr77BASE::IIr77RETURN OIr77ADD::IsInvalid(IIr77BASE::IIr77RETURN& conditions) {
    if (!IsValid)
        return IIr77BASE::Ir77_INVALIDATED{*this, "Invalidated."};

    else
        conditions = InvalidationCondition;
    return IIr77BASE::Ir77_TRUE{};
}
}  // namespace winrt::IIr77FILESYS::m_implementation