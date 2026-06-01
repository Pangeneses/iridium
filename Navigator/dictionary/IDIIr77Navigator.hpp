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

inline Ir77GUID GUIDIIr77Navigator{(static_cast<unsigned __int128>(0x8A133A45C7594DD3) << 64) | 0x814B472347CEEBAF};

inline Ir77GUID GUIDIIr77NavEvent{(static_cast<unsigned __int128>(0xCC2D9E14CF4A4412) << 64) | 0xAA901268910B707B};
inline Ir77GUID GUIDIIr77NavState{(static_cast<unsigned __int128>(0x7A6594B98D0F4B7D) << 64) | 0x92CB77C55673F86F};
inline Ir77GUID GUIDIIr77NavigatedState{(static_cast<unsigned __int128>(0x9B596DA7C5164492) << 64) | 0x9D0F37D8C14D268E};
inline Ir77GUID GUIDIIr77NavCancel{(static_cast<unsigned __int128>(0xA1E339575B7440FE) << 64) | 0x89C62EE8A41F029A};
inline Ir77GUID GUIDIIr77NavActive{(static_cast<unsigned __int128>(0xF3E8B96F83F944B0) << 64) | 0xB13B967F97499184};
inline Ir77GUID GUIDIIr77NavError{(static_cast<unsigned __int128>(0xFAE7FBEFD6EB4BDD) << 64) | 0x818C1067DC732867};
inline Ir77GUID GUIDIIr77NavTree{(static_cast<unsigned __int128>(0x5976409553644050) << 64) | 0x914D3A0B76331F07};
inline Ir77GUID GUIDIIr77NavSaveContext{(static_cast<unsigned __int128>(0x9D73469B70D84A08) << 64) | 0xB9546CB8D437BEC0};
inline Ir77GUID GUIDIIr77BindingContext{(static_cast<unsigned __int128>(0x78FC355515604604) << 64) | 0x96431F848660A102};
inline Ir77GUID GUIDIIr77OnInitNav{(static_cast<unsigned __int128>(0x9F4A084285354F40) << 64) | 0x95C9ADB0E9DA5D46};
inline Ir77GUID GUIDIIr77OnViewNav{(static_cast<unsigned __int128>(0x37FE22F352FD4F49) << 64) | 0xA5992D70C5FDE2CE};
inline Ir77GUID GUIDIIr77OnPostNav{(static_cast<unsigned __int128>(0x6F9726D45AB2424E) << 64) | 0x86DCFB7ED8179069};
inline Ir77GUID GUIDIIr77NavGuard{(static_cast<unsigned __int128>(0xA9E867933758496E) << 64) | 0xBB54C7B0A0074FDB};
inline Ir77GUID GUIDIIr77NixGuard{(static_cast<unsigned __int128>(0xA7390AAB757B4786) << 64) | 0x85377B5E440F732F};
inline Ir77GUID GUIDIIr77Load{(static_cast<unsigned __int128>(0x3E586591C30C48A4) << 64) | 0x901439EE9D29FF78};
inline Ir77GUID GUIDIIr77LoadInit{(static_cast<unsigned __int128>(0x92B9C510B88C4DC6) << 64) | 0x8FB6C6490D90C21F};
inline Ir77GUID GUIDIIr77LoadView{(static_cast<unsigned __int128>(0xAEA756D9E6F14A35) << 64) | 0x84228BBC0A4D8CA1};
inline Ir77GUID GUIDIIr77LoadPost{(static_cast<unsigned __int128>(0xB539E5956AC845AF) << 64) | 0x97473BC7465469B4};
inline Ir77GUID GUIDIIr77LoadGuard{(static_cast<unsigned __int128>(0xEEB6774CC0244BAA) << 64) | 0x9E60B363941100F2};
inline Ir77GUID GUIDIIr77NixLoadGuard{(static_cast<unsigned __int128>(0xBE1AD0E414EE41EF) << 64) | 0x83BCD6E45B44C281};
inline Ir77GUID GUIDIIr77RouteData{(static_cast<unsigned __int128>(0xA0321460B5954E13) << 64) | 0xAFC62AB4814DD71A};
inline Ir77GUID GUIDIIr77Route{(static_cast<unsigned __int128>(0x32DBEC0DCACE405B) << 64) | 0x9F6B3C75E77B93FA};
inline Ir77GUID GUIDIIr77Router{(static_cast<unsigned __int128>(0xD072FDDAC93E4CBD) << 64) | 0xAB963E38DAAA698F};
inline Ir77GUID GUIDIIr77RouterContext{(static_cast<unsigned __int128>(0x957F02DDE09E4DF7) << 64) | 0xAD593F2E01DDB7A7};
inline Ir77GUID GUIDIIr77RouterState{(static_cast<unsigned __int128>(0x69702CB1C4534479) << 64) | 0x8BDB93B8888F5B35};
inline Ir77GUID GUIDIIr77RouterError{(static_cast<unsigned __int128>(0x302C470B557D4BCF) << 64) | 0x886B1966BFDCBB0F};
inline Ir77GUID GUIDIIr77RouterOutlet{(static_cast<unsigned __int128>(0x582CA6A43B144674) << 64) | 0xAF3F67CB0682CF5E};
inline Ir77GUID GUIDIIr77OutletContext{(static_cast<unsigned __int128>(0x869D0ADCB87846EE) << 64) | 0x97658F273CFB99A0};
inline Ir77GUID GUIDIIr77OutletRedirect{(static_cast<unsigned __int128>(0xA5FE1ABD157541D9) << 64) | 0xBF5ABFB30EAABEB5};
inline Ir77GUID GUIDIIr77OutletResolve{(static_cast<unsigned __int128>(0xC0673F43F81444B7) << 64) | 0xBB75CE1337A3A3D7};
inline Ir77GUID GUIDIIr77OutletError{(static_cast<unsigned __int128>(0x5BA5FF4B77A74860) << 64) | 0x9BA544D5B4BBD5BB};
inline Ir77GUID GUIDIIr77DebugTrace{(static_cast<unsigned __int128>(0x02C50816BE2A4C42) << 64) | 0xAFD76621742A1559};

