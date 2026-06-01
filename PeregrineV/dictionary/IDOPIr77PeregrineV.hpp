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

inline Ir77GUID GUIDOPIr77PeregrineV{(static_cast<unsigned __int128>(0x7371AD5F4E4C4219) << 64) | 0x8EEB4250394B260D};

inline Ir77GUID GUIDOPIr77PVLifetime{(static_cast<unsigned __int128>(0x7371AD5F4E4C4219) << 64) | 0x8EEB4250394B260D};
inline Ir77GUID GUIDOPIr77PVAsset{(static_cast<unsigned __int128>(0x7371AD5F4E4C4219) << 64) | 0x8EEB4250394B260D};
inline Ir77GUID GUIDOPIr77PVPump{(static_cast<unsigned __int128>(0x7371AD5F4E4C4219) << 64) | 0x8EEB4250394B260D};

class IDOPIr77PeregrineV : public Ir77Enlisted, public IIr77Dictionary, public std::enable_shared_from_this<IDIIr77MPVM> {
   public:
    IDOPIr77PeregrineV() {
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
        seat_shared_uuid<&GUIDOPIr77PeregrineV>(uid);
        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CollectionUuid(std::shared_ptr<IIr77GUID const>& uid) {
        seat_shared_uuid<&GUIDOPIr77PeregrineV>(uid);
        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    IIr77GUID* const QueryInterface(IIr77GUID const* iid, std::shared_ptr<void>& obj) {
        if (iid == &GUIDIIr77Enlisted)
            obj = std::shared_ptr<IIr77Enlisted>(shared_from_this(), static_cast<IIr77Enlisted*>(this));

        else if (iid == &GUIDIIr77Dictionary)
            obj = std::shared_ptr<IIr77Dictionary>(shared_from_this(), static_cast<IIr77Dictionary*>(this));

        else if (iid == &GUIDOPIr77PeregrineV)
            obj = std::shared_ptr<IDOPIr77PeregrineV>(shared_from_this(), static_cast<IDOPIr77PeregrineV*>(this));

        else
            return &GUIDQueryFailed;

        return &GUIDQuerySucceeded;
    }

    // ── IIr77Dictionary ──────────────────────────────────────────
   public:
    IIr77GUID const* Collection() const { return const_cast<IIr77GUID*>(static_cast<IIr77GUID const*>(&GUIDOPIr77PeregrineV)); }

    IIr77GUID const* Member(std::string const& member) const { return m_index.at(member); }

    std::map<std::string, IIr77GUID const*> const& Index() const { return m_index; }

   private:
    std::map<std::string, IIr77GUID const*> m_index{
        {"GUIDOPIr77PeregrineV", &GUIDOPIr77PeregrineV},
        {"GUIDOPIr77PVLifetime", &GUIDOPIr77PVLifetime},
        {"GUIDOPIr77PVAsset", &GUIDOPIr77PVAsset},
        {"GUIDOPIr77PVPump", &GUIDOPIr77PVPump},
    };
};

}  // namespace NSIr77REDOS