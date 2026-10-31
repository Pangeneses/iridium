#pragma once

#include <algorithm>
#include <cctype>
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

#include "../interface/IIr77Animate.hpp"
#include "../types/Ir77Graph.hpp"

namespace NSIr77Scene {

// =============================================================================================================================================
// Ir77Graph -- the scene graph's working copy (types/Ir77Graph.hpp). The scene server owns it; Animate does the drawing: a model entity is an
// Animate entity, a light entity a world light. Every change here lands on Animate at once -- world transforms composed down the tree,
// visibility too -- so what's drawn is always the graph. Saving and loading are the UI's (it reads Dump, it replays Add).
//
//   Add(entity, path) -> id      a new entity (id made here unless the entity brings one -- a load keeps its ids)
//   Remove(id)                   it and everything under it
//   Move(id, parent, order)      re-parent / re-order -- never under itself
//   Set(entity)                  name, transform, visibility, payload -- the whole core and payload replaced
//   Clear()                      empty -- the scene server calls it with scene.create (Animate's runtime ids start over)
// Errors come back as a message (error) and false.
// =============================================================================================================================================
class Ir77Graph {
   public:
    Ir77Graph() : m_random{std::random_device{}()} {}

    void SetAnimate(std::shared_ptr<IIr77Animate> const& animate) { m_animate = animate; }

    void Clear() { m_entities.clear(); }

    // A new entity. A model loads path (the scene server resolved it); a light becomes a world light. order 0 on a new entity = after its
    // siblings.
    bool Add(Ir77GraphEntity entity, std::string const& path, bool const use_mesh, std::string& id, std::string& error) {
        if (!m_animate) return Fail("no Animate", error);

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
            if (m_animate->ImportEntity(path, use_mesh, false, runtime)->ID() != GUIDIr77OperationSucceeded)
                return Fail("model " + entity.model + " didn't load (" + path + ")", error);

            entity.runtime = runtime;
        }

        if (entity.kind == Ir77GraphKind::Light) {
            std::uint32_t runtime{0};
            if (m_animate->AddLight(entity.light, glm::vec3(0.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f), runtime)->ID() != GUIDIr77OperationSucceeded)
                return Fail("the light couldn't be made", error);

            entity.runtime = runtime;
        }

        id = entity.id;
        m_entities.emplace(id, std::move(entity));

        Apply(id);
        return true;
    }

    // It and everything under it -- children first
    bool Remove(std::string const& id, std::string& error) {
        if (!m_entities.count(id)) return Fail("no entity " + id, error);

        std::vector<std::string> doomed{};
        Subtree(id, doomed);

        for (auto it = doomed.rbegin(); it != doomed.rend(); ++it) {
            Ir77GraphEntity const& entity = m_entities.at(*it);

            if (entity.runtime != GRAPH_NO_RUNTIME) {
                if (entity.kind == Ir77GraphKind::Model) m_animate->RemoveEntity(entity.runtime);
                if (entity.kind == Ir77GraphKind::Light) m_animate->RemoveLight(entity.runtime);
            }

            m_entities.erase(*it);
        }

        return true;
    }

    // parent '' = the scene root; order 0 = after the new siblings
    bool Move(std::string const& id, std::string const& parent, std::uint32_t const order, std::string& error) {
        auto const found = m_entities.find(id);
        if (found == m_entities.end()) return Fail("no entity " + id, error);
        if (!parent.empty() && !m_entities.count(parent)) return Fail("no parent " + parent, error);

        // never under itself
        for (std::string at = parent; !at.empty(); at = m_entities.at(at).parent)
            if (at == id) return Fail("an entity can't go under itself", error);

        found->second.parent = parent;
        found->second.order = order == 0 ? NextOrder(parent) : order;

        Apply(id);
        return true;
    }

    // Core and payload from changed -- id, kind and parent stay (Move re-parents); a light's settings go to its world light
    bool Set(Ir77GraphEntity const& changed, std::string& error) {
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
        return true;
    }

    bool Get(std::string const& id, Ir77GraphEntity& entity) const {
        auto const found = m_entities.find(id);
        if (found == m_entities.end()) return false;

        entity = found->second;
        return true;
    }

    // Every entity, parents before children, siblings in order -- what a dump writes and a load replays
    std::vector<Ir77GraphEntity const*> Ordered() const {
        std::vector<Ir77GraphEntity const*> ordered{};

        std::function<void(std::string const&)> walk = [&](std::string const& parent) {
            for (Ir77GraphEntity const* child : Children(parent)) {
                ordered.push_back(child);
                walk(child->id);
            }
        };

        walk("");
        return ordered;
    }

    // The model entity behind an Animate entity -- '' when none
    std::string FindModel(std::uint32_t const runtime) const {
        for (auto const& [id, entity] : m_entities)
            if (entity.kind == Ir77GraphKind::Model && entity.runtime == runtime) return id;

        return {};
    }

    // Where it sits in the world -- its parents' transforms, then its own
    glm::mat4 World(std::string const& id) const {
        glm::mat4 world{1.0f};

        for (std::string at = id; !at.empty();) {
            Ir77GraphEntity const& entity = m_entities.at(at);
            world = Local(entity) * world;
            at = entity.parent;
        }

        return world;
    }

    // Shown only when it and every parent are
    bool Shown(std::string const& id) const {
        for (std::string at = id; !at.empty();) {
            Ir77GraphEntity const& entity = m_entities.at(at);
            if (!entity.visible) return false;
            at = entity.parent;
        }

        return true;
    }

    static bool ValidId(std::string const& id) {
        return id.size() == GRAPH_ID_LENGTH && std::all_of(id.begin(), id.end(), [](char const c) { return std::isxdigit(static_cast<unsigned char>(c)); });
    }

   private:
    // it and everything under it onto Animate -- world transforms and visibility
    void Apply(std::string const& id) {
        std::vector<std::string> subtree{};
        Subtree(id, subtree);

        for (std::string const& at : subtree) {
            Ir77GraphEntity const& entity = m_entities.at(at);
            if (entity.runtime == GRAPH_NO_RUNTIME) continue;

            glm::vec3 t{0.0f};
            glm::quat r{1.0f, 0.0f, 0.0f, 0.0f};
            glm::vec3 s{1.0f};
            Decompose(World(at), t, r, s);

            bool const shown = Shown(at);

            if (entity.kind == Ir77GraphKind::Model) {
                m_animate->SetEntityTransform(entity.runtime, t, r, s);
                m_animate->SetEntityVisible(entity.runtime, shown);
            }

            if (entity.kind == Ir77GraphKind::Light) {
                m_animate->SetLight(entity.runtime, entity.light, t, r);
                m_animate->SetLightVisible(entity.runtime, shown);
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

    static bool Fail(std::string const& message, std::string& error) {
        error = message;
        return false;
    }

    std::shared_ptr<IIr77Animate> m_animate{};

    std::map<std::string, Ir77GraphEntity> m_entities{};  // by id

    std::mt19937_64 m_random;
};

}  // namespace NSIr77Scene
