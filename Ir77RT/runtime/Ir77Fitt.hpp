#pragma once

#include <chrono>

#include "../dictionary/IDIIr77MPVM.hpp"
#include "../dictionary/IDIr77MPVM.hpp"

#include "../interface/IIr77GUID.hpp"
#include "../interface/IIr77Enlisted.hpp"
#include "../interface/IIr77Iterator.hpp"
#include "../interface/IIr77Return.hpp"

#include "../runtime/Ir77GUID.hpp"
#include "../runtime/Ir77Enlisted.hpp"

namespace NSIr77RT {

class Ir77Stack;

class Ir77Fitt : public Ir77Enlisted, public IIr77Iterator, public std::enable_shared_from_this<Ir77Fitt> {
   public:
    Ir77Fitt() {
        try {
            m_enlisted_uuid.Generate();
        } catch (std::invalid_argument a) {
            throw a;
        }

        m_enlisted = std::chrono::system_clock::now();
    }

   public:
    std::shared_ptr<IIr77Return const> MemberOfUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77Fitt>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CollectionUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIIr77MPVM>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    IIr77GUID* const QueryInterface(IIr77GUID const* iid, std::shared_ptr<void>& obj) {
        if (iid == &GUIDIIr77Enlisted)
            obj = std::shared_ptr<IIr77Enlisted>(shared_from_this(), static_cast<IIr77Enlisted*>(this));

        else if (iid == &GUIDIIr77Iterator)
            obj = std::shared_ptr<IIr77Iterator>(shared_from_this(), static_cast<IIr77Iterator*>(this));

        else if (iid == &GUIDIr77Fitt)
            obj = std::shared_ptr<Ir77Fitt>(shared_from_this(), static_cast<Ir77Fitt*>(this));

        else
            return &GUIDQueryFailed;

        return &GUIDQuerySucceeded;
    }

   public:
    std::shared_ptr<IIr77Return const> InvalidateIterator(std::shared_ptr<IIr77Return const>& condition) {
        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        m_valid = false;

        m_invalidation_condition = condition;

        m_delisted = std::chrono::system_clock::now();

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> IsInvalid(std::shared_ptr<IIr77Return const>& condition) const {
        if (!m_valid) {
            return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");
        } else {
            condition = m_invalidation_condition;

            return Ir77RETURN<Ir77True>();
        }
    }

    std::shared_ptr<IIr77Return const> IttBegin() {
        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        if (m_data->empty()) return Ir77RETURN<Ir77Empty>();

        m_itt = m_data->begin();

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> IttMove(std::int32_t& mov) {
        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        if (m_data->empty()) return Ir77RETURN<Ir77Empty>();

        if (!mov) return Ir77RETURN<Ir77InvalidArgument>(this, "Move is zero.");

        if (!(m_itt >= m_data->begin() && m_itt <= m_data->end())) {
            m_itt = m_data->begin();
        }

        uint32_t step = mov / std::abs(mov);

        std::vector<Ir77Operation>::iterator jtt{};

        for (jtt = m_itt; mov != 0; jtt += step) {
            if (jtt < m_data->begin() || jtt > m_data->end()) return Ir77RETURN<Ir77OutOfRange>();

            mov -= step;
        }

        m_itt = jtt;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> IttAt(std::uint32_t const& at) {
        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        if (m_data->empty()) return Ir77RETURN<Ir77Empty>();

        if ((m_data->size() - at) < 1) return Ir77RETURN<Ir77OutOfRange>();

        m_itt = m_data->begin();

        m_itt += at;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> IttCurrentOperation(Ir77Operation& current) {
        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        if (m_data->empty()) return Ir77RETURN<Ir77Empty>();

        if (m_itt == m_data->end()) return Ir77RETURN<Ir77OutOfRange>();

        if (!(m_itt >= m_data->begin() && m_itt <= m_data->end())) {
            m_itt = m_data->begin();
        }

        current = *m_itt;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> IttEnd() {
        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        if (m_data->empty()) return Ir77RETURN<Ir77Empty>();

        m_itt = m_data->end();

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> IttEndOfStack() {
        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        if (m_data->empty()) return Ir77RETURN<Ir77Empty>();

        if (m_itt == m_data->end())
            return Ir77RETURN<Ir77True>();

        else
            return Ir77RETURN<Ir77False>();
    }

    // ── Data ─────────────────────────────────────────────────────
   private:
    friend Ir77Stack;

    Ir77GUID m_enlisted_uuid{};

    std::chrono::system_clock::time_point m_enlisted{};

    std::chrono::system_clock::time_point m_delisted{};

    std::shared_ptr<IIr77Enlisted const> m_sender{nullptr};

    std::string m_message{""};

    bool m_valid{true};

    std::shared_ptr<IIr77Return const> m_invalidation_condition{nullptr};

    std::shared_ptr<std::vector<Ir77Operation>> m_data;

    std::vector<Ir77Operation>::iterator m_itt{};
};

}  // namespace NSIr77RT