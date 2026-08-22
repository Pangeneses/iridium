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

/**************** GPU Interface ****************/
inline Ir77GUID GUIDIr77PVInstance{(static_cast<unsigned __int128>(0x7D4E91A3C6F28B05) << 64) | 0x1B9E50A472C3D8F6};
inline Ir77GUID GUIDIr77PVDevice{(static_cast<unsigned __int128>(0xC2B74F8E3A916D05) << 64) | 0x5F3A10E274B9C806};
inline Ir77GUID GUIDIr77PVQueue{(static_cast<unsigned __int128>(0x81F3D6A94C720E5B) << 64) | 0xD4A80F1362C95E73};
inline Ir77GUID GUIDIr77PVSwapchain{(static_cast<unsigned __int128>(0x4A9C15E7F3806D2B) << 64) | 0x8C3E72B1054FA96D};
inline Ir77GUID GUIDIr77PVCmdBuffer{(static_cast<unsigned __int128>(0x83E6A0D2F1794C5B) << 64) | 0xC1A4E7930F2D85B6};
inline Ir77GUID GUIDIr77PVBarrier{(static_cast<unsigned __int128>(0xD9F3C1A87E524B06) << 64) | 0x3E60B7F29A1D4C85};
inline Ir77GUID GUIDIr77PVSemaphore{(static_cast<unsigned __int128>(0x4C9B72F5A1E308D6) << 64) | 0x7F2E04A985C1B3D6};

/**************** Pipeline ****************/
inline Ir77GUID GUIDIr77PVPipelineGFX{(static_cast<unsigned __int128>(0x7B3F90D2C5A418E6) << 64) | 0x4A8E610F93B2D75C};

/**************** Pipeline Layout ****************/
inline Ir77GUID GUIDIr77PVLayout001{(static_cast<unsigned __int128>(0xF9C41B8E3A706D25) << 64) | 0x53BD70F1248A9CE6};

/**************** Render Pass ****************/
inline Ir77GUID GUIDIr77PVRenderPassColor{(static_cast<unsigned __int128>(0x5F2A91D8E4703C6B) << 64) | 0x0B8E53C1F96A4D72};

/**************** Buffer ****************/
inline Ir77GUID GUIDIr77PVBuffer{(static_cast<unsigned __int128>(0xD1A93F6E4C807B25) << 64) | 0x064BCE8317F95A2D};

/**************** Compute ****************/
inline Ir77GUID GUIDIr77PVCompute{(static_cast<unsigned __int128>(0x6F520D9B3E71A8C4) << 64) | 0xA8D4731F50C2E96B};

/**************** Asset ****************/
inline Ir77GUID GUIDIr77PVShader{(static_cast<unsigned __int128>(0xC6E4819A3F720B5D) << 64) | 0x1D30A7F852C9E4B6};
inline Ir77GUID GUIDIr77PVMaterial{(static_cast<unsigned __int128>(0x3A5D72F1E9C840B6) << 64) | 0x8F2BC4A1057E93D6};
inline Ir77GUID GUIDIr77PVMesh{(static_cast<unsigned __int128>(0x6E8C30A1D4F97B25) << 64) | 0x2B7D4F9A61E30C85};

