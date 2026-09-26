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

inline Ir77GUID GUIDIIr77PeregrineV{(static_cast<unsigned __int128>(0xA67D31AC1DDF4A7E) << 64) | 0xB628CE2A9641A8A2};
inline Ir77GUID GUIDIIr77PVPaint{(static_cast<unsigned __int128>(0x3B1F109C15C24A2D) << 64) | 0x8D13DF17E5848FD2};
inline Ir77GUID GUIDIIr77PVAsset{(static_cast<unsigned __int128>(0x230418A411684215) << 64) | 0xA67EB2411A2FCA48};
inline Ir77GUID GUIDIIr77PVCEF{(static_cast<unsigned __int128>(0x50C4C87A4D8A41B5) << 64) | 0xA1481E8D50E5DA94};

/*************************************************************************************************************************************************************/
inline Ir77GUID GUIDIIr77PVInstance{(static_cast<unsigned __int128>(0xB1136C9B855A803E) << 64) | 0x7A5E662149F03242};
inline Ir77GUID GUIDIIr77PVDevice{(static_cast<unsigned __int128>(0xE29226960D283AA0) << 64) | 0x55C041024DC6C909};
inline Ir77GUID GUIDIIr77PVSwapchain{(static_cast<unsigned __int128>(0x798E5BB1B58EC0BE) << 64) | 0xA69DFA5475A9111A};
inline Ir77GUID GUIDIIr77PVRenderPass{(static_cast<unsigned __int128>(0xE66F8BE0C5FC877E) << 64) | 0x880D9BEBB37CD1A7};
inline Ir77GUID GUIDIIr77PVOverlay{(static_cast<unsigned __int128>(0x5D4F5B76C7174A65) << 64) | 0xAA00D922518E8572};
inline Ir77GUID GUIDIIr77PVBuffer{(static_cast<unsigned __int128>(0xA665143705AD4866) << 64) | 0xB09007114EB3F257};
inline Ir77GUID GUIDIIr77PVCompute{(static_cast<unsigned __int128>(0x4CF567680DCB4A88) << 64) | 0x9A25F925F2778211};
inline Ir77GUID GUIDIIr77PVDescriptorSet{(static_cast<unsigned __int128>(0xC7EFCD677B114AC8) << 64) | 0x9EF48FEFF8B288F8};
inline Ir77GUID GUIDIIr77PVLayout{(static_cast<unsigned __int128>(0xF1FC7315669F4091) << 64) | 0x8DC0297D45CD38CD};
inline Ir77GUID GUIDIIr77PVPipeline{(static_cast<unsigned __int128>(0xB10BE474EAC70D1A) << 64) | 0xFEFDBF8EA6C37F9D};
inline Ir77GUID GUIDIIr77PVCmdBuffer{(static_cast<unsigned __int128>(0xF71D19D109D2114D) << 64) | 0x1186ADBD29983077};
inline Ir77GUID GUIDIIr77PVTexture{(static_cast<unsigned __int128>(0x3748583BB9C81720) << 64) | 0x08ACB330BEBFAC4F};
inline Ir77GUID GUIDIIr77PVShader{(static_cast<unsigned __int128>(0xF4D60A74F19932D3) << 64) | 0xBEADFF0A12D9F258};
inline Ir77GUID GUIDIIr77PVMaterial{(static_cast<unsigned __int128>(0x033C555AF2E0ABF5) << 64) | 0x1EF45C1405FEA446};
inline Ir77GUID GUIDIIr77PVMesh{(static_cast<unsigned __int128>(0xF8C99EAD7B7CF55C) << 64) | 0xC944FDC0A7F18F94};

class IDIIr77PeregrineV : public Ir77Enlisted, public IIr77Dictionary, public std::enable_shared_from_this<IDIIr77PeregrineV> {
   public:
    IDIIr77PeregrineV() {
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

    std::shared_ptr<IIr77Return const> MemberUuid(std::shared_ptr<IIr77GUID const>& uid) {
        seat_shared_uuid<&GUIDIIr77PeregrineV>(uid);
        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CollectionUuid(std::shared_ptr<IIr77GUID const>& uid) {
        seat_shared_uuid<&GUIDIIr77PeregrineV>(uid);
        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    IIr77GUID* const QueryInterface(IIr77GUID const* iid, std::shared_ptr<void>& obj) {
        if (iid == &GUIDIIr77Enlisted)
            obj = std::shared_ptr<IIr77Enlisted>(shared_from_this(), static_cast<IIr77Enlisted*>(this));

        else if (iid == &GUIDIIr77Dictionary)
            obj = std::shared_ptr<IIr77Dictionary>(shared_from_this(), static_cast<IIr77Dictionary*>(this));

        else if (iid == &GUIDIIr77PeregrineV)
            obj = std::shared_ptr<IDIIr77PeregrineV>(shared_from_this(), static_cast<IDIIr77PeregrineV*>(this));

        else
            return &GUIDQueryFailed;

        return &GUIDQuerySucceeded;
    }

    // ── IIr77Dictionary ──────────────────────────────────────────
   public:
    IIr77GUID const* Collection() const { return const_cast<IIr77GUID*>(static_cast<IIr77GUID const*>(&GUIDIIr77PeregrineV)); }

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
        {"GUIDIIr77PeregrineV", &GUIDIIr77PeregrineV},
        {"GUIDIIr77PVPaint", &GUIDIIr77PVPaint},
        {"GUIDIIr77PVAsset", &GUIDIIr77PVAsset},
        {"GUIDIIr77PVCEF", &GUIDIIr77PVCEF},
        {"GUIDIIr77PVInstance", &GUIDIIr77PVInstance},
        {"GUIDIIr77PVDevice", &GUIDIIr77PVDevice},
        {"GUIDIIr77PVSwapchain", &GUIDIIr77PVSwapchain},
        {"GUIDIIr77PVRenderPass", &GUIDIIr77PVRenderPass},
        {"GUIDIIr77PVOverlay", &GUIDIIr77PVOverlay},
        {"GUIDIIr77PVBuffer", &GUIDIIr77PVBuffer},
        {"GUIDIIr77PVCompute", &GUIDIIr77PVCompute},
        {"GUIDIIr77PVDescriptorSet", &GUIDIIr77PVDescriptorSet},
        {"GUIDIIr77PVLayout", &GUIDIIr77PVLayout},
        {"GUIDIIr77PVPipeline", &GUIDIIr77PVPipeline},
        {"GUIDIIr77PVTexture", &GUIDIIr77PVTexture},
        {"GUIDIIr77PVCmdBuffer", &GUIDIIr77PVCmdBuffer},
        {"GUIDIIr77PVShader", &GUIDIIr77PVShader},
        {"GUIDIIr77PVMaterial", &GUIDIIr77PVMaterial},
        {"GUIDIIr77PVMesh", &GUIDIIr77PVMesh},
    };
};

}  // namespace NSIr77PeregrineV