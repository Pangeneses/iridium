#pragma once

#include <chrono>
#include <memory>
#include <string>

#include "../interface/IIr77GUID.hpp"
#include "../interface/IIr77Dictionary.hpp"
#include "../interface/IIr77Enlisted.hpp"
#include "../interface/IIr77Return.hpp"

#include "../runtime/Ir77GUID.hpp"

namespace NSIr77RT {
inline Ir77GUID GUIDIr77RETURN{(static_cast<unsigned __int128>(0xBE11AEB5923A4854) << 64) | 0xB8B71CC80D56A296};

inline Ir77GUID GUIDIr77Unknown{(static_cast<unsigned __int128>(0x9E5DF7990B114A2B) << 64) | 0xAB3C11A3980128D3};
inline Ir77GUID GUIDIr77False{(static_cast<unsigned __int128>(0x5DE62A6FD9F849BD) << 64) | 0x80914DDBCAD9509A};
inline Ir77GUID GUIDIr77True{(static_cast<unsigned __int128>(0x45FC03965A374386) << 64) | 0xB82B8772F7180CA8};
inline Ir77GUID GUIDIr77IsEqual{(static_cast<unsigned __int128>(0x5722B3779F2743F3) << 64) | 0x8AD2A914D1CFE987};
inline Ir77GUID GUIDIr77IsLesser{(static_cast<unsigned __int128>(0x84B4D96612274B85) << 64) | 0x9EF459F16D84FBD7};
inline Ir77GUID GUIDIr77IsGreater{(static_cast<unsigned __int128>(0xCF21B52FE58A4E3E) << 64) | 0x87E5C1A919B37C8A};
inline Ir77GUID GUIDIr77InvalidArgument{(static_cast<unsigned __int128>(0x5F927FFD1B4F42CD) << 64) | 0x94F5E6CFCBD4B27B};
inline Ir77GUID GUIDIr77ValidArgument{(static_cast<unsigned __int128>(0x19369F2EC49343ED) << 64) | 0x8C1CED82BE10181C};
inline Ir77GUID GUIDIr77OperationFailed{(static_cast<unsigned __int128>(0xB72C862FDA2B45C8) << 64) | 0x92F58F257DB0974C};
inline Ir77GUID GUIDIr77OperationConflict{(static_cast<unsigned __int128>(0x8030334DB3684201) << 64) | 0x9FFBFAEC2635EC65};
inline Ir77GUID GUIDIr77OperationSucceeded{(static_cast<unsigned __int128>(0x6CD78C706C544EDB) << 64) | 0x8C76A83169BF7AFE};
inline Ir77GUID GUIDIr77InvalidOperation{(static_cast<unsigned __int128>(0x04DF984D34404C03) << 64) | 0xAF65CF939BCD85E0};
inline Ir77GUID GUIDIr77NotConfigured{(static_cast<unsigned __int128>(0x36D19823064E4F88) << 64) | 0xBC36C441B1D27410};
inline Ir77GUID GUIDIr77AlreadyConfigure{(static_cast<unsigned __int128>(0xC11449FCD86841D3) << 64) | 0xB27EE6CA72EA11CC};
inline Ir77GUID GUIDIr77ImproperlyConfiged{(static_cast<unsigned __int128>(0xC4246620B9FF4A96) << 64) | 0xB5D65EF203C0E881};
inline Ir77GUID GUIDIr77ProperlyConfigured{(static_cast<unsigned __int128>(0xDF9D797ECE424C6E) << 64) | 0x953E7103CE45E269};
inline Ir77GUID GUIDIr77MPVMError{(static_cast<unsigned __int128>(0x4AC74417AEBD4E10) << 64) | 0xAA81D76E97FBBCAA};
inline Ir77GUID GUIDIr77OutOfMemory{(static_cast<unsigned __int128>(0x39043756CA274E8B) << 64) | 0x9BB664363443CF9A};
inline Ir77GUID GUIDIr77OutOfRange{(static_cast<unsigned __int128>(0xC1A1E9E65B694A8A) << 64) | 0xBFF2951F514A651B};
inline Ir77GUID GUIDIr77KeyNotFound{(static_cast<unsigned __int128>(0xC8A81C3EA4754185) << 64) | 0x9AFC4EB0144CE6E2};
inline Ir77GUID GUIDIr77InvalidPointer{(static_cast<unsigned __int128>(0xB157017113304DE1) << 64) | 0xB80566756D3E75C1};
inline Ir77GUID GUIDIr77InvalidInterface{(static_cast<unsigned __int128>(0xC7E4A47E6A2D4CDF) << 64) | 0x86C203E22CDEBC98};
inline Ir77GUID GUIDIr77InvalidToken{(static_cast<unsigned __int128>(0x7A119C977C724F1B) << 64) | 0x9D135752009F5EB1};
inline Ir77GUID GUIDIr77Empty{(static_cast<unsigned __int128>(0x493D0559DF584979) << 64) | 0xB904030A6F8C0A73};
inline Ir77GUID GUIDIr77Dirty{(static_cast<unsigned __int128>(0x81E3FBB5EA384C27) << 64) | 0x9887B7CF86FAB846};
inline Ir77GUID GUIDIr77Sealed{(static_cast<unsigned __int128>(0x23B714FF6A4043FD) << 64) | 0xB3303743F0C955B5};
inline Ir77GUID GUIDIr77Invalidated{(static_cast<unsigned __int128>(0x786212027B2C4207) << 64) | 0xAFE2908529F3D087};
inline Ir77GUID GUIDIr77NotSystemModule{(static_cast<unsigned __int128>(0x8377EDB1189545BA) << 64) | 0x90BF1606AAD7BAB9};
inline Ir77GUID GUIDIr77ModuleNotFound{(static_cast<unsigned __int128>(0x51E53030191E4591) << 64) | 0xA54E1FB0FFAE5137};
inline Ir77GUID GUIDIr77RTClassNotFound{(static_cast<unsigned __int128>(0x058DAD2CB3234B9E) << 64) | 0x96FC861E9EBF1879};
inline Ir77GUID GUIDIr77ProcessNotFound{(static_cast<unsigned __int128>(0xAF016669F1194C26) << 64) | 0x8503859ED6AB5B59};
inline Ir77GUID GUIDIr77ProcessNotComplete{(static_cast<unsigned __int128>(0x517EA2D3961F4AD9) << 64) | 0x9748754CE55A52C8};
inline Ir77GUID GUIDIr77ProcessComplete{(static_cast<unsigned __int128>(0xE5912CF9AE514335) << 64) | 0x888FBF17B648EB28};
inline Ir77GUID GUIDIr77ImageNotLoaded{(static_cast<unsigned __int128>(0xA169E9E2E8D44C55) << 64) | 0xAD33E5C1FBC388FF};
inline Ir77GUID GUIDIr77MPVMFailed{(static_cast<unsigned __int128>(0x0A23A540031D4394) << 64) | 0x836400DC585AA362};
inline Ir77GUID GUIDIr77MPVMSucceeded{(static_cast<unsigned __int128>(0xAB7C4D183BAB45BC) << 64) | 0x8B96A1407CA08A43};
inline Ir77GUID GUIDIr77DirDoesntExist{(static_cast<unsigned __int128>(0xC80CC92B771441A8) << 64) | 0x9471E17CD874F0AE};
inline Ir77GUID GUIDIr77DirAlreadyExists{(static_cast<unsigned __int128>(0xDEF5D37EF0FF411A) << 64) | 0x873C65A09AED089A};
inline Ir77GUID GUIDIr77PageAlreadyExist{(static_cast<unsigned __int128>(0xA85AA7481A314C0A) << 64) | 0xB19D7C850FFC960C};
inline Ir77GUID GUIDIr77NoFileOpen{(static_cast<unsigned __int128>(0x51312ED8FBE84B2B) << 64) | 0xA36F0D3CEB2EBB76};
inline Ir77GUID GUIDIr77FileDoesntExist{(static_cast<unsigned __int128>(0x73DECBE27CE14CB7) << 64) | 0x9ECC3413119492F6};
inline Ir77GUID GUIDIr77FileAlreadyExist{(static_cast<unsigned __int128>(0x74F793901CA14AD8) << 64) | 0xB5352964112AA7AA};
inline Ir77GUID GUIDIr77FileHandleOpen{(static_cast<unsigned __int128>(0x36A0A7EB59944BE5) << 64) | 0x97B0E7F14AC420E2};
inline Ir77GUID GUIDIr77FileBadFormatting{(static_cast<unsigned __int128>(0xB3B467CEE47F4557) << 64) | 0xB3457DA5F594249B};

class IDIr77Return : public IIr77Dictionary, public std::enable_shared_from_this<IDIr77Return> {
   public:
    IDIr77Return() {
        m_enlisted = std::chrono::system_clock::now();

        try {
            m_enlisted_uuid.Generate();
        } catch (std::invalid_argument a) {
            throw a;
        }
    }

