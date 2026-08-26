#pragma once

#include <map>

#include "../dictionary/IDIIr77MPVM.hpp"
#include "../dictionary/IDIr77MPVM.hpp"

#include "../interface/IIr77GUID.hpp"
#include "../interface/IIr77Enlisted.hpp"
#include "../interface/IIr77Patch.hpp"
#include "../interface/IIr77Stack.hpp"
#include "../interface/IIr77Iterator.hpp"
#include "../interface/IIr77Operand.hpp"

#include "Ir77Enlisted.hpp"
#include "Ir77GUID.hpp"

namespace NSIr77RT {

class Ir77Patch : public Ir77Enlisted, public IIr77Patch, public std::enable_shared_from_this<Ir77Patch> {
   public:
    Ir77Patch() {
        try {
            m_enlisted_uuid.Generate();
        } catch (std::invalid_argument a) {
            throw a;
        }

        m_enlisted = std::chrono::system_clock::now();
    }

   public:
    std::shared_ptr<IIr77Return const> EnlistedAs(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIIr77Patch>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> MemberOfUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77Patch>(uid);

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

        else if (iid == &GUIDIIr77Patch)
            obj = std::shared_ptr<IIr77Patch>(shared_from_this(), static_cast<IIr77Patch*>(this));

        else if (iid == &GUIDIr77Patch)
            obj = std::shared_ptr<Ir77Patch>(shared_from_this(), static_cast<Ir77Patch*>(this));

        else
            return &GUIDQueryFailed;

        return &GUIDQuerySucceeded;
    }

    std::shared_ptr<IIr77Return const> AddOperation(Ir77Operator op, Ir7Execute execute) {
        if (execute == nullptr) return Ir77RETURN<Ir77InvalidOperation>(this, "Invalid function pointer.");

        if (!m_implementation.contains(op)) m_implementation.emplace(op, execute);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> Forward(std::shared_ptr<IIr77Stack const>& stack) {
        std::shared_ptr<IIr77Iterator const> fitt{};

        auto stack_mut = std::const_pointer_cast<IIr77Stack>(stack);

        if (stack_mut->ForwardIterator(fitt)->ID() != &GUIDIr77OperationSucceeded) {
            return Ir77RETURN<Ir77MPVMError>(this, "Cannot acquire iterator.");
        }

        auto fitt_mut = std::const_pointer_cast<IIr77Iterator>(fitt);

        std::int32_t step{1};

        for (fitt_mut->IttBegin(); fitt_mut->IttEndOfStack()->ID() != &GUIDIr77True; fitt_mut->IttMove(step)) {
            Ir77Operation operation{};

            if (fitt_mut->IttCurrentOperation(operation)->ID() != &GUIDIr77OperationSucceeded)
                return Ir77RETURN<Ir77MPVMError>(this, "Cannot acquire operation from stack.");

            operation.ret = m_implementation.at(operation.op)(operation.lhs, operation.rhs);

            if (operation.ret->ID() == &GUIDIr77OperationSucceeded) {
                std::shared_ptr<IIr77Return const> invalidated = Ir77RETURN<Ir77OperationFailed>(this, "Could not perform operation #.");

                fitt_mut->InvalidateIterator(invalidated);

                stack_mut->InvalidateStack(invalidated);

                return operation.ret;
            }
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   private:
    std::map<Ir77Operator, Ir7Execute> m_implementation{};
};

}  // namespace NSIr77RT