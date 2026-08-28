#include "DIr77OPERATION.h"

#include "pch.h"

#if __has_include("DIr77OPERATION.g.cpp")
#include "DIr77OPERATION.g.cpp"
#endif

namespace winrt::IIr77PROJECT::m_implementation {
DIr77OPERATION::DIr77OPERATION() {
    GUID uid;

    HRESULT hr = CoCreateGuid(&uid);

    if (SUCCEEDED(hr)) {
        Instance = uid;
    } else {
        throw std::runtime_error{"Failed to generate Uuid."};
    }

    Enlisted = std::chrono::system_clock::now();
}

IIr77BASE::IIr77RETURN DIr77OPERATION::EnlistedAs(winrt::guid& uid) {
    uid = IIr77BASE::IDIIr77ENLISTED::HVIDIIr77BALE();

    if (!IsValid) return IIr77BASE::Ir77_INVALIDATED{*this, "Enlisted has been invalidated."};

    return IIr77BASE::Ir77_OPERATION_SUCCEEDED{};
}

IIr77BASE::IIr77RETURN DIr77OPERATION::EnlistedUuid(winrt::guid& uid) {
    uid = Instance;

    if (!IsValid) return IIr77BASE::Ir77_INVALIDATED{*this, "Enlisted has been invalidated."};

    return IIr77BASE::Ir77_OPERATION_SUCCEEDED{};
}

IIr77BASE::IIr77RETURN DIr77OPERATION::EnlistedChrono(uint64_t& moment) {
    moment = (uint64_t)std::chrono::system_clock::to_time_t(Enlisted);

    if (!IsValid) return IIr77BASE::Ir77_INVALIDATED{*this, "Enlisted has been invalidated."};

    return IIr77BASE::Ir77_OPERATION_SUCCEEDED{};
}

IIr77BASE::IIr77RETURN DIr77OPERATION::DelistedChrono(uint64_t& moment) {
    moment = (uint64_t)std::chrono::system_clock::to_time_t(Delisted);

    if (!IsValid) return IIr77BASE::Ir77_INVALIDATED{*this, "Enlisted has been invalidated."};

    return IIr77BASE::Ir77_OPERATION_SUCCEEDED{};
}

IIr77BASE::IIr77RETURN DIr77OPERATION::MemberUuid(winrt::guid& uid) {
    uid = IIr77PROJECT::IDDIr77CAD::HVIDDIr77OPERATION();

    if (!IsValid) return IIr77BASE::Ir77_INVALIDATED{*this, "Enlisted has been invalidated."};

    return IIr77BASE::Ir77_OPERATION_SUCCEEDED{};
}

IIr77BASE::IIr77RETURN DIr77OPERATION::CollectionUuid(winrt::guid& uid) {
    uid = IIr77PROJECT::IDDIr77CAD::HVIDDIr77CAD();

    if (!IsValid) return IIr77BASE::Ir77_INVALIDATED{*this, "Enlisted has been invalidated."};

    return IIr77BASE::Ir77_OPERATION_SUCCEEDED{};
}

IIr77BASE::IIr77RETURN DIr77OPERATION::Resize(size_t const& size) {
    (void)size;

    if (!IsValid) return IIr77BASE::Ir77_INVALIDATED{*this, "Invalidated."};

    if (Sealed) return IIr77BASE::Ir77_SEALED{*this, "Heap is sealed."};

    return IIr77BASE::Ir77_OPERATION_SUCCEEDED{};
}

IIr77BASE::IIr77RETURN DIr77OPERATION::Size(uint32_t& size) {
    if (!IsValid) return IIr77BASE::Ir77_INVALIDATED{*this, "Invalidated."};

    size = 1;

    return IIr77BASE::Ir77_OPERATION_SUCCEEDED{};
}

IIr77BASE::IIr77RETURN DIr77OPERATION::IsEmpty() {
    if (!IsValid) return IIr77BASE::Ir77_INVALIDATED{*this, "Invalidated."};

    return IIr77BASE::Ir77_FALSE{};
}

IIr77BASE::IIr77RETURN DIr77OPERATION::Set(size_t const& at, WF::IInspectable const& value) {
    if (!IsValid) return IIr77BASE::Ir77_INVALIDATED{*this, "Invalidated."};

    if (Sealed) return IIr77BASE::Ir77_SEALED{};

    if (at > 0) return IIr77BASE::Ir77_OUT_OF_RANGE{};

    std::optional<IIr77BASE::Ir77COLLECT> collection = value.try_as<IIr77BASE::Ir77COLLECT>();

    if (!collection.has_value()) return IIr77BASE::Ir77_INVALID_ARGUMENT{*this, "Invalid Operation Vector."};

    WF::Collections::IVector<WF::IInspectable> vector = collection.value().Collection();

    if (vector == nullptr) return IIr77BASE::Ir77_INVALID_ARGUMENT{*this, "Invalid Operation Vector."};

    for (WF::IInspectable obj : vector) {
        std::optional<IIr77BASE::Ir77ITEM> item = obj.try_as<IIr77BASE::Ir77ITEM>();

        if (!item.has_value()) return IIr77BASE::Ir77_INVALID_ARGUMENT{*this, "Invalid Operation Vector List Item."};
    }

    Heap[0] = value;
}

IIr77BASE::IIr77RETURN DIr77OPERATION::SealOperand() {
    if (!IsValid) return IIr77BASE::Ir77_INVALIDATED{*this, "Invalidated."};

    Sealed = true;

    return IIr77BASE::Ir77_OPERATION_SUCCEEDED{};
}

IIr77BASE::IIr77RETURN DIr77OPERATION::IsSealed() {
    if (!IsValid) return IIr77BASE::Ir77_INVALIDATED{*this, "Invalidated."};

    if (Sealed)
        return IIr77BASE::Ir77_TRUE{*this, "List is sealed."};

    else
        return IIr77BASE::Ir77_FALSE{*this, "List is sealed."};
}

IIr77BASE::IIr77RETURN DIr77OPERATION::At(uint32_t const& at, WF::IInspectable& value) {
    if (!IsValid) return IIr77BASE::Ir77_INVALIDATED{*this, "Invalidated."};

    if (at > 0) return IIr77BASE::Ir77_OUT_OF_RANGE{};

    value = Operand[at];

    return IIr77BASE::Ir77_OPERATION_SUCCEEDED{};
}

IIr77BASE::IIr77RETURN DIr77OPERATION::InvalidateHeap(IIr77BASE::IIr77RETURN const& conditions) {
    if (!IsValid) return IIr77BASE::Ir77_INVALIDATED{*this, "Invalidated."};

    IsValid = false;

    InvalidationCondition = conditions;

    Delisted = std::chrono::system_clock::now();

    return IIr77BASE::Ir77_OPERATION_SUCCEEDED{};
}

IIr77BASE::IIr77RETURN DIr77OPERATION::IsInvalid() {
    if (!IsValid)
        return IIr77BASE::Ir77_INVALIDATED{*this, "Invalidated."};

    else
        return IIr77BASE::Ir77_TRUE{};
}

IIr77BASE::IIr77RETURN DIr77OPERATION::IsInvalid(IIr77BASE::IIr77RETURN& conditions) {
    if (!IsValid)
        return IIr77BASE::Ir77_INVALIDATED{*this, "Invalidated."};

    else
        conditions = InvalidationCondition;
    return IIr77BASE::Ir77_TRUE{};
}
}  // namespace winrt::IIr77PROJECT::m_implementation