    // ── IIr77Enlisted ────────────────────────────────────────────
   public:
    std::shared_ptr<IIr77Return const> EnlistedAs(std::shared_ptr<IIr77GUID const>& uid) const {
        static Ir77GUID s_uid{(static_cast<unsigned __int128>(0xEDA0BCD07ED54DE1) << 64) | 0xA97AD08DAA70563B};
        uid.reset(&s_uid, [](auto*) {});
        return nullptr;
    }

    std::shared_ptr<IIr77Return const> EnlistedUuid(std::shared_ptr<IIr77GUID const>& uid) {
        uid.reset(reinterpret_cast<IIr77GUID const*>(&m_enlisted_uuid), [](auto*) {});
        return nullptr;
    }

    std::shared_ptr<IIr77Return const> EnlistedChrono(std::chrono::system_clock::time_point& t) {
        t = m_enlisted;
        return nullptr;
    }

    std::shared_ptr<IIr77Return const> DelistedChrono(std::chrono::system_clock::time_point& t, std::shared_ptr<IIr77Return const>& condition) {
        t = m_delisted;

        condition = m_invalidation_condition;

        return nullptr;
    }

    std::shared_ptr<IIr77Return const> SetSender(std::shared_ptr<IIr77Enlisted const>& sender) {
        m_sender = sender;

        if (!m_valid) return nullptr;

        return nullptr;
    }

