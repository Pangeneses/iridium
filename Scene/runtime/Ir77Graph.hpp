#pragma once

#include <algorithm>
#include <cctype>
#include <chrono>
#include <stdexcept>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <map>
#include <memory>
#include <random>
#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL  // glm::decompose
#endif
#include <glm/gtx/matrix_decompose.hpp>

#include "../../Ir77RT/dictionary/IDIIr77MPVM.hpp"

#include "../dictionary/IDIIr77Scene.hpp"
#include "../dictionary/IDIr77Scene.hpp"

#include "../../Ir77RT/types/Ir77Types.hpp"

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"
#include "../../Ir77RT/runtime/Ir77Return.hpp"

#include "../interface/IIr77World.hpp"
#include "../interface/IIr77Graph.hpp"

#include "../types/Ir77Types.hpp"

using namespace NSIr77RT;

namespace NSIr77Scene {

// =============================================================================================================================================
// Ir77Graph -- the scene graph's working copy (types/Ir77Types.hpp). The scene server owns it; World does the drawing: a model entity is an
// World entity, a light entity a world light. Every change here lands on World at once -- world transforms composed down the tree,
// visibility too -- so what's drawn is always the graph. Saving and loading are the UI's (it reads Dump, it replays Add).
//
//   Add(entity, path) -> id      a new entity (id made here unless the entity brings one -- a load keeps its ids)
//   Remove(id)                   it and everything under it
//   Move(id, parent, order)      re-parent / re-order -- never under itself
//   Set(entity)                  name, transform, visibility, payload -- the whole core and payload replaced
//   Clear()                      empty -- the scene server calls it with scene.create (World's runtime ids start over)
// Errors come back as a message (error) and an Ir77NotConfigured return.
// =============================================================================================================================================
class Ir77Graph : public Ir77Enlisted, public IIr77Graph, public std::enable_shared_from_this<Ir77Graph> {
   public:
    Ir77Graph() : m_random{std::random_device{}()} {
        try {
            m_enlisted_uuid.Generate();
        } catch (std::invalid_argument a) {
            throw a;
        }

        m_enlisted = std::chrono::system_clock::now();
    }

