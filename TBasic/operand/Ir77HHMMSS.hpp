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
class Ir77HHMMSS : public Ir77Enlisted, public Ir77Operand, public std::enable_shared_from_this<Ir77HHMMSS> {
   public:
    Ir77HHMMSS() {
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
        seat_shared_uuid<&GUIDOPIr77HHMMSS>(uid);

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

        else if (iid == &GUIDOPIr77HHMMSS)
            obj = std::shared_ptr<Ir77HHMMSS>(shared_from_this(), static_cast<Ir77HHMMSS*>(this));

        else
            return &GUIDQueryFailed;

        return &GUIDQuerySucceeded;
    }

   public:
    std::shared_ptr<IIr77Return const> SetIndexed(std::uint64_t const& at, std::shared_ptr<IIr77Enlisted const>& obj) {
        auto raw = QueryAs<IIr77MPVMOP<Ir77String>>(&GUIDOPIr77TimePoint, std::const_pointer_cast<IIr77Enlisted>(obj).get());

        if (!raw.get()) return Ir77RETURN<Ir77OperationFailed>(nullptr, "Invalid RHS Operand.");

        try {
            IsValidHHMMSS(raw->Get());
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

        auto raw = QueryAs<IIr77MPVMOP<Ir77String>>(&GUIDOPIr77String, std::const_pointer_cast<IIr77Enlisted>(enlisted_rhs).get());

        if (!raw.get()) return Ir77RETURN<Ir77OperationFailed>(nullptr, "Invalid RHS Operand.");

        try {
            IsValidHHMMSS(raw->Get());
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

        auto raw_lhs = QueryAs<IIr77MPVMOP<Ir77String>>(&GUIDOPIr77String, std::const_pointer_cast<IIr77Enlisted>(enlisted_lhs).get());

        auto raw_rhs = QueryAs<IIr77MPVMOP<Ir77String>>(&GUIDOPIr77String, std::const_pointer_cast<IIr77Enlisted>(enlisted_rhs).get());

        if (!raw_lhs.get()) return Ir77RETURN<Ir77OperationFailed>(nullptr, "Invalid LHS Operand.");

        if (!raw_rhs.get()) return Ir77RETURN<Ir77OperationFailed>(nullptr, "Invalid RHS Operand.");

        try {
            IsValidHHMMSS(raw_lhs->Get());
        } catch (std::domain_error) {
            return Ir77RETURN<Ir77OperationFailed>(nullptr, "Invalid LHS Operand.");
        }

        try {
            IsValidHHMMSS(raw_rhs->Get());
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

        auto raw_lhs = QueryAs<IIr77MPVMOP<Ir77String>>(&GUIDOPIr77String, std::const_pointer_cast<IIr77Enlisted>(enlisted_lhs).get());

        auto raw_rhs = QueryAs<IIr77MPVMOP<Ir77String>>(&GUIDOPIr77String, std::const_pointer_cast<IIr77Enlisted>(enlisted_rhs).get());

        if (!raw_lhs.get()) return Ir77RETURN<Ir77OperationFailed>(nullptr, "Invalid LHS Operand.");

        if (!raw_rhs.get()) return Ir77RETURN<Ir77OperationFailed>(nullptr, "Invalid RHS Operand.");

        try {
            IsValidHHMMSS(raw_lhs->Get());
        } catch (std::domain_error) {
            return Ir77RETURN<Ir77OperationFailed>(nullptr, "Invalid LHS Operand.");
        }

        try {
            IsValidHHMMSS(raw_rhs->Get());
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

        auto raw_lhs = QueryAs<IIr77MPVMOP<Ir77String>>(&GUIDOPIr77String, std::const_pointer_cast<IIr77Enlisted>(enlisted_lhs).get());

        auto raw_rhs = QueryAs<IIr77MPVMOP<Ir77String>>(&GUIDOPIr77String, std::const_pointer_cast<IIr77Enlisted>(enlisted_rhs).get());

        if (!raw_lhs.get()) return Ir77RETURN<Ir77OperationFailed>(nullptr, "Invalid LHS Operand.");

        if (!raw_rhs.get()) return Ir77RETURN<Ir77OperationFailed>(nullptr, "Invalid RHS Operand.");

        try {
            IsValidHHMMSS(raw_lhs->Get());
        } catch (std::domain_error) {
            return Ir77RETURN<Ir77OperationFailed>(nullptr, "Invalid LHS Operand.");
        }

        try {
            IsValidHHMMSS(raw_rhs->Get());
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

        auto raw_lhs = QueryAs<IIr77MPVMOP<Ir77String>>(&GUIDOPIr77String, std::const_pointer_cast<IIr77Enlisted>(enlisted_lhs).get());

        auto raw_rhs = QueryAs<IIr77MPVMOP<Ir77String>>(&GUIDOPIr77String, std::const_pointer_cast<IIr77Enlisted>(enlisted_rhs).get());

        if (!raw_lhs.get()) return Ir77RETURN<Ir77OperationFailed>(nullptr, "Invalid LHS Operand.");

        if (!raw_rhs.get()) return Ir77RETURN<Ir77OperationFailed>(nullptr, "Invalid RHS Operand.");

        try {
            IsValidHHMMSS(raw_lhs->Get());
        } catch (std::domain_error) {
            return Ir77RETURN<Ir77OperationFailed>(nullptr, "Invalid LHS Operand.");
        }

        try {
            IsValidHHMMSS(raw_rhs->Get());
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
            return Ir77RETURN<Ir77OperationFailed>(nullptr, "Local System Time creation failed.");
        }

        char buffer[9];

        size_t written = std::strftime(buffer, sizeof(buffer), "%H:%M:%S", &now);

        if (written == 0) {
            return Ir77RETURN<Ir77OperationFailed>(nullptr, "Local System Time creation failed.");
        }

        std::string str{buffer, written};

        try {
            IsValidHHMMSS(str);
        } catch (std::domain_error) {
            return Ir77RETURN<Ir77OperationFailed>(nullptr, "Local System Time creation failed.");
        }

        auto result = std::make_shared<Ir77MPVMOP<Ir77String>>();
        result->Set(str);

        auto result_immutable = std::const_pointer_cast<const IIr77Enlisted>(result);

        auto lhs_mutable = std::const_pointer_cast<IIr77Operand>(lhs);
        lhs_mutable->SetIndexed(ID_RETURN, result_immutable);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    static void IsValidHHMMSS(std::string const& hhmmss) {
        if (hhmmss.size() != 8) {
            throw std::domain_error{"Invalid Time String."};
        }

        if (hhmmss[2] != ':' || hhmmss[5] != ':') {
            throw std::domain_error{"Invalid Time String."};
        }

        static const std::vector<std::string> hours{"00", "01", "02", "03", "04", "05", "06", "07", "08", "09", "10", "11",
                                                    "12", "13", "14", "15", "16", "17", "18", "19", "20", "21", "22", "23"};

        static const std::vector<std::string> minutes_seconds{"00", "01", "02", "03", "04", "05", "06", "07", "08", "09", "10", "11", "12", "13", "14",
                                                              "15", "16", "17", "18", "19", "20", "21", "22", "23", "24", "25", "26", "27", "28", "29",
                                                              "30", "31", "32", "33", "34", "35", "36", "37", "38", "39", "40", "41", "42", "43", "44",
                                                              "45", "46", "47", "48", "49", "50", "51", "52", "53", "54", "55", "56", "57", "58", "59"};

        if (std::find(hours.begin(), hours.end(), hhmmss.substr(0, 2)) == hours.end()) {
            throw std::domain_error{"Time String invalid hour."};
        }

        if (std::find(minutes_seconds.begin(), minutes_seconds.end(), hhmmss.substr(3, 2)) == minutes_seconds.end()) {
            throw std::domain_error{"Time String invalid minute."};
        }

        if (std::find(minutes_seconds.begin(), minutes_seconds.end(), hhmmss.substr(6, 2)) == minutes_seconds.end()) {
            throw std::domain_error{"Time String invalid second."};
        }
    }

    static std::tuple<uint8_t, uint8_t, uint8_t> TupleFromString(std::string const& hhmmss) {
        try {
            IsValidHHMMSS(hhmmss);
        } catch (std::domain_error) {
            throw std::invalid_argument{"Invalid System Time String."};
        }

        uint8_t hh = static_cast<uint8_t>(hhmmss[0] - '0') * 10 + static_cast<uint8_t>(hhmmss[1] - '0');
        uint8_t mm = static_cast<uint8_t>(hhmmss[3] - '0') * 10 + static_cast<uint8_t>(hhmmss[4] - '0');
        uint8_t ss = static_cast<uint8_t>(hhmmss[6] - '0') * 10 + static_cast<uint8_t>(hhmmss[7] - '0');

        return {hh, mm, ss};
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