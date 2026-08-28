#include "DIr77HEADER.h"

#include "pch.h"

#if __has_include("DIr77HEADER.g.cpp")
#include "DIr77HEADER.g.cpp"
#endif

namespace winrt::IIr77FILESYS::m_implementation {
DIr77HEADER::DIr77HEADER() {
    GUID uid;

    HRESULT hr = CoCreateGuid(&uid);

    if (SUCCEEDED(hr)) {
        Instance = uid;
    } else {
        throw std::runtime_error{"Failed to generate Uuid."};
    }

    Enlisted = std::chrono::system_clock::now();
}

IIr77BASE::IIr77RETURN DIr77HEADER::EnlistedAs(winrt::guid& uid) {
    uid = IIr77BASE::IDIIr77ENLISTED::HVIDIIr77BALE();

    if (!IsValid) return IIr77BASE::Ir77_INVALIDATED{*this, "Enlisted has been invalidated."};

    return IIr77BASE::Ir77_OPERATION_SUCCEEDED{};
}

IIr77BASE::IIr77RETURN DIr77HEADER::EnlistedUuid(winrt::guid& uid) {
    uid = Instance;

    if (!IsValid) return IIr77BASE::Ir77_INVALIDATED{*this, "Enlisted has been invalidated."};

    return IIr77BASE::Ir77_OPERATION_SUCCEEDED{};
}

IIr77BASE::IIr77RETURN DIr77HEADER::EnlistedChrono(uint64_t& moment) {
    moment = (uint64_t)std::chrono::system_clock::to_time_t(Enlisted);

    if (!IsValid) return IIr77BASE::Ir77_INVALIDATED{*this, "Enlisted has been invalidated."};

    return IIr77BASE::Ir77_OPERATION_SUCCEEDED{};
}

IIr77BASE::IIr77RETURN DIr77HEADER::DelistedChrono(uint64_t& moment) {
    moment = (uint64_t)std::chrono::system_clock::to_time_t(Delisted);

    if (!IsValid) return IIr77BASE::Ir77_INVALIDATED{*this, "Enlisted has been invalidated."};

    return IIr77BASE::Ir77_OPERATION_SUCCEEDED{};
}

IIr77BASE::IIr77RETURN DIr77HEADER::MemberUuid(winrt::guid& uid) {
    uid = IIr77FILESYS::IDDIr77HEADER::HVIDDIr77HEADER();

    if (!IsValid) return IIr77BASE::Ir77_INVALIDATED{*this, "Enlisted has been invalidated."};

    return IIr77BASE::Ir77_OPERATION_SUCCEEDED{};
}

IIr77BASE::IIr77RETURN DIr77HEADER::CollectionUuid(winrt::guid& uid) {
    uid = IIr77FILESYS::IDDIr77HEADER::HVIDDIr77HEADER();

    if (!IsValid) return IIr77BASE::Ir77_INVALIDATED{*this, "Enlisted has been invalidated."};

    return IIr77BASE::Ir77_OPERATION_SUCCEEDED{};
}

IIr77BASE::IIr77RETURN DIr77HEADER::Resize(uint32_t const& size) {
    (void)size;

    if (!IsValid) return IIr77BASE::Ir77_INVALIDATED{*this, "Invalidated."};

    if (Sealed) return IIr77BASE::Ir77_SEALED{*this, "Heap is IsSealed."};

    return IIr77BASE::Ir77_OPERATION_SUCCEEDED{};
}

IIr77BASE::IIr77RETURN DIr77HEADER::Size(uint32_t& size) {
    if (!IsValid) return IIr77BASE::Ir77_INVALIDATED{*this, "Invalidated."};

    size = 1;

    return IIr77BASE::Ir77_OPERATION_SUCCEEDED{};
}

IIr77BASE::IIr77RETURN DIr77HEADER::IsEmpty() {
    if (!IsValid) return IIr77BASE::Ir77_INVALIDATED{*this, "Invalidated."};

    return IIr77BASE::Ir77_FALSE{};
}

IIr77BASE::IIr77RETURN DIr77HEADER::Set(uint32_t const& at, WF::IInspectable const& value) {
    if (!IsValid) return IIr77BASE::Ir77_INVALIDATED{*this, "Invalidated."};

    if (Sealed) return IIr77BASE::Ir77_SEALED{};

    if (at > 0) return IIr77BASE::Ir77_OUT_OF_RANGE{};

    std::optional<WF::Collections::IVector<WF::IInspectable>> vector = value.try_as<WF::Collections::IVector<WF::IInspectable>>();

    if (!vector.has_value()) return IIr77BASE::Ir77_INVALID_ARGUMENT{*this, "Invalid State Vector."};

    for (WF::IInspectable obj : vector.value()) {
        std::optional<IIr77BASE::Ir77ITEM> item = obj.try_as<IIr77BASE::Ir77ITEM>();

        if (!item.has_value()) return IIr77BASE::Ir77_INVALID_ARGUMENT{*this, "Invalid State Vector List Item."};
    }

    Heap[0] = value;

    return IIr77BASE::Ir77_OPERATION_SUCCEEDED{};
}

IIr77BASE::IIr77RETURN DIr77HEADER::SealOperand() {
    if (!IsValid) return IIr77BASE::Ir77_INVALIDATED{*this, "Invalidated."};

    Sealed = true;

    return IIr77BASE::Ir77_INVALID_OPERATION{};
}

IIr77BASE::IIr77RETURN DIr77HEADER::IsSealed() {
    if (!IsValid) return IIr77BASE::Ir77_INVALIDATED{*this, "Invalidated."};

    if (Sealed)
        return IIr77BASE::Ir77_TRUE{*this, "List is sealed."};

    else
        return IIr77BASE::Ir77_FALSE{*this, "List is sealed."};
}

IIr77BASE::IIr77RETURN DIr77HEADER::At(uint32_t const& at, WF::IInspectable& value) {
    if (!IsValid) return IIr77BASE::Ir77_INVALIDATED{*this, "Invalidated."};

    if (at > 0) return IIr77BASE::Ir77_OUT_OF_RANGE{};

    value = Operand[at];

    return IIr77BASE::Ir77_OPERATION_SUCCEEDED{};
}

IIr77BASE::IIr77RETURN DIr77HEADER::InvalidateHeap(IIr77BASE::IIr77RETURN const& conditions) {
    if (!IsValid) return IIr77BASE::Ir77_INVALIDATED{*this, "Invalidated."};

    IsValid = false;

    InvalidationCondition = conditions;

    Delisted = std::chrono::system_clock::now();

    return IIr77BASE::Ir77_OPERATION_SUCCEEDED{};
}

IIr77BASE::IIr77RETURN DIr77HEADER::IsInvalid() {
    if (!IsValid)
        return IIr77BASE::Ir77_INVALIDATED{*this, "Invalidated."};

    else
        return IIr77BASE::Ir77_TRUE{};
}

IIr77BASE::IIr77RETURN DIr77HEADER::IsInvalid(IIr77BASE::IIr77RETURN& conditions) {
    if (!IsValid)
        return IIr77BASE::Ir77_INVALIDATED{*this, "Invalidated."};

    else
        conditions = InvalidationCondition;
    return IIr77BASE::Ir77_TRUE{};
}
}  // namespace winrt::IIr77FILESYS::m_implementation
