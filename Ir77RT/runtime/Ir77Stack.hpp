#pragma once

#include <chrono>
#include <memory>

#include "../dictionary/IDIIr77MPVM.hpp"
#include "../dictionary/IDIr77MPVM.hpp"

#include "../interface/IIr77GUID.hpp"
#include "../interface/IIr77Enlisted.hpp"
#include "../interface/IIr77Stack.hpp"
#include "../interface/IIr77Iterator.hpp"
#include "../interface/IIr77Return.hpp"

#include "IIr77Operand.hpp"
#include "Ir77GUID.hpp"

#include "Ir77Enlisted.hpp"
#include "Ir77Fitt.hpp"
#include "Ir77Ritt.hpp"

namespace NSIr77RT {

class Ir77Stack : public Ir77Enlisted, public IIr77Stack, public std::enable_shared_from_this<Ir77Stack> {
   public:
    Ir77Stack() {
        try {
            m_enlisted_uuid.Generate();
        } catch (std::invalid_argument a) {
            throw a;
        }

        m_enlisted = std::chrono::system_clock::now();
    }

   public:
    std::shared_ptr<IIr77Return const> EnlistedAs(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIIr77Stack>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> MemberUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77Stack>(uid);

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

        else if (iid == &GUIDIIr77Stack)
            obj = std::shared_ptr<IIr77Stack>(shared_from_this(), static_cast<IIr77Stack*>(this));

        else if (iid == &GUIDIr77Stack)
            obj = std::shared_ptr<Ir77Stack>(shared_from_this(), static_cast<Ir77Stack*>(this));

        else
            return &GUIDQueryFailed;

        return &GUIDQuerySucceeded;
    }

   public:
    std::shared_ptr<IIr77Return const> IsEmpty() const {
        if (m_data->empty())
            return Ir77RETURN<Ir77True>();

        else
            return Ir77RETURN<Ir77False>();
    }

    std::shared_ptr<IIr77Return const> PushOperation(Ir77Operation operation) {
        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        if (m_sealed) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        m_data->push_back(operation);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SealStack() {
        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        if (m_sealed) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        m_sealed = true;

        return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");
    }

    std::shared_ptr<IIr77Return const> IsSealed() const {
        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        if (m_sealed)
            return Ir77RETURN<Ir77True>();

        else
            return Ir77RETURN<Ir77False>();
    }

    std::shared_ptr<IIr77Return const> ForwardIterator(std::shared_ptr<IIr77Iterator const>& itt) {
        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        if (m_data->empty()) return Ir77RETURN<Ir77Empty>();

        std::shared_ptr<Ir77Fitt> fitt = std::make_shared<Ir77Fitt>();

        fitt->m_data = m_data;

        m_forward_iterators.push_back(fitt);

        m_forward_iterators.back()->IttBegin();

        itt = m_forward_iterators.back();

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> ReverseIterator(std::shared_ptr<IIr77Iterator const>& itt) {
        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        if (m_data->empty()) return Ir77RETURN<Ir77Empty>();

        std::shared_ptr<Ir77Ritt> ritt = std::make_shared<Ir77Ritt>();

        ritt->m_data = m_data;

        m_reverse_iterators.push_back(ritt);

        m_reverse_iterators.back()->IttBegin();

        itt = m_reverse_iterators.back();

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> InvalidateStack(std::shared_ptr<IIr77Return const>& condition) {
        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        m_valid = false;

        m_invalidation_condition = condition;

        std::vector<std::shared_ptr<Ir77Fitt>>::iterator itt{};

        for (itt = m_forward_iterators.begin(); itt != m_forward_iterators.end(); itt++) {
            (*itt)->InvalidateIterator(condition);
        }

        std::vector<std::shared_ptr<Ir77Ritt>>::iterator jtt{};

        for (jtt = m_reverse_iterators.begin(); jtt != m_reverse_iterators.end(); jtt++) {
            (*jtt)->InvalidateIterator(condition);
        }

        m_delisted = std::chrono::system_clock::now();

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> IsInvalid(std::shared_ptr<IIr77Return const>& condition) const {
        if (!m_valid)
            return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        else
            condition = m_invalidation_condition;
        return Ir77RETURN<Ir77True>();
    }

   public:
    std::shared_ptr<IIr77Return const> SetState(OperandState const& get) {
        m_state = get;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetState(OperandState& get) {
        get = m_state;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> Gate(std::shared_ptr<IIr77Operand const> op) {
        std::vector<std::shared_ptr<IIr77Operand const>>::iterator itt;

        for (itt = m_locked.begin(); itt != m_locked.end(); itt++) {
            std::shared_ptr<const IIr77GUID> op_uid;
            op->EnlistedUuid(op_uid);
            std::shared_ptr<const IIr77GUID> itt_uid;
            (*itt)->EnlistedUuid(itt_uid);
            if (op_uid == itt_uid) {
                std::swap(*itt, m_locked.back());
                m_locked.pop_back();
                break;
            }
        }

        if (m_locked.size() == 0) m_state = OperandState::Run;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> Complete() {
        m_state = OperandState::Retired;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // ── Data ─────────────────────────────────────────────────────
   private:
    Ir77GUID m_enlisted_uuid{};

    std::chrono::system_clock::time_point m_enlisted{};

    std::chrono::system_clock::time_point m_delisted{};

    std::shared_ptr<IIr77Enlisted const> m_sender{nullptr};

    std::string m_message{""};

    bool m_valid{true};

    bool m_sealed{false};

    std::vector<std::shared_ptr<Ir77Fitt>> m_forward_iterators{};

    std::vector<std::shared_ptr<Ir77Ritt>> m_reverse_iterators{};

    std::shared_ptr<std::vector<Ir77Operation>> m_data{};

    OperandState m_state = OperandState::Wait;

    std::vector<std::shared_ptr<IIr77Operand const>> m_locked;

    std::shared_ptr<IIr77Return const> m_invalidation_condition{nullptr};
};

}  // namespace NSIr77RT