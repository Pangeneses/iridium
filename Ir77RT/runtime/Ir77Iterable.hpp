#pragma once

#include <chrono>

#include "../dictionary/IDIIr77MPVM.hpp"
#include "../dictionary/IDIr77MPVM.hpp"

#include "../interface/IIr77GUID.hpp"
#include "../interface/IIr77Enlisted.hpp"
#include "../interface/IIr77Iterable.hpp"
#include "../interface/IIr77Return.hpp"

#include "../runtime/Ir77GUID.hpp"
#include "../runtime/Ir77Enlisted.hpp"

namespace NSIr77RT {

class Ir77Iterable : public Ir77Enlisted, public IIr77Iterable, public std::enable_shared_from_this<Ir77Iterable> {
   public:
    Ir77Iterable() {
        try {
            m_enlisted_uuid.Generate();
        } catch (std::invalid_argument a) {
            throw a;
        }

        m_enlisted = std::chrono::system_clock::now();
    }

   public:
    std::shared_ptr<IIr77Return const> EnlistedAs(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIIr77Iterable>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> MemberUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77Iterable>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CollectionUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIIr77MPVM>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    Ir77GUID QueryInterface(Ir77GUID iid, std::shared_ptr<void>& obj) {
        if (iid == GUIDIIr77Enlisted)
            obj = std::shared_ptr<IIr77Enlisted>(shared_from_this(), static_cast<IIr77Enlisted*>(this));

        else if (iid == GUIDIIr77Iterable)
            obj = std::shared_ptr<IIr77Iterable>(shared_from_this(), static_cast<IIr77Iterable*>(this));

        else if (iid == GUIDIr77Iterable)
            obj = std::shared_ptr<Ir77Iterable>(shared_from_this(), static_cast<Ir77Iterable*>(this));

        else
            return GUIDQueryFailed;

        return GUIDQuerySucceeded;
    }

   public:
    Ir77TypeEnum IsType() const { return m_type; }

    bool IsTypeOf(Ir77TypeEnum const& type) { return m_type == type; }

    bool IsBefore() const { return m_at == -1; }

    bool Begin() {
        m_at = 0;

        return true;
    }

    bool Next() {
        if (m_at == m_data->size()) return false;

        m_at++;

        return true;
    }

    bool Previous() {
        if (m_at == -1) return false;

        m_at--;

        return true;
    }

    bool End() { return m_at == m_data->size() - 1; }

    bool IsAfter() const { return m_at == m_data->size(); }

    std::shared_ptr<IIr77Enlisted const> Current() const {
        if (m_at == -1 || m_at == m_data->size())
            return nullptr;

        else
            return m_data->at(m_at);
    }

    std::shared_ptr<IIr77Return const> SetCurrent(std::shared_ptr<IIr77Enlisted const>& current) {
        if (m_at == UINT32_MAX || m_at == m_data->size()) return Ir77RETURN<Ir77False>();

        m_data->at(m_at) = std::const_pointer_cast<IIr77Enlisted>(current);

        return Ir77RETURN<Ir77True>();
    }

   private:
    Ir77TypeEnum m_type;

    std::shared_ptr<std::vector<std::shared_ptr<IIr77Enlisted>>> m_data{};

    uint32_t m_at{0};
};

}  // namespace NSIr77RT