// inline Ir77GUID GUIDIr77PV{(static_cast<unsigned __int128>(0xB3A8071F5C294E6D) << 64) | 0x2E96D4B07A1F83C5};
// inline Ir77GUID GUIDIr77PV{(static_cast<unsigned __int128>(0x48D7C2A91F630B5E) << 64) | 0x9C51E8A274F036BD};
// inline Ir77GUID GUIDIr77PV{(static_cast<unsigned __int128>(0x20E7B4A6F8D193C5) << 64) | 0xE17F3A9428D05B6C};
// inline Ir77GUID GUIDIr77PV{(static_cast<unsigned __int128>(0xE81F4B7A2C903D56) << 64) | 0x6A3D09F274B1C8E5};
// inline Ir77GUID GUIDIr77PV{(static_cast<unsigned __int128>(0x94C3E07B1F852A6D) << 64) | 0xD2F1A8360C74E9B5};
// inline Ir77GUID GUIDIr77PV{(static_cast<unsigned __int128>(0xA7D40E6C3B921F58) << 64) | 0x7C2F81A504E3B9D6};
// inline Ir77GUID GUIDIr77PV{(static_cast<unsigned __int128>(0x31B6F9C5A270E8D4) << 64) | 0xF4A91C8350D27B6E};
// inline Ir77GUID GUIDIr77PV{(static_cast<unsigned __int128>(0xB4F270E9C1A83D56) << 64) | 0x90E3C5B17F2A4D68};
// inline Ir77GUID GUIDIr77PV{(static_cast<unsigned __int128>(0x27A5D8F3E0641C9B) << 64) | 0x5C8B1F740A2E93D6};
// inline Ir77GUID GUIDIr77PV{(static_cast<unsigned __int128>(0xE6B30A7F2D941C58) << 64) | 0x3079C4F851A6E2BD};
// inline Ir77GUID GUIDIr77PV{(static_cast<unsigned __int128>(0x92D4F1C8B603A75E) << 64) | 0x7F1A4E9362D08C5B};
// inline Ir77GUID GUIDIr77PV{(static_cast<unsigned __int128>(0x5C8E20B7A4F19D63) << 64) | 0xB2F07C3A95E481D6};
// inline Ir77GUID GUIDIr77PV{(static_cast<unsigned __int128>(0x690CE1612AE1A814) << 64) | 0x3514B39F50F18A60};
// inline Ir77GUID GUIDIr77PV{(static_cast<unsigned __int128>(0x5097B3A320CA9533) << 64) | 0x4777A111AED34BD0};
// inline Ir77GUID GUIDIr77PV{(static_cast<unsigned __int128>(0xD1A5A1A0E9329097) << 64) | 0x0C47FBD6D4D957CC};
// inline Ir77GUID GUIDIr77PV{(static_cast<unsigned __int128>(0x10FED262E0F1654C) << 64) | 0xB428FCE3783EB997};
// inline Ir77GUID GUIDIr77PV{(static_cast<unsigned __int128>(0x19C13ED4A0FBB016) << 64) | 0xCBE042F8B550997F};
// inline Ir77GUID GUIDIr77PV{(static_cast<unsigned __int128>(0x54124F332515B85F) << 64) | 0x238E0190181A4F83};
// inline Ir77GUID GUIDIr77PV{(static_cast<unsigned __int128>(0x9F97122AA00F85E9) << 64) | 0x674C230CA048975F};
// inline Ir77GUID GUIDIr77PV{(static_cast<unsigned __int128>(0x0FDBCAD888E8B072) << 64) | 0xE9B695057B4F125D};
// inline Ir77GUID GUIDIr77PV{(static_cast<unsigned __int128>(0xDFB39C310FC847FF) << 64) | 0x2A1BF6FF2CFB11D2};
// inline Ir77GUID GUIDIr77PV{(static_cast<unsigned __int128>(0x77B557DE3F015D7A) << 64) | 0x3C9EB53293BB0DDD};
// inline Ir77GUID GUIDIr77PV{(static_cast<unsigned __int128>(0x2DB6A39E46AABC03) << 64) | 0x5DEFE52FFF5ED979};
// inline Ir77GUID GUIDIr77PV{(static_cast<unsigned __int128>(0xF5043E58391F3525) << 64) | 0x0DD142E588130BF1};

class IDIr77PeregrineV : public Ir77Enlisted, public IIr77Dictionary, public std::enable_shared_from_this<IDIIr77MPVM> {
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

    std::shared_ptr<IIr77Return const> MemberOfUuid(std::shared_ptr<IIr77GUID const>& uid) {
        seat_shared_uuid<&GUIDIr77PeregrineV>(uid);
        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CollectionUuid(std::shared_ptr<IIr77GUID const>& uid) {
        seat_shared_uuid<&GUIDIr77PeregrineV>(uid);
        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    IIr77GUID* const QueryInterface(IIr77GUID const* iid, std::shared_ptr<void>& obj) {
        if (iid == &GUIDIIr77Enlisted)
            obj = std::shared_ptr<IIr77Enlisted>(shared_from_this(), static_cast<IIr77Enlisted*>(this));

        else if (iid == &GUIDIIr77Dictionary)
            obj = std::shared_ptr<IIr77Dictionary>(shared_from_this(), static_cast<IIr77Dictionary*>(this));

        else if (iid == &GUIDIr77PeregrineV)
            obj = std::shared_ptr<IDIr77PeregrineV>(shared_from_this(), static_cast<IDIr77PeregrineV*>(this));

        else
            return &GUIDQueryFailed;

        return &GUIDQuerySucceeded;
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
        {"GUIDIr77PVQueue", &GUIDIr77PVQueue},
        {"GUIDIr77PVSwapchain", &GUIDIr77PVSwapchain},
        {"GUIDIr77PVCmdBuffer", &GUIDIr77PVCmdBuffer},
        {"GUIDIr77PVBarrier", &GUIDIr77PVBarrier},
        {"GUIDIr77PVSemaphore", &GUIDIr77PVSemaphore},
        {"GUIDIr77PVPipelineGFX", &GUIDIr77PVPipelineGFX},
        {"GUIDIr77PVLayout001", &GUIDIr77PVLayout001},
        {"GUIDIr77PVRenderPassColor", &GUIDIr77PVRenderPassColor},
        {"GUIDIr77PVBuffer", &GUIDIr77PVBuffer},
        {"GUIDIr77PVCompute", &GUIDIr77PVCompute},
        {"GUIDIr77PVShader", &GUIDIr77PVShader},
        {"GUIDIr77PVMaterial", &GUIDIr77PVMaterial},
        {"GUIDIr77PVMesh", &GUIDIr77PVMesh},
    };
};
}  // namespace NSIr77PeregrineV