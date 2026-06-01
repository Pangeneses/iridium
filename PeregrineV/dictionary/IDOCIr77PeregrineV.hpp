#pragma once

#include <chrono>
#include <map>
#include <memory>
#include <string>

#include "../../Ir77RT/dictionary/IDIIr77MPVM.hpp"

#include "../../Ir77RT/interface/IIr77GUID.hpp"
#include "../../Ir77RT/interface/IIr77Dictionary.hpp"
#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../../Ir77RT/runtime/Ir77GUID.hpp"
#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"
#include "../../Ir77RT/runtime/Ir77Return.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

inline Ir77GUID GUIDOCIr77PeregrineV{(static_cast<unsigned __int128>(0xA3F8C2E1D4B67059) << 64) | 0x1E9A3C7F82D4B605};

/* LIFETIME */
inline Ir77GUID GUIDOCIr77Initialize{(static_cast<unsigned __int128>(0x7B2E9F4A1C8D3E56) << 64) | 0xF0A5C3D2E7B49018};

/* ASSET */
inline Ir77GUID GUIDOCIr77AssetUpload{(static_cast<unsigned __int128>(0xa101bebaf62c47de) << 64) | 0x9ed95eaf9b1f2344};

/* PUMP */
inline Ir77GUID GUIDOCIr77PumpNextFrame{(static_cast<unsigned __int128>(0x4D1A8F3C7E2B9056) << 64) | 0xC5E0A7F3D2B81694};
inline Ir77GUID GUIDOCIr77PumpCommand{(static_cast<unsigned __int128>(0x9E3C5A1F8D4B7026) << 64) | 0x2F7A4E9C1D3B8057};
inline Ir77GUID GUIDOCIr77PumpBarrier{(static_cast<unsigned __int128>(0x6F2D4E8B1A9C3057) << 64) | 0xE4B7F1A3C9D20568};
inline Ir77GUID GUIDOCIr77PumpSubmit{(static_cast<unsigned __int128>(0xB5A7E3F9C2D41068) << 64) | 0x3A8C1F5D7E9B4026};
inline Ir77GUID GUIDOCIr77PumpCobalt{(static_cast<unsigned __int128>(0x1C9E7A4F3D8B2056) << 64) | 0x7F3E5A9C2D4B1068};
inline Ir77GUID GUIDOCIr77PumpFrame{(static_cast<unsigned __int128>(0xD4B2F8E1A3C97056) << 64) | 0x5E1C9A7F4D3B8026};
inline Ir77GUID GUIDOCIr77PumpPresent{(static_cast<unsigned __int128>(0x8A3E1C9F5D7B4026) << 64) | 0xF2D4B7A1C3E95068};

class IDOCIr77PeregrineV : public Ir77Enlisted, public IIr77Dictionary, public std::enable_shared_from_this<IDIIr77MPVM> {
   public:
    IDOCIr77PeregrineV() {
        m_enlisted = std::chrono::system_clock::now();

        try {
            m_enlisted_uuid.Generate();
        } catch (std::invalid_argument a) {
            throw a;
        }
    }

    // ── IIr77Enlisted ────────────────────────────────────────────
   public:
    std::shared_ptr<IIr77Return const> MemberOfUuid(std::shared_ptr<IIr77GUID const>& uid) {
        seat_shared_uuid<&GUIDOCIr77PeregrineV>(uid);
        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CollectionUuid(std::shared_ptr<IIr77GUID const>& uid) {
        seat_shared_uuid<&GUIDOCIr77PeregrineV>(uid);
        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    IIr77GUID* const QueryInterface(IIr77GUID const* iid, std::shared_ptr<void>& obj) {
        if (iid == &GUIDIIr77Enlisted)
            obj = std::shared_ptr<IIr77Enlisted>(shared_from_this(), static_cast<IIr77Enlisted*>(this));

        else if (iid == &GUIDIIr77Dictionary)
            obj = std::shared_ptr<IIr77Dictionary>(shared_from_this(), static_cast<IIr77Dictionary*>(this));

        else if (iid == &GUIDOCIr77PeregrineV)
            obj = std::shared_ptr<IDOCIr77PeregrineV>(shared_from_this(), static_cast<IDOCIr77PeregrineV*>(this));

        else
            return &GUIDQueryFailed;

        return &GUIDQuerySucceeded;
    }

    // ── IIr77Dictionary ──────────────────────────────────────────
   public:
    IIr77GUID const* Collection() const { return const_cast<IIr77GUID*>(static_cast<IIr77GUID const*>(&GUIDOCIr77PeregrineV)); }

    IIr77GUID const* Member(std::string const& member) const { return m_index.at(member); }

    std::map<std::string, IIr77GUID const*> const& Index() const { return m_index; }

   private:
    std::map<std::string, IIr77GUID const*> m_index{
        {"GUIDOCIr77PeregrineV", &GUIDOCIr77PeregrineV},
        {"GUIDOCIr77Initialize", &GUIDOCIr77Initialize},
        {"GUIDOCIr77AssetUpload", &GUIDOCIr77AssetUpload},
        {"GUIDOCIr77PumpNextFrame", &GUIDOCIr77PumpNextFrame},
        {"GUIDOCIr77PumpCommand", &GUIDOCIr77PumpCommand},
        {"GUIDOCIr77PumpBarrier", &GUIDOCIr77PumpBarrier},
        {"GUIDOCIr77PumpSubmit", &GUIDOCIr77PumpSubmit},
        {"GUIDOCIr77PumpCobalt", &GUIDOCIr77PumpCobalt},
        {"GUIDOCIr77PumpFrame", &GUIDOCIr77PumpFrame},
        {"GUIDOCIr77PumpPresent", &GUIDOCIr77PumpPresent},
    };
};
}  // namespace NSIr77REDOS