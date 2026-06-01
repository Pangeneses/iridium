#include "DIr77BIND.h"

#include "pch.h"

#if __has_include("DIr77BIND.g.cpp")
#include "DIr77BIND.g.cpp"
#endif

namespace winrt::IIr77APP::m_implementation {
DIr77BIND::DIr77BIND() {
    GUID uid;

    HRESULT hr = CoCreateGuid(&uid);

    if (SUCCEEDED(hr)) {
        Instance = uid;
    } else {
        throw std::runtime_error("Failed to generate Uuid.");
    }

    Enlisted = std::chrono::system_clock::now();
}

IIr77BASE::IIr77RETURN DIr77BIND::EnlistedAs(winrt::guid& uid) {
    uid = IIr77BASE::IDIIr77ENLISTED::HVIDIIr77BALE();

    if (!IsValid) return IIr77BASE::Ir77_INVALIDATED{*this, "Enlisted has been invalidated."};

    return IIr77BASE::Ir77_OPERATION_SUCCEEDED{};
}

IIr77BASE::IIr77RETURN DIr77BIND::EnlistedUuid(winrt::guid& uid) {
    uid = Instance;

    if (!IsValid) return IIr77BASE::Ir77_INVALIDATED{*this, "Enlisted has been invalidated."};

    return IIr77BASE::Ir77_OPERATION_SUCCEEDED{};
}

IIr77BASE::IIr77RETURN DIr77BIND::EnlistedChrono(uint64_t& moment) {
    moment = (uint64_t)std::chrono::system_clock::to_time_t(Enlisted);

    if (!IsValid) return IIr77BASE::Ir77_INVALIDATED{*this, "Enlisted has been invalidated."};

    return IIr77BASE::Ir77_OPERATION_SUCCEEDED{};
}

IIr77BASE::IIr77RETURN DIr77BIND::DelistedChrono(uint64_t& moment) {
    moment = (uint64_t)std::chrono::system_clock::to_time_t(Delisted);

    if (!IsValid) return IIr77BASE::Ir77_INVALIDATED{*this, "Enlisted has been invalidated."};

    return IIr77BASE::Ir77_OPERATION_SUCCEEDED{};
}

IIr77BASE::IIr77RETURN DIr77BIND::MemberOfUuid(winrt::guid& uid) {
    uid = IIr77APP::IDDIr77APP::HVIDDIr77BIND();

    if (!IsValid) return IIr77BASE::Ir77_INVALIDATED{*this, "Enlisted has been invalidated."};

    return IIr77BASE::Ir77_OPERATION_SUCCEEDED{};
}

IIr77BASE::IIr77RETURN DIr77BIND::CollectionUuid(winrt::guid& uid) {
    uid = IIr77APP::IDDIr77APP::HVIDDIr77APP();

    if (!IsValid) return IIr77BASE::Ir77_INVALIDATED{*this, "Enlisted has been invalidated."};

    return IIr77BASE::Ir77_OPERATION_SUCCEEDED{};
}

IIr77BASE::IIr77RETURN DIr77BIND::Resize(uint32_t const& size) {
    if (!IsValid) return IIr77BASE::Ir77_INVALIDATED{*this, "Invalidated."};

    return IIr77BASE::Ir77_SEALED{*this, "Heap is sealed."};
}

IIr77BASE::IIr77RETURN DIr77BIND::Size(uint32_t& size) {
    if (!IsValid) return IIr77BASE::Ir77_INVALIDATED{*this, "Invalidated."};

    size = 1;

    return IIr77BASE::Ir77_OPERATION_SUCCEEDED{};
}

IIr77BASE::IIr77RETURN DIr77BIND::IsEmpty() {
    if (!IsValid) return IIr77BASE::Ir77_INVALIDATED{*this, "Invalidated."};

    return IIr77BASE::Ir77_FALSE{};
}

IIr77BASE::IIr77RETURN DIr77BIND::Set(uint32_t const& at, WF::IInspectable const& value) {
    if (!IsValid) return IIr77BASE::Ir77_INVALIDATED{*this, "Invalidated."};

    if (Sealed) return IIr77BASE::Ir77_SEALED{};

    if (at > 0) return IIr77BASE::Ir77_OUT_OF_RANGE{};

    std::optional<WF::Collections::IVector<WF::IInspectable>> vector = value.try_as<WF::Collections::IVector<WF::IInspectable>>();

    if (vector == std::nullopt) return IIr77BASE::Ir77_INVALID_ARGUMENT{*this, "Invalid State Vector."};

    for (WF::IInspectable obj : vector.value()) {
        std::optional<WF::Collections::IKeyValuePair<IIr77BASE::Ir77ITEM, WF::IInspectable>> item =
            obj.try_as<WF::Collections::IKeyValuePair<IIr77BASE::Ir77ITEM, WF::IInspectable>>();

        if (item == std::nullopt) return IIr77BASE::Ir77_INVALID_ARGUMENT{*this, "Invalid State Vector List Item."};
    }

    Heap[0] = value;

    return IIr77BASE::Ir77_OPERATION_SUCCEEDED{};
}

IIr77BASE::IIr77RETURN DIr77BIND::SealOperand() {
    if (!IsValid) return IIr77BASE::Ir77_INVALIDATED{*this, "Invalidated."};

    Sealed = true;

    return IIr77BASE::Ir77_INVALID_OPERATION{};
}

IIr77BASE::IIr77RETURN DIr77BIND::IsSealed() {
    if (!IsValid) return IIr77BASE::Ir77_INVALIDATED{*this, "Invalidated."};

    if (Sealed)
        return IIr77BASE::Ir77_TRUE{*this, "List is sealed."};

    else
        return IIr77BASE::Ir77_FALSE{*this, "List is sealed."};
}

IIr77BASE::IIr77RETURN DIr77BIND::At(uint32_t const& at, WF::IInspectable& value) {
    if (!IsValid) return IIr77BASE::Ir77_INVALIDATED{*this, "Invalidated."};

    if (at > 0) return IIr77BASE::Ir77_OUT_OF_RANGE{};

    value = Operand[at];

    return IIr77BASE::Ir77_OPERATION_SUCCEEDED{};
}

IIr77BASE::IIr77RETURN DIr77BIND::InvalidateHeap(IIr77BASE::IIr77RETURN const& conditions) {
    if (!IsValid) return IIr77BASE::Ir77_INVALIDATED{*this, "Invalidated."};

    IsValid = false;

    InvalidationCondition = conditions;

    Delisted = std::chrono::system_clock::now();

    return IIr77BASE::Ir77_OPERATION_SUCCEEDED{};
}

IIr77BASE::IIr77RETURN DIr77BIND::IsInvalid() {
    if (!IsValid)
        return IIr77BASE::Ir77_INVALIDATED{*this, "Invalidated."};

    else
        return IIr77BASE::Ir77_TRUE{};
}

IIr77BASE::IIr77RETURN DIr77BIND::IsInvalid(IIr77BASE::IIr77RETURN& conditions) {
    if (!IsValid)
        return IIr77BASE::Ir77_INVALIDATED{*this, "Invalidated."};

    else
        conditions = InvalidationCondition;
    return IIr77BASE::Ir77_TRUE{};
}
}  // namespace winrt::IIr77APP::m_implementation
