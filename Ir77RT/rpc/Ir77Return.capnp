#pragma once

#include <capnp/capability.h>
#include <kj/async.h>

#include <chrono>
#include <cstdint>

#include "../interface/IIr77Return.hpp"
#include "Ir77Return.hpp"
#include "Ir77Types.hpp"

namespace NSIr77RT {

class IDIr77ReturnServer : public IIr77RETURN::Server {
   public:
    IDIr77ReturnServer(uint64_t typeHigh, uint64_t typeLow, uint64_t groupHigh, uint64_t groupLow)
        : typeHigh_{typeHigh}, typeLow_{typeLow}, groupHigh_{groupHigh}, groupLow_{groupLow}, enlisted_{std::chrono::system_clock::now()}, isValid_{true} {
        instance_ = {typeHigh, typeLow};
    }

    IDIr77ReturnServer(uint64_t typeHigh, uint64_t typeLow, uint64_t groupHigh, uint64_t groupLow, capnp::AnyPointer::Reader sender, kj::StringPtr log)
        : IDIr77ReturnServer{typeHigh, typeLow, groupHigh, groupLow} {
        sender_ = kj::str(sender);
        message_ = kj::str(log);
    }

   public:
    kj::Promise<void> id(IdContext ctx) override {
        auto result = ctx.getResults().getId();
        result.setHigh(typeHigh_);
        result.setLow(typeLow_);
        return kj::READY_NOW;
    }

    kj::Promise<void> gid(GidContext ctx) override {
        auto result = ctx.getResults().getGid();
        result.setHigh(groupHigh_);
        result.setLow(groupLow_);
        return kj::READY_NOW;
    }

    kj::Promise<void> sender(SenderContext ctx) override {
        checkValid();
        ctx.getResults().setSender(sender_);
        return kj::READY_NOW;
    }

    kj::Promise<void> log(LogContext ctx) override {
        checkValid();
        ctx.getResults().setMsg(message_);
        return kj::READY_NOW;
    }

    kj::Promise<void> invalidateReturn(InvalidateReturnContext ctx) override {
        if (!isValid_) return kj::READY_NOW;

        isValid_ = false;
        delisted_ = std::chrono::system_clock::now();
        condition_ = ctx.getParams().getcondition();
        return kj::READY_NOW;
    }

    kj::Promise<void> isInvalid(IsInvalidContext ctx) override {
        ctx.getResults().setResult(capnp::Capability::Client(kj::heap<Ir77ReturnServer>(*this)));
        return kj::READY_NOW;
    }

    kj::Promise<void> isInvalidWith(IsInvalidWithContext ctx) override {
        ctx.getResults().setcondition(condition_);
        return kj::READY_NOW;
    }

   private:
    void checkValid() {
        if (!isValid_) throw kj::Exception(kj::Exception::Type::FAILED, __FILE__, __LINE__, kj::str("Return has been invalidated."));
    }

   private:
    uint64_t typeHigh_;
    uint64_t typeLow_;
    uint64_t groupHigh_;
    uint64_t groupLow_;

    struct {
        uint64_t high;
        uint64_t low;
    } instance_;

    std::chrono::system_clock::time_point enlisted_{};
    std::chrono::system_clock::time_point delisted_{};

    kj::String sender_{};
    kj::String message_{};
    bool isValid_{true};

    IIr77RETURN::Client condition_{nullptr};
};

class IDiIr77RETURNServer : public IDiIr77RETURN::Server {
   public:
    IDiIr77RETURNServer() = default;

