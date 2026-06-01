#pragma once

#include <capnp/capability.h>
#include <kj/async.h>

#include <chrono>
#include <random>

export module OIr77TAssign;

import Ir77TYPES;
import IIr77RETURN;

export namespace NSIr77TBasic {

class OIr77TBasicBase : public IIr77Opcode ::Server {
   public:
    OIr77TBasicBase(uint64_t memberHigh, uint64_t memberLow)
        : memberHigh_{memberHigh}, memberLow_{memberLow}, enlisted_{std::chrono::system_clock::now()}, isValid_{true}, complete_{false} {
        auto [high, low] = generateUuid();
        instanceHigh_ = high;
        instanceLow_ = low;
    }

   public:
    // ── IIr77ENLISTED ────────────────────────────────────────────
    kj::Promise<void> enlistedAs(EnlistedAsContext ctx) override {
        setGuid(ctx.getResults().getUid(), 0x9D448AD2C99C45B5, 0xBCD9846CAA7726A8);
        return ret(ctx, isValid_);
    }

    kj::Promise<void> enlistedUuid(EnlistedUuidContext ctx) override {
        setGuid(ctx.getResults().getUid(), instanceHigh_, instanceLow_);
        return ret(ctx, isValid_);
    }

    kj::Promise<void> enlistedChrono(EnlistedChronoContext ctx) override {
        ctx.getResults().setUid(toUint64(enlisted_));
        return ret(ctx, isValid_);
    }

    kj::Promise<void> delistedChrono(DelistedChronoContext ctx) override {
        ctx.getResults().setUid(toUint64(delisted_));
        return ret(ctx, isValid_);
    }

    kj::Promise<void> memberOfUuid(MemberOfUuidContext ctx) override {
        setGuid(ctx.getResults().getUid(), memberHigh_, memberLow_);
        return ret(ctx, isValid_);
    }

    kj::Promise<void> collectionUuid(CollectionUuidContext ctx) override {
        setGuid(ctx.getResults().getUid(), 0xD1975E2CB0C548BF, 0x9F00FC290B8E672A);
        return ret(ctx, isValid_);
    }

   public:
    // ── IIr77Opcode ─────────────────────────────────────────────────
    kj::Promise<void> id(IdContext ctx) override {
        setGuid(ctx.getResults().getId(), memberHigh_, memberLow_);
        return kj::READY_NOW;
    }

    kj::Promise<void> gid(GidContext ctx) override {
        setGuid(ctx.getResults().getGid(), 0xD1975E2CB0C548BF, 0x9F00FC290B8E672A);
        return kj::READY_NOW;
    }

    kj::Promise<void> setOperand(SetOperandContext ctx) override {
        if (!isValid_) return invalidated(ctx);
        operand_ = ctx.getParams().getInput();
        return operationSucceeded(ctx);
    }

    kj::Promise<void> getOperand(GetOperandContext ctx) override {
        if (!isValid_) return invalidated(ctx);
        ctx.getResults().setInput(operand_);
        return operationSucceeded(ctx);
    }

    kj::Promise<void> result(ResultContext ctx) override {
        if (!isValid_) return invalidated(ctx);
        if (complete_) return processComplete(ctx);

        outcome_ = ctx.getParams().getOutcome();
        return_ = ctx.getParams().getRet();
        complete_ = true;

        return operationSucceeded(ctx);
    }

    kj::Promise<void> resultOut(ResultOutContext ctx) override {
        if (!isValid_) return invalidated(ctx);
        if (!complete_) return processNotComplete(ctx);

        ctx.getResults().setOutcome(outcome_);
        ctx.getResults().setResult(return_);
        return kj::READY_NOW;
    }

    kj::Promise<void> invalidateOperation(InvalidateOperationContext ctx) override {
        if (!isValid_) return invalidated(ctx);
        isValid_ = false;
        condition_ = ctx.getParams().getcondition();
        delisted_ = std::chrono::system_clock::now();
        return operationSucceeded(ctx);
    }

    kj::Promise<void> isInvalid(IsInvalidContext ctx) override {
        if (!isValid_) return invalidated(ctx);
        return True(ctx);
    }

    kj::Promise<void> isInvalidWith(IsInvalidWithContext ctx) override {
        if (!isValid_) return invalidated(ctx);
        ctx.getResults().setcondition(condition_);
        return True(ctx);
    }

   protected:
    // ── helpers ──────────────────────────────────────────────────
    static std::pair<uint64_t, uint64_t> generateUuid() {
        std::random_device rd;
        std::mt19937_64 gen(rd());
        return {gen(), gen()};
    }

    static uint64_t toUint64(std::chrono::system_clock::time_point tp) { return (uint64_t)std::chrono::system_clock::to_time_t(tp); }

    template <typename Builder>
    static void setGuid(Builder b, uint64_t high, uint64_t low) {
        b.setHigh(high);
        b.setLow(low);
    }

