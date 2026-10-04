#pragma once

#include <chrono>
#include <memory>
#include <string>
#include <mutex>
#include <unordered_map>

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

static const std::vector<char> Ir77MMDDYYYYNumeric{'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};

static const std::unordered_map<std::string, std::string> Ir77MMDD{{"01", "31"}, {"02", "28"}, {"03", "31"}, {"04", "30"}, {"05", "31"}, {"06", "30"},
                                                                   {"07", "31"}, {"08", "31"}, {"09", "30"}, {"10", "31"}, {"11", "30"}, {"12", "31"}};

static const std::vector<std::string> Ir77Leap{"2004", "2008", "2012", "2016", "2020", "2024", "2028", "2032", "2036", "2040", "2044", "2048",
                                               "2052", "2056", "2060", "2064", "2068", "2072", "2076", "2080", "2084", "2088", "2092", "2096"};

namespace NSIr77TBasic {
class Ir77MMDDYYYY : public Ir77Enlisted, public Ir77Operand, public std::enable_shared_from_this<Ir77MMDDYYYY> {
   public:
    Ir77MMDDYYYY() {
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
        seat_shared_uuid<&GUIDOPIr77MMDDYYYY>(uid);

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

        else if (iid == GUIDOPIr77MMDDYYYY)
            obj = std::shared_ptr<Ir77MMDDYYYY>(shared_from_this(), static_cast<Ir77MMDDYYYY*>(this));

        else
            return GUIDQueryFailed;

        return GUIDQuerySucceeded;
    }

   public:
    std::shared_ptr<IIr77Return const> SetIndexed(std::uint64_t const& at, std::shared_ptr<IIr77Enlisted const>& obj) {
        auto raw = QueryAs<IIr77MPVMOP<Ir77String>>(GUIDOPIr77TimePoint, std::const_pointer_cast<IIr77Enlisted>(obj).get());

        if (!raw.get()) return Ir77RETURN<Ir77OperationFailed>(nullptr, "Invalid RHS Operand.");

        try {
            IsValidMMDDYYYY(raw->Get());
        } catch (std::domain_error) {
            return Ir77RETURN<Ir77OperationFailed>(nullptr, "Invalid Operand.");
        }

        m_operand.at(at) = obj;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   public:
    static std::shared_ptr<IIr77Return const> Assign(std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) {
        std::shared_ptr<IIr77Enlisted const> enlisted_rhs;
        rhs->GetIndexed(ID_VALUE, enlisted_rhs);

        auto raw = QueryAs<IIr77MPVMOP<Ir77String>>(GUIDOPIr77String, std::const_pointer_cast<IIr77Enlisted>(enlisted_rhs).get());

        if (!raw.get()) return Ir77RETURN<Ir77OperationFailed>(nullptr, "Invalid RHS Operand.");

        try {
            IsValidMMDDYYYY(raw->Get());
        } catch (std::domain_error) {
            return Ir77RETURN<Ir77OperationFailed>(nullptr, "Invalid Operand.");
        }

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

        auto raw_lhs = QueryAs<IIr77MPVMOP<Ir77String>>(GUIDOPIr77String, std::const_pointer_cast<IIr77Enlisted>(enlisted_lhs).get());

        auto raw_rhs = QueryAs<IIr77MPVMOP<Ir77String>>(GUIDOPIr77String, std::const_pointer_cast<IIr77Enlisted>(enlisted_rhs).get());

        if (!raw_lhs.get()) return Ir77RETURN<Ir77OperationFailed>(nullptr, "Invalid LHS Operand.");

        if (!raw_rhs.get()) return Ir77RETURN<Ir77OperationFailed>(nullptr, "Invalid RHS Operand.");

        try {
            IsValidMMDDYYYY(raw_lhs->Get());
        } catch (std::domain_error) {
            return Ir77RETURN<Ir77OperationFailed>(nullptr, "Invalid LHS Operand.");
        }

        try {
            IsValidMMDDYYYY(raw_rhs->Get());
        } catch (std::domain_error) {
            return Ir77RETURN<Ir77OperationFailed>(nullptr, "Invalid RHS Operand.");
        }

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

        auto raw_lhs = QueryAs<IIr77MPVMOP<Ir77String>>(GUIDOPIr77String, std::const_pointer_cast<IIr77Enlisted>(enlisted_lhs).get());

        auto raw_rhs = QueryAs<IIr77MPVMOP<Ir77String>>(GUIDOPIr77String, std::const_pointer_cast<IIr77Enlisted>(enlisted_rhs).get());

        if (!raw_lhs.get()) return Ir77RETURN<Ir77OperationFailed>(nullptr, "Invalid LHS Operand.");

        if (!raw_rhs.get()) return Ir77RETURN<Ir77OperationFailed>(nullptr, "Invalid RHS Operand.");

        try {
            IsValidMMDDYYYY(raw_lhs->Get());
        } catch (std::domain_error) {
            return Ir77RETURN<Ir77OperationFailed>(nullptr, "Invalid LHS Operand.");
        }

        try {
            IsValidMMDDYYYY(raw_rhs->Get());
        } catch (std::domain_error) {
            return Ir77RETURN<Ir77OperationFailed>(nullptr, "Invalid RHS Operand.");
        }

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

    static std::shared_ptr<IIr77Return const> Lesser(std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) {
        std::shared_ptr<IIr77Enlisted const> enlisted_lhs;
        lhs->GetIndexed(ID_VALUE, enlisted_lhs);

        std::shared_ptr<IIr77Enlisted const> enlisted_rhs;
        rhs->GetIndexed(ID_VALUE, enlisted_rhs);

        auto raw_lhs = QueryAs<IIr77MPVMOP<Ir77String>>(GUIDOPIr77String, std::const_pointer_cast<IIr77Enlisted>(enlisted_lhs).get());

        auto raw_rhs = QueryAs<IIr77MPVMOP<Ir77String>>(GUIDOPIr77String, std::const_pointer_cast<IIr77Enlisted>(enlisted_rhs).get());

        if (!raw_lhs.get()) return Ir77RETURN<Ir77OperationFailed>(nullptr, "Invalid LHS Operand.");

        if (!raw_rhs.get()) return Ir77RETURN<Ir77OperationFailed>(nullptr, "Invalid RHS Operand.");

        try {
            IsValidMMDDYYYY(raw_lhs->Get());
        } catch (std::domain_error) {
            return Ir77RETURN<Ir77OperationFailed>(nullptr, "Invalid LHS Operand.");
        }

        try {
            IsValidMMDDYYYY(raw_rhs->Get());
        } catch (std::domain_error) {
            return Ir77RETURN<Ir77OperationFailed>(nullptr, "Invalid RHS Operand.");
        }

        auto lhs_mutable = std::const_pointer_cast<IIr77Operand>(lhs);

        std::shared_ptr<Ir77MPVMOP<Ir77Boolean>> ret = std::make_shared<Ir77MPVMOP<Ir77Boolean>>();

        if (raw_lhs->Get() < raw_rhs->Get()) {
            ret->Set(true);
        } else {
            ret->Set(false);
        }

        auto ret_immutable = std::reinterpret_pointer_cast<IIr77Enlisted const>(std::const_pointer_cast<IIr77Operand const>(ret));

        lhs_mutable->SetIndexed(ID_RETURN, ret_immutable);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    static std::shared_ptr<IIr77Return const> Greater(std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) {
        std::shared_ptr<IIr77Enlisted const> enlisted_lhs;
        lhs->GetIndexed(ID_VALUE, enlisted_lhs);

        std::shared_ptr<IIr77Enlisted const> enlisted_rhs;
        rhs->GetIndexed(ID_VALUE, enlisted_rhs);

        auto raw_lhs = QueryAs<IIr77MPVMOP<Ir77String>>(GUIDOPIr77String, std::const_pointer_cast<IIr77Enlisted>(enlisted_lhs).get());

        auto raw_rhs = QueryAs<IIr77MPVMOP<Ir77String>>(GUIDOPIr77String, std::const_pointer_cast<IIr77Enlisted>(enlisted_rhs).get());

        if (!raw_lhs.get()) return Ir77RETURN<Ir77OperationFailed>(nullptr, "Invalid LHS Operand.");

        if (!raw_rhs.get()) return Ir77RETURN<Ir77OperationFailed>(nullptr, "Invalid RHS Operand.");

        try {
            IsValidMMDDYYYY(raw_lhs->Get());
        } catch (std::domain_error) {
            return Ir77RETURN<Ir77OperationFailed>(nullptr, "Invalid LHS Operand.");
        }

        try {
            IsValidMMDDYYYY(raw_rhs->Get());
        } catch (std::domain_error) {
            return Ir77RETURN<Ir77OperationFailed>(nullptr, "Invalid RHS Operand.");
        }

        auto lhs_mutable = std::const_pointer_cast<IIr77Operand>(lhs);

        std::shared_ptr<Ir77MPVMOP<Ir77Boolean>> ret = std::make_shared<Ir77MPVMOP<Ir77Boolean>>();

        if (raw_lhs->Get() > raw_rhs->Get()) {
            ret->Set(true);
        } else {
            ret->Set(false);
        }

        auto ret_immutable = std::reinterpret_pointer_cast<IIr77Enlisted const>(std::const_pointer_cast<IIr77Operand const>(ret));

        lhs_mutable->SetIndexed(ID_RETURN, ret_immutable);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    static std::shared_ptr<IIr77Return const> Current(std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) {
        std::time_t t = std::time(nullptr);

        std::tm now{};

        if (::localtime_r(&t, &now) == nullptr) {
            return Ir77RETURN<Ir77OperationFailed>(nullptr, "Local Time Creation failed.");
        }

        char buffer[11];

        size_t written = std::strftime(buffer, sizeof(buffer), "%m/%d/%Y", &now);

        if (written == 0) {
            return Ir77RETURN<Ir77OperationFailed>(nullptr, "Local Time Creation failed.");
        }

        std::string str{buffer, written};

        try {
            IsValidMMDDYYYY(str);
        } catch (std::domain_error) {
            return Ir77RETURN<Ir77OperationFailed>(nullptr, "Could not get Current System Time.");
        }

        auto result = std::make_shared<Ir77MPVMOP<Ir77String>>();
        result->Set(str);

        auto result_immutable = std::const_pointer_cast<const IIr77Enlisted>(result);

        auto lhs_mutable = std::const_pointer_cast<IIr77Operand>(lhs);
        lhs_mutable->SetIndexed(ID_RETURN, result_immutable);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    static void IsValidMMDDYYYY(std::string const& mmddyyyy) {
        try {
            TupleFromString(mmddyyyy);
        } catch (std::domain_error) {
            throw std::domain_error{"Invalid Date String."};
        }
    }

    static std::tuple<uint32_t, uint32_t, uint32_t> TupleFromString(std::string const& mmddyyyy) {
        if (mmddyyyy.size() != 10) throw std::domain_error{"Invalid Date String."};

        if (mmddyyyy[2] != '/' || mmddyyyy[5] != '/') throw std::domain_error{"Invalid Date String."};

        for (uint8_t i = 0; i < 10; i++) {
            if (i == 2 || i == 5) continue;

            if (std::find(Ir77MMDDYYYYNumeric.begin(), Ir77MMDDYYYYNumeric.end(), mmddyyyy[i]) == Ir77MMDDYYYYNumeric.end())
                throw std::domain_error{"Invalid character in Date String."};
        }

        uint32_t mm = static_cast<uint32_t>(mmddyyyy[0] - '0') * 10 + static_cast<uint32_t>(mmddyyyy[1] - '0');
        uint32_t dd = static_cast<uint32_t>(mmddyyyy[3] - '0') * 10 + static_cast<uint32_t>(mmddyyyy[4] - '0');
        uint32_t yyyy = static_cast<uint32_t>(mmddyyyy[6] - '0') * 1000 + static_cast<uint32_t>(mmddyyyy[7] - '0') * 100 +
                        static_cast<uint32_t>(mmddyyyy[8] - '0') * 10 + static_cast<uint32_t>(mmddyyyy[9] - '0');

        if (mm < 1 || mm > 12) throw std::domain_error{"Invalid month."};

        if (yyyy < 2004 || yyyy > 2096) throw std::domain_error{"Year out of range."};

        std::string mm_str = mmddyyyy.substr(0, 2);
        uint32_t dd_max = static_cast<uint32_t>(std::stoul(Ir77MMDD.at(mm_str)));

        if (std::find(Ir77Leap.begin(), Ir77Leap.end(), mmddyyyy.substr(6, 4)) != Ir77Leap.end() && mm == 2) dd_max += 1;

        if (dd < 1 || dd > dd_max) throw std::domain_error{"Invalid day."};

        return {mm, dd, yyyy};
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