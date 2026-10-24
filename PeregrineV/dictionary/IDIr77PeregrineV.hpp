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
inline Ir77GUID GUIDIr77PeregrineV{(static_cast<unsigned __int128>(0x3F9A72C4E1B85D06) << 64) | 0xA2C7F03951E84B1D};

/*************************************************************************************************************************************************************/
inline Ir77GUID GUIDIr77PVInit{(static_cast<unsigned __int128>(0x171ADB88436A4D15) << 64) | 0x97765EBAA0C7DE09};
inline Ir77GUID GUIDIr77PVInstance{(static_cast<unsigned __int128>(0x7D4E91A3C6F28B05) << 64) | 0x1B9E50A472C3D8F6};
inline Ir77GUID GUIDIr77PVDevice{(static_cast<unsigned __int128>(0xC2B74F8E3A916D05) << 64) | 0x5F3A10E274B9C806};
inline Ir77GUID GUIDIr77PVSwapchain{(static_cast<unsigned __int128>(0x4A9C15E7F3806D2B) << 64) | 0x8C3E72B1054FA96D};
inline Ir77GUID GUIDIr77PVRenderPass{(static_cast<unsigned __int128>(0x5F2A91D8E4703C6B) << 64) | 0x0B8E53C1F96A4D72};
inline Ir77GUID GUIDIr77PVOverlay{(static_cast<unsigned __int128>(0x5755F1BBCF6F4E4D) << 64) | 0xB88960386284D17C};
inline Ir77GUID GUIDIr77PVBuffer{(static_cast<unsigned __int128>(0xD1A93F6E4C807B25) << 64) | 0x064BCE8317F95A2D};
inline Ir77GUID GUIDIr77PVCompute{(static_cast<unsigned __int128>(0x6F520D9B3E71A8C4) << 64) | 0xA8D4731F50C2E96B};
inline Ir77GUID GUIDIr77PVDescriptorSet{(static_cast<unsigned __int128>(0xA8E21D0018AA4998) << 64) | 0xADDDE66142752D4C};
inline Ir77GUID GUIDIr77PVLayout{(static_cast<unsigned __int128>(0xF9C41B8E3A706D25) << 64) | 0x53BD70F1248A9CE6};
inline Ir77GUID GUIDIr77PVPipeline{(static_cast<unsigned __int128>(0x7B3F90D2C5A418E6) << 64) | 0x4A8E610F93B2D75C};
inline Ir77GUID GUIDIr77PVPipelineCache{(static_cast<unsigned __int128>(0x4A8BE9D7C5864E4E) << 64) | 0x9DAF83B8725FE597};
inline Ir77GUID GUIDIr77PVPipelineCPT{(static_cast<unsigned __int128>(0x57FC2CBC7273408B) << 64) | 0x9A8AB5F638EA5DBB};
inline Ir77GUID GUIDIr77PVPipelineRay{(static_cast<unsigned __int128>(0x6840B8B4F51A4C90) << 64) | 0xB774318E8E8DA57A};
inline Ir77GUID GUIDIr77PVCmdBuffer{(static_cast<unsigned __int128>(0x83E6A0D2F1794C5B) << 64) | 0xC1A4E7930F2D85B6};
inline Ir77GUID GUIDIr77PVTexture{(static_cast<unsigned __int128>(0x6092FBAA42AB407D) << 64) | 0x86EC49B4247721D3};
inline Ir77GUID GUIDIr77PVShader{(static_cast<unsigned __int128>(0xC6E4819A3F720B5D) << 64) | 0x1D30A7F852C9E4B6};
inline Ir77GUID GUIDIr77PVMaterial{(static_cast<unsigned __int128>(0x3A5D72F1E9C840B6) << 64) | 0x8F2BC4A1057E93D6};
inline Ir77GUID GUIDIr77PVGeometry{(static_cast<unsigned __int128>(0xF1BA46E129CB4367) << 64) | 0xB63BE55CFE4F84A5};
inline Ir77GUID GUIDIr77PVTransfer{(static_cast<unsigned __int128>(0x81163A56E7134832) << 64) | 0x8F4126169ED7A5CF};
inline Ir77GUID GUIDIr77PVPaint{(static_cast<unsigned __int128>(0xC440AC67BB674B9F) << 64) | 0xB3189D0572DE97E4};
inline Ir77GUID GUIDIr77PVCEF{(static_cast<unsigned __int128>(0xE39ABBA104F7426F) << 64) | 0x8827C847BD1F8936};