   public:
    std::shared_ptr<IIr77Return const> EnlistedAs(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIIr77Enlisted>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> MemberUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77Graph>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CollectionUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77Scene>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    Ir77GUID QueryInterface(Ir77GUID iid, std::shared_ptr<void>& obj) {
        if (iid == GUIDIIr77Enlisted)
            obj = std::shared_ptr<IIr77Enlisted>(shared_from_this(), static_cast<IIr77Enlisted*>(this));

        else if (iid == GUIDIIr77Graph)
            obj = std::shared_ptr<IIr77Graph>(shared_from_this(), static_cast<IIr77Graph*>(this));

        else if (iid == GUIDIr77Graph)
            obj = std::shared_ptr<Ir77Graph>(shared_from_this(), static_cast<Ir77Graph*>(this));

        else
            return GUIDQueryFailed;

        return GUIDQuerySucceeded;
    }

   public:
    std::shared_ptr<IIr77Return const> SetWorld(std::shared_ptr<IIr77World> const& world) {
        m_world = world;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> Clear() {
        m_entities.clear();

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // A new entity. A model loads path (the scene server resolved it); a light becomes a world light. order 0 on a new entity = after its
    // siblings.
    std::shared_ptr<IIr77Return const> Add(Ir77GraphEntity const& added, std::string const& path, bool const& use_mesh, std::string& id,
                                           std::string& error) {
        if (!m_world) return Fail("no World", error);

        Ir77GraphEntity entity = added;

        if (entity.id.empty()) {
            entity.id = NewId();
        } else if (!ValidId(entity.id)) {
            return Fail("an id is 16 hex digits", error);
        } else if (m_entities.count(entity.id)) {
            return Fail("id " + entity.id + " is taken", error);
        }

        if (!entity.parent.empty() && !m_entities.count(entity.parent)) return Fail("no parent " + entity.parent, error);

        if (entity.order == 0) entity.order = NextOrder(entity.parent);

        entity.runtime = GRAPH_NO_RUNTIME;

        if (entity.kind == Ir77GraphKind::Model) {
            if (path.empty()) return Fail("a model entity needs a model", error);

            std::uint32_t runtime{0};
            if (m_world->ImportEntity(path, use_mesh, false, runtime)->ID() != GUIDIr77OperationSucceeded)
                return Fail("model " + entity.model + " didn't load (" + path + ")", error);

            entity.runtime = runtime;
        }

        if (entity.kind == Ir77GraphKind::Light) {
            std::uint32_t runtime{0};
            if (m_world->AddLight(entity.light, glm::vec3(0.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f), runtime)->ID() != GUIDIr77OperationSucceeded)
                return Fail("the light couldn't be made", error);

            entity.runtime = runtime;
        }

        id = entity.id;
        m_entities.emplace(id, std::move(entity));

        Apply(id);
        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // It and everything under it -- children first
    std::shared_ptr<IIr77Return const> Remove(std::string const& id, std::string& error) {
        if (!m_entities.count(id)) return Fail("no entity " + id, error);

        std::vector<std::string> doomed{};
        Subtree(id, doomed);

        for (auto it = doomed.rbegin(); it != doomed.rend(); ++it) {
            Ir77GraphEntity const& entity = m_entities.at(*it);

            if (entity.runtime != GRAPH_NO_RUNTIME) {
                if (entity.kind == Ir77GraphKind::Model) m_world->RemoveEntity(entity.runtime);
                if (entity.kind == Ir77GraphKind::Light) m_world->RemoveLight(entity.runtime);
            }

            m_entities.erase(*it);
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // parent '' = the scene root; order 0 = after the new siblings
    std::shared_ptr<IIr77Return const> Move(std::string const& id, std::string const& parent, std::uint32_t const& order, std::string& error) {
        auto const found = m_entities.find(id);
        if (found == m_entities.end()) return Fail("no entity " + id, error);
        if (!parent.empty() && !m_entities.count(parent)) return Fail("no parent " + parent, error);

        // never under itself
        for (std::string at = parent; !at.empty(); at = m_entities.at(at).parent)
            if (at == id) return Fail("an entity can't go under itself", error);

        found->second.parent = parent;
        found->second.order = order == 0 ? NextOrder(parent) : order;

        Apply(id);
        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Core and payload from changed -- id, kind and parent stay (Move re-parents); a light's settings go to its world light
    std::shared_ptr<IIr77Return const> Set(Ir77GraphEntity const& changed, std::string& error) {
        auto const found = m_entities.find(changed.id);
        if (found == m_entities.end()) return Fail("no entity " + changed.id, error);

        Ir77GraphEntity& entity = found->second;
        entity.name = changed.name;
        entity.order = changed.order;
        entity.visible = changed.visible;
        entity.t = changed.t;
        entity.r = glm::normalize(changed.r);
        entity.s = changed.s;
        entity.light = changed.light;
        entity.camera = changed.camera;

        Apply(entity.id);
        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> Get(std::string const& id, Ir77GraphEntity& entity) const {
        auto const found = m_entities.find(id);
        if (found == m_entities.end()) return Ir77RETURN<Ir77NotConfigured>(this);  // quiet -- asking is how callers test for an entity

        entity = found->second;
        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Every entity, parents before children, siblings in order -- what a dump writes and a load replays
    std::shared_ptr<IIr77Return const> Ordered(std::vector<Ir77GraphEntity const*>& ordered) const {
        ordered.clear();

        std::function<void(std::string const&)> walk = [&](std::string const& parent) {
            for (Ir77GraphEntity const* child : Children(parent)) {
                ordered.push_back(child);
                walk(child->id);
            }
        };

        walk("");
        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // The model entity behind a World entity -- '' when none
    std::shared_ptr<IIr77Return const> FindModel(std::uint32_t const& runtime, std::string& id) const {
        id.clear();

        for (auto const& [key, entity] : m_entities)
            if (entity.kind == Ir77GraphKind::Model && entity.runtime == runtime) {
                id = key;
                break;
            }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Where it sits in the world -- its parents' transforms, then its own
    std::shared_ptr<IIr77Return const> World(std::string const& id, glm::mat4& world) const {
        if (!m_entities.count(id)) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Graph: no entity " + id);

        world = WorldOf(id);
        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Shown only when it and every parent are
    std::shared_ptr<IIr77Return const> Shown(std::string const& id, bool& shown) const {
        if (!m_entities.count(id)) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Graph: no entity " + id);

        shown = ShownOf(id);
        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    static bool ValidId(std::string const& id) {
        return id.size() == GRAPH_ID_LENGTH && std::all_of(id.begin(), id.end(), [](char const c) { return std::isxdigit(static_cast<unsigned char>(c)); });
    }

   private:
    // the walks behind World / Shown -- id is known to exist
    glm::mat4 WorldOf(std::string const& id) const {
        glm::mat4 world{1.0f};

        for (std::string at = id; !at.empty();) {
            Ir77GraphEntity const& entity = m_entities.at(at);
            world = Local(entity) * world;
            at = entity.parent;
        }

        return world;
    }

    bool ShownOf(std::string const& id) const {
        for (std::string at = id; !at.empty();) {
            Ir77GraphEntity const& entity = m_entities.at(at);
            if (!entity.visible) return false;
            at = entity.parent;
        }

        return true;
    }

    // it and everything under it onto World -- world transforms and visibility
    void Apply(std::string const& id) {
        std::vector<std::string> subtree{};
        Subtree(id, subtree);

        for (std::string const& at : subtree) {
            Ir77GraphEntity const& entity = m_entities.at(at);
            if (entity.runtime == GRAPH_NO_RUNTIME) continue;

            glm::vec3 t{0.0f};
            glm::quat r{1.0f, 0.0f, 0.0f, 0.0f};
            glm::vec3 s{1.0f};
            Decompose(WorldOf(at), t, r, s);

            bool const shown = ShownOf(at);

            if (entity.kind == Ir77GraphKind::Model) {
                m_world->SetEntityTransform(entity.runtime, t, r, s);
                m_world->SetEntityVisible(entity.runtime, shown);
            }

            if (entity.kind == Ir77GraphKind::Light) {
                m_world->SetLight(entity.runtime, entity.light, t, r);
                m_world->SetLightVisible(entity.runtime, shown);
            }
        }
    }

    // id, then everything under it, depth first
    void Subtree(std::string const& id, std::vector<std::string>& out) const {
        out.push_back(id);
        for (Ir77GraphEntity const* child : Children(id)) Subtree(child->id, out);
    }

    std::vector<Ir77GraphEntity const*> Children(std::string const& parent) const {
        std::vector<Ir77GraphEntity const*> children{};
        for (auto const& [id, entity] : m_entities)
            if (entity.parent == parent) children.push_back(&entity);

        std::sort(children.begin(), children.end(), [](auto const* a, auto const* b) { return a->order != b->order ? a->order < b->order : a->id < b->id; });
        return children;
    }

    std::uint32_t NextOrder(std::string const& parent) const {
        std::uint32_t last{0};
        for (auto const& [id, entity] : m_entities)
            if (entity.parent == parent) last = std::max(last, entity.order);

        return last + 1;
    }

    // 64 random bits as 16 lower-case hex digits -- tried again in the (astronomically unlikely) case it's taken
    std::string NewId() {
        for (;;) {
            char text[GRAPH_ID_LENGTH + 1]{};
            std::snprintf(text, sizeof(text), "%016llx", static_cast<unsigned long long>(m_random()));

            std::string const id{text};
            if (!m_entities.count(id)) return id;
        }
    }

    static glm::mat4 Local(Ir77GraphEntity const& entity) {
        return glm::translate(glm::mat4(1.0f), entity.t) * glm::mat4_cast(glm::normalize(entity.r)) * glm::scale(glm::mat4(1.0f), entity.s);
    }

    // world matrix -> t, r, s (a sheared parent chain loses its shear -- non-uniform scale under rotation)
    static void Decompose(glm::mat4 const& m, glm::vec3& t, glm::quat& r, glm::vec3& s) {
        glm::vec3 skew{0.0f};
        glm::vec4 perspective{0.0f};
        if (!glm::decompose(m, s, r, t, skew, perspective)) {
            t = glm::vec3(m[3]);
            r = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
            s = glm::vec3(1.0f);
        }
        r = glm::normalize(r);
    }

    std::shared_ptr<IIr77Return const> Fail(std::string const& message, std::string& error) const {
        error = message;
        return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Graph: " + message);
    }

    std::shared_ptr<IIr77World> m_world{};

    std::map<std::string, Ir77GraphEntity> m_entities{};  // by id

    std::mt19937_64 m_random;
};

}  // namespace NSIr77Scene
