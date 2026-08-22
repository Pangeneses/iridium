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

inline Ir77GUID GUIDSRVIr77PeregrineV{(static_cast<unsigned __int128>(0x3426061D055D4582) << 64) | 0xB6A65FF13C183CE8};
inline Ir77GUID GUIDSRVIr77WindowInfo{(static_cast<unsigned __int128>(0x1432B5EA3EE74F22) << 64) | 0x8F6C32449E78E9A1};
inline Ir77GUID GUIDSRVIr77DeviceInfo{(static_cast<unsigned __int128>(0x29894DC0F3244520) << 64) | 0xA111DACDC24E37E7};
inline Ir77GUID GUIDSRVIr77ShaderStack{(static_cast<unsigned __int128>(0xDB54322D4160464C) << 64) | 0x8F6AAA956BBA27A2};
inline Ir77GUID GUIDSRVIr77MaterialStack{(static_cast<unsigned __int128>(0x1C436E5DCE604214) << 64) | 0x94304FCD61C3D66E};
inline Ir77GUID GUIDSRVIr77BufferStack{(static_cast<unsigned __int128>(0xCF0737A5F3AA4435) << 64) | 0x97A76A0EF5E1A40F};
inline Ir77GUID GUIDSRVIr77ComputeStack{(static_cast<unsigned __int128>(0x12159939E5194E1E) << 64) | 0x9DE1F39DAC77B07D};

class IDSRVIr77PeregrineV : public Ir77Enlisted, public IIr77Dictionary, public std::enable_shared_from_this<IDIIr77MPVM> {
   public:
    IDSRVIr77PeregrineV() {
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
        seat_shared_uuid<&GUIDIIr77Dictionary>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> MemberOfUuid(std::shared_ptr<IIr77GUID const>& uid) {
        seat_shared_uuid<&GUIDSRVIr77PeregrineV>(uid);
        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CollectionUuid(std::shared_ptr<IIr77GUID const>& uid) {
        seat_shared_uuid<&GUIDSRVIr77PeregrineV>(uid);
        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    IIr77GUID* const QueryInterface(IIr77GUID const* iid, std::shared_ptr<void>& obj) {
        if (iid == &GUIDIIr77Enlisted)
            obj = std::shared_ptr<IIr77Enlisted>(shared_from_this(), static_cast<IIr77Enlisted*>(this));

        else if (iid == &GUIDIIr77Dictionary)
            obj = std::shared_ptr<IIr77Dictionary>(shared_from_this(), static_cast<IIr77Dictionary*>(this));

        else if (iid == &GUIDSRVIr77PeregrineV)
            obj = std::shared_ptr<IDSRVIr77PeregrineV>(shared_from_this(), static_cast<IDSRVIr77PeregrineV*>(this));

        else
            return &GUIDQueryFailed;

        return &GUIDQuerySucceeded;
    }

    // ── IIr77Dictionary ──────────────────────────────────────────
   public:
    IIr77GUID const* Collection() const { return const_cast<IIr77GUID*>(static_cast<IIr77GUID const*>(&GUIDSRVIr77PeregrineV)); }

    IIr77GUID const* Member(std::string const& member) const { return m_index.at(member); }

    std::map<std::string, IIr77GUID const*> const& Index() const { return m_index; }

   private:
    std::map<std::string, IIr77GUID const*> m_index{
        {"GUIDSRVIr77PeregrineV", &GUIDSRVIr77PeregrineV},   {"GUIDSRVIr77PeregrineV", &GUIDSRVIr77PeregrineV},
        {"GUIDSRVIr77WindowInfo", &GUIDSRVIr77WindowInfo},   {"GUIDSRVIr77DeviceInfo", &GUIDSRVIr77DeviceInfo},
        {"GUIDSRVIr77ShaderStack", &GUIDSRVIr77ShaderStack}, {"GUIDSRVIr77MaterialStack", &GUIDSRVIr77MaterialStack},
        {"GUIDSRVIr77BufferStack", &GUIDSRVIr77BufferStack}, {"GUIDSRVIr77ComputeStack", &GUIDSRVIr77ComputeStack},

    };
};

}  // namespace NSIr77PeregrineV