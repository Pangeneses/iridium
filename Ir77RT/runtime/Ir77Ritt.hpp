#pragma once

#include <chrono>

#include "../dictionary/IDIIr77MPVM.hpp"
#include "../dictionary/IDIr77MPVM.hpp"

#include "../interface/IIr77GUID.hpp"
#include "../interface/IIr77Enlisted.hpp"
#include "../interface/IIr77Iterator.hpp"
#include "../interface/IIr77Return.hpp"

#include "../runtime/Ir77Enlisted.hpp"
#include "../runtime/Ir77GUID.hpp"

namespace NSIr77RT {

class Ir77Stack;

class Ir77Ritt : public Ir77Enlisted, public IIr77Iterator, public std::enable_shared_from_this<Ir77Ritt> {
   public:
    Ir77Ritt() {
        try {
            m_enlisted_uuid.Generate();
        } catch (std::invalid_argument a) {
            throw a;
        }

        m_enlisted = std::chrono::system_clock::now();
    }

   public:
    std::shared_ptr<IIr77Return const> EnlistedAs(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIIr77Iterator>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> MemberUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77Ritt>(uid);

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

        else if (iid == GUIDIIr77Iterator)
            obj = std::shared_ptr<IIr77Iterator>(shared_from_this(), static_cast<IIr77Iterator*>(this));

        else if (iid == GUIDIr77Ritt)
            obj = std::shared_ptr<Ir77Ritt>(shared_from_this(), static_cast<Ir77Ritt*>(this));

        else
            return GUIDQueryFailed;

        return GUIDQuerySucceeded;
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

        m_ritt = m_data->rbegin();

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> IttMove(std::int32_t& mov) {
        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        if (m_data->empty()) return Ir77RETURN<Ir77Empty>();

        if (!mov) return Ir77RETURN<Ir77InvalidArgument>(this, "Move is zero.");

        if (!(m_ritt >= m_data->rbegin() && m_ritt <= m_data->rend())) {
            m_ritt = m_data->rbegin();
        }

        uint32_t step = mov / std::abs(mov);

        std::vector<Ir77Operation>::reverse_iterator jtt{};

        for (jtt = m_ritt; mov != 0; jtt += step) {
            if (jtt < m_data->rbegin() || jtt > m_data->rend()) return Ir77RETURN<Ir77OutOfRange>();

            mov -= step;
        }

        m_ritt = jtt;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> IttAt(std::uint32_t const& at) {
        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        if (m_data->empty()) return Ir77RETURN<Ir77Empty>();

        if ((m_data->size() - at) < 1) return Ir77RETURN<Ir77OutOfRange>();

        m_ritt = m_data->rbegin();

        m_ritt += at;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> IttCurrentOperation(Ir77Operation& operation) {
        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        if (m_data->empty()) return Ir77RETURN<Ir77Empty>();

        if (m_ritt == m_data->rend()) return Ir77RETURN<Ir77OutOfRange>();

        if (!(m_ritt >= m_data->rbegin() && m_ritt <= m_data->rend())) {
            m_ritt = m_data->rbegin();
        }

        operation = *m_ritt;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> IttEnd() {
        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        if (m_data->empty()) return Ir77RETURN<Ir77Empty>();

        m_ritt = m_data->rend();

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> IttEndOfStack() {
        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        if (m_data->empty()) return Ir77RETURN<Ir77Empty>();

        if (m_ritt == m_data->rend())
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

    std::vector<Ir77Operation>::reverse_iterator m_ritt{};
};

}  // namespace NSIr77RT