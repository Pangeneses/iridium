#pragma once

#include <chrono>
#include <memory>
#include <string>

#include "../../Ir77RT/dictionary/IDOPIr77MPVM.hpp"

#include "../dictionary/IDOPIr77TBASIC.hpp"

#include "../../Ir77RT/interface/IIr77GUID.hpp"
#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Patch.hpp"
#include "../../Ir77RT/interface/IIr77Operand.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"
#include "../../Ir77RT/runtime/Ir77Operand.hpp"
#include "../../Ir77RT/runtime/Ir77Return.hpp"

#include "../../Ir77RT/operand/Ir77MPVMOP.hpp"
#include "IDIIr77MPVM.hpp"

using namespace NSIr77RT;

namespace NSIr77TBasic {
class Ir77AlphaNumeric : public Ir77Enlisted, public Ir77Operand, public std::enable_shared_from_this<Ir77AlphaNumeric> {
   public:
    Ir77AlphaNumeric() {
        try {
            m_enlisted_uuid.Generate();
        } catch (std::invalid_argument a) {
            throw a;
        }

        m_enlisted = std::chrono::system_clock::now();

        m_operand.resize(1);
    }

   public:
    std::shared_ptr<IIr77Return const> EnlistedAs(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIIr77Operand>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> MemberOfUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDOPIr77AlphaNumeric>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CollectionUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDOPIr77TBasic>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    IIr77GUID* const QueryInterface(IIr77GUID const* iid, std::shared_ptr<void>& obj) {
        if (iid == &GUIDIIr77Enlisted)
            obj = std::shared_ptr<IIr77Enlisted>(shared_from_this(), static_cast<IIr77Enlisted*>(this));

        else if (iid == &GUIDIIr77Operand)
            obj = std::shared_ptr<IIr77Operand>(shared_from_this(), static_cast<IIr77Operand*>(this));

        else if (iid == &GUIDOPIr77AlphaNumeric)
            obj = std::shared_ptr<Ir77AlphaNumeric>(shared_from_this(), static_cast<Ir77AlphaNumeric*>(this));

        else
            return &GUIDQueryFailed;

        return &GUIDQuerySucceeded;
    }

   public:
    std::shared_ptr<IIr77Return const> SetIndexed(std::uint32_t const& at, std::shared_ptr<IIr77Enlisted const>& obj) {
        auto raw = QueryAs<IIr77MPVMOP<Ir77String>>(&GUIDOPIr77String, std::const_pointer_cast<IIr77Enlisted>(obj).get());

        if (!raw.get()) return Ir77RETURN<Ir77OperationFailed>(nullptr, "Invalid RHS Operand.");

        try {
            IsValidAlphaNumeric(raw->Get());
        } catch (std::domain_error) {
            return Ir77RETURN<Ir77OperationFailed>(nullptr, "Invalid Operand.");
        }

        m_operand.at(at) = obj;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   public:
    static std::shared_ptr<IIr77Return const> Assign(std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) {
        std::shared_ptr<IIr77Enlisted const> enlisted_rhs;
        rhs->GetIndexed(0, enlisted_rhs);

        auto rhs_mutable = std::const_pointer_cast<IIr77Operand>(rhs);

        auto raw = QueryAs<IIr77MPVMOP<Ir77String>>(&GUIDOPIr77String, std::const_pointer_cast<IIr77Enlisted>(enlisted_rhs).get());

        if (!raw.get()) return Ir77RETURN<Ir77OperationFailed>(nullptr, "Invalid RHS Operand.");

        try {
            IsValidAlphaNumeric(raw->Get());
        } catch (std::domain_error) {
            return Ir77RETURN<Ir77OperationFailed>(nullptr, "Invalid Operand.");
        }

        auto lhs_mutable = std::const_pointer_cast<IIr77Operand>(lhs);

        lhs_mutable->SetIndexed(0, enlisted_rhs);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    static std::shared_ptr<IIr77Return const> Get(std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) {
        std::shared_ptr<IIr77Enlisted const> enlisted_lhs;
        lhs->GetIndexed(0, enlisted_lhs);

        auto rhs_mutable = std::const_pointer_cast<IIr77Operand>(rhs);

        rhs_mutable->SetIndexed(0, enlisted_lhs);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    static std::shared_ptr<IIr77Return const> Equal(std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) {
        std::shared_ptr<IIr77Enlisted const> enlisted_lhs;
        lhs->GetIndexed(0, enlisted_lhs);

        std::shared_ptr<IIr77Enlisted const> enlisted_rhs;
        rhs->GetIndexed(0, enlisted_rhs);

        auto raw_lhs = QueryAs<IIr77MPVMOP<Ir77String>>(&GUIDOPIr77String, std::const_pointer_cast<IIr77Enlisted>(enlisted_lhs).get());

        auto raw_rhs = QueryAs<IIr77MPVMOP<Ir77String>>(&GUIDOPIr77String, std::const_pointer_cast<IIr77Enlisted>(enlisted_rhs).get());

        if (!raw_lhs.get()) return Ir77RETURN<Ir77OperationFailed>(nullptr, "Invalid LHS Operand.");

        if (!raw_rhs.get()) return Ir77RETURN<Ir77OperationFailed>(nullptr, "Invalid RHS Operand.");

        try {
            IsValidAlphaNumeric(raw_lhs->Get());
        } catch (std::domain_error) {
            return Ir77RETURN<Ir77OperationFailed>(nullptr, "Invalid LHS Operand.");
        }

        try {
            IsValidAlphaNumeric(raw_rhs->Get());
        } catch (std::domain_error) {
            return Ir77RETURN<Ir77OperationFailed>(nullptr, "Invalid RHS Operand.");
        }

        if (raw_lhs->Get() == raw_rhs->Get()) {
            return Ir77RETURN<Ir77True>();
        } else {
            return Ir77RETURN<Ir77False>();
        }
    }

    static std::shared_ptr<IIr77Return const> Not(std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) {
        std::shared_ptr<IIr77Enlisted const> enlisted_lhs;
        lhs->GetIndexed(0, enlisted_lhs);

        std::shared_ptr<IIr77Enlisted const> enlisted_rhs;
        rhs->GetIndexed(0, enlisted_rhs);

        auto raw_lhs = QueryAs<IIr77MPVMOP<Ir77String>>(&GUIDOPIr77String, std::const_pointer_cast<IIr77Enlisted>(enlisted_lhs).get());

        auto raw_rhs = QueryAs<IIr77MPVMOP<Ir77String>>(&GUIDOPIr77String, std::const_pointer_cast<IIr77Enlisted>(enlisted_rhs).get());

        if (!raw_lhs.get()) return Ir77RETURN<Ir77OperationFailed>(nullptr, "Invalid LHS Operand.");

        if (!raw_rhs.get()) return Ir77RETURN<Ir77OperationFailed>(nullptr, "Invalid RHS Operand.");

        try {
            IsValidAlphaNumeric(raw_lhs->Get());
        } catch (std::domain_error) {
            return Ir77RETURN<Ir77OperationFailed>(nullptr, "Invalid LHS Operand.");
        }

        try {
            IsValidAlphaNumeric(raw_rhs->Get());
        } catch (std::domain_error) {
            return Ir77RETURN<Ir77OperationFailed>(nullptr, "Invalid RHS Operand.");
        }

        if (raw_lhs->Get() != raw_rhs->Get()) {
            return Ir77RETURN<Ir77True>();
        } else {
            return Ir77RETURN<Ir77False>();
        }
    }

    static void IsValidAlphaNumeric(std::string const& input) {
        static const std::vector<char> Ir77AlphaNumericW{'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p',
                                                         'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', 'A', 'B', 'C', 'D', 'E', 'F',
                                                         'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V',
                                                         'W', 'X', 'Y', 'Z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};

        for (char c : input) {
            if (std::find(Ir77AlphaNumericW.begin(), Ir77AlphaNumericW.end(), c) == Ir77AlphaNumericW.end()) {
                throw std::domain_error{"Non-AlphaNumeric character."};
            }
        }
    }

   private:
    std::shared_ptr<IIr77Patch> m_patch{nullptr};
};

}  // namespace NSIr77TBasic
