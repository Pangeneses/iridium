#pragma once

#include <memory>

#include "../../Ir77RT/interface/IIr77Operand.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../../Ir77RT/interface/IIr77GUID.hpp"

#include "../../Ir77RT/runtime/Ir77Return.hpp"

using namespace NSIr77RT;

namespace NSIr77TBasic {
struct OIr77TOperand : public IIr77Operand, public std::enable_shared_from_this<OIr77TOperand> {
   public:
    OIr77TOperand();

   public:
    std::shared_ptr<IIr77Return const> EnlistedAs(std::shared_ptr<IIr77GUID const>& uid) const;

    std::shared_ptr<IIr77Return const> EnlistedUuid(std::shared_ptr<IIr77GUID const>& uid) const;

    std::shared_ptr<IIr77Return const> EnlistedChrono(std::chrono::system_clock::time_point& t) const;

    std::shared_ptr<IIr77Return const> Delist(std::shared_ptr<IIr77Return const>& condition);

    std::shared_ptr<IIr77Return const> DelistedChrono(std::chrono::system_clock::time_point& t, std::shared_ptr<IIr77Return const>& condition) const;

    std::shared_ptr<IIr77Return const> SetSender(std::shared_ptr<IIr77Enlisted const>& sender);

    std::shared_ptr<IIr77Return const> GetSender(std::shared_ptr<IIr77Enlisted const>& sender) const;

    std::shared_ptr<IIr77Return const> SetSenderMsg(std::string const& sender);

    std::shared_ptr<IIr77Return const> GetSenderMsg(std::string& sender) const;

    std::shared_ptr<IIr77Return const> MemberOfUuid(std::shared_ptr<IIr77GUID const>& uid) const;

    std::shared_ptr<IIr77Return const> CollectionUuid(std::shared_ptr<IIr77GUID const>& uid) const;

    IIr77GUID* const QueryInterface(IIr77GUID const* iid, std::shared_ptr<void>&);

   public:
    std::shared_ptr<IIr77Return const> Resize(std::uint32_t const& sz);

    std::shared_ptr<IIr77Return const> Size(std::uint32_t& sz) const;

    std::shared_ptr<IIr77Return const> IsEmpty() const;

    std::shared_ptr<IIr77Return const> SetIndexed(std::uint32_t const& at, std::shared_ptr<IIr77Enlisted const> obj);

    std::shared_ptr<IIr77Return const> GetIndexed(std::uint32_t const& at, std::shared_ptr<IIr77Enlisted const>& obj) const;

    std::shared_ptr<IIr77Return const> SealOperand();

    std::shared_ptr<IIr77Return const> IsSealed() const;

    std::shared_ptr<IIr77Return const> SetResult(std::shared_ptr<IIr77Operand const> const& outcome, std::shared_ptr<IIr77Return const> const& ret);

    std::shared_ptr<IIr77Return const> GetResult(std::shared_ptr<IIr77Operand const>& outcome, std::shared_ptr<IIr77Return const>& ret) const;

   private:
    Ir77GUID m_enlisted_uuid{};

    std::chrono::system_clock::time_point m_enlisted{};

    std::chrono::system_clock::time_point m_delisted{};

    std::shared_ptr<IIr77Enlisted const> m_sender{nullptr};

    std::string m_message{""};

    bool m_valid{true};

    bool m_sealed{false};

    bool m_complete{false};

    std::vector<std::shared_ptr<IIr77Enlisted const>> m_operand;

    std::shared_ptr<IIr77Operand const> m_outcome{};

    std::shared_ptr<IIr77Return const> m_invalidation_condition{nullptr};

    std::shared_ptr<IIr77Return const> m_return = Ir77RETURN<Ir77Unknown>();
};
}  // namespace NSIr77TBasic
