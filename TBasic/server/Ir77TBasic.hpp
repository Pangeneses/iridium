#pragma once

#include <chrono>
#include <random>
#include <cstdint>
#include <memory>
#include <string>

#include "../../Ir77RT/dictionary/IDIIr77MPVM.hpp"

#include "../dictionary/IDOPIr77TBASIC.hpp"
#include "../dictionary/IDOCIr77TBASIC.hpp"
#include "../dictionary/IDIr77TBASIC.hpp"

#include "../../Ir77RT/interface/IIr77GUID.hpp"
#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Patch.hpp"
#include "../../Ir77RT/interface/IIr77Dispatch.hpp"
#include "../../Ir77RT/interface/IIr77Operand.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"
#include "../../Ir77RT/runtime/Ir77Patch.hpp"

#include "../operand/Ir77AlphaNumeric.hpp"
#include "../operand/Ir77CHRONO.hpp"
#include "../operand/Ir77Hex.hpp"
#include "../operand/Ir77HHMMSS.hpp"
#include "../operand/Ir77MaxPath.hpp"
#include "../operand/Ir77MMDDYYYY.hpp"
#include "../operand/Ir77MMM.hpp"
#include "../operand/Ir77Numeric.hpp"
#include "../operand/Ir77SysID.hpp"
#include "../operand/Ir77TextBlobA.hpp"
#include "../operand/Ir77TextBlobW.hpp"
#include "../operand/Ir77UID.hpp"

using namespace NSIr77RT;
// using namespace NSIr77REDOS;

