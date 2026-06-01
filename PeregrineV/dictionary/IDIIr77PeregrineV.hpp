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

inline Ir77GUID GUIDIIr77PVContext{(static_cast<unsigned __int128>(0x77DFD659D2AB4750) << 64) | 0xAC8581A603B4C82A};

/**************** Foundation layer ****************/
inline Ir77GUID GUIDIIr77PVInstance{(static_cast<unsigned __int128>(0xB1136C9B855A803E) << 64) | 0x7A5E662149F03242};
inline Ir77GUID GUIDIIr77PVDevice{(static_cast<unsigned __int128>(0xE29226960D283AA0) << 64) | 0x55C041024DC6C909};
inline Ir77GUID GUIDIIr77PVQueue{(static_cast<unsigned __int128>(0x9808C7422F216732) << 64) | 0xF160AB859A9538F8};
inline Ir77GUID GUIDIIr77PVSurface{(static_cast<unsigned __int128>(0x798E5BB1B58EC0BE) << 64) | 0xA69DFA5475A9111A};

/**************** Memory layer ****************/
inline Ir77GUID GUIDIIr77PVMemory{(static_cast<unsigned __int128>(0x0FDBCAD888E8B072) << 64) | 0xE9B695057B4F125D};
inline Ir77GUID GUIDIIr77PVMemoryStrategy{(static_cast<unsigned __int128>(0xDFB39C310FC847FF) << 64) | 0x2A1BF6FF2CFB11D2};
inline Ir77GUID GUIDIIr77PVAllocation{(static_cast<unsigned __int128>(0x77B557DE3F015D7A) << 64) | 0x3C9EB53293BB0DDD};

/**************** Resource layer ****************/
inline Ir77GUID GUIDIIr77PVBuffer{(static_cast<unsigned __int128>(0x4AE3F3A2F0EC509A) << 64) | 0xFF19E0E25672C88D};
inline Ir77GUID GUIDIIr77PVImage{(static_cast<unsigned __int128>(0x47856223A51ADF91) << 64) | 0x35D3E7A600D856EA};
inline Ir77GUID GUIDIIr77PVImageView{(static_cast<unsigned __int128>(0x690CE1612AE1A814) << 64) | 0x3514B39F50F18A60};
inline Ir77GUID GUIDIIr77PVSampler{(static_cast<unsigned __int128>(0x5097B3A320CA9533) << 64) | 0x4777A111AED34BD0};

/**************** Descriptor layer ****************/
inline Ir77GUID GUIDIIr77PVDescriptorLayout{(static_cast<unsigned __int128>(0x3748583BB9C81720) << 64) | 0x08ACB330BEBFAC4F};
inline Ir77GUID GUIDIIr77PVDescriptorSet{(static_cast<unsigned __int128>(0x9F97122AA00F85E9) << 64) | 0x674C230CA048975F};

/**************** Shader and pipeline layer ****************/
inline Ir77GUID GUIDIIr77PVPipeline{(static_cast<unsigned __int128>(0xB10BE474EAC70D1A) << 64) | 0xFEFDBF8EA6C37F9D};
inline Ir77GUID GUIDIIr77PVShader{(static_cast<unsigned __int128>(0xF4D60A74F19932D3) << 64) | 0xBEADFF0A12D9F258};
inline Ir77GUID GUIDIIr77PVMaterial{(static_cast<unsigned __int128>(0x033C555AF2E0ABF5) << 64) | 0x1EF45C1405FEA446};

/**************** Render graph layer ****************/
inline Ir77GUID GUIDIIr77PVRenderGraph{(static_cast<unsigned __int128>(0x19C13ED4A0FBB016) << 64) | 0xCBE042F8B550997F};
inline Ir77GUID GUIDIIr77PVRenderStage{(static_cast<unsigned __int128>(0x54124F332515B85F) << 64) | 0x238E0190181A4F83};
inline Ir77GUID GUIDIIr77PVRenderPass{(static_cast<unsigned __int128>(0xE66F8BE0C5FC877E) << 64) | 0x880D9BEBB37CD1A7};
inline Ir77GUID GUIDIIr77PVRenderTarget{(static_cast<unsigned __int128>(0xD1A5A1A0E9329097) << 64) | 0x0C47FBD6D4D957CC};
inline Ir77GUID GUIDIIr77PVDepthTarget{(static_cast<unsigned __int128>(0x10FED262E0F1654C) << 64) | 0xB428FCE3783EB997};

