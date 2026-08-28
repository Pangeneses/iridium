#include "DIr77PIPELINE.h"

#include "pch.h"

#if __has_include("DIr77PIPELINE.g.cpp")
#include "DIr77PIPELINE.g.cpp"
#endif

namespace winrt::IIr77PROJECT::m_implementation {
DIr77PIPELINE::DIr77PIPELINE() {
    GUID uid;

    HRESULT hr = CoCreateGuid(&uid);

    if (SUCCEEDED(hr)) {
        Instance = uid;
    } else {
        throw std::runtime_error{"Failed to generate Uuid."};
    }

    Enlisted = std::chrono::system_clock::now();
}

IIr77BASE::IIr77RETURN DIr77PIPELINE::EnlistedAs(winrt::guid& uid) {
    uid = IIr77BASE::IDIIr77ENLISTED::HVIDIIr77BALE();

    if (!IsValid) return IIr77BASE::Ir77_INVALIDATED{*this, "Enlisted has been invalidated."};

    return IIr77BASE::Ir77_OPERATION_SUCCEEDED{};
}

IIr77BASE::IIr77RETURN DIr77PIPELINE::EnlistedUuid(winrt::guid& uid) {
    uid = Instance;

    if (!IsValid) return IIr77BASE::Ir77_INVALIDATED{*this, "Enlisted has been invalidated."};

    return IIr77BASE::Ir77_OPERATION_SUCCEEDED{};
}

IIr77BASE::IIr77RETURN DIr77PIPELINE::EnlistedChrono(uint64_t& moment) {
    moment = (uint64_t)std::chrono::system_clock::to_time_t(Enlisted);

    if (!IsValid) return IIr77BASE::Ir77_INVALIDATED{*this, "Enlisted has been invalidated."};

    return IIr77BASE::Ir77_OPERATION_SUCCEEDED{};
}

IIr77BASE::IIr77RETURN DIr77PIPELINE::DelistedChrono(uint64_t& moment) {
    moment = (uint64_t)std::chrono::system_clock::to_time_t(Delisted);

    if (!IsValid) return IIr77BASE::Ir77_INVALIDATED{*this, "Enlisted has been invalidated."};

    return IIr77BASE::Ir77_OPERATION_SUCCEEDED{};
}

IIr77BASE::IIr77RETURN DIr77PIPELINE::MemberUuid(winrt::guid& uid) {
    uid = IIr77PROJECT::IDDIr77GPU::HVIDDIr77PIPELINE();

    if (!IsValid) return IIr77BASE::Ir77_INVALIDATED{*this, "Enlisted has been invalidated."};

    return IIr77BASE::Ir77_OPERATION_SUCCEEDED{};
}

IIr77BASE::IIr77RETURN DIr77PIPELINE::CollectionUuid(winrt::guid& uid) {
    uid = IIr77PROJECT::IDDIr77GPU::HVIDDIr77GPU();

    if (!IsValid) return IIr77BASE::Ir77_INVALIDATED{*this, "Enlisted has been invalidated."};

    return IIr77BASE::Ir77_OPERATION_SUCCEEDED{};
}

IIr77BASE::IIr77RETURN DIr77PIPELINE::Resize(size_t const& size) {
    (void)size;

    if (!IsValid) return IIr77BASE::Ir77_INVALIDATED{*this, "Invalidated."};

    if (Sealed) return IIr77BASE::Ir77_SEALED{*this, "Heap is sealed."};

    return IIr77BASE::Ir77_OPERATION_SUCCEEDED{};
}

IIr77BASE::IIr77RETURN DIr77PIPELINE::Size(uint32_t& size) {
    if (!IsValid) return IIr77BASE::Ir77_INVALIDATED{*this, "Invalidated."};

    size = 1;

    return IIr77BASE::Ir77_OPERATION_SUCCEEDED{};
}

IIr77BASE::IIr77RETURN DIr77PIPELINE::IsEmpty() {
    if (!IsValid) return IIr77BASE::Ir77_INVALIDATED{*this, "Invalidated."};

    return IIr77BASE::Ir77_FALSE{};
}

IIr77BASE::IIr77RETURN DIr77PIPELINE::Set(size_t const& at, WF::IInspectable const& value) {
    if (!IsValid) return IIr77BASE::Ir77_INVALIDATED{*this, "Invalidated."};

    if (Sealed) return IIr77BASE::Ir77_SEALED{};

    if (at > 0) return IIr77BASE::Ir77_OUT_OF_RANGE{};

    std::optional<IIr77BASE::Ir77WINRT> m_enlisted = value.try_as<IIr77BASE::Ir77WINRT>();

    if (!m_enlisted.has_value()) return IIr77BASE::Ir77_INVALID_ARGUMENT{*this, "Invalid Item."};

    std::optional<IIr77BASE::IIr77SERVICE> pipeline = m_enlisted.value().Field().try_as<IIr77BASE::IIr77SERVICE>();

    if (!pipeline.has_value()) return IIr77BASE::Ir77_INVALID_ARGUMENT{*this, "Invalid Pipeline."};

    Heap[0] = value;

    return IIr77BASE::Ir77_OPERATION_SUCCEEDED{};
}

IIr77BASE::IIr77RETURN DIr77PIPELINE::SealOperand() {
    if (!IsValid) return IIr77BASE::Ir77_INVALIDATED{*this, "Invalidated."};

    Sealed = true;

    return IIr77BASE::Ir77_OPERATION_SUCCEEDED{};
}

IIr77BASE::IIr77RETURN DIr77PIPELINE::IsSealed() {
    if (!IsValid) return IIr77BASE::Ir77_INVALIDATED{*this, "Invalidated."};

    if (Sealed)
        return IIr77BASE::Ir77_TRUE{*this, "List is sealed."};

    else
        return IIr77BASE::Ir77_FALSE{*this, "List is sealed."};
}

IIr77BASE::IIr77RETURN DIr77PIPELINE::At(uint32_t const& at, WF::IInspectable& value) {
    if (!IsValid) return IIr77BASE::Ir77_INVALIDATED{*this, "Invalidated."};

    if (at > 0) return IIr77BASE::Ir77_OUT_OF_RANGE{};

    value = Operand[at];

    return IIr77BASE::Ir77_OPERATION_SUCCEEDED{};
}

IIr77BASE::IIr77RETURN DIr77PIPELINE::InvalidateHeap(IIr77BASE::IIr77RETURN const& conditions) {
    if (!IsValid) return IIr77BASE::Ir77_INVALIDATED{*this, "Invalidated."};

    IsValid = false;

    InvalidationCondition = conditions;

    Delisted = std::chrono::system_clock::now();

    return IIr77BASE::Ir77_OPERATION_SUCCEEDED{};
}

IIr77BASE::IIr77RETURN DIr77PIPELINE::IsInvalid() {
    if (!IsValid)
        return IIr77BASE::Ir77_INVALIDATED{*this, "Invalidated."};

    else
        return IIr77BASE::Ir77_TRUE{};
}

IIr77BASE::IIr77RETURN DIr77PIPELINE::IsInvalid(IIr77BASE::IIr77RETURN& conditions) {
    if (!IsValid)
        return IIr77BASE::Ir77_INVALIDATED{*this, "Invalidated."};

    else
        conditions = InvalidationCondition;
    return IIr77BASE::Ir77_TRUE{};
}
}  // namespace winrt::IIr77PROJECT::m_implementation