    template <typename Ctx>
    kj::Promise<void> ret(Ctx& ctx, bool valid) {
        ctx.getResults().setResult(valid ? makeOperationSucceeded() : makeInvalidated());
        return kj::READY_NOW;
    }

    template <typename Ctx>
    kj::Promise<void> invalidated(Ctx& ctx) {
        ctx.getResults().setResult(makeInvalidated());
        return kj::READY_NOW;
    }
    template <typename Ctx>
    kj::Promise<void> operationSucceeded(Ctx& ctx) {
        ctx.getResults().setResult(makeOperationSucceeded());
        return kj::READY_NOW;
    }
    template <typename Ctx>
    kj::Promise<void> processComplete(Ctx& ctx) {
        ctx.getResults().setResult(makeProcessComplete());
        return kj::READY_NOW;
    }
    template <typename Ctx>
    kj::Promise<void> processNotComplete(Ctx& ctx) {
        ctx.getResults().setResult(makeProcessNotComplete());
        return kj::READY_NOW;
    }
    template <typename Ctx>
    kj::Promise<void> True(Ctx& ctx) {
        ctx.getResults().setResult(makeTrue());
        return kj::READY_NOW;
    }
    template <typename Ctx>
    kj::Promise<void> False(Ctx& ctx) {
        ctx.getResults().setResult(makeFalse());
        return kj::READY_NOW;
    }
    template <typename Ctx>
    kj::Promise<void> invalidArgument(Ctx& ctx) {
        ctx.getResults().setResult(makeInvalidArgument());
        return kj::READY_NOW;
    }

   protected:
    uint64_t memberHigh_{};
    uint64_t memberLow_{};
    uint64_t instanceHigh_{};
    uint64_t instanceLow_{};

    std::chrono::system_clock::time_point enlisted_{};
    std::chrono::system_clock::time_point delisted_{};

    IIr77BALE::Client operand_{nullptr};
    IIr77BALE::Client outcome_{nullptr};
    IIr77RETURN::Client return_{nullptr};
    IIr77RETURN::Client condition_{nullptr};

    bool isValid_{true};
    bool complete_{false};
};

class OIr77TAssignServer : public OIr77TBasicBase {
   public:
    OIr77TAssignServer() : OIr77TBasicBase(0xB40C793D8C384EB6, 0x853FEF6CF943E84F) {}
};

class OIr77TBALEServer : public OIr77TBasicBase {
   public:
    OIr77TBALEServer() : OIr77TBasicBase(0x918D86B22E7F4E24, 0x8EED88058470E433) {}
};

class OIr77TEQUALServer : public OIr77TBasicBase {
   public:
    OIr77TEQUALServer() : OIr77TBasicBase(0xB8F35B2953B946ED, 0x85BEC4A141B929DE) {}
};

class OIr77TLESSERServer : public OIr77TBasicBase {
   public:
    OIr77TLESSERServer() : OIr77TBasicBase(0xD2E2578EEE594BD5, 0x8C5C0AD60D5ADAC0) {}
};

class OIr77TGREATERServer : public OIr77TBasicBase {
   public:
    OIr77TGREATERServer() : OIr77TBasicBase(0xC5EB4F50BE7F4771, 0xAF3CACBF11B13C47) {}
};

class OIr77TNOTServer : public OIr77TBasicBase {
   public:
    OIr77TNOTServer() : OIr77TBasicBase(0x43253A5760D7407C, 0xA2A7745FD0F99871) {}
};

class OIr77TGENERATEServer : public OIr77TBasicBase {
   public:
    OIr77TGENERATEServer() : OIr77TBasicBase(0xBA2F4F37D0694064, 0xB28B7AD52BECA35E) {}
};

class OIr77TTOTUPLEServer : public OIr77TBasicBase {
   public:
    OIr77TTOTUPLEServer() : OIr77TBasicBase(0x029932E43AE34E03, 0x97728BC5C5142913) {}
};

inline IIr77Opcode ::Client makeOTASSIGN() { return kj::heap<OIr77TAssignServer>(); }
inline IIr77Opcode ::Client makeOTBALE() { return kj::heap<OIr77TBALEServer>(); }
inline IIr77Opcode ::Client makeOTEQUAL() { return kj::heap<OIr77TEQUALServer>(); }
inline IIr77Opcode ::Client makeOTLESSER() { return kj::heap<OIr77TLESSERServer>(); }
inline IIr77Opcode ::Client makeOTGREATER() { return kj::heap<OIr77TGREATERServer>(); }
inline IIr77Opcode ::Client makeOTNOT() { return kj::heap<OIr77TNOTServer>(); }
inline IIr77Opcode ::Client makeOTGENERATE() { return kj::heap<OIr77TGENERATORServer>(); }
inline IIr77Opcode ::Client makeOTTOTUPLE() { return kj::heap<OIr77TTOTUPLEServer>(); }

}  // namespace NSIr77TBasic