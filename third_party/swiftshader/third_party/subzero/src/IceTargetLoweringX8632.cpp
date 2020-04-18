//===- subzero/src/IceTargetLoweringX8632.cpp - x86-32 lowering -----------===//
//
//                        The Subzero Code Generator
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// \brief Implements the TargetLoweringX8632 class, which consists almost
/// entirely of the lowering sequence for each high-level instruction.
///
//===----------------------------------------------------------------------===//

#include "IceTargetLoweringX8632.h"

#include "IceTargetLoweringX8632Traits.h"

namespace X8632 {
std::unique_ptr<::Ice::TargetLowering> createTargetLowering(::Ice::Cfg *Func) {
  return ::Ice::X8632::TargetX8632::create(Func);
}

std::unique_ptr<::Ice::TargetDataLowering>
createTargetDataLowering(::Ice::GlobalContext *Ctx) {
  return ::Ice::X8632::TargetDataX86<::Ice::X8632::TargetX8632Traits>::create(
      Ctx);
}

std::unique_ptr<::Ice::TargetHeaderLowering>
createTargetHeaderLowering(::Ice::GlobalContext *Ctx) {
  return ::Ice::X8632::TargetHeaderX86::create(Ctx);
}

void staticInit(::Ice::GlobalContext *Ctx) {
  ::Ice::X8632::TargetX8632::staticInit(Ctx);
  if (Ice::getFlags().getUseNonsfi()) {
    // In nonsfi, we need to reference the _GLOBAL_OFFSET_TABLE_ for accessing
    // globals. The GOT is an external symbol (i.e., it is not defined in the
    // pexe) so we need to register it as such so that ELF emission won't barf
    // on an "unknown" symbol. The GOT is added to the External symbols list
    // here because staticInit() is invoked in a single-thread context.
    Ctx->getConstantExternSym(Ctx->getGlobalString(::Ice::GlobalOffsetTable));
  }
}

bool shouldBePooled(const class ::Ice::Constant *C) {
  return ::Ice::X8632::TargetX8632::shouldBePooled(C);
}

::Ice::Type getPointerType() {
  return ::Ice::X8632::TargetX8632::getPointerType();
}

} // end of namespace X8632

