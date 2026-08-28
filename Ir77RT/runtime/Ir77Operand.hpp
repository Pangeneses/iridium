#pragma once

#include <string>
#include <map>
#include <memory>

#include "../interface/IIr77Enlisted.hpp"
#include "../interface/IIr77Operand.hpp"
#include "../interface/IIr77Return.hpp"

#include "IIr77GUID.hpp"

#include "../runtime/Ir77Return.hpp"

namespace NSIr77RT {
class Ir77Operand : public IIr77Operand {
   public:
    Ir77Operand() = default;

    std::shared_ptr<IIr77Return const> IsEmpty() const {
        if (m_sealed) return Ir77RETURN<Ir77Sealed>(this, "Operand has been sealed.");

        if (m_operand.size() == 0) {
            return Ir77RETURN<Ir77True>();
        } else {
            return Ir77RETURN<Ir77False>();
        }
    }

    std::shared_ptr<IIr77Return const> SealOperand() {
        if (m_sealed) return Ir77RETURN<Ir77Sealed>(this, "Operand has been sealed.");

        m_sealed = true;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> IsSealed() const {
        if (m_sealed) {
            return Ir77RETURN<Ir77True>();
        } else {
            return Ir77RETURN<Ir77False>();
        }
    }

    virtual std::shared_ptr<IIr77Return const> SetIndexed(std::uint64_t const& at, std::shared_ptr<IIr77Enlisted const>& obj) = 0;

    std::shared_ptr<IIr77Return const> GetIndexed(std::uint64_t const& at, std::shared_ptr<IIr77Enlisted const>& obj) const {
        if (m_sealed) return Ir77RETURN<Ir77Sealed>(this, "Operand has been sealed.");

        obj = m_operand.at(at);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   protected:
    bool m_sealed{false};

    std::map<std::uint64_t, std::shared_ptr<IIr77Enlisted const>> m_operand;

    bool m_complete{false};
};

}  // namespace NSIr77RT