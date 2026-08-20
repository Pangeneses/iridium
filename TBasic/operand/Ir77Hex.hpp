#pragma once

#include <chrono>
#include <memory>
#include <string>
#include <mutex>

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

using namespace NSIr77RT;

static std::vector<char> Ir77HexValidation{'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'a', 'B', 'b', 'C', 'c', 'D', 'd', 'E', 'e', 'F', 'f'};

namespace NSIr77TBasic {
class Ir77Hex : public Ir77Enlisted, public Ir77Operand, public std::enable_shared_from_this<Ir77Hex> {
   public:
    Ir77Hex() {
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
        seat_shared_uuid<&GUIDOPIr77Hex>(uid);

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

        else if (iid == &GUIDOPIr77Hex)
            obj = std::shared_ptr<Ir77Hex>(shared_from_this(), static_cast<Ir77Hex*>(this));

        else
            return &GUIDQueryFailed;

        return &GUIDQuerySucceeded;
    }

   public:
    std::shared_ptr<IIr77Return const> SetIndexed(std::uint32_t const& at, std::shared_ptr<IIr77Enlisted const>& obj) {
        auto raw = QueryAs<IIr77MPVMOP<Ir77String>>(&GUIDOPIr77TimePoint, std::const_pointer_cast<IIr77Enlisted>(obj).get());

        if (!raw.get()) return Ir77RETURN<Ir77OperationFailed>(nullptr, "Invalid RHS Operand.");

        try {
            IsValidHEX(raw->Get());
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
            IsValidHEX(raw->Get());
        } catch (std::domain_error) {
            return Ir77RETURN<Ir77OperationFailed>(nullptr, "Invalid Operand.");
        }

        auto lhs_mutable = std::const_pointer_cast<IIr77Operand>(lhs);

        lhs_mutable->SetIndexed(0, enlisted_rhs);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    static std::shared_ptr<IIr77Return const> Get(std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) {
        Ir77MPVMOP<Ir77String> ret;

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

        try {
            IsValidHEX(raw_lhs->Get());
        } catch (std::domain_error) {
            return Ir77RETURN<Ir77OperationFailed>(nullptr, "Invalid Operand.");
        }

        if (!raw_rhs.get()) return Ir77RETURN<Ir77OperationFailed>(nullptr, "Invalid RHS Operand.");

        try {
            IsValidHEX(raw_rhs->Get());
        } catch (std::domain_error) {
            return Ir77RETURN<Ir77OperationFailed>(nullptr, "Invalid Operand.");
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

        try {
            IsValidHEX(raw_lhs->Get());
        } catch (std::domain_error) {
            return Ir77RETURN<Ir77OperationFailed>(nullptr, "Invalid Operand.");
        }

        if (!raw_rhs.get()) return Ir77RETURN<Ir77OperationFailed>(nullptr, "Invalid RHS Operand.");

        try {
            IsValidHEX(raw_rhs->Get());
        } catch (std::domain_error) {
            return Ir77RETURN<Ir77OperationFailed>(nullptr, "Invalid Operand.");
        }

        if (raw_lhs->Get() != raw_rhs->Get()) {
            return Ir77RETURN<Ir77True>();
        } else {
            return Ir77RETURN<Ir77False>();
        }
    }

    static std::shared_ptr<IIr77Return const> Lesser(std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) {
        std::shared_ptr<IIr77Enlisted const> enlisted_lhs;
        lhs->GetIndexed(0, enlisted_lhs);

        std::shared_ptr<IIr77Enlisted const> enlisted_rhs;
        rhs->GetIndexed(0, enlisted_rhs);

        auto raw_lhs = QueryAs<IIr77MPVMOP<Ir77String>>(&GUIDOPIr77String, std::const_pointer_cast<IIr77Enlisted>(enlisted_lhs).get());

        auto raw_rhs = QueryAs<IIr77MPVMOP<Ir77String>>(&GUIDOPIr77String, std::const_pointer_cast<IIr77Enlisted>(enlisted_rhs).get());

        if (!raw_lhs.get()) return Ir77RETURN<Ir77OperationFailed>(nullptr, "Invalid LHS Operand.");

        try {
            IsValidHEX(raw_lhs->Get());
        } catch (std::domain_error) {
            return Ir77RETURN<Ir77OperationFailed>(nullptr, "Invalid Operand.");
        }

        if (!raw_rhs.get()) return Ir77RETURN<Ir77OperationFailed>(nullptr, "Invalid RHS Operand.");

        try {
            IsValidHEX(raw_rhs->Get());
        } catch (std::domain_error) {
            return Ir77RETURN<Ir77OperationFailed>(nullptr, "Invalid Operand.");
        }

        if (raw_lhs->Get() < raw_rhs->Get()) {
            return Ir77RETURN<Ir77True>();
        } else {
            return Ir77RETURN<Ir77False>();
        }
    }

    static std::shared_ptr<IIr77Return const> Greater(std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) {
        std::shared_ptr<IIr77Enlisted const> enlisted_lhs;
        lhs->GetIndexed(0, enlisted_lhs);

        std::shared_ptr<IIr77Enlisted const> enlisted_rhs;
        rhs->GetIndexed(0, enlisted_rhs);

        auto raw_lhs = QueryAs<IIr77MPVMOP<Ir77String>>(&GUIDOPIr77String, std::const_pointer_cast<IIr77Enlisted>(enlisted_lhs).get());

        auto raw_rhs = QueryAs<IIr77MPVMOP<Ir77String>>(&GUIDOPIr77String, std::const_pointer_cast<IIr77Enlisted>(enlisted_rhs).get());

        if (!raw_lhs.get()) return Ir77RETURN<Ir77OperationFailed>(nullptr, "Invalid LHS Operand.");

        try {
            IsValidHEX(raw_lhs->Get());
        } catch (std::domain_error) {
            return Ir77RETURN<Ir77OperationFailed>(nullptr, "Invalid Operand.");
        }

        if (!raw_rhs.get()) return Ir77RETURN<Ir77OperationFailed>(nullptr, "Invalid RHS Operand.");

        try {
            IsValidHEX(raw_rhs->Get());
        } catch (std::domain_error) {
            return Ir77RETURN<Ir77OperationFailed>(nullptr, "Invalid Operand.");
        }

        if (raw_lhs->Get() > raw_rhs->Get()) {
            return Ir77RETURN<Ir77True>();
        } else {
            return Ir77RETURN<Ir77False>();
        }
    }

    static void IsValidHEX(std::string const& hex) {
        for (char c : hex) {
            if (std::find(Ir77HexValidation.begin(), Ir77HexValidation.end(), c) == Ir77HexValidation.end()) {
                throw std::domain_error{"Invalid Hex Character."};
            }
        }
    }

   private:
    Ir77GUID m_enlisted_uuid{};

    std::chrono::system_clock::time_point m_enlisted{};

    std::chrono::system_clock::time_point m_delisted{};

    std::shared_ptr<IIr77Enlisted const> m_sender{nullptr};

    std::string m_message{""};

    bool m_valid{true};

    bool m_sealed{false};

    std::shared_ptr<IIr77Return const> m_invalidation_condition{nullptr};

    std::shared_ptr<IIr77Patch> m_patch{nullptr};

    std::mutex g_persist_mutex;
};
}  // namespace NSIr77TBasic