class IDIr77PeregrineV : public Ir77Enlisted, public IIr77Dictionary, public std::enable_shared_from_this<IDIr77PeregrineV> {
   public:
    IDIr77PeregrineV() {
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

    std::shared_ptr<IIr77Return const> MemberUuid(std::shared_ptr<IIr77GUID const>& uid) {
        seat_shared_uuid<&GUIDIr77PeregrineV>(uid);
        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CollectionUuid(std::shared_ptr<IIr77GUID const>& uid) {
        seat_shared_uuid<&GUIDIr77PeregrineV>(uid);
        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    Ir77GUID QueryInterface(Ir77GUID iid, std::shared_ptr<void>& obj) {
        if (iid == GUIDIIr77Enlisted)
            obj = std::shared_ptr<IIr77Enlisted>(shared_from_this(), static_cast<IIr77Enlisted*>(this));

        else if (iid == GUIDIIr77Dictionary)
            obj = std::shared_ptr<IIr77Dictionary>(shared_from_this(), static_cast<IIr77Dictionary*>(this));

        else if (iid == GUIDIr77PeregrineV)
            obj = std::shared_ptr<IDIr77PeregrineV>(shared_from_this(), static_cast<IDIr77PeregrineV*>(this));

        else
            return GUIDQueryFailed;

        return GUIDQuerySucceeded;
    }

    // ── IIr77Dictionary ──────────────────────────────────────────
   public:
    IIr77GUID const* Collection() const { return const_cast<IIr77GUID*>(static_cast<IIr77GUID const*>(&GUIDIr77PeregrineV)); }

    IIr77GUID const* Member(std::string const& member) const { return m_index.at(member); }

    std::map<std::string, IIr77GUID const*> const& Index() const { return m_index; }

   private:
    std::map<std::string, IIr77GUID const*> m_index{
        {"GUIDIr77PeregrineV", &GUIDIr77PeregrineV},
        {"GUIDIr77PVInstance", &GUIDIr77PVInstance},
        {"GUIDIr77PVDevice", &GUIDIr77PVDevice},
        {"GUIDIr77PVSwapchain", &GUIDIr77PVSwapchain},
        {"GUIDIr77PVPipeline", &GUIDIr77PVPipeline},
        {"GUIDIr77PVPipelineCache", &GUIDIr77PVPipelineCache},
        {"GUIDIr77PVPipelineCPT", &GUIDIr77PVPipelineCPT},
        {"GUIDIr77PVPipelineRay", &GUIDIr77PVPipelineRay},
        {"GUIDIr77PVRenderPass", &GUIDIr77PVRenderPass},
        {"GUIDIr77PVOverlay", &GUIDIr77PVOverlay},
        {"GUIDIr77PVBuffer", &GUIDIr77PVBuffer},
        {"GUIDIr77PVCompute", &GUIDIr77PVCompute},
        {"GUIDIr77PVLayout", &GUIDIr77PVLayout},
        {"GUIDIr77PVDescriptorSet", &GUIDIr77PVDescriptorSet},
        {"GUIDIr77PVCmdBuffer", &GUIDIr77PVCmdBuffer},
        {"GUIDIr77PVTexture", &GUIDIr77PVTexture},
        {"GUIDIr77PVTransfer", &GUIDIr77PVTransfer},
        {"GUIDIr77PVShader", &GUIDIr77PVShader},
        {"GUIDIr77PVMaterial", &GUIDIr77PVMaterial},
        {"GUIDIr77PVGeometry", &GUIDIr77PVGeometry},
        {"GUIDIr77PVInit", &GUIDIr77PVInit},
        {"GUIDIr77PVPaint", &GUIDIr77PVPaint},
        {"GUIDIr77PVCEF", &GUIDIr77PVCEF},
    };
};
}  // namespace NSIr77PeregrineV