   public:
    kj::Promise<void> getRET(GetRETContext ctx) override {
        set(ctx, 0xBE11AEB5923A4854, 0xB8B71CC80D56A296);
        return kj::READY_NOW;
    }
    kj::Promise<void> getUnknown(GetUnknownContext ctx) override {
        set(ctx, 0x9E5DF7990B114A2B, 0xAB3C11A3980128D3);
        return kj::READY_NOW;
    }
    kj::Promise<void> getFalse(GetFalseContext ctx) override {
        set(ctx, 0x5DE62A6FD9F849BD, 0x80914DDBCAD9509A);
        return kj::READY_NOW;
    }
    kj::Promise<void> getTrue(GetTrueContext ctx) override {
        set(ctx, 0x45FC03965A374386, 0xB82B8772F7180CA8);
        return kj::READY_NOW;
    }
    kj::Promise<void> getIsEqual(GetIsEqualContext ctx) override {
        set(ctx, 0x5722B3779F2743F3, 0x8AD2A914D1CFE987);
        return kj::READY_NOW;
    }
    kj::Promise<void> getIsLesser(GetIsLesserContext ctx) override {
        set(ctx, 0x84B4D96612274B85, 0x9EF459F16D84FBD7);
        return kj::READY_NOW;
    }
    kj::Promise<void> getIsGreater(GetIsGreaterContext ctx) override {
        set(ctx, 0xCF21B52FE58A4E3E, 0x87E5C1A919B37C8A);
        return kj::READY_NOW;
    }
    kj::Promise<void> getInvalidArgument(GetInvalidArgumentContext ctx) override {
        set(ctx, 0x5F927FFD1B4F42CD, 0x94F5E6CFCBD4B27B);
        return kj::READY_NOW;
    }
    kj::Promise<void> getValidArgument(GetValidArgumentContext ctx) override {
        set(ctx, 0x19369F2EC49343ED, 0x8C1CED82BE10181C);
        return kj::READY_NOW;
    }
    kj::Promise<void> getOpFailed(GetOpFailedContext ctx) override {
        set(ctx, 0xB72C862FDA2B45C8, 0x92F58F257DB0974C);
        return kj::READY_NOW;
    }
    kj::Promise<void> getOpConflict(GetOpConflictContext ctx) override {
        set(ctx, 0x8030334DB3684201, 0x9FFBFAEC2635EC65);
        return kj::READY_NOW;
    }
    kj::Promise<void> getOpSucceeded(GetOpSucceededContext ctx) override {
        set(ctx, 0x6CD78C706C544EDB, 0x8C76A83169BF7AFE);
        return kj::READY_NOW;
    }
    kj::Promise<void> getInvalidOp(GetInvalidOpContext ctx) override {
        set(ctx, 0x04DF984D34404C03, 0xAF65CF939BCD85E0);
        return kj::READY_NOW;
    }
    kj::Promise<void> getNotConfigured(GetNotConfiguredContext ctx) override {
        set(ctx, 0x36D19823064E4F88, 0xBC36C441B1D27410);
        return kj::READY_NOW;
    }
    kj::Promise<void> getAlreadyConfig(GetAlreadyConfigContext ctx) override {
        set(ctx, 0xC11449FCD86841D3, 0xB27EE6CA72EA11CC);
        return kj::READY_NOW;
    }
    kj::Promise<void> getImproperlyConfig(GetImproperlyConfigContext ctx) override {
        set(ctx, 0xC4246620B9FF4A96, 0xB5D65EF203C0E881);
        return kj::READY_NOW;
    }
    kj::Promise<void> getProperlyConfig(GetProperlyConfigContext ctx) override {
        set(ctx, 0xDF9D797ECE424C6E, 0x953E7103CE45E269);
        return kj::READY_NOW;
    }
    kj::Promise<void> getRuntimeError(GetRuntimeErrorContext ctx) override {
        set(ctx, 0x4AC74417AEBD4E10, 0xAA81D76E97FBBCAA);
        return kj::READY_NOW;
    }
    kj::Promise<void> getOutOfMemory(GetOutOfMemoryContext ctx) override {
        set(ctx, 0x39043756CA274E8B, 0x9BB664363443CF9A);
        return kj::READY_NOW;
    }
    kj::Promise<void> getOutOfRange(GetOutOfRangeContext ctx) override {
        set(ctx, 0xC1A1E9E65B694A8A, 0xBFF2951F514A651B);
        return kj::READY_NOW;
    }
    kj::Promise<void> getKeyNotFound(GetKeyNotFoundContext ctx) override {
        set(ctx, 0xC8A81C3EA4754185, 0x9AFC4EB0144CE6E2);
        return kj::READY_NOW;
    }
    kj::Promise<void> getInvalidPointer(GetInvalidPointerContext ctx) override {
        set(ctx, 0xB157017113304DE1, 0xB80566756D3E75C1);
        return kj::READY_NOW;
    }
    kj::Promise<void> getInvalidInterface(GetInvalidInterfaceContext ctx) override {
        set(ctx, 0xC7E4A47E6A2D4CDF, 0x86C203E22CDEBC98);
        return kj::READY_NOW;
    }
    kj::Promise<void> getInvalidToken(GetInvalidTokenContext ctx) override {
        set(ctx, 0x7A119C977C724F1B, 0x9D135752009F5EB1);
        return kj::READY_NOW;
    }
    kj::Promise<void> getEmpty(GetEmptyContext ctx) override {
        set(ctx, 0x493D0559DF584979, 0xB904030A6F8C0A73);
        return kj::READY_NOW;
    }
    kj::Promise<void> getDirty(GetDirtyContext ctx) override {
        set(ctx, 0x81E3FBB5EA384C27, 0x9887B7CF86FAB846);
        return kj::READY_NOW;
    }
    kj::Promise<void> getSealed(GetSealedContext ctx) override {
        set(ctx, 0x23B714FF6A4043FD, 0xB3303743F0C955B5);
        return kj::READY_NOW;
    }
    kj::Promise<void> getInvalidated(GetInvalidatedContext ctx) override {
        set(ctx, 0x786212027B2C4207, 0xAFE2908529F3D087);
        return kj::READY_NOW;
    }
    kj::Promise<void> getNotSystemModule(GetNotSystemModuleContext ctx) override {
        set(ctx, 0x8377EDB1189545BA, 0x90BF1606AAD7BAB9);
        return kj::READY_NOW;
    }
    kj::Promise<void> getModuleNotFound(GetModuleNotFoundContext ctx) override {
        set(ctx, 0x51E53030191E4591, 0xA54E1FB0FFAE5137);
        return kj::READY_NOW;
    }
    kj::Promise<void> getRtclassNotFound(GetRtclassNotFoundContext ctx) override {
        set(ctx, 0x058DAD2CB3234B9E, 0x96FC861E9EBF1879);
        return kj::READY_NOW;
    }
    kj::Promise<void> getProcessNotFound(GetProcessNotFoundContext ctx) override {
        set(ctx, 0xAF016669F1194C26, 0x8503859ED6AB5B59);
        return kj::READY_NOW;
    }
    kj::Promise<void> getProcessNotComp(GetProcessNotCompContext ctx) override {
        set(ctx, 0x517EA2D3961F4AD9, 0x9748754CE55A52C8);
        return kj::READY_NOW;
    }
    kj::Promise<void> getProcessComplete(GetProcessCompleteContext ctx) override {
        set(ctx, 0xE5912CF9AE514335, 0x888FBF17B648EB28);
        return kj::READY_NOW;
    }
    kj::Promise<void> getImageNotLoaded(GetImageNotLoadedContext ctx) override {
        set(ctx, 0xA169E9E2E8D44C55, 0xAD33E5C1FBC388FF);
        return kj::READY_NOW;
    }
    kj::Promise<void> getComFailed(GetComFailedContext ctx) override {
        set(ctx, 0x0A23A540031D4394, 0x836400DC585AA362);
        return kj::READY_NOW;
    }
    kj::Promise<void> getComSucceeded(GetComSucceededContext ctx) override {
        set(ctx, 0xAB7C4D183BAB45BC, 0x8B96A1407CA08A43);
        return kj::READY_NOW;
    }
    kj::Promise<void> getDirDoesntExist(GetDirDoesntExistContext ctx) override {
        set(ctx, 0xC80CC92B771441A8, 0x9471E17CD874F0AE);
        return kj::READY_NOW;
    }
    kj::Promise<void> getDirAlreadyExists(GetDirAlreadyExistsContext ctx) override {
        set(ctx, 0xDEF5D37EF0FF411A, 0x873C65A09AED089A);
        return kj::READY_NOW;
    }
    kj::Promise<void> getPageAlreadyEx(GetPageAlreadyExContext ctx) override {
        set(ctx, 0xA85AA7481A314C0A, 0xB19D7C850FFC960C);
        return kj::READY_NOW;
    }
    kj::Promise<void> getNoFileOpen(GetNoFileOpenContext ctx) override {
        set(ctx, 0x51312ED8FBE84B2B, 0xA36F0D3CEB2EBB76);
        return kj::READY_NOW;
    }
    kj::Promise<void> getFileDoesntExist(GetFileDoesntExistContext ctx) override {
        set(ctx, 0x73DECBE27CE14CB7, 0x9ECC3413119492F6);
        return kj::READY_NOW;
    }
    kj::Promise<void> getFileAlreadyEx(GetFileAlreadyExContext ctx) override {
        set(ctx, 0x74F793901CA14AD8, 0xB5352964112AA7AA);
        return kj::READY_NOW;
    }
    kj::Promise<void> getFileHandleOpen(GetFileHandleOpenContext ctx) override {
        set(ctx, 0x36A0A7EB59944BE5, 0x97B0E7F14AC420E2);
        return kj::READY_NOW;
    }
    kj::Promise<void> getFileBadFormat(GetFileBadFormatContext ctx) override {
        set(ctx, 0xB3B467CEE47F4557, 0xB3457DA5F594249B);
        return kj::READY_NOW;
    }