namespace Ice {
namespace X8632 {

//------------------------------------------------------------------------------
//      ______   ______     ______     __     ______   ______
//     /\__  _\ /\  == \   /\  __ \   /\ \   /\__  _\ /\  ___\
//     \/_/\ \/ \ \  __<   \ \  __ \  \ \ \  \/_/\ \/ \ \___  \
//        \ \_\  \ \_\ \_\  \ \_\ \_\  \ \_\    \ \_\  \/\_____\
//         \/_/   \/_/ /_/   \/_/\/_/   \/_/     \/_/   \/_____/
//
//------------------------------------------------------------------------------
const TargetX8632Traits::TableFcmpType TargetX8632Traits::TableFcmp[] = {
#define X(val, dflt, swapS, C1, C2, swapV, pred)                               \
  {                                                                            \
    dflt, swapS, X8632::Traits::Cond::C1, X8632::Traits::Cond::C2, swapV,      \
        X8632::Traits::Cond::pred                                              \
  }                                                                            \
  ,
    FCMPX8632_TABLE
#undef X
};

const size_t TargetX8632Traits::TableFcmpSize = llvm::array_lengthof(TableFcmp);

const TargetX8632Traits::TableIcmp32Type TargetX8632Traits::TableIcmp32[] = {
#define X(val, C_32, C1_64, C2_64, C3_64)                                      \
  { X8632::Traits::Cond::C_32 }                                                \
  ,
    ICMPX8632_TABLE
#undef X
};

const size_t TargetX8632Traits::TableIcmp32Size =
    llvm::array_lengthof(TableIcmp32);

const TargetX8632Traits::TableIcmp64Type TargetX8632Traits::TableIcmp64[] = {
#define X(val, C_32, C1_64, C2_64, C3_64)                                      \
  {                                                                            \
    X8632::Traits::Cond::C1_64, X8632::Traits::Cond::C2_64,                    \
        X8632::Traits::Cond::C3_64                                             \
  }                                                                            \
  ,
    ICMPX8632_TABLE
#undef X
};

const size_t TargetX8632Traits::TableIcmp64Size =
    llvm::array_lengthof(TableIcmp64);

const TargetX8632Traits::TableTypeX8632AttributesType
    TargetX8632Traits::TableTypeX8632Attributes[] = {
#define X(tag, elty, cvt, sdss, pdps, spsd, int_, unpack, pack, width, fld)    \
  { IceType_##elty }                                                           \
  ,
        ICETYPEX8632_TABLE
#undef X
};

const size_t TargetX8632Traits::TableTypeX8632AttributesSize =
    llvm::array_lengthof(TableTypeX8632Attributes);

#if defined(SUBZERO_USE_MICROSOFT_ABI)
// Windows 32-bit only guarantees 4 byte stack alignment
const uint32_t TargetX8632Traits::X86_STACK_ALIGNMENT_BYTES = 4;
#else
const uint32_t TargetX8632Traits::X86_STACK_ALIGNMENT_BYTES = 16;
#endif
const char *TargetX8632Traits::TargetName = "X8632";

template <>
std::array<SmallBitVector, RCX86_NUM>
    TargetX86Base<X8632::Traits>::TypeToRegisterSet = {{}};

template <>
std::array<SmallBitVector, RCX86_NUM>
    TargetX86Base<X8632::Traits>::TypeToRegisterSetUnfiltered = {{}};

template <>
std::array<SmallBitVector,
           TargetX86Base<X8632::Traits>::Traits::RegisterSet::Reg_NUM>
    TargetX86Base<X8632::Traits>::RegisterAliases = {{}};

template <>
FixupKind TargetX86Base<X8632::Traits>::PcRelFixup =
    TargetX86Base<X8632::Traits>::Traits::FK_PcRel;

template <>
FixupKind TargetX86Base<X8632::Traits>::AbsFixup =
    TargetX86Base<X8632::Traits>::Traits::FK_Abs;

//------------------------------------------------------------------------------
//     __      ______  __     __  ______  ______  __  __   __  ______
//    /\ \    /\  __ \/\ \  _ \ \/\  ___\/\  == \/\ \/\ "-.\ \/\  ___\
//    \ \ \___\ \ \/\ \ \ \/ ".\ \ \  __\\ \  __<\ \ \ \ \-.  \ \ \__ \
//     \ \_____\ \_____\ \__/".~\_\ \_____\ \_\ \_\ \_\ \_\\"\_\ \_____\
//      \/_____/\/_____/\/_/   \/_/\/_____/\/_/ /_/\/_/\/_/ \/_/\/_____/
//
//------------------------------------------------------------------------------
void TargetX8632::_add_sp(Operand *Adjustment) {
  Variable *esp = getPhysicalRegister(Traits::RegisterSet::Reg_esp);
  _add(esp, Adjustment);
}

void TargetX8632::_mov_sp(Operand *NewValue) {
  Variable *esp = getPhysicalRegister(Traits::RegisterSet::Reg_esp);
  _redefined(_mov(esp, NewValue));
}

Traits::X86OperandMem *TargetX8632::_sandbox_mem_reference(X86OperandMem *Mem) {
  switch (SandboxingType) {
  case ST_None:
  case ST_NaCl:
    return Mem;
  case ST_Nonsfi: {
    if (Mem->getIsRebased()) {
      return Mem;
    }
    // For Non-SFI mode, if the Offset field is a ConstantRelocatable, we
    // replace either Base or Index with a legalized RebasePtr. At emission
    // time, the ConstantRelocatable will be emitted with the @GOTOFF
    // relocation.
    if (llvm::dyn_cast_or_null<ConstantRelocatable>(Mem->getOffset()) ==
        nullptr) {
      return Mem;
    }
    Variable *T;
    uint16_t Shift = 0;
    if (Mem->getIndex() == nullptr) {
      T = Mem->getBase();
    } else if (Mem->getBase() == nullptr) {
      T = Mem->getIndex();
      Shift = Mem->getShift();
    } else {
      llvm::report_fatal_error(
          "Either Base or Index must be unused in Non-SFI mode");
    }
    Variable *RebasePtrR = legalizeToReg(RebasePtr);
    static constexpr bool IsRebased = true;
    return Traits::X86OperandMem::create(
        Func, Mem->getType(), RebasePtrR, Mem->getOffset(), T, Shift,
        Traits::X86OperandMem::DefaultSegment, IsRebased);
  }
  }
  llvm::report_fatal_error("Unhandled sandboxing type: " +
                           std::to_string(SandboxingType));
}

void TargetX8632::_sub_sp(Operand *Adjustment) {
  Variable *esp = getPhysicalRegister(Traits::RegisterSet::Reg_esp);
  _sub(esp, Adjustment);
  // Add a fake use of the stack pointer, to prevent the stack pointer adustment
  // from being dead-code eliminated in a function that doesn't return.
  Context.insert<InstFakeUse>(esp);
}

void TargetX8632::_link_bp() {
  Variable *ebp = getPhysicalRegister(Traits::RegisterSet::Reg_ebp);
  Variable *esp = getPhysicalRegister(Traits::RegisterSet::Reg_esp);
  _push(ebp);
  _mov(ebp, esp);
  // Keep ebp live for late-stage liveness analysis (e.g. asm-verbose mode).
  Context.insert<InstFakeUse>(ebp);
}

void TargetX8632::_unlink_bp() {
  Variable *esp = getPhysicalRegister(Traits::RegisterSet::Reg_esp);
  Variable *ebp = getPhysicalRegister(Traits::RegisterSet::Reg_ebp);
  // For late-stage liveness analysis (e.g. asm-verbose mode), adding a fake
  // use of esp before the assignment of esp=ebp keeps previous esp
  // adjustments from being dead-code eliminated.
  Context.insert<InstFakeUse>(esp);
  _mov(esp, ebp);
  _pop(ebp);
}

void TargetX8632::_push_reg(Variable *Reg) { _push(Reg); }

void TargetX8632::emitGetIP(CfgNode *Node) {
  // If there is a non-deleted InstX86GetIP instruction, we need to move it to
  // the point after the stack frame has stabilized but before
  // register-allocated in-args are copied into their home registers.  It would
  // be slightly faster to search for the GetIP instruction before other prolog
  // instructions are inserted, but it's more clear to do the whole
  // transformation in a single place.
  Traits::Insts::GetIP *GetIPInst = nullptr;
  if (getFlags().getUseNonsfi()) {
    for (Inst &Instr : Node->getInsts()) {
      if (auto *GetIP = llvm::dyn_cast<Traits::Insts::GetIP>(&Instr)) {
        if (!Instr.isDeleted())
          GetIPInst = GetIP;
        break;
      }
    }
  }
  // Delete any existing InstX86GetIP instruction and reinsert it here.  Also,
  // insert the call to the helper function and the spill to the stack, to
  // simplify emission.
  if (GetIPInst) {
    GetIPInst->setDeleted();
    Variable *Dest = GetIPInst->getDest();
    Variable *CallDest =
        Dest->hasReg() ? Dest
                       : getPhysicalRegister(Traits::RegisterSet::Reg_eax);
    auto *BeforeAddReloc = RelocOffset::create(Ctx);
    BeforeAddReloc->setSubtract(true);
    auto *BeforeAdd = InstX86Label::create(Func, this);
    BeforeAdd->setRelocOffset(BeforeAddReloc);

    auto *AfterAddReloc = RelocOffset::create(Ctx);
    auto *AfterAdd = InstX86Label::create(Func, this);
    AfterAdd->setRelocOffset(AfterAddReloc);

    const RelocOffsetT ImmSize = -typeWidthInBytes(IceType_i32);

    auto *GotFromPc =
        llvm::cast<ConstantRelocatable>(Ctx->getConstantSymWithEmitString(
            ImmSize, {AfterAddReloc, BeforeAddReloc},
            Ctx->getGlobalString(GlobalOffsetTable), GlobalOffsetTable));

    // Insert a new version of InstX86GetIP.
    Context.insert<Traits::Insts::GetIP>(CallDest);

    Context.insert(BeforeAdd);
    _add(CallDest, GotFromPc);
    Context.insert(AfterAdd);

    // Spill the register to its home stack location if necessary.
    if (Dest != CallDest) {
      _mov(Dest, CallDest);
    }
  }
}

void TargetX8632::lowerIndirectJump(Variable *JumpTarget) {
  AutoBundle _(this);

  if (NeedSandboxing) {
    const SizeT BundleSize =
        1 << Func->getAssembler<>()->getBundleAlignLog2Bytes();
    _and(JumpTarget, Ctx->getConstantInt32(~(BundleSize - 1)));
  }

  _jmp(JumpTarget);
}

void TargetX8632::initRebasePtr() {
  if (SandboxingType == ST_Nonsfi) {
    RebasePtr = Func->makeVariable(IceType_i32);
  }
}

void TargetX8632::initSandbox() {
  if (SandboxingType != ST_Nonsfi) {
    return;
  }
  // Insert the RebasePtr assignment as the very first lowered instruction.
  // Later, it will be moved into the right place - after the stack frame is set
  // up but before in-args are copied into registers.
  Context.init(Func->getEntryNode());
  Context.setInsertPoint(Context.getCur());
  Context.insert<Traits::Insts::GetIP>(RebasePtr);
}

bool TargetX8632::legalizeOptAddrForSandbox(OptAddr *Addr) {
  if (Addr->Relocatable == nullptr || SandboxingType != ST_Nonsfi) {
    return true;
  }

  if (Addr->Base == RebasePtr || Addr->Index == RebasePtr) {
    return true;
  }

  if (Addr->Base == nullptr) {
    Addr->Base = RebasePtr;
    return true;
  }

  if (Addr->Index == nullptr) {
    Addr->Index = RebasePtr;
    Addr->Shift = 0;
    return true;
  }

  return false;
}

Inst *TargetX8632::emitCallToTarget(Operand *CallTarget, Variable *ReturnReg) {
  std::unique_ptr<AutoBundle> Bundle;
  if (NeedSandboxing) {
    if (llvm::isa<Constant>(CallTarget)) {
      Bundle = makeUnique<AutoBundle>(this, InstBundleLock::Opt_AlignToEnd);
    } else {
      Variable *CallTargetVar = nullptr;
      _mov(CallTargetVar, CallTarget);
      Bundle = makeUnique<AutoBundle>(this, InstBundleLock::Opt_AlignToEnd);
      const SizeT BundleSize =
          1 << Func->getAssembler<>()->getBundleAlignLog2Bytes();
      _and(CallTargetVar, Ctx->getConstantInt32(~(BundleSize - 1)));
      CallTarget = CallTargetVar;
    }
  }
  return Context.insert<Traits::Insts::Call>(ReturnReg, CallTarget);
}

Variable *TargetX8632::moveReturnValueToRegister(Operand *Value,
                                                 Type ReturnType) {
  if (isVectorType(ReturnType)) {
    return legalizeToReg(Value, Traits::RegisterSet::Reg_xmm0);
  } else if (isScalarFloatingType(ReturnType)) {
    _fld(Value);
    return nullptr;
  } else {
    assert(ReturnType == IceType_i32 || ReturnType == IceType_i64);
    if (ReturnType == IceType_i64) {
      Variable *eax =
          legalizeToReg(loOperand(Value), Traits::RegisterSet::Reg_eax);
      Variable *edx =
          legalizeToReg(hiOperand(Value), Traits::RegisterSet::Reg_edx);
      Context.insert<InstFakeUse>(edx);
      return eax;
    } else {
      Variable *Reg = nullptr;
      _mov(Reg, Value, Traits::RegisterSet::Reg_eax);
      return Reg;
    }
  }
}

void TargetX8632::emitSandboxedReturn() {
  // Change the original ret instruction into a sandboxed return sequence.
  // t:ecx = pop
  // bundle_lock
  // and t, ~31
  // jmp *t
  // bundle_unlock
  // FakeUse <original_ret_operand>
  Variable *T_ecx = makeReg(IceType_i32, Traits::RegisterSet::Reg_ecx);
  _pop(T_ecx);
  lowerIndirectJump(T_ecx);
}

// In some cases, there are x-macros tables for both high-level and low-level
// instructions/operands that use the same enum key value. The tables are kept
// separate to maintain a proper separation between abstraction layers. There
// is a risk that the tables could get out of sync if enum values are reordered
// or if entries are added or deleted. The following dummy namespaces use
// static_asserts to ensure everything is kept in sync.

namespace {
// Validate the enum values in FCMPX8632_TABLE.
namespace dummy1 {
// Define a temporary set of enum values based on low-level table entries.
enum _tmp_enum {
#define X(val, dflt, swapS, C1, C2, swapV, pred) _tmp_##val,
  FCMPX8632_TABLE
#undef X
      _num
};
// Define a set of constants based on high-level table entries.
#define X(tag, str) static const int _table1_##tag = InstFcmp::tag;
ICEINSTFCMP_TABLE
#undef X
// Define a set of constants based on low-level table entries, and ensure the
// table entry keys are consistent.
#define X(val, dflt, swapS, C1, C2, swapV, pred)                               \
  static const int _table2_##val = _tmp_##val;                                 \
  static_assert(                                                               \
      _table1_##val == _table2_##val,                                          \
      "Inconsistency between FCMPX8632_TABLE and ICEINSTFCMP_TABLE");
FCMPX8632_TABLE
#undef X
// Repeat the static asserts with respect to the high-level table entries in
// case the high-level table has extra entries.
#define X(tag, str)                                                            \
  static_assert(                                                               \
      _table1_##tag == _table2_##tag,                                          \
      "Inconsistency between FCMPX8632_TABLE and ICEINSTFCMP_TABLE");
ICEINSTFCMP_TABLE
#undef X
} // end of namespace dummy1

// Validate the enum values in ICMPX8632_TABLE.
namespace dummy2 {
// Define a temporary set of enum values based on low-level table entries.
enum _tmp_enum {
#define X(val, C_32, C1_64, C2_64, C3_64) _tmp_##val,
  ICMPX8632_TABLE
#undef X
      _num
};
// Define a set of constants based on high-level table entries.
#define X(tag, reverse, str) static const int _table1_##tag = InstIcmp::tag;
ICEINSTICMP_TABLE
#undef X
// Define a set of constants based on low-level table entries, and ensure the
// table entry keys are consistent.
#define X(val, C_32, C1_64, C2_64, C3_64)                                      \
  static const int _table2_##val = _tmp_##val;                                 \
  static_assert(                                                               \
      _table1_##val == _table2_##val,                                          \
      "Inconsistency between ICMPX8632_TABLE and ICEINSTICMP_TABLE");
ICMPX8632_TABLE
#undef X
// Repeat the static asserts with respect to the high-level table entries in
// case the high-level table has extra entries.
#define X(tag, reverse, str)                                                   \
  static_assert(                                                               \
      _table1_##tag == _table2_##tag,                                          \
      "Inconsistency between ICMPX8632_TABLE and ICEINSTICMP_TABLE");
ICEINSTICMP_TABLE
#undef X
} // end of namespace dummy2

// Validate the enum values in ICETYPEX8632_TABLE.
namespace dummy3 {
// Define a temporary set of enum values based on low-level table entries.
enum _tmp_enum {
#define X(tag, elty, cvt, sdss, pdps, spsd, int_, unpack, pack, width, fld)    \
  _tmp_##tag,
  ICETYPEX8632_TABLE
#undef X
      _num
};
// Define a set of constants based on high-level table entries.
#define X(tag, sizeLog2, align, elts, elty, str, rcstr)                        \
  static const int _table1_##tag = IceType_##tag;
ICETYPE_TABLE
#undef X
// Define a set of constants based on low-level table entries, and ensure the
// table entry keys are consistent.
#define X(tag, elty, cvt, sdss, pdps, spsd, int_, unpack, pack, width, fld)    \
  static const int _table2_##tag = _tmp_##tag;                                 \
  static_assert(_table1_##tag == _table2_##tag,                                \
                "Inconsistency between ICETYPEX8632_TABLE and ICETYPE_TABLE");
ICETYPEX8632_TABLE
#undef X
// Repeat the static asserts with respect to the high-level table entries in
// case the high-level table has extra entries.
#define X(tag, sizeLog2, align, elts, elty, str, rcstr)                        \
  static_assert(_table1_##tag == _table2_##tag,                                \
                "Inconsistency between ICETYPEX8632_TABLE and ICETYPE_TABLE");
ICETYPE_TABLE
#undef X
} // end of namespace dummy3
} // end of anonymous namespace

} // end of namespace X8632
} // end of namespace Ice
