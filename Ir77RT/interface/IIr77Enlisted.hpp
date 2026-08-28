#pragma once

#include <chrono>
#include <memory>

#include "IIr77GUID.hpp"

#include "../runtime/Ir77GUID.hpp"

namespace NSIr77RT {
struct IIr77Return;

typedef struct IIr77Enlisted {
    IIr77Enlisted() = default;

    virtual std::shared_ptr<IIr77Return const> EnlistedAs(std::shared_ptr<IIr77GUID const>& uid) const = 0;

    virtual std::shared_ptr<IIr77Return const> EnlistedUuid(std::shared_ptr<IIr77GUID const>& uid) const = 0;

    virtual std::shared_ptr<IIr77Return const> EnlistedChrono(std::chrono::system_clock::time_point& t) const = 0;

    virtual std::shared_ptr<IIr77Return const> Delist(std::shared_ptr<IIr77Return const>& condition) = 0;

    virtual std::shared_ptr<IIr77Return const> DelistedChrono(std::chrono::system_clock::time_point& t,
                                                              std::shared_ptr<IIr77Return const>& condition) const = 0;

    virtual std::shared_ptr<IIr77Return const> SetSender(std::shared_ptr<IIr77Enlisted const>& sender) = 0;

    virtual std::shared_ptr<IIr77Return const> GetSender(std::shared_ptr<IIr77Enlisted const>& sender) const = 0;

    virtual std::shared_ptr<IIr77Return const> SetSenderMsg(std::string const& sender) = 0;

    virtual std::shared_ptr<IIr77Return const> GetSenderMsg(std::string& sender) const = 0;

    virtual std::shared_ptr<IIr77Return const> MemberUuid(std::shared_ptr<IIr77GUID const>& uid) const = 0;

    virtual std::shared_ptr<IIr77Return const> CollectionUuid(std::shared_ptr<IIr77GUID const>& uid) const = 0;

    virtual IIr77GUID* const QueryInterface(IIr77GUID const* iid, std::shared_ptr<void>& obj) = 0;

    virtual ~IIr77Enlisted() = default;

}* PIr77Enlisted;

static Ir77GUID GUIDQuerySucceeded{(static_cast<unsigned __int128>(0x08EE51633DE04390) << 64) | 0xAAAA2CAE6F127E59};
static Ir77GUID GUIDQueryFailed{(static_cast<unsigned __int128>(0x33E0AA3E5AFC46E1) << 64) | 0x8207214888E2FA17};

template <typename T>
std::shared_ptr<T> QueryAs(IIr77GUID const* iid, IIr77Enlisted* obj) {
    std::shared_ptr<void> iface{};

    if (obj->QueryInterface(iid, iface) != &GUIDQuerySucceeded) return nullptr;

    return std::static_pointer_cast<T>(iface);
}
}  // namespace NSIr77RT