    std::shared_ptr<IIr77Return const> GetSender(std::shared_ptr<IIr77Enlisted const>& sender) const {
        sender = m_sender;

        if (!m_valid) return nullptr;

        return nullptr;
    }

    std::shared_ptr<IIr77Return const> SetSenderMsg(std::string const& msg) {
        m_message = msg;

        if (!m_valid) return nullptr;

        return nullptr;
    }

    std::shared_ptr<IIr77Return const> GetSenderMsg(std::string& msg) const {
        msg = m_message;

        if (!m_valid) return nullptr;

        return nullptr;
    }

    std::shared_ptr<IIr77Return const> MemberUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        static Ir77GUID s_uid{(static_cast<unsigned __int128>(0xBE11AEB5923A4854) << 64) | 0xB8B71CC80D56A296};
        uid.reset(&s_uid, [](auto*) {});
        return nullptr;
    }

    std::shared_ptr<IIr77Return const> CollectionUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        static Ir77GUID s_uid{(static_cast<unsigned __int128>(0xBE11AEB5923A4854) << 64) | 0xB8B71CC80D56A296};
        uid.reset(&s_uid, [](auto*) {});
        return nullptr;
    }

    Ir77GUID QueryInterface(Ir77GUID iid, std::shared_ptr<void>& obj) {
        static Ir77GUID s_enlisted{(static_cast<unsigned __int128>(0xC052EA89334C43AE) << 64) | 0x9F976D7E354FC406};
        static Ir77GUID s_dictionary{(static_cast<unsigned __int128>(0xEDA0BCD07ED54DE1) << 64) | 0xA97AD08DAA70563B};
        static Ir77GUID s_mpvm{(static_cast<unsigned __int128>(0x9347D36C009246A9) << 64) | 0x93ADB344A22135D2};

        if (iid == &s_enlisted)
            obj = std::shared_ptr<IIr77Enlisted>(shared_from_this(), static_cast<IIr77Enlisted*>(this));
        else if (iid == &s_dictionary)
            obj = std::shared_ptr<IIr77Dictionary>(shared_from_this(), static_cast<IIr77Dictionary*>(this));
        else if (iid == &s_mpvm)
            obj = std::shared_ptr<IDIr77Return>(shared_from_this(), static_cast<IDIr77Return*>(this));
        else
            return GUIDQueryFailed;

        return GUIDQuerySucceeded;
    }

    // ── IIr77Dictionary ──────────────────────────────────────────
   public:
    IIr77GUID const* Collection() const { return const_cast<IIr77GUID*>(static_cast<IIr77GUID const*>(&GUIDIr77RETURN)); }

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

    std::shared_ptr<IIr77Return> m_invalidation_condition{nullptr};

    std::map<std::string, IIr77GUID const*> m_index{
        {"GUIDIr77RETURN", &GUIDIr77RETURN},
        {"GUIDIr77Unknown", &GUIDIr77Unknown},
        {"GUIDIr77False", &GUIDIr77False},
        {"GUIDIr77True", &GUIDIr77True},
        {"GUIDIr77IsEqual", &GUIDIr77IsEqual},
        {"GUIDIr77IsLesser", &GUIDIr77IsLesser},
        {"GUIDIr77IsGreater", &GUIDIr77IsGreater},
        {"GUIDIr77InvalidArgument", &GUIDIr77InvalidArgument},
        {"GUIDIr77ValidArgument", &GUIDIr77ValidArgument},
        {"GUIDIr77OperationFailed", &GUIDIr77OperationFailed},
        {"GUIDIr77OperatorConflic", &GUIDIr77OperationConflict},
        {"GUIDIr77OperationSucceeded", &GUIDIr77OperationSucceeded},
        {"GUIDIr77InvalidOperation", &GUIDIr77InvalidOperation},
        {"GUIDIr77NotConfigured", &GUIDIr77NotConfigured},
        {"GUIDIr77AlreadyConfigure", &GUIDIr77AlreadyConfigure},
        {"GUIDIr77ImproperlyConfiged", &GUIDIr77ImproperlyConfiged},
        {"GUIDIr77ProperlyConfigur", &GUIDIr77ProperlyConfigured},
        {"GUIDIr77MPVMError", &GUIDIr77MPVMError},
        {"GUIDIr77OutOfMemory", &GUIDIr77OutOfMemory},
        {"GUIDIr77OutOfRange", &GUIDIr77OutOfRange},
        {"GUIDIr77KeyNotFound", &GUIDIr77KeyNotFound},
        {"GUIDIr77InvalidPointer", &GUIDIr77InvalidPointer},
        {"GUIDIr77InvalidInterface", &GUIDIr77InvalidInterface},
        {"GUIDIr77InvalidToken", &GUIDIr77InvalidToken},
        {"GUIDIr77Empty", &GUIDIr77Empty},
        {"GUIDIr77Dirty", &GUIDIr77Dirty},
        {"GUIDIr77Sealed", &GUIDIr77Sealed},
        {"GUIDIr77Invalidated", &GUIDIr77Invalidated},
        {"GUIDIr77NotSystemModule", &GUIDIr77NotSystemModule},
        {"GUIDIr77ModuleNotFound", &GUIDIr77ModuleNotFound},
        {"GUIDIr77RTClassNotFound", &GUIDIr77RTClassNotFound},
        {"GUIDIr77ProcessNotFound", &GUIDIr77ProcessNotFound},
        {"GUIDIr77ProcessNotComplete", &GUIDIr77ProcessNotComplete},
        {"GUIDIr77ProcessComplete", &GUIDIr77ProcessComplete},
        {"GUIDIr77ImageNotLoaded", &GUIDIr77ImageNotLoaded},
        {"GUIDIr77MPVMFailed", &GUIDIr77MPVMFailed},
        {"GUIDIr77MPVMSucceeded", &GUIDIr77MPVMSucceeded},
        {"GUIDIr77DirDoesntExist", &GUIDIr77DirDoesntExist},
        {"GUIDIr77DirAlreadyExists", &GUIDIr77DirAlreadyExists},
        {"GUIDIr77PageAlreadyExist", &GUIDIr77PageAlreadyExist},
        {"GUIDIr77NoFileOpen", &GUIDIr77NoFileOpen},
        {"GUIDIr77FileDoesntExist", &GUIDIr77FileDoesntExist},
        {"GUIDIr77FileAlreadyExist", &GUIDIr77FileAlreadyExist},
        {"GUIDIr77FileHandleOpen", &GUIDIr77FileHandleOpen},
        {"GUIDIr77FileBadFormattin", &GUIDIr77FileBadFormatting},
    };
};

}  // namespace NSIr77RT
