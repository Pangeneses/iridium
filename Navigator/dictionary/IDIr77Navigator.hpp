#pragma once

#include <chrono>
#include <map>
#include <memory>
#include <string>

#include "../../Ir77RT/interface/IIr77GUID.hpp"
#include "../../Ir77RT/interface/IIr77Dictionary.hpp"
#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../../Ir77RT/runtime/Ir77GUID.hpp"
#include "../../Ir77RT/runtime/Ir77Return.hpp"

namespace NSIr77RT {

inline Ir77GUID GUIDIr77Navigator{(static_cast<unsigned __int128>(0xBE9408033CA84DE3) << 64) | 0x8CD694DA715F8A2A};

inline Ir77GUID GUIDIr77NavEvent{(static_cast<unsigned __int128>(0xBC097A375BC047DA) << 64) | 0xB0E80B59C6BC5C61};
inline Ir77GUID GUIDIr77NavState{(static_cast<unsigned __int128>(0xB7CB6F8DC3C0498F) << 64) | 0xBBFDD87DDBD71AF8};
inline Ir77GUID GUIDIr77NavigatedState{(static_cast<unsigned __int128>(0x1E26F696A9584C84) << 64) | 0xB70276C09970DD16};
inline Ir77GUID GUIDIr77NavCancel{(static_cast<unsigned __int128>(0xF868E928949B404D) << 64) | 0xB4BB9B280AD5BC5F};
inline Ir77GUID GUIDIr77NavActive{(static_cast<unsigned __int128>(0xA4FAE8913BCB4DA7) << 64) | 0xB35879FC43F6DC77};
inline Ir77GUID GUIDIr77NavError{(static_cast<unsigned __int128>(0xAB3D2947C1414558) << 64) | 0x87D65DA1C18310FB};
inline Ir77GUID GUIDIr77NavTree{(static_cast<unsigned __int128>(0x3E20C428CB744CD0) << 64) | 0xA9728A3C2F866513};
inline Ir77GUID GUIDIr77NavSaveContext{(static_cast<unsigned __int128>(0xACD0DEB1A96E49AF) << 64) | 0x8CCCBA246B69B02E};
inline Ir77GUID GUIDIr77BindingContext{(static_cast<unsigned __int128>(0x55A1FDB4EA9648AD) << 64) | 0xA321402CB538D22D};
inline Ir77GUID GUIDIr77OnInitNav{(static_cast<unsigned __int128>(0x0D515335371D4DA3) << 64) | 0x95A5D98350D4D5A4};
inline Ir77GUID GUIDIr77OnViewNav{(static_cast<unsigned __int128>(0x29D68ACAE4C4447D) << 64) | 0x978D5AEAE68C651D};
inline Ir77GUID GUIDIr77OnPostNav{(static_cast<unsigned __int128>(0xA5591254EE9C4F36) << 64) | 0x972D858D8FFBCFB6};
inline Ir77GUID GUIDIr77NavGuard{(static_cast<unsigned __int128>(0xC5B240A833D84CF4) << 64) | 0xAEB41EF3FA2D225F};
inline Ir77GUID GUIDIr77NixGuard{(static_cast<unsigned __int128>(0xB9B446D072814105) << 64) | 0x8C37C0958D9678DF};
inline Ir77GUID GUIDIr77Load{(static_cast<unsigned __int128>(0xE4973252CB014D4B) << 64) | 0xB9F4DABD665A43BA};
inline Ir77GUID GUIDIr77LoadInit{(static_cast<unsigned __int128>(0x07A405ECEB074094) << 64) | 0xB4F880F2B2ADB0BE};
inline Ir77GUID GUIDIr77LoadView{(static_cast<unsigned __int128>(0x9ACF0E04D3E3427D) << 64) | 0xB1109F94D2E6F0D6};
inline Ir77GUID GUIDIr77LoadPost{(static_cast<unsigned __int128>(0x09E140F996F84486) << 64) | 0xB1EF63D331144FB7};
inline Ir77GUID GUIDIr77LoadGuard{(static_cast<unsigned __int128>(0x4AE020EE61784B52) << 64) | 0x9535B476026D3D45};
inline Ir77GUID GUIDIr77NixLoadGuard{(static_cast<unsigned __int128>(0xD66C7F983A874E6E) << 64) | 0x9BE27B7A0C49A12C};
inline Ir77GUID GUIDIr77RouteData{(static_cast<unsigned __int128>(0x7B83EBD0CE5849C7) << 64) | 0xA08FFA549552CA99};
inline Ir77GUID GUIDIr77Route{(static_cast<unsigned __int128>(0x6E34EACA56F049A2) << 64) | 0xA1E58F9806C7C111};
inline Ir77GUID GUIDIr77Router{(static_cast<unsigned __int128>(0xF99F16E43FC84D47) << 64) | 0xA4E72738ED960A04};
inline Ir77GUID GUIDIr77RouterContext{(static_cast<unsigned __int128>(0x617000398C254534) << 64) | 0xA13176E8CAD340F5};
inline Ir77GUID GUIDIr77RouterState{(static_cast<unsigned __int128>(0xD1BA50AC4F4448DB) << 64) | 0xA43A637681B0DBE4};
inline Ir77GUID GUIDIr77RouterError{(static_cast<unsigned __int128>(0x1B60FE52CFE2496B) << 64) | 0xAE97E8B40BBB2204};
inline Ir77GUID GUIDIr77RouterOutlet{(static_cast<unsigned __int128>(0xAB7DB654C77140DF) << 64) | 0x880C9B5780A08014};
inline Ir77GUID GUIDIr77OutletContext{(static_cast<unsigned __int128>(0x75311298E65A48CA) << 64) | 0xA4AF509636D243BF};
inline Ir77GUID GUIDIr77OutletRedirect{(static_cast<unsigned __int128>(0x8A07FA6329A14401) << 64) | 0x9CFD1BF62D237704};
inline Ir77GUID GUIDIr77OutletResolve{(static_cast<unsigned __int128>(0x9172FA00762B4EEF) << 64) | 0xAA2451285A8F15DA};
inline Ir77GUID GUIDIr77OutletError{(static_cast<unsigned __int128>(0x137216B880AA4835) << 64) | 0xBAF58BF315E0C4C1};
inline Ir77GUID GUIDIr77DebugTrace{(static_cast<unsigned __int128>(0xD98F742B57F447DB) << 64) | 0x95DCBE8BC60AE69C};

class IDIr77Navigator : public IIr77Dictionary, public std::enable_shared_from_this<IDIr77Navigator> {
   public:
    IDIr77Navigator() {
        m_enlisted = std::chrono::system_clock::now();

        try {
            m_enlisted_uuid.Generate();
        } catch (std::invalid_argument a) {
            throw a;
        }
    }