class IDIIr77Navigator : public IIr77Dictionary, public std::enable_shared_from_this<IDIIr77Navigator> {
   public:
    IDIIr77Navigator() {
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

    std::shared_ptr<IIr77Return const> MemberOfUuid(std::shared_ptr<IIr77GUID const>& uid) {
        seat_shared_uuid<&GUIDIIr77Navigator>(uid);
        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CollectionUuid(std::shared_ptr<IIr77GUID const>& uid) {
        seat_shared_uuid<&GUIDIIr77Navigator>(uid);
        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    IIr77GUID* const QueryInterface(IIr77GUID const* iid, std::shared_ptr<void>& obj) {
        if (iid == &GUIDIIr77Enlisted)
            obj = std::shared_ptr<IIr77Enlisted>(shared_from_this(), static_cast<IIr77Enlisted*>(this));

        else if (iid == &GUIDIIr77Dictionary)
            obj = std::shared_ptr<IIr77Dictionary>(shared_from_this(), static_cast<IIr77Dictionary*>(this));

        else if (iid == &GUIDIIr77Navigator)
            obj = std::shared_ptr<IDIIr77Navigator>(shared_from_this(), static_cast<IDIIr77Navigator*>(this));

        else
            return &GUIDQueryFailed;

        return &GUIDQuerySucceeded;
    }

    // ── IIr77Dictionary ──────────────────────────────────────────
   public:
    IIr77GUID const* Collection() const { return const_cast<IIr77GUID*>(static_cast<IIr77GUID const*>(&GUIDIIr77Navigator)); }

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
        {"GUIDIIr77Navigator", &GUIDIIr77Navigator},
        {"GUIDIIr77NavEvent", &GUIDIIr77NavEvent},
        {"GUIDIIr77NavState", &GUIDIIr77NavState},
        {"GUIDIIr77NavigatedState", &GUIDIIr77NavigatedState},
        {"GUIDIIr77NavCancel", &GUIDIIr77NavCancel},
        {"GUIDIIr77NavActive", &GUIDIIr77NavActive},
        {"GUIDIIr77NavError", &GUIDIIr77NavError},
        {"GUIDIIr77NavTree", &GUIDIIr77NavTree},
        {"GUIDIIr77NavSaveContext", &GUIDIIr77NavSaveContext},
        {"GUIDIIr77BindingContext", &GUIDIIr77BindingContext},
        {"GUIDIIr77OnInitNav", &GUIDIIr77OnInitNav},
        {"GUIDIIr77OnViewNav", &GUIDIIr77OnViewNav},
        {"GUIDIIr77OnPostNav", &GUIDIIr77OnPostNav},
        {"GUIDIIr77NavGuard", &GUIDIIr77NavGuard},
        {"GUIDIIr77NixGuard", &GUIDIIr77NixGuard},
        {"GUIDIIr77Load", &GUIDIIr77Load},
        {"GUIDIIr77LoadInit", &GUIDIIr77LoadInit},
        {"GUIDIIr77LoadView", &GUIDIIr77LoadView},
        {"GUIDIIr77LoadPost", &GUIDIIr77LoadPost},
        {"GUIDIIr77LoadGuard", &GUIDIIr77LoadGuard},
        {"GUIDIIr77NixLoadGuard", &GUIDIIr77NixLoadGuard},
        {"GUIDIIr77RouteData", &GUIDIIr77RouteData},
        {"GUIDIIr77Route", &GUIDIIr77Route},
        {"GUIDIIr77Router", &GUIDIIr77Router},
        {"GUIDIIr77RouterContext", &GUIDIIr77RouterContext},
        {"GUIDIIr77RouterState", &GUIDIIr77RouterState},
        {"GUIDIIr77RouterError", &GUIDIIr77RouterError},
        {"GUIDIIr77RouterOutlet", &GUIDIIr77RouterOutlet},
        {"GUIDIIr77OutletContext", &GUIDIIr77OutletContext},
        {"GUIDIIr77OutletRedirect", &GUIDIIr77OutletRedirect},
        {"GUIDIIr77OutletResolve", &GUIDIIr77OutletResolve},
        {"GUIDIIr77OutletError", &GUIDIIr77OutletError},
        {"GUIDIIr77DebugTrace", &GUIDIIr77DebugTrace},
    };
};

}  // namespace NSIr77RT