    kj::Promise<void> getReturnList(GetReturnListContext ctx) override {
        auto list = ctx.getResults().initList(45);

        list[0].setName("RET");
        list[0].initGuid().setHigh(0xBE11AEB5923A4854);
        list[0].getGuid().setLow(0xB8B71CC80D56A296);
        list[1].setName("unknown");
        list[1].initGuid().setHigh(0x9E5DF7990B114A2B);
        list[1].getGuid().setLow(0xAB3C11A3980128D3);
        list[2].setName("False");
        list[2].initGuid().setHigh(0x5DE62A6FD9F849BD);
        list[2].getGuid().setLow(0x80914DDBCAD9509A);
        list[3].setName("True");
        list[3].initGuid().setHigh(0x45FC03965A374386);
        list[3].getGuid().setLow(0xB82B8772F7180CA8);
        list[4].setName("isEqual");
        list[4].initGuid().setHigh(0x5722B3779F2743F3);
        list[4].getGuid().setLow(0x8AD2A914D1CFE987);
        list[5].setName("isLesser");
        list[5].initGuid().setHigh(0x84B4D96612274B85);
        list[5].getGuid().setLow(0x9EF459F16D84FBD7);
        list[6].setName("isGreater");
        list[6].initGuid().setHigh(0xCF21B52FE58A4E3E);
        list[6].getGuid().setLow(0x87E5C1A919B37C8A);
        list[7].setName("invalidArgument");
        list[7].initGuid().setHigh(0x5F927FFD1B4F42CD);
        list[7].getGuid().setLow(0x94F5E6CFCBD4B27B);
        list[8].setName("validArgument");
        list[8].initGuid().setHigh(0x19369F2EC49343ED);
        list[8].getGuid().setLow(0x8C1CED82BE10181C);
        list[9].setName("operationFailed");
        list[9].initGuid().setHigh(0xB72C862FDA2B45C8);
        list[9].getGuid().setLow(0x92F58F257DB0974C);
        list[10].setName("operationConflict");
        list[10].initGuid().setHigh(0x8030334DB3684201);
        list[10].getGuid().setLow(0x9FFBFAEC2635EC65);
        list[11].setName("operationSucceeded");
        list[11].initGuid().setHigh(0x6CD78C706C544EDB);
        list[11].getGuid().setLow(0x8C76A83169BF7AFE);
        list[12].setName("invalidOperation");
        list[12].initGuid().setHigh(0x04DF984D34404C03);
        list[12].getGuid().setLow(0xAF65CF939BCD85E0);
        list[13].setName("notConfigured");
        list[13].initGuid().setHigh(0x36D19823064E4F88);
        list[13].getGuid().setLow(0xBC36C441B1D27410);
        list[14].setName("alreadyConfigured");
        list[14].initGuid().setHigh(0xC11449FCD86841D3);
        list[14].getGuid().setLow(0xB27EE6CA72EA11CC);
        list[15].setName("improperlyConfig");
        list[15].initGuid().setHigh(0xC4246620B9FF4A96);
        list[15].getGuid().setLow(0xB5D65EF203C0E881);
        list[16].setName("properlyConfigured");
        list[16].initGuid().setHigh(0xDF9D797ECE424C6E);
        list[16].getGuid().setLow(0x953E7103CE45E269);
        list[17].setName("runtimeError");
        list[17].initGuid().setHigh(0x4AC74417AEBD4E10);
        list[17].getGuid().setLow(0xAA81D76E97FBBCAA);
        list[18].setName("outOfMemory");
        list[18].initGuid().setHigh(0x39043756CA274E8B);
        list[18].getGuid().setLow(0x9BB664363443CF9A);
        list[19].setName("outOfRange");
        list[19].initGuid().setHigh(0xC1A1E9E65B694A8A);
        list[19].getGuid().setLow(0xBFF2951F514A651B);
        list[20].setName("keyNotFound");
        list[20].initGuid().setHigh(0xC8A81C3EA4754185);
        list[20].getGuid().setLow(0x9AFC4EB0144CE6E2);
        list[21].setName("invalidPointer");
        list[21].initGuid().setHigh(0xB157017113304DE1);
        list[21].getGuid().setLow(0xB80566756D3E75C1);
        list[22].setName("invalidInterface");
        list[22].initGuid().setHigh(0xC7E4A47E6A2D4CDF);
        list[22].getGuid().setLow(0x86C203E22CDEBC98);
        list[23].setName("invalidToken");
        list[23].initGuid().setHigh(0x7A119C977C724F1B);
        list[23].getGuid().setLow(0x9D135752009F5EB1);
        list[24].setName("empty");
        list[24].initGuid().setHigh(0x493D0559DF584979);
        list[24].getGuid().setLow(0xB904030A6F8C0A73);
        list[25].setName("dirty");
        list[25].initGuid().setHigh(0x81E3FBB5EA384C27);
        list[25].getGuid().setLow(0x9887B7CF86FAB846);
        list[26].setName("sealed");
        list[26].initGuid().setHigh(0x23B714FF6A4043FD);
        list[26].getGuid().setLow(0xB3303743F0C955B5);
        list[27].setName("invalidated");
        list[27].initGuid().setHigh(0x786212027B2C4207);
        list[27].getGuid().setLow(0xAFE2908529F3D087);
        list[28].setName("notSystemModule");
        list[28].initGuid().setHigh(0x8377EDB1189545BA);
        list[28].getGuid().setLow(0x90BF1606AAD7BAB9);
        list[29].setName("moduleNotFound");
        list[29].initGuid().setHigh(0x51E53030191E4591);
        list[29].getGuid().setLow(0xA54E1FB0FFAE5137);
        list[30].setName("rtclassNotFound");
        list[30].initGuid().setHigh(0x058DAD2CB3234B9E);
        list[30].getGuid().setLow(0x96FC861E9EBF1879);
        list[31].setName("processNotFound");
        list[31].initGuid().setHigh(0xAF016669F1194C26);
        list[31].getGuid().setLow(0x8503859ED6AB5B59);
        list[32].setName("processNotComplete");
        list[32].initGuid().setHigh(0x517EA2D3961F4AD9);
        list[32].getGuid().setLow(0x9748754CE55A52C8);
        list[33].setName("processComplete");
        list[33].initGuid().setHigh(0xE5912CF9AE514335);
        list[33].getGuid().setLow(0x888FBF17B648EB28);
        list[34].setName("imageNotLoaded");
        list[34].initGuid().setHigh(0xA169E9E2E8D44C55);
        list[34].getGuid().setLow(0xAD33E5C1FBC388FF);
        list[35].setName("comFailed");
        list[35].initGuid().setHigh(0x0A23A540031D4394);
        list[35].getGuid().setLow(0x836400DC585AA362);
        list[36].setName("comSucceeded");
        list[36].initGuid().setHigh(0xAB7C4D183BAB45BC);
        list[36].getGuid().setLow(0x8B96A1407CA08A43);
        list[37].setName("dirDoesntExist");
        list[37].initGuid().setHigh(0xC80CC92B771441A8);
        list[37].getGuid().setLow(0x9471E17CD874F0AE);
        list[38].setName("dirAlreadyExists");
        list[38].initGuid().setHigh(0xDEF5D37EF0FF411A);
        list[38].getGuid().setLow(0x873C65A09AED089A);
        list[39].setName("pageAlreadyExists");
        list[39].initGuid().setHigh(0xA85AA7481A314C0A);
        list[39].getGuid().setLow(0xB19D7C850FFC960C);
        list[40].setName("noFileOpen");
        list[40].initGuid().setHigh(0x51312ED8FBE84B2B);
        list[40].getGuid().setLow(0xA36F0D3CEB2EBB76);
        list[41].setName("fileDoesntExist");
        list[41].initGuid().setHigh(0x73DECBE27CE14CB7);
        list[41].getGuid().setLow(0x9ECC3413119492F6);
        list[42].setName("fileAlreadyExists");
        list[42].initGuid().setHigh(0x74F793901CA14AD8);
        list[42].getGuid().setLow(0xB5352964112AA7AA);
        list[43].setName("fileHandleOpen");
        list[43].initGuid().setHigh(0x36A0A7EB59944BE5);
        list[43].getGuid().setLow(0x97B0E7F14AC420E2);
        list[44].setName("fileBadFormatting");
        list[44].initGuid().setHigh(0xB3B467CEE47F4557);
        list[44].getGuid().setLow(0xB3457DA5F594249B);

        return kj::READY_NOW;
    }

