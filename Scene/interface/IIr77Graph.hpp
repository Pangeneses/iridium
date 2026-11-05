#pragma once

#include <glm/glm.hpp>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "IIr77Animate.hpp"

#include "../types/Ir77Types.hpp"

using namespace NSIr77RT;

namespace NSIr77Scene {

// The scene graph's working copy -- every entity by its id (types/Ir77Types.hpp, Ir77GraphEntity). Every change lands on Animate at once, so
// what's drawn is always the graph. A failure fills error with the message the op answers with.
//   SetAnimate -> Add / Remove / Move / Set -> Get / Ordered / FindModel / World / Shown -> Clear (a new scene)
typedef struct IIr77Graph : virtual public IIr77Enlisted {
    IIr77Graph() = default;

    // Animate -- what a model / light entity puts its runtime side into
    virtual std::shared_ptr<IIr77Return const> SetAnimate(std::shared_ptr<IIr77Animate> const& animate) = 0;

    // empty -- the scene server calls it with scene.create (Animate's runtime ids start over)
    virtual std::shared_ptr<IIr77Return const> Clear() = 0;

    // A new entity. A model loads path (the scene server resolved it); a light becomes a world light. order 0 = after its siblings. id = its
    // id (made here unless the entity brings one -- a load keeps its ids).
    virtual std::shared_ptr<IIr77Return const> Add(Ir77GraphEntity const& entity, std::string const& path, bool const& use_mesh, std::string& id,
                                                   std::string& error) = 0;

    // It and everything under it -- children first
    virtual std::shared_ptr<IIr77Return const> Remove(std::string const& id, std::string& error) = 0;

    // parent '' = the scene root; order 0 = after the new siblings; never under itself
    virtual std::shared_ptr<IIr77Return const> Move(std::string const& id, std::string const& parent, std::uint32_t const& order, std::string& error) = 0;

    // Core and payload from changed -- id, kind and parent stay (Move re-parents); a light's settings go to its world light
    virtual std::shared_ptr<IIr77Return const> Set(Ir77GraphEntity const& changed, std::string& error) = 0;

    // A copy of one entity -- not there: Ir77NotConfigured
    virtual std::shared_ptr<IIr77Return const> Get(std::string const& id, Ir77GraphEntity& entity) const = 0;

    // Every entity, parents before children, siblings in order -- what a dump writes and a load replays. Valid until the graph changes.
    virtual std::shared_ptr<IIr77Return const> Ordered(std::vector<Ir77GraphEntity const*>& ordered) const = 0;

    // The model entity behind an Animate entity -- id '' when none
    virtual std::shared_ptr<IIr77Return const> FindModel(std::uint32_t const& runtime, std::string& id) const = 0;

    // Where it sits in the world -- its parents' transforms, then its own
    virtual std::shared_ptr<IIr77Return const> World(std::string const& id, glm::mat4& world) const = 0;

    // Shown only when it and every parent are
    virtual std::shared_ptr<IIr77Return const> Shown(std::string const& id, bool& shown) const = 0;

    virtual ~IIr77Graph() = default;
}* pIIr77Graph;
}  // namespace NSIr77Scene
