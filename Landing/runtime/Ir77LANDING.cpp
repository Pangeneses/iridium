#include "Ir77LANDING.h"

#include "pch.h"

#if __has_include("Ir77LANDING.g.cpp")
#include "Ir77LANDING.g.cpp"
#endif

namespace winrt::Landing::m_implementation {
Ir77LANDING::Ir77LANDING() {
    GUID uid;

    HRESULT hr = CoCreateGuid(&uid);

    if (SUCCEEDED(hr)) {
        Instance = uid;
    } else {
        throw std::runtime_error{"Failed to generate Uuid."};
    }

    Enlisted = std::chrono::system_clock::now();

    Patch = IIr77BASE::Ir77PATCH{}.as<IIr77BASE::IIr77PATCH>();

    Patch.AddOperation(IIr77APP::IDIIr77APP::HVIDIIr77APP(), IIr77APP::IDOIr77APP::HVIDOIr77LOAD(), {this, &Ir77LANDING::Load});
    Patch.AddOperation(IIr77APP::IDIIr77APP::HVIDIIr77APP(), IIr77APP::IDOIr77APP::HVIDOIr77UNLOAD(), {this, &Ir77LANDING::Unload});
    Patch.AddOperation(IIr77APP::IDIIr77APP::HVIDIIr77APP(), IIr77APP::IDOIr77APP::HVIDOIr77PARK(), {this, &Ir77LANDING::Park});
    Patch.AddOperation(IIr77APP::IDIIr77APP::HVIDIIr77APP(), IIr77APP::IDOIr77APP::HVIDOIr77SETSTATE(), {this, &Ir77LANDING::SetState});
    Patch.AddOperation(IIr77APP::IDIIr77APP::HVIDIIr77APP(), IIr77APP::IDOIr77APP::HVIDOIr77GETSTATE(), {this, &Ir77LANDING::GetState});
}

IIr77BASE::IIr77RETURN Ir77LANDING::EnlistedAs(winrt::guid& uid) {
    uid = IIr77BASE::IDIIr77ENLISTED::HVIDIIr77SERVICE();

    return IIr77BASE::Ir77_OPERATION_SUCCEEDED{};
}

IIr77BASE::IIr77RETURN Ir77LANDING::EnlistedUuid(winrt::guid& uid) {
    uid = Instance;

    return IIr77BASE::Ir77_OPERATION_SUCCEEDED{};
}

IIr77BASE::IIr77RETURN Ir77LANDING::EnlistedChrono(uint64_t& moment) {
    moment = (uint64_t)std::chrono::system_clock::to_time_t(Enlisted);

    return IIr77BASE::Ir77_OPERATION_SUCCEEDED{};
}

IIr77BASE::IIr77RETURN Ir77LANDING::DelistedChrono(uint64_t& moment) {
    moment = (uint64_t)std::chrono::system_clock::to_time_t(Delisted);

    return IIr77BASE::Ir77_OPERATION_SUCCEEDED{};
}

IIr77BASE::IIr77RETURN Ir77LANDING::MemberUuid(winrt::guid& uid) {
    uid = Landing::IDIr77LANDING::HVIDIr77LANDING();

    return IIr77BASE::Ir77_OPERATION_SUCCEEDED{};
}

IIr77BASE::IIr77RETURN Ir77LANDING::CollectionUuid(winrt::guid& uid) {
    uid = Landing::IDIr77LANDING::HVIDIr77LANDING();

    return IIr77BASE::Ir77_OPERATION_SUCCEEDED{};
}

winrt::guid Ir77LANDING::ID() { return Landing::IDIr77LANDING::HVIDIr77LANDING(); }

winrt::guid Ir77LANDING::GID() { return Landing::IDIr77LANDING::HVIDIr77LANDING(); }

IIr77BASE::IIr77RETURN Ir77LANDING::Dispatch(winrt::guid const& uid, IIr77BASE::IIr77STACK const& stack) { return Patch.Forward(uid, stack); }

IIr77BASE::IIr77RETURN Ir77LANDING::SwapWorkload(winrt::guid const& uid) {
    if (Navigate(IIr77APP::IDLIr77APP::HVIDLIr77CAD()).ID() != IIr77BASE::Ir77RETURN::HVID_Ir77_OPERATION_SUCCEEDED()) {
        MUX::Application().Exit();
    }

    return IIr77BASE::Ir77_OPERATION_SUCCEEDED{};
}

IIr77BASE::IIr77RETURN Ir77LANDING::Load(IIr77BASE::IIr77Opcode const& cmd) {
    IIr77BASE::IIr77BALE ret = IIr77BASE::Ir77RETVAR{}.as<IIr77BASE::IIr77BALE>();

    IIr77BASE::IIr77BALE operand{nullptr};

    cmd.GetHeap(heap);

    if (heap != nullptr) {
        ret.Set(0, nullptr);

        ret.SealOperand();

        if (cmd.Result(ret, IIr77BASE::Ir77_INVALID_ARGUMENT{}).ID() == IIr77BASE::Ir77RETURN::HVID_Ir77_INVALID_ARGUMENT()) {
            throw std::runtime_error{"Invalid argument RetVar."};
        }

        return IIr77BASE::Ir77_OPERATION_FAILED{*this, "InvalidOperand."};
    }

    UXPage = UXIr77LANDING{}.as<MUXC::UserControl>();

    ret.Set(0, nullptr);

    ret.SealOperand();

    if (cmd.Result(ret, IIr77BASE::Ir77_INVALID_ARGUMENT{}).ID() == IIr77BASE::Ir77RETURN::HVID_Ir77_INVALID_ARGUMENT()) {
        throw std::runtime_error{"Invalid argument RetVar."};
    }

    return IIr77BASE::Ir77_OPERATION_SUCCEEDED{};
}

IIr77BASE::IIr77RETURN Ir77LANDING::Unload(IIr77BASE::IIr77Opcode const& cmd) {
    IIr77BASE::IIr77BALE ret = IIr77BASE::Ir77RETVAR{}.as<IIr77BASE::IIr77BALE>();

    IIr77BASE::IIr77BALE operand{nullptr};

    cmd.GetHeap(heap);

    if (heap != nullptr) {
        ret.Set(0, nullptr);

        ret.SealOperand();

        if (cmd.Result(ret, IIr77BASE::Ir77_INVALID_ARGUMENT{}).ID() == IIr77BASE::Ir77RETURN::HVID_Ir77_INVALID_ARGUMENT()) {
            throw std::runtime_error{"Invalid argument RetVar."};
        }

        return IIr77BASE::Ir77_OPERATION_FAILED{*this, "InvalidOperand."};
    }

    UXPage = nullptr;

    ret.Set(0, nullptr);

    ret.SealOperand();

    if (cmd.Result(ret, IIr77BASE::Ir77_VALID_ARGUMENT{}).ID() == IIr77BASE::Ir77RETURN::HVID_Ir77_INVALID_ARGUMENT()) {
        throw std::runtime_error{"Invalid argument RetVar."};
    }

    return IIr77BASE::Ir77_OPERATION_SUCCEEDED{};
}

IIr77BASE::IIr77RETURN Ir77LANDING::Park(IIr77BASE::IIr77Opcode const& cmd) {
    IIr77BASE::IIr77BALE ret = IIr77BASE::Ir77RETVAR{}.as<IIr77BASE::IIr77BALE>();

    IIr77BASE::IIr77BALE operand{nullptr};

    cmd.GetHeap(heap);

    if (heap != nullptr) {
        ret.Set(0, nullptr);

        ret.SealOperand();

        if (cmd.Result(ret, IIr77BASE::Ir77_INVALID_ARGUMENT{}).ID() == IIr77BASE::Ir77RETURN::HVID_Ir77_INVALID_ARGUMENT()) {
            throw std::runtime_error{"Invalid argument RetVar."};
        }

        return IIr77BASE::Ir77_OPERATION_FAILED{*this, "InvalidOperand."};
    }

    ret.Set(0, nullptr);

    ret.SealOperand();

    if (cmd.Result(ret, IIr77BASE::Ir77_INVALID_ARGUMENT{}).ID() == IIr77BASE::Ir77RETURN::HVID_Ir77_INVALID_ARGUMENT()) {
        throw std::runtime_error{"Invalid argument RetVar."};
    }

    return IIr77BASE::Ir77_OPERATION_SUCCEEDED{};
}

IIr77BASE::IIr77RETURN Ir77LANDING::SetState(IIr77BASE::IIr77Opcode const& cmd) {
    IIr77BASE::IIr77BALE ret = IIr77BASE::Ir77RETVAR{}.as<IIr77BASE::IIr77BALE>();

    IIr77BASE::IIr77BALE iheap{nullptr};

    cmd.GetHeap(iheap);

    std::optional<IIr77APP::DIr77STATE> operand = iheap.try_as<IIr77APP::DIr77STATE>();

    if (!heap.has_value()) {
        ret.Set(0, nullptr);

        ret.SealOperand();

        if (cmd.Result(ret, IIr77BASE::Ir77_INVALID_ARGUMENT{}).ID() == IIr77BASE::Ir77RETURN::HVID_Ir77_INVALID_ARGUMENT()) {
            throw std::runtime_error{"Invalid argument RetVar."};
        }

        return IIr77BASE::Ir77_INVALID_ARGUMENT{*this, "InvalidOperand."};
    }

    WF::IInspectable obj{nullptr};

    heap.value().At(0, obj);

    std::optional<WFC::IVector<WF::IInspectable>> state = obj.try_as<WFC::IVector<WF::IInspectable>>();

    if (!state.has_value()) {
        ret.Set(0, nullptr);

        ret.SealOperand();

        if (cmd.Result(ret, IIr77BASE::Ir77_INVALID_ARGUMENT{}).ID() == IIr77BASE::Ir77RETURN::HVID_Ir77_INVALID_ARGUMENT()) {
            throw std::runtime_error{"Invalid argument RetVar."};
        }

        return IIr77BASE::Ir77_OPERATION_FAILED{*this, "InvalidOperand."};
    }

    for (WF::IInspectable inspectable : state.value()) {
        std::optional<IIr77BASE::Ir77ITEM> item = inspectable.try_as<IIr77BASE::Ir77ITEM>();

        if (!item.has_value()) {
            ret.Set(0, nullptr);

            ret.SealOperand();

            if (cmd.Result(ret, IIr77BASE::Ir77_INVALID_ARGUMENT{}).ID() == IIr77BASE::Ir77RETURN::HVID_Ir77_INVALID_ARGUMENT()) {
                throw std::runtime_error{"Invalid argument RetVar."};
            }

            IIr77BASE::Ir77_OPERATION_FAILED{*this, "Invalid State Listed."};
        }
    }

    for (WF::IInspectable inspectable : state.value()) {
        IIr77BASE::Ir77ITEM item = inspectable.try_as<IIr77BASE::Ir77ITEM>();

        IIr77BASE::Ir77TAG tag = item.Tag().as<IIr77BASE::Ir77TAG>();

        IIr77BASE::IIr77ENLISTED runtime = item.Item().as<IIr77BASE::IIr77ENLISTED>();

        if (tag.Uuid().Field() == IIr77APP::IDXIr77APP::HVIDXIr77HEALTH()) {
            std::optional<IIr77BASE::IIr77SERVICE> health = runtime.try_as<IIr77BASE::IIr77SERVICE>();

            if (!health.has_value()) {
                ret.Set(0, nullptr);

                ret.SealOperand();

                if (cmd.Result(ret, IIr77BASE::Ir77_INVALID_ARGUMENT{}).ID() == IIr77BASE::Ir77RETURN::HVID_Ir77_INVALID_ARGUMENT()) {
                    throw std::runtime_error{"Invalid argument RetVar."};
                }

                IIr77BASE::Ir77_OPERATION_FAILED{*this, "Invalid Health Module."};
            }

            Health = health.value();
        } else if (tag.Uuid().Field() == IIr77APP::IDXIr77APP::HVIDXIr77NAV()) {
            std::optional<IIr77BASE::Ir77IUNKN> unk = runtime.try_as<IIr77BASE::Ir77IUNKN>();

            std::optional<IIr77BASE::Ir77NAVIGATE> nav = winrt::unbox_value<IIr77BASE::Ir77NAVIGATE>(unk.value().Field());

            if (!nav.has_value()) {
                ret.Set(0, nullptr);

                ret.SealOperand();

                if (cmd.Result(ret, IIr77BASE::Ir77_INVALID_ARGUMENT{}).ID() == IIr77BASE::Ir77RETURN::HVID_Ir77_INVALID_ARGUMENT()) {
                    throw std::runtime_error{"Invalid argument RetVar."};
                }

                IIr77BASE::Ir77_OPERATION_FAILED{*this, "Invalid Navigation Delegate."};
            }

            Navigate = nav.value();
        } else if (tag.Uuid().Field() == IIr77APP::IDXIr77APP::HVIDXIr77PAGE()) {
            std::optional<IIr77BASE::Ir77IUNKN> unk = runtime.try_as<IIr77BASE::Ir77IUNKN>();

            std::optional<MUXC::UserControl> page = unk.value().try_as<MUXC::UserControl>();

            if (!page.has_value()) {
                ret.Set(0, nullptr);

                ret.SealOperand();

                if (cmd.Result(ret, IIr77BASE::Ir77_INVALID_ARGUMENT{}).ID() == IIr77BASE::Ir77RETURN::HVID_Ir77_INVALID_ARGUMENT()) {
                    throw std::runtime_error{"Invalid argument RetVar."};
                }

                IIr77BASE::Ir77_OPERATION_FAILED{*this, "Invalid Page File."};
            }

            UXPage = page.value();
        } else if (tag.Uuid().Field() == IIr77APP::IDXIr77APP::HVIDXIr77PROJECT()) {
            std::optional<IIr77BASE::IIr77SERVICE> project = runtime.try_as<IIr77BASE::IIr77SERVICE>();

            if (!project.has_value()) {
                ret.Set(0, nullptr);

                ret.SealOperand();

                if (cmd.Result(ret, IIr77BASE::Ir77_INVALID_ARGUMENT{}).ID() == IIr77BASE::Ir77RETURN::HVID_Ir77_INVALID_ARGUMENT()) {
                    throw std::runtime_error{"Invalid argument RetVar."};
                }

                IIr77BASE::Ir77_OPERATION_FAILED{*this, "Invalid Project Module."};
            }

            Project = project.value();
        }
    }

    ret.Set(0, nullptr);

    ret.SealOperand();

    if (cmd.Result(ret, IIr77BASE::Ir77_VALID_ARGUMENT{}).ID() == IIr77BASE::Ir77RETURN::HVID_Ir77_INVALID_ARGUMENT()) {
        throw std::runtime_error{"Invalid argument RetVar."};
    }

    return IIr77BASE::Ir77_OPERATION_SUCCEEDED{};
}

IIr77BASE::IIr77RETURN Ir77LANDING::GetState(IIr77BASE::IIr77Opcode const& cmd) {
    IIr77BASE::IIr77BALE ret = IIr77BASE::Ir77RETVAR{}.as<IIr77BASE::IIr77BALE>();

    IIr77BASE::IIr77BALE iheap{nullptr};

    cmd.GetHeap(iheap);

    std::optional<IIr77APP::DIr77STATE> operand = iheap.try_as<IIr77APP::DIr77STATE>();

    if (!heap.has_value()) {
        ret.Set(0, nullptr);

        ret.SealOperand();

        if (cmd.Result(ret, IIr77BASE::Ir77_INVALID_ARGUMENT{}).ID() == IIr77BASE::Ir77RETURN::HVID_Ir77_INVALID_ARGUMENT()) {
            throw std::runtime_error{"Invalid argument RetVar."};
        }

        return IIr77BASE::Ir77_OPERATION_FAILED{*this, "InvalidOperand."};
    }

    WF::IInspectable obj{nullptr};

    heap.value().At(0, obj);

    std::optional<WFC::IVector<WF::IInspectable>> state = obj.try_as<WFC::IVector<WF::IInspectable>>();

    if (!state.has_value()) {
        ret.Set(0, nullptr);

        ret.SealOperand();

        if (cmd.Result(ret, IIr77BASE::Ir77_INVALID_ARGUMENT{}).ID() == IIr77BASE::Ir77RETURN::HVID_Ir77_INVALID_ARGUMENT()) {
            throw std::runtime_error{"Invalid argument RetVar."};
        }

        return IIr77BASE::Ir77_OPERATION_FAILED{*this, "InvalidOperand."};
    }

    for (WF::IInspectable inspectable : state.value()) {
        std::optional<IIr77BASE::Ir77ITEM> item = inspectable.try_as<IIr77BASE::Ir77ITEM>();

        if (!item.has_value()) {
            ret.Set(0, nullptr);

            ret.SealOperand();

            if (cmd.Result(ret, IIr77BASE::Ir77_INVALID_ARGUMENT{}).ID() == IIr77BASE::Ir77RETURN::HVID_Ir77_INVALID_ARGUMENT()) {
                throw std::runtime_error{"Invalid argument RetVar."};
            }

            IIr77BASE::Ir77_OPERATION_FAILED{*this, "Invalid State Listed."};
        }
    }

    for (WF::IInspectable inspectable : state.value()) {
        IIr77BASE::Ir77ITEM item = inspectable.try_as<IIr77BASE::Ir77ITEM>();

        IIr77BASE::Ir77TAG tag = item.Tag().as<IIr77BASE::Ir77TAG>();

        IIr77BASE::IIr77ENLISTED runtime = item.Item().as<IIr77BASE::IIr77ENLISTED>();

        if (tag.Uuid().Field() == IIr77APP::IDXIr77APP::HVIDXIr77HEALTH()) {
            runtime = Health;
        } else if (tag.Uuid().Field() == IIr77APP::IDXIr77APP::HVIDXIr77NAV()) {
            runtime = IIr77BASE::Ir77IUNKN{winrt::box_value(Navigate)};
        } else if (tag.Uuid().Field() == IIr77APP::IDXIr77APP::HVIDXIr77PAGE()) {
            UXPage.as<Landing::UXIr77LANDING>().SetNavigate({this, &Ir77LANDING::SwapWorkload});

            runtime = IIr77BASE::Ir77IUNKN{UXPage.as<WF::IInspectable>()};
        } else if (tag.Uuid().Field() == IIr77APP::IDXIr77APP::HVIDXIr77PROJECT()) {
            runtime = Project;
        }

        ret.Set(0, runtime);

        ret.SealOperand();

        if (cmd.Result(ret, IIr77BASE::Ir77_VALID_ARGUMENT{}).ID() == IIr77BASE::Ir77RETURN::HVID_Ir77_INVALID_ARGUMENT()) {
            throw std::runtime_error{"Invalid argument RetVar."};
        }
    }

    return IIr77BASE::Ir77_OPERATION_SUCCEEDED{};
}
}  // namespace winrt::Landing::m_implementation
