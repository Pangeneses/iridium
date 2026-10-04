#pragma once

#include <chrono>
#include <memory>
#include <string>
#include <mutex>

#include "../../Ir77RT/dictionary/IDOPIr77MPVM.hpp"

#include "../dictionary/IDOPIr77TBASIC.hpp"
#include "../dictionary/IDMIr77TBASIC.hpp"

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

namespace NSIr77TBasic {
class Ir77TextBlobW : public Ir77Enlisted, public Ir77Operand, public std::enable_shared_from_this<Ir77TextBlobW> {
   public:
    Ir77TextBlobW() {
        try {
            m_enlisted_uuid.Generate();
        } catch (std::invalid_argument a) {
            throw a;
        }

        m_enlisted = std::chrono::system_clock::now();
    }

   public:
    std::shared_ptr<IIr77Return const> EnlistedAs(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIIr77Operand>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> MemberUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDOPIr77TextBlobW>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CollectionUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDOPIr77TBasic>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    Ir77GUID QueryInterface(Ir77GUID iid, std::shared_ptr<void>& obj) {
        if (iid == GUIDIIr77Enlisted)
            obj = std::shared_ptr<IIr77Enlisted>(shared_from_this(), static_cast<IIr77Enlisted*>(this));

        else if (iid == GUIDIIr77Operand)
            obj = std::shared_ptr<IIr77Operand>(shared_from_this(), static_cast<IIr77Operand*>(this));

        else if (iid == GUIDOPIr77TextBlobW)
            obj = std::shared_ptr<Ir77TextBlobW>(shared_from_this(), static_cast<Ir77TextBlobW*>(this));

        else
            return GUIDQueryFailed;

        return GUIDQuerySucceeded;
    }

   public:
    std::shared_ptr<IIr77Return const> SetIndexed(std::uint64_t const& at, std::shared_ptr<IIr77Enlisted const>& obj) {
        auto raw = QueryAs<IIr77MPVMOP<Ir77String>>(GUIDOPIr77TimePoint, std::const_pointer_cast<IIr77Enlisted>(obj).get());

        if (!raw.get()) return Ir77RETURN<Ir77OperationFailed>(nullptr, "Invalid RHS Operand.");

        m_operand.at(at) = obj;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   public:
    static std::shared_ptr<IIr77Return const> Assign(std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) {
        std::shared_ptr<IIr77Enlisted const> enlisted_rhs;
        rhs->GetIndexed(ID_VALUE, enlisted_rhs);

        auto raw = QueryAs<IIr77MPVMOP<Ir77WString>>(GUIDOPIr77WString, std::const_pointer_cast<IIr77Enlisted>(enlisted_rhs).get());

        if (!raw.get()) return Ir77RETURN<Ir77OperationFailed>(nullptr, "Invalid RHS Operand.");

        auto lhs_mutable = std::const_pointer_cast<IIr77Operand>(lhs);

        lhs_mutable->SetIndexed(ID_RETURN, enlisted_rhs);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    static std::shared_ptr<IIr77Return const> Get(std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) {
        std::shared_ptr<IIr77Enlisted const> enlisted_lhs;
        lhs->GetIndexed(ID_VALUE, enlisted_lhs);

        auto rhs_mutable = std::const_pointer_cast<IIr77Operand>(rhs);

        rhs_mutable->SetIndexed(ID_RETURN, enlisted_lhs);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    static std::shared_ptr<IIr77Return const> Equal(std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) {
        std::shared_ptr<IIr77Enlisted const> enlisted_lhs;
        lhs->GetIndexed(ID_VALUE, enlisted_lhs);

        std::shared_ptr<IIr77Enlisted const> enlisted_rhs;
        rhs->GetIndexed(ID_VALUE, enlisted_rhs);

        auto raw_lhs = QueryAs<IIr77MPVMOP<Ir77WString>>(GUIDOPIr77WString, std::const_pointer_cast<IIr77Enlisted>(enlisted_lhs).get());

        auto raw_rhs = QueryAs<IIr77MPVMOP<Ir77WString>>(GUIDOPIr77WString, std::const_pointer_cast<IIr77Enlisted>(enlisted_rhs).get());

        if (!raw_lhs.get()) return Ir77RETURN<Ir77OperationFailed>(nullptr, "Invalid LHS Operand.");

        if (!raw_rhs.get()) return Ir77RETURN<Ir77OperationFailed>(nullptr, "Invalid RHS Operand.");

        auto lhs_mutable = std::const_pointer_cast<IIr77Operand>(lhs);

        std::shared_ptr<Ir77MPVMOP<Ir77Boolean>> ret = std::make_shared<Ir77MPVMOP<Ir77Boolean>>();

        if (raw_lhs->Get() == raw_rhs->Get()) {
            ret->Set(true);
        } else {
            ret->Set(false);
        }

        auto ret_immutable = std::reinterpret_pointer_cast<IIr77Enlisted const>(std::const_pointer_cast<IIr77Operand const>(ret));

        lhs_mutable->SetIndexed(ID_RETURN, ret_immutable);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    static std::shared_ptr<IIr77Return const> Not(std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) {
        std::shared_ptr<IIr77Enlisted const> enlisted_lhs;
        lhs->GetIndexed(ID_VALUE, enlisted_lhs);

        std::shared_ptr<IIr77Enlisted const> enlisted_rhs;
        rhs->GetIndexed(ID_VALUE, enlisted_rhs);

        auto raw_lhs = QueryAs<IIr77MPVMOP<Ir77WString>>(GUIDOPIr77WString, std::const_pointer_cast<IIr77Enlisted>(enlisted_lhs).get());

        auto raw_rhs = QueryAs<IIr77MPVMOP<Ir77WString>>(GUIDOPIr77WString, std::const_pointer_cast<IIr77Enlisted>(enlisted_rhs).get());

        if (!raw_lhs.get()) return Ir77RETURN<Ir77OperationFailed>(nullptr, "Invalid LHS Operand.");

        if (!raw_rhs.get()) return Ir77RETURN<Ir77OperationFailed>(nullptr, "Invalid RHS Operand.");

        auto lhs_mutable = std::const_pointer_cast<IIr77Operand>(lhs);

        std::shared_ptr<Ir77MPVMOP<Ir77Boolean>> ret = std::make_shared<Ir77MPVMOP<Ir77Boolean>>();

        if (raw_lhs->Get() != raw_rhs->Get()) {
            ret->Set(true);
        } else {
            ret->Set(false);
        }

        auto ret_immutable = std::reinterpret_pointer_cast<IIr77Enlisted const>(std::const_pointer_cast<IIr77Operand const>(ret));

        lhs_mutable->SetIndexed(ID_RETURN, ret_immutable);

        return Ir77RETURN<Ir77OperationSucceeded>();
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

    mutable std::mutex g_persist_mutex;
};

}  // namespace NSIr77TBasic