namespace NSIr77TBasic {
class Ir77Basic : public Ir77Enlisted, public IIr77Dispatch, public std::enable_shared_from_this<Ir77Basic> {
   public:
    Ir77Basic() {
        try {
            m_enlisted_uuid.Generate();
        } catch (std::invalid_argument a) {
            throw a;
        }

        m_enlisted = std::chrono::system_clock::now();

        m_patch = std::make_shared<Ir77Patch>();

        m_alpha_numeric = std::make_shared<Ir77AlphaNumeric>();
        m_chrono = std::make_shared<Ir77CHRONO>();
        m_hex = std::make_shared<Ir77Hex>();
        m_hhmmss = std::make_shared<Ir77HHMMSS>();
        m_maxpath = std::make_shared<Ir77MaxPath>();
        m_mmddyyyy = std::make_shared<Ir77MMDDYYYY>();
        m_mmm = std::make_shared<Ir77MMM>();
        m_numeric = std::make_shared<Ir77Numeric>();
        m_sysid = std::make_shared<Ir77SysID>();
        m_textblob_a = std::make_shared<Ir77TextBlobA>();
        m_textblob_w = std::make_shared<Ir77TextBlobW>();
        m_uid = std::make_shared<Ir77UID>();

        m_patch->AddOperation({GUIDOPIr77AlphaNumeric, GUIDOCIr77Assign},
                              [this](std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) -> std::shared_ptr<IIr77Return const> {
                                  return m_alpha_numeric->Assign(rhs, lhs);
                              });
        m_patch->AddOperation({GUIDOPIr77AlphaNumeric, GUIDOCIr77Get},
                              [this](std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) -> std::shared_ptr<IIr77Return const> {
                                  return m_alpha_numeric->Get(rhs, lhs);
                              });
        m_patch->AddOperation({GUIDOPIr77AlphaNumeric, GUIDOCIr77Equal},
                              [this](std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) -> std::shared_ptr<IIr77Return const> {
                                  return m_alpha_numeric->Equal(rhs, lhs);
                              });
        m_patch->AddOperation({GUIDOPIr77AlphaNumeric, GUIDOCIr77Not},
                              [this](std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) -> std::shared_ptr<IIr77Return const> {
                                  return m_alpha_numeric->Not(rhs, lhs);
                              });

        m_patch->AddOperation({GUIDOPIr77Numeric, GUIDOCIr77Assign},
                              [this](std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) -> std::shared_ptr<IIr77Return const> {
                                  return m_numeric->Assign(rhs, lhs);
                              });
        m_patch->AddOperation({GUIDOPIr77Numeric, GUIDOCIr77Get},
                              [this](std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) -> std::shared_ptr<IIr77Return const> {
                                  return m_numeric->Get(rhs, lhs);
                              });
        m_patch->AddOperation({GUIDOPIr77Numeric, GUIDOCIr77Equal},
                              [this](std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) -> std::shared_ptr<IIr77Return const> {
                                  return m_numeric->Equal(rhs, lhs);
                              });
        m_patch->AddOperation({GUIDOPIr77Numeric, GUIDOCIr77Not},
                              [this](std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) -> std::shared_ptr<IIr77Return const> {
                                  return m_numeric->Not(rhs, lhs);
                              });
        m_patch->AddOperation({GUIDOPIr77Numeric, GUIDOCIr77Lesser},
                              [this](std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) -> std::shared_ptr<IIr77Return const> {
                                  return m_numeric->Lesser(rhs, lhs);
                              });
        m_patch->AddOperation({GUIDOPIr77Numeric, GUIDOCIr77Greater},
                              [this](std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) -> std::shared_ptr<IIr77Return const> {
                                  return m_numeric->Greater(rhs, lhs);
                              });

        m_patch->AddOperation({GUIDOPIr77UID, GUIDOCIr77Assign},
                              [this](std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) -> std::shared_ptr<IIr77Return const> {
                                  return m_uid->Assign(rhs, lhs);
                              });
        m_patch->AddOperation({GUIDOPIr77UID, GUIDOCIr77Get},
                              [this](std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) -> std::shared_ptr<IIr77Return const> {
                                  return m_uid->Get(rhs, lhs);
                              });
        m_patch->AddOperation({GUIDOPIr77UID, GUIDOCIr77Equal},
                              [this](std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) -> std::shared_ptr<IIr77Return const> {
                                  return m_uid->Equal(rhs, lhs);
                              });
        m_patch->AddOperation({GUIDOPIr77UID, GUIDOCIr77Not},
                              [this](std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) -> std::shared_ptr<IIr77Return const> {
                                  return m_uid->Not(rhs, lhs);
                              });
        m_patch->AddOperation({GUIDOPIr77UID, GUIDOCIr77Lesser},
                              [this](std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) -> std::shared_ptr<IIr77Return const> {
                                  return m_uid->Lesser(rhs, lhs);
                              });
        m_patch->AddOperation({GUIDOPIr77UID, GUIDOCIr77Greater},
                              [this](std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) -> std::shared_ptr<IIr77Return const> {
                                  return m_uid->Greater(rhs, lhs);
                              });
        m_patch->AddOperation({GUIDOPIr77UID, GUIDOCIr77Generate},
                              [this](std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) -> std::shared_ptr<IIr77Return const> {
                                  return m_uid->Generate(rhs, lhs);
                              });

        m_patch->AddOperation({GUIDOPIr77MMM, GUIDOCIr77Assign},
                              [this](std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) -> std::shared_ptr<IIr77Return const> {
                                  return m_mmm->Assign(rhs, lhs);
                              });
        m_patch->AddOperation({GUIDOPIr77MMM, GUIDOCIr77Get},
                              [this](std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) -> std::shared_ptr<IIr77Return const> {
                                  return m_mmm->Get(rhs, lhs);
                              });
        m_patch->AddOperation({GUIDOPIr77MMM, GUIDOCIr77Equal},
                              [this](std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) -> std::shared_ptr<IIr77Return const> {
                                  return m_mmm->Equal(rhs, lhs);
                              });
        m_patch->AddOperation({GUIDOPIr77MMM, GUIDOCIr77Not},
                              [this](std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) -> std::shared_ptr<IIr77Return const> {
                                  return m_mmm->Not(rhs, lhs);
                              });
        m_patch->AddOperation({GUIDOPIr77MMM, GUIDOCIr77Lesser},
                              [this](std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) -> std::shared_ptr<IIr77Return const> {
                                  return m_mmm->Lesser(rhs, lhs);
                              });
        m_patch->AddOperation({GUIDOPIr77MMM, GUIDOCIr77Greater},
                              [this](std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) -> std::shared_ptr<IIr77Return const> {
                                  return m_mmm->Greater(rhs, lhs);
                              });

        m_patch->AddOperation({GUIDOPIr77CHRONO, GUIDOCIr77Assign},
                              [this](std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) -> std::shared_ptr<IIr77Return const> {
                                  return m_chrono->Assign(rhs, lhs);
                              });
        m_patch->AddOperation({GUIDOPIr77CHRONO, GUIDOCIr77Get},
                              [this](std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) -> std::shared_ptr<IIr77Return const> {
                                  return m_chrono->Get(rhs, lhs);
                              });
        m_patch->AddOperation({GUIDOPIr77CHRONO, GUIDOCIr77Equal},
                              [this](std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) -> std::shared_ptr<IIr77Return const> {
                                  return m_chrono->Equal(rhs, lhs);
                              });
        m_patch->AddOperation({GUIDOPIr77CHRONO, GUIDOCIr77Not},
                              [this](std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) -> std::shared_ptr<IIr77Return const> {
                                  return m_chrono->Not(rhs, lhs);
                              });
        m_patch->AddOperation({GUIDOPIr77CHRONO, GUIDOCIr77Lesser},
                              [this](std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) -> std::shared_ptr<IIr77Return const> {
                                  return m_chrono->Lesser(rhs, lhs);
                              });
        m_patch->AddOperation({GUIDOPIr77CHRONO, GUIDOCIr77Greater},
                              [this](std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) -> std::shared_ptr<IIr77Return const> {
                                  return m_chrono->Greater(rhs, lhs);
                              });
        m_patch->AddOperation({GUIDOPIr77CHRONO, GUIDOCIr77Current},
                              [this](std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) -> std::shared_ptr<IIr77Return const> {
                                  return m_chrono->Current(rhs, lhs);
                              });

        m_patch->AddOperation({GUIDOPIr77MMDDYYYY, GUIDOCIr77Assign},
                              [this](std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) -> std::shared_ptr<IIr77Return const> {
                                  return m_mmddyyyy->Assign(rhs, lhs);
                              });
        m_patch->AddOperation({GUIDOPIr77MMDDYYYY, GUIDOCIr77Get},
                              [this](std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) -> std::shared_ptr<IIr77Return const> {
                                  return m_mmddyyyy->Get(rhs, lhs);
                              });
        m_patch->AddOperation({GUIDOPIr77MMDDYYYY, GUIDOCIr77Equal},
                              [this](std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) -> std::shared_ptr<IIr77Return const> {
                                  return m_mmddyyyy->Equal(rhs, lhs);
                              });
        m_patch->AddOperation({GUIDOPIr77MMDDYYYY, GUIDOCIr77Not},
                              [this](std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) -> std::shared_ptr<IIr77Return const> {
                                  return m_mmddyyyy->Not(rhs, lhs);
                              });
        m_patch->AddOperation({GUIDOPIr77MMDDYYYY, GUIDOCIr77Lesser},
                              [this](std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) -> std::shared_ptr<IIr77Return const> {
                                  return m_mmddyyyy->Lesser(rhs, lhs);
                              });
        m_patch->AddOperation({GUIDOPIr77MMDDYYYY, GUIDOCIr77Greater},
                              [this](std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) -> std::shared_ptr<IIr77Return const> {
                                  return m_mmddyyyy->Greater(rhs, lhs);
                              });
        m_patch->AddOperation({GUIDOPIr77MMDDYYYY, GUIDOCIr77Current},
                              [this](std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) -> std::shared_ptr<IIr77Return const> {
                                  return m_mmddyyyy->Current(rhs, lhs);
                              });

        m_patch->AddOperation({GUIDOPIr77HHMMSS, GUIDOCIr77Assign},
                              [this](std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) -> std::shared_ptr<IIr77Return const> {
                                  return m_hhmmss->Assign(rhs, lhs);
                              });
        m_patch->AddOperation({GUIDOPIr77HHMMSS, GUIDOCIr77Get},
                              [this](std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) -> std::shared_ptr<IIr77Return const> {
                                  return m_hhmmss->Get(rhs, lhs);
                              });
        m_patch->AddOperation({GUIDOPIr77HHMMSS, GUIDOCIr77Equal},
                              [this](std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) -> std::shared_ptr<IIr77Return const> {
                                  return m_hhmmss->Equal(rhs, lhs);
                              });
        m_patch->AddOperation({GUIDOPIr77HHMMSS, GUIDOCIr77Not},
                              [this](std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) -> std::shared_ptr<IIr77Return const> {
                                  return m_hhmmss->Not(rhs, lhs);
                              });
        m_patch->AddOperation({GUIDOPIr77HHMMSS, GUIDOCIr77Lesser},
                              [this](std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) -> std::shared_ptr<IIr77Return const> {
                                  return m_hhmmss->Lesser(rhs, lhs);
                              });
        m_patch->AddOperation({GUIDOPIr77HHMMSS, GUIDOCIr77Greater},
                              [this](std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) -> std::shared_ptr<IIr77Return const> {
                                  return m_hhmmss->Greater(rhs, lhs);
                              });
        m_patch->AddOperation({GUIDOPIr77HHMMSS, GUIDOCIr77Current},
                              [this](std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) -> std::shared_ptr<IIr77Return const> {
                                  return m_hhmmss->Current(rhs, lhs);
                              });

        m_patch->AddOperation({GUIDOPIr77MaxPath, GUIDOCIr77Assign},
                              [this](std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) -> std::shared_ptr<IIr77Return const> {
                                  return m_maxpath->Assign(rhs, lhs);
                              });
        m_patch->AddOperation({GUIDOPIr77MaxPath, GUIDOCIr77Get},
                              [this](std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) -> std::shared_ptr<IIr77Return const> {
                                  return m_maxpath->Get(rhs, lhs);
                              });
        m_patch->AddOperation({GUIDOPIr77MaxPath, GUIDOCIr77Equal},
                              [this](std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) -> std::shared_ptr<IIr77Return const> {
                                  return m_maxpath->Equal(rhs, lhs);
                              });
        m_patch->AddOperation({GUIDOPIr77MaxPath, GUIDOCIr77Not},
                              [this](std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) -> std::shared_ptr<IIr77Return const> {
                                  return m_maxpath->Not(rhs, lhs);
                              });

        m_patch->AddOperation({GUIDOPIr77SysID, GUIDOCIr77Assign},
                              [this](std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) -> std::shared_ptr<IIr77Return const> {
                                  return m_sysid->Assign(rhs, lhs);
                              });
        m_patch->AddOperation({GUIDOPIr77SysID, GUIDOCIr77Get},
                              [this](std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) -> std::shared_ptr<IIr77Return const> {
                                  return m_sysid->Get(rhs, lhs);
                              });
        m_patch->AddOperation({GUIDOPIr77SysID, GUIDOCIr77Equal},
                              [this](std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) -> std::shared_ptr<IIr77Return const> {
                                  return m_sysid->Equal(rhs, lhs);
                              });
        m_patch->AddOperation({GUIDOPIr77SysID, GUIDOCIr77Not},
                              [this](std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) -> std::shared_ptr<IIr77Return const> {
                                  return m_sysid->Not(rhs, lhs);
                              });

        m_patch->AddOperation({GUIDOPIr77Hex, GUIDOCIr77Assign},
                              [this](std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) -> std::shared_ptr<IIr77Return const> {
                                  return m_hex->Assign(rhs, lhs);
                              });
        m_patch->AddOperation({GUIDOPIr77Hex, GUIDOCIr77Get},
                              [this](std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) -> std::shared_ptr<IIr77Return const> {
                                  return m_hex->Get(rhs, lhs);
                              });
        m_patch->AddOperation({GUIDOPIr77Hex, GUIDOCIr77Equal},
                              [this](std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) -> std::shared_ptr<IIr77Return const> {
                                  return m_hex->Equal(rhs, lhs);
                              });
        m_patch->AddOperation({GUIDOPIr77Hex, GUIDOCIr77Not},
                              [this](std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) -> std::shared_ptr<IIr77Return const> {
                                  return m_hex->Not(rhs, lhs);
                              });
        m_patch->AddOperation({GUIDOPIr77Hex, GUIDOCIr77Lesser},
                              [this](std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) -> std::shared_ptr<IIr77Return const> {
                                  return m_hex->Lesser(rhs, lhs);
                              });
        m_patch->AddOperation({GUIDOPIr77Hex, GUIDOCIr77Greater},
                              [this](std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) -> std::shared_ptr<IIr77Return const> {
                                  return m_hex->Greater(rhs, lhs);
                              });

        m_patch->AddOperation({GUIDOPIr77TextBlobA, GUIDOCIr77Assign},
                              [this](std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) -> std::shared_ptr<IIr77Return const> {
                                  return m_textblob_a->Assign(rhs, lhs);
                              });
        m_patch->AddOperation({GUIDOPIr77TextBlobA, GUIDOCIr77Get},
                              [this](std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) -> std::shared_ptr<IIr77Return const> {
                                  return m_textblob_a->Get(rhs, lhs);
                              });
        m_patch->AddOperation({GUIDOPIr77TextBlobA, GUIDOCIr77Equal},
                              [this](std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) -> std::shared_ptr<IIr77Return const> {
                                  return m_textblob_a->Equal(rhs, lhs);
                              });
        m_patch->AddOperation({GUIDOPIr77TextBlobA, GUIDOCIr77Not},
                              [this](std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) -> std::shared_ptr<IIr77Return const> {
                                  return m_textblob_a->Not(rhs, lhs);
                              });

        m_patch->AddOperation({GUIDOPIr77TextBlobW, GUIDOCIr77Assign},
                              [this](std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) -> std::shared_ptr<IIr77Return const> {
                                  return m_textblob_w->Assign(rhs, lhs);
                              });
        m_patch->AddOperation({GUIDOPIr77TextBlobW, GUIDOCIr77Get},
                              [this](std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) -> std::shared_ptr<IIr77Return const> {
                                  return m_textblob_w->Get(rhs, lhs);
                              });
        m_patch->AddOperation({GUIDOPIr77TextBlobW, GUIDOCIr77Equal},
                              [this](std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) -> std::shared_ptr<IIr77Return const> {
                                  return m_textblob_w->Equal(rhs, lhs);
                              });
        m_patch->AddOperation({GUIDOPIr77TextBlobW, GUIDOCIr77Not},
                              [this](std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) -> std::shared_ptr<IIr77Return const> {
                                  return m_textblob_w->Not(rhs, lhs);
                              });
    }