    // ── IIr77Enlisted ────────────────────────────────────────────
   public:
    std::shared_ptr<IIr77Return const> EnlistedAs(std::shared_ptr<IIr77GUID const>& uid) {
        seat_shared_uuid<&GUIDIIr77Dictionary>(uid);
        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> EnlistedUuid(std::shared_ptr<IIr77GUID const>& uid) {
        uid.reset(reinterpret_cast<IIr77GUID const*>(&m_enlisted_uuid), [](auto*) {});
        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> EnlistedChrono(std::chrono::system_clock::time_point& t) {
        t = m_enlisted;
        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> DelistedChrono(std::chrono::system_clock::time_point& t) {
        t = m_delisted;
        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SetSender(std::shared_ptr<IIr77Enlisted const>& sender) {
        m_sender = sender;

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetSender(std::shared_ptr<IIr77Enlisted const>& sender) const {
        sender = m_sender;

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SetSenderMsg(std::string const& msg) {
        m_message = msg;

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetSenderMsg(std::string& msg) const {
        msg = m_message;

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> MemberUuid(std::shared_ptr<IIr77GUID const>& uid) {
        seat_shared_uuid<&GUIDIr77Navigator>(uid);
        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CollectionUuid(std::shared_ptr<IIr77GUID const>& uid) {
        seat_shared_uuid<&GUIDIr77Navigator>(uid);
        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    IIr77GUID* const QueryInterface(IIr77GUID const* iid, std::shared_ptr<void>& obj) {
        if (iid == &GUIDIIr77Enlisted)
            obj = std::shared_ptr<IIr77Enlisted>(shared_from_this(), static_cast<IIr77Enlisted*>(this));

        else if (iid == &GUIDIIr77Dictionary)
            obj = std::shared_ptr<IIr77Dictionary>(shared_from_this(), static_cast<IIr77Dictionary*>(this));

        else if (iid == &GUIDIr77Navigator)
            obj = std::shared_ptr<IDIr77Navigator>(shared_from_this(), static_cast<IDIr77Navigator*>(this));

        else
            return &GUIDQueryFailed;

        return &GUIDQuerySucceeded;
    }

    // ── IIr77Dictionary ──────────────────────────────────────────
   public:
    IIr77GUID const* Collection() const { return const_cast<IIr77GUID*>(static_cast<IIr77GUID const*>(&GUIDIr77Navigator)); }

    IIr77GUID const* Member(std::string const& member) const { return m_index.at(member); }

    std::map<std::string, IIr77GUID const*> const& Index() const { return m_index; }

    // ── Data ─────────────────────────────────────────────────────
   private:
    Ir77GUID m_enlisted_uuid{};

    std::chrono::system_clock::time_point m_enlisted{};

    std::chrono::system_clock::time_point m_delisted{};

    std::shared_ptr<IIr77Enlisted const> m_sender{nullptr};

    std::string m_message{""};

    bool m_valid{true};

    std::shared_ptr<IIr77Return const> m_invalidation_condition{nullptr};

    std::map<std::string, IIr77GUID const*> m_index{
        {"GUIDIr77Navigator", &GUIDIr77Navigator},
        {"GUIDIr77NavEvent", &GUIDIr77NavEvent},
        {"GUIDIr77NavState", &GUIDIr77NavState},
        {"GUIDIr77NavigatedState", &GUIDIr77NavigatedState},
        {"GUIDIr77NavCancel", &GUIDIr77NavCancel},
        {"GUIDIr77NavActive", &GUIDIr77NavActive},
        {"GUIDIr77NavError", &GUIDIr77NavError},
        {"GUIDIr77NavTree", &GUIDIr77NavTree},
        {"GUIDIr77NavSaveContext", &GUIDIr77NavSaveContext},
        {"GUIDIr77BindingContext", &GUIDIr77BindingContext},
        {"GUIDIr77OnInitNav", &GUIDIr77OnInitNav},
        {"GUIDIr77OnViewNav", &GUIDIr77OnViewNav},
        {"GUIDIr77OnPostNav", &GUIDIr77OnPostNav},
        {"GUIDIr77NavGuard", &GUIDIr77NavGuard},
        {"GUIDIr77NixGuard", &GUIDIr77NixGuard},
        {"GUIDIr77Load", &GUIDIr77Load},
        {"GUIDIr77LoadInit", &GUIDIr77LoadInit},
        {"GUIDIr77LoadView", &GUIDIr77LoadView},
        {"GUIDIr77LoadPost", &GUIDIr77LoadPost},
        {"GUIDIr77LoadGuard", &GUIDIr77LoadGuard},
        {"GUIDIr77NixLoadGuard", &GUIDIr77NixLoadGuard},
        {"GUIDIr77RouteData", &GUIDIr77RouteData},
        {"GUIDIr77Route", &GUIDIr77Route},
        {"GUIDIr77Router", &GUIDIr77Router},
        {"GUIDIr77RouterContext", &GUIDIr77RouterContext},
        {"GUIDIr77RouterState", &GUIDIr77RouterState},
        {"GUIDIr77RouterError", &GUIDIr77RouterError},
        {"GUIDIr77RouterOutlet", &GUIDIr77RouterOutlet},
        {"GUIDIr77OutletContext", &GUIDIr77OutletContext},
        {"GUIDIr77OutletRedirect", &GUIDIr77OutletRedirect},
        {"GUIDIr77OutletResolve", &GUIDIr77OutletResolve},
        {"GUIDIr77OutletError", &GUIDIr77OutletError},
        {"GUIDIr77DebugTrace", &GUIDIr77DebugTrace},
    };
};

}  // namespace NSIr77RT