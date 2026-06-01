#pragma once

#include <string>
#include <map>
#include <memory>

#include "IIr77GUID.hpp"

#include "IIr77Enlisted.hpp"

namespace NSIr77RT {

typedef struct IIr77Dictionary : virtual public IIr77Enlisted {
    IIr77Dictionary() = default;

    virtual IIr77GUID const* Collection() const = 0;

    virtual IIr77GUID const* Member(std::string const& member) const = 0;

    virtual std::map<std::string, IIr77GUID const*> const& Index() const = 0;

    virtual ~IIr77Dictionary() = default;

}* PIr77Dictionary;

template <Ir77GUID* T>
std::shared_ptr<IIr77GUID const> make_shared_uuid() {
    return std::shared_ptr<IIr77GUID const>(T, [](auto*) {});
}

template <Ir77GUID* T>
void seat_shared_uuid(std::shared_ptr<IIr77GUID const>& uid) {
    uid.reset(reinterpret_cast<IIr77GUID const*>(T), [](auto*) {});
}

}  // namespace NSIr77RT