   public:
    std::shared_ptr<IIr77Return const> EnlistedAs(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIIr77Dispatch>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> MemberUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77TBASIC>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CollectionUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77TBASIC>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    IIr77GUID* const QueryInterface(IIr77GUID const* iid, std::shared_ptr<void>& obj) {
        if (iid == &GUIDIIr77Enlisted)
            obj = std::shared_ptr<IIr77Enlisted>(shared_from_this(), static_cast<IIr77Enlisted*>(this));

        else if (iid == &GUIDIIr77Dispatch)
            obj = std::shared_ptr<IIr77Dispatch>(shared_from_this(), static_cast<IIr77Dispatch*>(this));

        else if (iid == &GUIDIr77TBASIC)
            obj = std::shared_ptr<Ir77Basic>(shared_from_this(), static_cast<Ir77Basic*>(this));

        else
            return &GUIDQueryFailed;

        return &GUIDQuerySucceeded;
    }

   public:
    std::shared_ptr<IIr77Return const> Dispatch(std::shared_ptr<IIr77Stack const>& stack) { return m_patch->Forward(stack); }

    std::shared_ptr<IIr77Return const> Factory(std::shared_ptr<IIr77GUID const>& uid, std::shared_ptr<IIr77Enlisted>& obj, std::uint64_t& id) {
        if (id == CREATE_NEW) id = random_u64();

        if (*uid == GUIDOPIr77AlphaNumeric) {
            m_alpha_numeric_map.emplace(id, std::make_shared<Ir77AlphaNumeric>());
            obj = std::dynamic_pointer_cast<IIr77Enlisted>(m_alpha_numeric_map.at(id));
        }
        if (*uid == GUIDOPIr77Numeric) {
            m_numeric_map.emplace(id, std::make_shared<Ir77Numeric>());
            obj = std::dynamic_pointer_cast<IIr77Enlisted>(m_numeric_map.at(id));
        }
        if (*uid == GUIDOPIr77UID) {
            m_uid_map.emplace(id, std::make_shared<Ir77UID>());
            obj = std::dynamic_pointer_cast<IIr77Enlisted>(m_uid_map.at(id));
        }
        if (*uid == GUIDOPIr77MMM) {
            m_mmm_map.emplace(id, std::make_shared<Ir77MMM>());
            obj = std::dynamic_pointer_cast<IIr77Enlisted>(m_mmm_map.at(id));
        }
        if (*uid == GUIDOPIr77CHRONO) {
            m_chrono_map.emplace(id, std::make_shared<Ir77CHRONO>());
            obj = std::dynamic_pointer_cast<IIr77Enlisted>(m_chrono_map.at(id));
        }
        if (*uid == GUIDOPIr77MMDDYYYY) {
            m_mmddyyyy_map.emplace(id, std::make_shared<Ir77MMDDYYYY>());
            obj = std::dynamic_pointer_cast<IIr77Enlisted>(m_mmddyyyy_map.at(id));
        }
        if (*uid == GUIDOPIr77HHMMSS) {
            m_hhmmss_map.emplace(id, std::make_shared<Ir77HHMMSS>());
            obj = std::dynamic_pointer_cast<IIr77Enlisted>(m_hhmmss_map.at(id));
        }
        if (*uid == GUIDOPIr77MaxPath) {
            m_maxpath_map.emplace(id, std::make_shared<Ir77MaxPath>());
            obj = std::dynamic_pointer_cast<IIr77Enlisted>(m_maxpath_map.at(id));
        }
        if (*uid == GUIDOPIr77SysID) {
            m_sysid_map.emplace(id, std::make_shared<Ir77SysID>());
            obj = std::dynamic_pointer_cast<IIr77Enlisted>(m_sysid_map.at(id));
        }
        if (*uid == GUIDOPIr77Hex) {
            m_hex_map.emplace(id, std::make_shared<Ir77Hex>());
            obj = std::dynamic_pointer_cast<IIr77Enlisted>(m_hex_map.at(id));
        }
        if (*uid == GUIDOPIr77TextBlobA) {
            m_textblob_a_map.emplace(id, std::make_shared<Ir77TextBlobA>());
            obj = std::dynamic_pointer_cast<IIr77Enlisted>(m_textblob_a_map.at(id));
        }
        if (*uid == GUIDOPIr77TextBlobW) {
            m_textblob_w_map.emplace(id, std::make_shared<Ir77TextBlobW>());
            obj = std::dynamic_pointer_cast<IIr77Enlisted>(m_textblob_w_map.at(id));
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> Garbage(std::shared_ptr<IIr77GUID const>& uid, std::uint64_t const& id) {
        if (*uid == GUIDOPIr77AlphaNumeric) {
            m_alpha_numeric_map.erase(id);
        }
        if (*uid == GUIDOPIr77Numeric) {
            m_numeric_map.erase(id);
        }
        if (*uid == GUIDOPIr77UID) {
            m_uid_map.erase(id);
        }
        if (*uid == GUIDOPIr77MMM) {
            m_mmm_map.erase(id);
        }
        if (*uid == GUIDOPIr77CHRONO) {
            m_chrono_map.erase(id);
        }
        if (*uid == GUIDOPIr77MMDDYYYY) {
            m_mmddyyyy_map.erase(id);
        }
        if (*uid == GUIDOPIr77HHMMSS) {
            m_hhmmss_map.erase(id);
        }
        if (*uid == GUIDOPIr77MaxPath) {
            m_maxpath_map.erase(id);
        }
        if (*uid == GUIDOPIr77SysID) {
            m_sysid_map.erase(id);
        }
        if (*uid == GUIDOPIr77Hex) {
            m_hex_map.erase(id);
        }
        if (*uid == GUIDOPIr77TextBlobA) {
            m_textblob_a_map.erase(id);
        }
        if (*uid == GUIDOPIr77TextBlobW) {
            m_textblob_w_map.erase(id);
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   private:
    uint64_t random_u64() {
        static std::mt19937_64 rng(std::random_device{}());
        std::uniform_int_distribution<uint64_t> dist;
        return dist(rng);
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

    std::string m_tag_text{'\0'};

   private:
    std::shared_ptr<Ir77AlphaNumeric> m_alpha_numeric{nullptr};
    std::shared_ptr<Ir77CHRONO> m_chrono{nullptr};
    std::shared_ptr<Ir77Hex> m_hex{nullptr};
    std::shared_ptr<Ir77HHMMSS> m_hhmmss{nullptr};
    std::shared_ptr<Ir77MaxPath> m_maxpath{nullptr};
    std::shared_ptr<Ir77MMDDYYYY> m_mmddyyyy{nullptr};
    std::shared_ptr<Ir77MMM> m_mmm{nullptr};
    std::shared_ptr<Ir77Numeric> m_numeric{nullptr};
    std::shared_ptr<Ir77SysID> m_sysid{nullptr};
    std::shared_ptr<Ir77TextBlobA> m_textblob_a{nullptr};
    std::shared_ptr<Ir77TextBlobW> m_textblob_w{nullptr};
    std::shared_ptr<Ir77UID> m_uid{nullptr};

    std::map<std::uint64_t const, std::shared_ptr<Ir77AlphaNumeric>> m_alpha_numeric_map;
    std::map<std::uint64_t const, std::shared_ptr<Ir77CHRONO>> m_chrono_map;
    std::map<std::uint64_t const, std::shared_ptr<Ir77Hex>> m_hex_map;
    std::map<std::uint64_t const, std::shared_ptr<Ir77HHMMSS>> m_hhmmss_map;
    std::map<std::uint64_t const, std::shared_ptr<Ir77MaxPath>> m_maxpath_map;
    std::map<std::uint64_t const, std::shared_ptr<Ir77MMDDYYYY>> m_mmddyyyy_map;
    std::map<std::uint64_t const, std::shared_ptr<Ir77MMM>> m_mmm_map;
    std::map<std::uint64_t const, std::shared_ptr<Ir77Numeric>> m_numeric_map;
    std::map<std::uint64_t const, std::shared_ptr<Ir77SysID>> m_sysid_map;
    std::map<std::uint64_t const, std::shared_ptr<Ir77TextBlobA>> m_textblob_a_map;
    std::map<std::uint64_t const, std::shared_ptr<Ir77TextBlobW>> m_textblob_w_map;
    std::map<std::uint64_t const, std::shared_ptr<Ir77UID>> m_uid_map;

   private:
    const std::vector<char> Ir77AlphaNumericW{'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u',
                                              'v', 'w', 'x', 'y', 'z', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
                                              'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
};

}  // namespace NSIr77TBasic
