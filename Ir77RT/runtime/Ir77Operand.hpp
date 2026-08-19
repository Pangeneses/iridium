#pragma once

#include <string>
#include <vector>
#include <memory>

#include "../interface/IIr77Enlisted.hpp"
#include "../interface/IIr77Operand.hpp"
#include "../interface/IIr77Return.hpp"

#include "../runtime/Ir77Return.hpp"

namespace NSIr77RT {
class Ir77Operand : public IIr77Operand {
   public:
    Ir77Operand() = default;

    std::shared_ptr<IIr77Return const> SetInstanceID(std::uint64_t const& id) {
        m_id = id;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> Resize(std::uint32_t const& sz) {
        if (m_sealed) return Ir77RETURN<Ir77Sealed>(this, "Operand has been sealed.");

        (void)sz;

        m_operand.resize(sz);

        for (std::shared_ptr<IIr77Enlisted const> enlisted : m_operand) {
            enlisted = nullptr;
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> Size(std::uint32_t& sz) const {
        if (m_sealed) return Ir77RETURN<Ir77Sealed>(this, "Operand has been sealed.");

        sz = m_operand.size();

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

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

    std::shared_ptr<IIr77Return const> SetOpcode(std::shared_ptr<IIr77GUID const>& obj) {
        m_opcode = obj;
        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetOpcode(std::shared_ptr<IIr77GUID const>& obj) const {
        obj = m_opcode;
        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    virtual std::shared_ptr<IIr77Return const> SetIndexed(std::uint32_t const& at, std::shared_ptr<IIr77Enlisted const>& obj) = 0;

    std::shared_ptr<IIr77Return const> GetIndexed(std::uint32_t const& at, std::shared_ptr<IIr77Enlisted const>& obj) const {
        if (m_sealed) return Ir77RETURN<Ir77Sealed>(this, "Operand has been sealed.");

        if (m_operand.size() < at + 1) Ir77RETURN<Ir77InvalidOperation>(this, "Modify RAW using Ir77MPVMOP interface functions.");

        obj = m_operand.at(at);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   protected:
    bool m_sealed{false};

    std::vector<std::shared_ptr<IIr77Enlisted const>> m_operand;

    std::uint64_t m_id;

    bool m_complete{false};

    std::shared_ptr<IIr77GUID const> m_opcode;

    std::shared_ptr<IIr77Operand const> m_outcome{};

    std::shared_ptr<IIr77Return const> m_return = Ir77RETURN<Ir77Unknown>();
};

}  // namespace NSIr77RT