/**************** Geometry layer ****************/
inline Ir77GUID GUIDIIr77PVMesh{(static_cast<unsigned __int128>(0xF8C99EAD7B7CF55C) << 64) | 0xC944FDC0A7F18F94};

/**************** Per-frame layer ****************/
inline Ir77GUID GUIDIIr77PVFrame{(static_cast<unsigned __int128>(0x2DB6A39E46AABC03) << 64) | 0x5DEFE52FFF5ED979};
inline Ir77GUID GUIDIIr77PVFramebuffer{(static_cast<unsigned __int128>(0xF5043E58391F3525) << 64) | 0x0DD142E588130BF1};

/**************** Inline / structural ****************/
inline Ir77GUID GUIDIIr77PVBarrier{(static_cast<unsigned __int128>(0x82FAAAE6FCCB86EA) << 64) | 0xD84BC19280E07DB5};
inline Ir77GUID GUIDIIr77PVCommandBuffer{(static_cast<unsigned __int128>(0xF71D19D109D2114D) << 64) | 0x1186ADBD29983077};
inline Ir77GUID GUIDIIr77PVSemaphore{(static_cast<unsigned __int128>(0x08D2B66138427009) << 64) | 0xD88C8BC038D1DE22};

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
    std::shared_ptr<IIr77Return const> MemberOfUuid(std::shared_ptr<IIr77GUID const>& uid) {
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
        /**************** Collection ****************/
        {"GUIDIIr77PeregrineV", &GUIDIIr77PeregrineV},
        {"GUIDIIr77PVContext", &GUIDIIr77PVContext},

        /**************** Foundation layer ****************/
        {"GUIDIIr77PVInstance", &GUIDIIr77PVInstance},
        {"GUIDIIr77PVDevice", &GUIDIIr77PVDevice},
        {"GUIDIIr77PVQueue", &GUIDIIr77PVQueue},
        {"GUIDIIr77PVSurface", &GUIDIIr77PVSurface},

        /**************** Memory layer ****************/
        {"GUIDIIr77PVMemory", &GUIDIIr77PVMemory},
        {"GUIDIIr77PVMemoryStrategy", &GUIDIIr77PVMemoryStrategy},
        {"GUIDIIr77PVAllocation", &GUIDIIr77PVAllocation},

        /**************** Resource layer ****************/
        {"GUIDIIr77PVBuffer", &GUIDIIr77PVBuffer},
        {"GUIDIIr77PVImage", &GUIDIIr77PVImage},
        {"GUIDIIr77PVImageView", &GUIDIIr77PVImageView},
        {"GUIDIIr77PVSampler", &GUIDIIr77PVSampler},

        /**************** Descriptor layer ****************/
        {"GUIDIIr77PVDescriptorLayout", &GUIDIIr77PVDescriptorLayout},
        {"GUIDIIr77PVDescriptorSet", &GUIDIIr77PVDescriptorSet},

        /**************** Shader and pipeline layer ****************/
        {"GUIDIIr77PVShader", &GUIDIIr77PVShader},
        {"GUIDIIr77PVPipeline", &GUIDIIr77PVPipeline},
        {"GUIDIIr77PVMaterial", &GUIDIIr77PVMaterial},

        /**************** Render graph layer ****************/
        {"GUIDIIr77PVRenderGraph", &GUIDIIr77PVRenderGraph},
        {"GUIDIIr77PVRenderStage", &GUIDIIr77PVRenderStage},
        {"GUIDIIr77PVRenderPass", &GUIDIIr77PVRenderPass},
        {"GUIDIIr77PVRenderTarget", &GUIDIIr77PVRenderTarget},
        {"GUIDIIr77PVDepthTarget", &GUIDIIr77PVDepthTarget},

        /**************** Geometry layer ****************/
        {"GUIDIIr77PVMesh", &GUIDIIr77PVMesh},

        /**************** Per-frame layer ****************/
        {"GUIDIIr77PVFrame", &GUIDIIr77PVFrame},
        {"GUIDIIr77PVFramebuffer", &GUIDIIr77PVFramebuffer},

        /**************** Inline / structural ****************/
        {"GUIDIIr77PVBarrier", &GUIDIIr77PVBarrier},
        {"GUIDIIr77PVCommandBuffer", &GUIDIIr77PVCommandBuffer},
        {"GUIDIIr77PVSemaphore", &GUIDIIr77PVSemaphore},

    };
};

}  // namespace NSIr77PeregrineV