   private:
    template <typename Ctx>
    static void set(Ctx& ctx, uint64_t high, uint64_t low) {
        auto id = ctx.getResults().getId();
        id.setHigh(high);
        id.setLow(low);
    }
};

// ─────────────────────────────────────────────────────────────────
//  Factory — one line per return code, GUIDs from your dictionary
// ─────────────────────────────────────────────────────────────────
inline IIr77RETURN::Client makeRET() { return kj::heap<Ir77ReturnServer>(0xBE11AEB5923A4854, 0xB8B71CC80D56A296, 0xBE11AEB5923A4854, 0xB8B71CC80D56A296); }
inline IIr77RETURN::Client makeUnknown() { return kj::heap<Ir77ReturnServer>(0x9E5DF7990B114A2B, 0xAB3C11A3980128D3, 0xBE11AEB5923A4854, 0xB8B71CC80D56A296); }
inline IIr77RETURN::Client makeFalse() { return kj::heap<Ir77ReturnServer>(0x5DE62A6FD9F849BD, 0x80914DDBCAD9509A, 0xBE11AEB5923A4854, 0xB8B71CC80D56A296); }
inline IIr77RETURN::Client makeTrue() { return kj::heap<Ir77ReturnServer>(0x45FC03965A374386, 0xB82B8772F7180CA8, 0xBE11AEB5923A4854, 0xB8B71CC80D56A296); }
inline IIr77RETURN::Client makeIsEqual() { return kj::heap<Ir77ReturnServer>(0x5722B3779F2743F3, 0x8AD2A914D1CFE987, 0xBE11AEB5923A4854, 0xB8B71CC80D56A296); }
inline IIr77RETURN::Client makeIsLesser() { return kj::heap<Ir77ReturnServer>(0x84B4D96612274B85, 0x9EF459F16D84FBD7, 0xBE11AEB5923A4854, 0xB8B71CC80D56A296); }
inline IIr77RETURN::Client makeIsGreater() {
    return kj::heap<Ir77ReturnServer>(0xCF21B52FE58A4E3E, 0x87E5C1A919B37C8A, 0xBE11AEB5923A4854, 0xB8B71CC80D56A296);
}
inline IIr77RETURN::Client makeInvalidArgument() {
    return kj::heap<Ir77ReturnServer>(0x5F927FFD1B4F42CD, 0x94F5E6CFCBD4B27B, 0xBE11AEB5923A4854, 0xB8B71CC80D56A296);
}
inline IIr77RETURN::Client makeValidArgument() {
    return kj::heap<Ir77ReturnServer>(0x19369F2EC49343ED, 0x8C1CED82BE10181C, 0xBE11AEB5923A4854, 0xB8B71CC80D56A296);
}
inline IIr77RETURN::Client makeOperationFailed() {
    return kj::heap<Ir77ReturnServer>(0xB72C862FDA2B45C8, 0x92F58F257DB0974C, 0xBE11AEB5923A4854, 0xB8B71CC80D56A296);
}
inline IIr77RETURN::Client makeOperationConflict() {
    return kj::heap<Ir77ReturnServer>(0x8030334DB3684201, 0x9FFBFAEC2635EC65, 0xBE11AEB5923A4854, 0xB8B71CC80D56A296);
}
inline IIr77RETURN::Client makeOperationSucceeded() {
    return kj::heap<Ir77ReturnServer>(0x6CD78C706C544EDB, 0x8C76A83169BF7AFE, 0xBE11AEB5923A4854, 0xB8B71CC80D56A296);
}
inline IIr77RETURN::Client makeInvalidOperation() {
    return kj::heap<Ir77ReturnServer>(0x04DF984D34404C03, 0xAF65CF939BCD85E0, 0xBE11AEB5923A4854, 0xB8B71CC80D56A296);
}
inline IIr77RETURN::Client makeNotConfigured() {
    return kj::heap<Ir77ReturnServer>(0x36D19823064E4F88, 0xBC36C441B1D27410, 0xBE11AEB5923A4854, 0xB8B71CC80D56A296);
}
inline IIr77RETURN::Client makeAlreadyConfigured() {
    return kj::heap<Ir77ReturnServer>(0xC11449FCD86841D3, 0xB27EE6CA72EA11CC, 0xBE11AEB5923A4854, 0xB8B71CC80D56A296);
}
inline IIr77RETURN::Client makeImproperlyConfig() {
    return kj::heap<Ir77ReturnServer>(0xC4246620B9FF4A96, 0xB5D65EF203C0E881, 0xBE11AEB5923A4854, 0xB8B71CC80D56A296);
}
inline IIr77RETURN::Client makeProperlyConfigured() {
    return kj::heap<Ir77ReturnServer>(0xDF9D797ECE424C6E, 0x953E7103CE45E269, 0xBE11AEB5923A4854, 0xB8B71CC80D56A296);
}
inline IIr77RETURN::Client makeRuntimeError() {
    return kj::heap<Ir77ReturnServer>(0x4AC74417AEBD4E10, 0xAA81D76E97FBBCAA, 0xBE11AEB5923A4854, 0xB8B71CC80D56A296);
}
inline IIr77RETURN::Client makeOutOfMemory() {
    return kj::heap<Ir77ReturnServer>(0x39043756CA274E8B, 0x9BB664363443CF9A, 0xBE11AEB5923A4854, 0xB8B71CC80D56A296);
}
inline IIr77RETURN::Client makeOutOfRange() {
    return kj::heap<Ir77ReturnServer>(0xC1A1E9E65B694A8A, 0xBFF2951F514A651B, 0xBE11AEB5923A4854, 0xB8B71CC80D56A296);
}
inline IIr77RETURN::Client makeKeyNotFound() {
    return kj::heap<Ir77ReturnServer>(0xC8A81C3EA4754185, 0x9AFC4EB0144CE6E2, 0xBE11AEB5923A4854, 0xB8B71CC80D56A296);
}
inline IIr77RETURN::Client makeInvalidPointer() {
    return kj::heap<Ir77ReturnServer>(0xB157017113304DE1, 0xB80566756D3E75C1, 0xBE11AEB5923A4854, 0xB8B71CC80D56A296);
}
inline IIr77RETURN::Client makeInvalidInterface() {
    return kj::heap<Ir77ReturnServer>(0xC7E4A47E6A2D4CDF, 0x86C203E22CDEBC98, 0xBE11AEB5923A4854, 0xB8B71CC80D56A296);
}
inline IIr77RETURN::Client makeInvalidToken() {
    return kj::heap<Ir77ReturnServer>(0x7A119C977C724F1B, 0x9D135752009F5EB1, 0xBE11AEB5923A4854, 0xB8B71CC80D56A296);
}
inline IIr77RETURN::Client makeEmpty() { return kj::heap<Ir77ReturnServer>(0x493D0559DF584979, 0xB904030A6F8C0A73, 0xBE11AEB5923A4854, 0xB8B71CC80D56A296); }
inline IIr77RETURN::Client makeDirty() { return kj::heap<Ir77ReturnServer>(0x81E3FBB5EA384C27, 0x9887B7CF86FAB846, 0xBE11AEB5923A4854, 0xB8B71CC80D56A296); }
inline IIr77RETURN::Client makeSealed() { return kj::heap<Ir77ReturnServer>(0x23B714FF6A4043FD, 0xB3303743F0C955B5, 0xBE11AEB5923A4854, 0xB8B71CC80D56A296); }
inline IIr77RETURN::Client makeInvalidated() {
    return kj::heap<Ir77ReturnServer>(0x786212027B2C4207, 0xAFE2908529F3D087, 0xBE11AEB5923A4854, 0xB8B71CC80D56A296);
}
inline IIr77RETURN::Client makeNotSystemModule() {
    return kj::heap<Ir77ReturnServer>(0x8377EDB1189545BA, 0x90BF1606AAD7BAB9, 0xBE11AEB5923A4854, 0xB8B71CC80D56A296);
}
inline IIr77RETURN::Client makeModuleNotFound() {
    return kj::heap<Ir77ReturnServer>(0x51E53030191E4591, 0xA54E1FB0FFAE5137, 0xBE11AEB5923A4854, 0xB8B71CC80D56A296);
}
inline IIr77RETURN::Client makeRtclassNotFound() {
    return kj::heap<Ir77ReturnServer>(0x058DAD2CB3234B9E, 0x96FC861E9EBF1879, 0xBE11AEB5923A4854, 0xB8B71CC80D56A296);
}
inline IIr77RETURN::Client makeProcessNotFound() {
    return kj::heap<Ir77ReturnServer>(0xAF016669F1194C26, 0x8503859ED6AB5B59, 0xBE11AEB5923A4854, 0xB8B71CC80D56A296);
}
inline IIr77RETURN::Client makeProcessNotComplete() {
    return kj::heap<Ir77ReturnServer>(0x517EA2D3961F4AD9, 0x9748754CE55A52C8, 0xBE11AEB5923A4854, 0xB8B71CC80D56A296);
}
inline IIr77RETURN::Client makeProcessComplete() {
    return kj::heap<Ir77ReturnServer>(0xE5912CF9AE514335, 0x888FBF17B648EB28, 0xBE11AEB5923A4854, 0xB8B71CC80D56A296);
}
inline IIr77RETURN::Client makeImageNotLoaded() {
    return kj::heap<Ir77ReturnServer>(0xA169E9E2E8D44C55, 0xAD33E5C1FBC388FF, 0xBE11AEB5923A4854, 0xB8B71CC80D56A296);
}
inline IIr77RETURN::Client makeComFailed() {
    return kj::heap<Ir77ReturnServer>(0x0A23A540031D4394, 0x836400DC585AA362, 0xBE11AEB5923A4854, 0xB8B71CC80D56A296);
}
inline IIr77RETURN::Client makeComSucceeded() {
    return kj::heap<Ir77ReturnServer>(0xAB7C4D183BAB45BC, 0x8B96A1407CA08A43, 0xBE11AEB5923A4854, 0xB8B71CC80D56A296);
}
inline IIr77RETURN::Client makeDirDoesntExist() {
    return kj::heap<Ir77ReturnServer>(0xC80CC92B771441A8, 0x9471E17CD874F0AE, 0xBE11AEB5923A4854, 0xB8B71CC80D56A296);
}
inline IIr77RETURN::Client makeDirAlreadyExists() {
    return kj::heap<Ir77ReturnServer>(0xDEF5D37EF0FF411A, 0x873C65A09AED089A, 0xBE11AEB5923A4854, 0xB8B71CC80D56A296);
}
inline IIr77RETURN::Client makePageAlreadyExists() {
    return kj::heap<Ir77ReturnServer>(0xA85AA7481A314C0A, 0xB19D7C850FFC960C, 0xBE11AEB5923A4854, 0xB8B71CC80D56A296);
}
inline IIr77RETURN::Client makeNoFileOpen() {
    return kj::heap<Ir77ReturnServer>(0x51312ED8FBE84B2B, 0xA36F0D3CEB2EBB76, 0xBE11AEB5923A4854, 0xB8B71CC80D56A296);
}
inline IIr77RETURN::Client makeFileDoesntExist() {
    return kj::heap<Ir77ReturnServer>(0x73DECBE27CE14CB7, 0x9ECC3413119492F6, 0xBE11AEB5923A4854, 0xB8B71CC80D56A296);
}
inline IIr77RETURN::Client makeFileAlreadyExists() {
    return kj::heap<Ir77ReturnServer>(0x74F793901CA14AD8, 0xB5352964112AA7AA, 0xBE11AEB5923A4854, 0xB8B71CC80D56A296);
}
inline IIr77RETURN::Client makeFileHandleOpen() {
    return kj::heap<Ir77ReturnServer>(0x36A0A7EB59944BE5, 0x97B0E7F14AC420E2, 0xBE11AEB5923A4854, 0xB8B71CC80D56A296);
}
inline IIr77RETURN::Client makeFileBadFormatting() {
    return kj::heap<Ir77ReturnServer>(0xB3B467CEE47F4557, 0xB3457DA5F594249B, 0xBE11AEB5923A4854, 0xB8B71CC80D56A296);
}

}  // namespace NSIr77RT