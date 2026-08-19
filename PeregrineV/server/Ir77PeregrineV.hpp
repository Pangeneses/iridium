#pragma once

#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan_core.h>

#include <chrono>
#include <random>
#include <cstdint>
#include <memory>
#include <string>

#include "../../Ir77RT/dictionary/IDIIr77MPVM.hpp"

#include "../dictionary/IDIIr77PeregrineV.hpp"
#include "../dictionary/IDIr77PeregrineV.hpp"
#include "../dictionary/IDOPIr77PeregrineV.hpp"
#include "../dictionary/IDOCIr77PeregrineV.hpp"

#include "../../Ir77RT/interface/IIr77GUID.hpp"
#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Patch.hpp"
#include "../../Ir77RT/interface/IIr77Dispatch.hpp"
#include "../../Ir77RT/interface/IIr77Operand.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"
#include "../../Ir77RT/runtime/Ir77Patch.hpp"

#include "../operand/Ir77PVLifetime.hpp"
#include "../operand/Ir77PVAsset.hpp"
#include "../operand/Ir77PVPump.hpp"

using namespace NSIr77RT;
// using namespace NSIr77REDOS;

namespace NSIr77PeregrineV {
class Ir77PeregrineV : public Ir77Enlisted, public IIr77Dispatch, public std::enable_shared_from_this<Ir77PeregrineV> {
   public:
    Ir77PeregrineV() {
        try {
            m_enlisted_uuid.Generate();
        } catch (std::invalid_argument a) {
            throw a;
        }

        m_enlisted = std::chrono::system_clock::now();

        m_patch = std::make_shared<Ir77Patch>();

        m_lifetime = std::make_shared<Ir77PVLifetime>();
        m_asset = std::make_shared<Ir77PVAsset>();
        m_pump = std::make_shared<Ir77PVPump>();

        m_patch->AddOperation({GUIDOPIr77PVLifetime, GUIDOCIr77Initialize},
                              [this](std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) -> std::shared_ptr<IIr77Return const> {
                                  return m_lifetime->Initialize(rhs, lhs);
                              });


        m_patch->AddOperation({GUIDOPIr77PVAsset, GUIDOCIr77AssetUpload},
                              [this](std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) -> std::shared_ptr<IIr77Return const> {
                                  return m_asset->AssetUpload(rhs, lhs);
                              });


        m_patch->AddOperation({GUIDOPIr77PVPump, GUIDOCIr77PumpNextFrame},
                              [this](std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) -> std::shared_ptr<IIr77Return const> {
                                  return m_pump->PumpNextFrame(rhs, lhs);
                              });
        m_patch->AddOperation({GUIDOPIr77PVPump, GUIDOCIr77PumpCommand},
                              [this](std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) -> std::shared_ptr<IIr77Return const> {
                                  return m_pump->PumpCommand(rhs, lhs);
                              });
        m_patch->AddOperation({GUIDOPIr77PVPump, GUIDOCIr77PumpBarrier},
                              [this](std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) -> std::shared_ptr<IIr77Return const> {
                                  return m_pump->PumpBarrier(rhs, lhs);
                              });
        m_patch->AddOperation({GUIDOPIr77PVPump, GUIDOCIr77PumpSubmit},
                              [this](std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) -> std::shared_ptr<IIr77Return const> {
                                  return m_pump->PumpSubmit(rhs, lhs);
                              });
        m_patch->AddOperation({GUIDOPIr77PVPump, GUIDOCIr77PumpCobalt},
                              [this](std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) -> std::shared_ptr<IIr77Return const> {
                                  return m_pump->PumpCobalt(rhs, lhs);
                              });
        m_patch->AddOperation({GUIDOPIr77PVPump, GUIDOCIr77PumpFrame},
                              [this](std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) -> std::shared_ptr<IIr77Return const> {
                                  return m_pump->PumpFrame(rhs, lhs);
                              });
        m_patch->AddOperation({GUIDOPIr77PVPump, GUIDOCIr77PumpPresent},
                              [this](std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) -> std::shared_ptr<IIr77Return const> {
                                  return m_pump->PumpPresent(rhs, lhs);
                              });
    }

   public:
    std::shared_ptr<IIr77Return const> MemberOfUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77PeregrineV>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CollectionUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77PeregrineV>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    IIr77GUID* const QueryInterface(IIr77GUID const* iid, std::shared_ptr<void>& obj) {
        if (iid == &GUIDIIr77Enlisted)
            obj = std::shared_ptr<IIr77Enlisted>(shared_from_this(), static_cast<IIr77Enlisted*>(this));

        else if (iid == &GUIDIIr77Dispatch)
            obj = std::shared_ptr<IIr77Dispatch>(shared_from_this(), static_cast<IIr77Dispatch*>(this));

        else if (iid == &GUIDIIr77PeregrineV)
            obj = std::shared_ptr<Ir77PeregrineV>(shared_from_this(), static_cast<Ir77PeregrineV*>(this));

        else if (iid == &GUIDIr77PeregrineV)
            obj = std::shared_ptr<Ir77PeregrineV>(shared_from_this(), static_cast<Ir77PeregrineV*>(this));

        else
            return &GUIDQueryFailed;

        return &GUIDQuerySucceeded;
    }

   public:
    std::shared_ptr<IIr77Return const> Dispatch(std::shared_ptr<IIr77Stack const>& stack) {
        return m_patch->Forward(stack);
    }

    uint64_t random_u64() {
        static std::mt19937_64 rng(std::random_device{}());
        std::uniform_int_distribution<uint64_t> dist;
        return dist(rng);
    }

    std::shared_ptr<IIr77Return const> Factory(std::shared_ptr<IIr77GUID const>& uid, std::shared_ptr<IIr77Enlisted>& obj, std::uint64_t& id) {
        if (id == CREATE_NEW) id = random_u64();

        if (*uid == GUIDOPIr77PVLifetime) {
            m_lifetime_map.emplace(id, std::make_shared<Ir77PVLifetime>());
            obj = m_lifetime_map.at(id);
        }

        if (*uid == GUIDOPIr77PVAsset) {
            m_asset_map.emplace(id, std::make_shared<Ir77PVAsset>());
            obj = m_asset_map.at(id);
        }

        if (*uid == GUIDOPIr77PVPump) {
            m_pump_map.emplace(id, std::make_shared<Ir77PVPump>());
            obj = m_pump_map.at(id);
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> Garbage(std::shared_ptr<IIr77GUID const>& uid, std::uint64_t const& id) {
        if (*uid == GUIDOPIr77PVLifetime) {
            m_lifetime_map.erase(id);
        }

        if (*uid == GUIDOPIr77PVAsset) {
            m_asset_map.erase(id);
        }

        if (*uid == GUIDOPIr77PVPump) {
            m_pump_map.erase(id);
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   private:
    std::shared_ptr<IIr77Patch> m_patch{nullptr};

    std::string m_tag_text{'\0'};

   private:
    std::shared_ptr<Ir77PVLifetime> m_lifetime{nullptr};

    std::shared_ptr<Ir77PVAsset> m_asset{nullptr};

    std::shared_ptr<Ir77PVPump> m_pump{nullptr};

    std::map<std::uint64_t const, std::shared_ptr<Ir77PVLifetime>> m_lifetime_map;

    std::map<std::uint64_t const, std::shared_ptr<Ir77PVAsset>> m_asset_map;

    std::map<std::uint64_t const, std::shared_ptr<Ir77PVPump>> m_pump_map;
};

}  // namespace NSIr77PeregrineV
