//===- subzero/src/IceTargetLoweringX8664.cpp - x86-64 lowering -----------===//
//
//                        The Subzero Code Generator
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// \brief Implements the TargetLoweringX8664 class, which consists almost
/// entirely of the lowering sequence for each high-level instruction.
///
//===----------------------------------------------------------------------===//
#include "IceTargetLoweringX8664.h"

#include "IceDefs.h"
#include "IceTargetLoweringX8664Traits.h"

namespace X8664 {
std::unique_ptr<::Ice::TargetLowering> createTargetLowering(::Ice::Cfg *Func) {
  return ::Ice::X8664::TargetX8664::create(Func);
}

std::unique_ptr<::Ice::TargetDataLowering>
createTargetDataLowering(::Ice::GlobalContext *Ctx) {
  return ::Ice::X8664::TargetDataX86<::Ice::X8664::TargetX8664Traits>::create(
      Ctx);
}

std::unique_ptr<::Ice::TargetHeaderLowering>
createTargetHeaderLowering(::Ice::GlobalContext *Ctx) {
  return ::Ice::X8664::TargetHeaderX86::create(Ctx);
}

void staticInit(::Ice::GlobalContext *Ctx) {
  ::Ice::X8664::TargetX8664::staticInit(Ctx);
}

bool shouldBePooled(const class ::Ice::Constant *C) {
  return ::Ice::X8664::TargetX8664::shouldBePooled(C);
}

::Ice::Type getPointerType() {
  return ::Ice::X8664::TargetX8664::getPointerType();
}

} // end of namespace X8664

namespace Ice {
namespace X8664 {

//------------------------------------------------------------------------------
//      ______   ______     ______     __     ______   ______
//     /\__  _\ /\  == \   /\  __ \   /\ \   /\__  _\ /\  ___\
//     \/_/\ \/ \ \  __<   \ \  __ \  \ \ \  \/_/\ \/ \ \___  \
//        \ \_\  \ \_\ \_\  \ \_\ \_\  \ \_\    \ \_\  \/\_____\
//         \/_/   \/_/ /_/   \/_/\/_/   \/_/     \/_/   \/_____/
//
//------------------------------------------------------------------------------
const TargetX8664Traits::TableFcmpType TargetX8664Traits::TableFcmp[] = {
#define X(val, dflt, swapS, C1, C2, swapV, pred)                               \
  {                                                                            \
    dflt, swapS, X8664::Traits::Cond::C1, X8664::Traits::Cond::C2, swapV,      \
        X8664::Traits::Cond::pred                                              \
  }                                                                            \
  ,
    FCMPX8664_TABLE
#undef X
};

const size_t TargetX8664Traits::TableFcmpSize = llvm::array_lengthof(TableFcmp);

const TargetX8664Traits::TableIcmp32Type TargetX8664Traits::TableIcmp32[] = {
#define X(val, C_32, C1_64, C2_64, C3_64)                                      \
  { X8664::Traits::Cond::C_32 }                                                \
  ,
    ICMPX8664_TABLE
#undef X
};

const size_t TargetX8664Traits::TableIcmp32Size =
    llvm::array_lengthof(TableIcmp32);

const TargetX8664Traits::TableIcmp64Type TargetX8664Traits::TableIcmp64[] = {
#define X(val, C_32, C1_64, C2_64, C3_64)                                      \
  {                                                                            \
    X8664::Traits::Cond::C1_64, X8664::Traits::Cond::C2_64,                    \
        X8664::Traits::Cond::C3_64                                             \
  }                                                                            \
  ,
    ICMPX8664_TABLE
#undef X
};

const size_t TargetX8664Traits::TableIcmp64Size =
    llvm::array_lengthof(TableIcmp64);

const TargetX8664Traits::TableTypeX8664AttributesType
    TargetX8664Traits::TableTypeX8664Attributes[] = {
#define X(tag, elty, cvt, sdss, pdps, spsd, int_, unpack, pack, width, fld)    \
  { IceType_##elty }                                                           \
  ,
        ICETYPEX8664_TABLE
#undef X
};

const size_t TargetX8664Traits::TableTypeX8664AttributesSize =
    llvm::array_lengthof(TableTypeX8664Attributes);

const uint32_t TargetX8664Traits::X86_STACK_ALIGNMENT_BYTES = 16;
const char *TargetX8664Traits::TargetName = "X8664";

template <>
std::array<SmallBitVector, RCX86_NUM>
    TargetX86Base<X8664::Traits>::TypeToRegisterSet = {{}};

template <>
std::array<SmallBitVector, RCX86_NUM>
    TargetX86Base<X8664::Traits>::TypeToRegisterSetUnfiltered = {{}};

template <>
std::array<SmallBitVector,
           TargetX86Base<X8664::Traits>::Traits::RegisterSet::Reg_NUM>
    TargetX86Base<X8664::Traits>::RegisterAliases = {{}};

template <>
FixupKind TargetX86Base<X8664::Traits>::PcRelFixup =
    TargetX86Base<X8664::Traits>::Traits::FK_PcRel;

template <>
FixupKind TargetX86Base<X8664::Traits>::AbsFixup =
    TargetX86Base<X8664::Traits>::Traits::FK_Abs;

//------------------------------------------------------------------------------
//     __      ______  __     __  ______  ______  __  __   __  ______
//    /\ \    /\  __ \/\ \  _ \ \/\  ___\/\  == \/\ \/\ "-.\ \/\  ___\
//    \ \ \___\ \ \/\ \ \ \/ ".\ \ \  __\\ \  __<\ \ \ \ \-.  \ \ \__ \
//     \ \_____\ \_____\ \__/".~\_\ \_____\ \_\ \_\ \_\ \_\\"\_\ \_____\
//      \/_____/\/_____/\/_/   \/_/\/_____/\/_/ /_/\/_/\/_/ \/_/\/_____/
//
//------------------------------------------------------------------------------
void TargetX8664::_add_sp(Operand *Adjustment) {
  Variable *rsp =
      getPhysicalRegister(Traits::RegisterSet::Reg_rsp, IceType_i64);
  if (!NeedSandboxing) {
    _add(rsp, Adjustment);
    return;
  }

  Variable *esp =
      getPhysicalRegister(Traits::RegisterSet::Reg_esp, IceType_i32);
  Variable *r15 =
      getPhysicalRegister(Traits::RegisterSet::Reg_r15, IceType_i64);

  // When incrementing rsp, NaCl sandboxing requires the following sequence
  //
  // .bundle_start
  // add Adjustment, %esp
  // add %r15, %rsp
  // .bundle_end
  //
  // In Subzero, even though rsp and esp alias each other, defining one does not
  // define the other. Therefore, we must emit
  //
  // .bundle_start
  // %esp = fake-def %rsp
  // add Adjustment, %esp
  // %rsp = fake-def %esp
  // add %r15, %rsp
  // .bundle_end
  //
  // The fake-defs ensure that the
  //
  // add Adjustment, %esp
  //
  // instruction is not DCE'd.
  AutoBundle _(this);
  _redefined(Context.insert<InstFakeDef>(esp, rsp));
  _add(esp, Adjustment);
  _redefined(Context.insert<InstFakeDef>(rsp, esp));
  _add(rsp, r15);
}

void TargetX8664::_mov_sp(Operand *NewValue) {
  assert(NewValue->getType() == IceType_i32);

  Variable *esp = getPhysicalRegister(Traits::RegisterSet::Reg_esp);
  Variable *rsp =
      getPhysicalRegister(Traits::RegisterSet::Reg_rsp, IceType_i64);

  AutoBundle _(this);

  _redefined(Context.insert<InstFakeDef>(esp, rsp));
  _redefined(_mov(esp, NewValue));
  _redefined(Context.insert<InstFakeDef>(rsp, esp));

  if (!NeedSandboxing) {
    return;
  }

  Variable *r15 =
      getPhysicalRegister(Traits::RegisterSet::Reg_r15, IceType_i64);
  _add(rsp, r15);
}

void TargetX8664::_push_rbp() {
  assert(NeedSandboxing);

  Constant *_0 = Ctx->getConstantZero(IceType_i32);
  Variable *ebp =
      getPhysicalRegister(Traits::RegisterSet::Reg_ebp, IceType_i32);
  Variable *rsp =
      getPhysicalRegister(Traits::RegisterSet::Reg_rsp, IceType_i64);
  auto *TopOfStack = llvm::cast<X86OperandMem>(
      legalize(X86OperandMem::create(Func, IceType_i32, rsp, _0),
               Legal_Reg | Legal_Mem));

  // Emits a sequence:
  //
  //   .bundle_start
  //   push 0
  //   mov %ebp, %(rsp)
  //   .bundle_end
  //
  // to avoid leaking the upper 32-bits (i.e., the sandbox address.)
  AutoBundle _(this);
  _push(_0);
  Context.insert<typename Traits::Insts::Store>(ebp, TopOfStack);
}

void TargetX8664::_link_bp() {
  Variable *esp =
      getPhysicalRegister(Traits::RegisterSet::Reg_esp, IceType_i32);
  Variable *rsp =
      getPhysicalRegister(Traits::RegisterSet::Reg_rsp, Traits::WordType);
  Variable *ebp =
      getPhysicalRegister(Traits::RegisterSet::Reg_ebp, IceType_i32);
  Variable *rbp =
      getPhysicalRegister(Traits::RegisterSet::Reg_rbp, Traits::WordType);
  Variable *r15 =
      getPhysicalRegister(Traits::RegisterSet::Reg_r15, Traits::WordType);

  if (!NeedSandboxing) {
    _push(rbp);
    _mov(rbp, rsp);
  } else {
    _push_rbp();

    AutoBundle _(this);
    _redefined(Context.insert<InstFakeDef>(ebp, rbp));
    _redefined(Context.insert<InstFakeDef>(esp, rsp));
    _mov(ebp, esp);
    _redefined(Context.insert<InstFakeDef>(rsp, esp));
    _add(rbp, r15);
  }
  // Keep ebp live for late-stage liveness analysis (e.g. asm-verbose mode).
  Context.insert<InstFakeUse>(rbp);
}

void TargetX8664::_unlink_bp() {
  Variable *rsp =
      getPhysicalRegister(Traits::RegisterSet::Reg_rsp, IceType_i64);
  Variable *rbp =
      getPhysicalRegister(Traits::RegisterSet::Reg_rbp, IceType_i64);
  Variable *ebp =
      getPhysicalRegister(Traits::RegisterSet::Reg_ebp, IceType_i32);
  // For late-stage liveness analysis (e.g. asm-verbose mode), adding a fake
  // use of rsp before the assignment of rsp=rbp keeps previous rsp
  // adjustments from being dead-code eliminated.
  Context.insert<InstFakeUse>(rsp);
  if (!NeedSandboxing) {
    _mov(rsp, rbp);
    _pop(rbp);
  } else {
    _mov_sp(ebp);

    Variable *r15 =
        getPhysicalRegister(Traits::RegisterSet::Reg_r15, IceType_i64);
    Variable *rcx =
        getPhysicalRegister(Traits::RegisterSet::Reg_rcx, IceType_i64);
    Variable *ecx =
        getPhysicalRegister(Traits::RegisterSet::Reg_ecx, IceType_i32);

    _pop(rcx);
    Context.insert<InstFakeDef>(ecx, rcx);
    AutoBundle _(this);
    _mov(ebp, ecx);

    _redefined(Context.insert<InstFakeDef>(rbp, ebp));
    _add(rbp, r15);
  }
}

void TargetX8664::_push_reg(Variable *Reg) {
  Variable *rbp =
      getPhysicalRegister(Traits::RegisterSet::Reg_rbp, Traits::WordType);
  if (Reg != rbp || !NeedSandboxing) {
    _push(Reg);
  } else {
    _push_rbp();
  }
}

void TargetX8664::emitGetIP(CfgNode *Node) {
  // No IP base register is needed on X86-64.
  (void)Node;
}

namespace {
bool isAssignedToRspOrRbp(const Variable *Var) {
  if (Var == nullptr) {
    return false;
  }

  if (Var->isRematerializable()) {
    return true;
  }

  if (!Var->hasReg()) {
    return false;
  }

  const auto RegNum = Var->getRegNum();
  if ((RegNum == Traits::RegisterSet::Reg_rsp) ||
      (RegNum == Traits::RegisterSet::Reg_rbp)) {
    return true;
  }

  return false;
}
} // end of anonymous namespace

Traits::X86OperandMem *TargetX8664::_sandbox_mem_reference(X86OperandMem *Mem) {
  if (SandboxingType == ST_None) {
    return Mem;
  }

  if (SandboxingType == ST_Nonsfi) {
    llvm::report_fatal_error(
        "_sandbox_mem_reference not implemented for nonsfi");
  }

  // In x86_64-nacl, all memory references are relative to a base register
  // (%r15, %rsp, %rbp, or %rip).

  Variable *Base = Mem->getBase();
  Variable *Index = Mem->getIndex();
  uint16_t Shift = 0;
  Variable *ZeroReg = RebasePtr;
  Constant *Offset = Mem->getOffset();
  Variable *T = nullptr;

  bool AbsoluteAddress = false;
  if (Base == nullptr && Index == nullptr) {
    if (llvm::isa<ConstantRelocatable>(Offset)) {
      // Mem is RIP-relative. There's no need to rebase it.
      return Mem;
    }
    // Offset is an absolute address, so we need to emit
    //   Offset(%r15)
    AbsoluteAddress = true;
  }

  if (Mem->getIsRebased()) {
    // If Mem.IsRebased, then we don't need to update Mem, as it's already been
    // updated to contain a reference to one of %rsp, %rbp, or %r15.
    // We don't return early because we still need to zero extend Index.
    assert(ZeroReg == Base || AbsoluteAddress || isAssignedToRspOrRbp(Base));
    if (!AbsoluteAddress) {
      // If Mem is an absolute address, no need to update ZeroReg (which is
      // already set to %r15.)
      ZeroReg = Base;
    }
    if (Index != nullptr) {
      T = makeReg(IceType_i32);
      _mov(T, Index);
      Shift = Mem->getShift();
    }
  } else {
    if (Base != nullptr) {
      // If Base is a valid base pointer we don't need to use the RebasePtr. By
      // doing this we might save us the need to zero extend the memory operand.
      if (isAssignedToRspOrRbp(Base)) {
        ZeroReg = Base;
      } else {
        T = Base;
      }
    }

    if (Index != nullptr) {
      assert(!Index->isRematerializable());
      // If Index is not nullptr, it is mandatory that T is a nullptr.
      // Otherwise, the lowering generated a memory operand with two registers.
      // Note that Base might still be non-nullptr, but it must be a valid
      // base register.
      if (T != nullptr) {
        llvm::report_fatal_error("memory reference contains base and index.");
      }
      // If the Index is not shifted, and it is a Valid Base, and the ZeroReg is
      // still RebasePtr, then we do ZeroReg = Index, and hopefully prevent the
      // need to zero-extend the memory operand (which may still happen -- see
      // NeedLea below.)
      if (Shift == 0 && isAssignedToRspOrRbp(Index) && ZeroReg == RebasePtr) {
        ZeroReg = Index;
      } else {
        T = Index;
        Shift = Mem->getShift();
      }
    }
  }

  // NeedsLea is a flag indicating whether Mem needs to be materialized to a GPR
  // prior to being used. A LEA is needed if Mem.Offset is a constant
  // relocatable with a nonzero offset, or if Mem.Offset is a nonzero immediate;
  // but only when the address mode contains a "user" register other than the
  // rsp/rbp/r15 base. In both these cases, the LEA is needed to ensure the
  // sandboxed memory operand will only use the lower 32-bits of T+Offset.
  bool NeedsLea = false;
  if (!Mem->getIsRebased()) {
    bool IsOffsetZero = false;
    if (Offset == nullptr) {
      IsOffsetZero = true;
    } else if (const auto *CR = llvm::dyn_cast<ConstantRelocatable>(Offset)) {
      IsOffsetZero = (CR->getOffset() == 0);
    } else if (const auto *Imm = llvm::dyn_cast<ConstantInteger32>(Offset)) {
      IsOffsetZero = (Imm->getValue() == 0);
    } else {
      llvm::report_fatal_error("Unexpected Offset type.");
    }
    if (!IsOffsetZero) {
      if (Base != nullptr && Base != ZeroReg)
        NeedsLea = true;
      if (Index != nullptr && Index != ZeroReg)
        NeedsLea = true;
    }
  }

  RegNumT RegNum, RegNum32;
  if (T != nullptr) {
    if (T->hasReg()) {
      RegNum = Traits::getGprForType(IceType_i64, T->getRegNum());
      RegNum32 = Traits::getGprForType(IceType_i32, RegNum);
      // At this point, if T was assigned to rsp/rbp, then we would have already
      // made this the ZeroReg.
      assert(RegNum != Traits::RegisterSet::Reg_rsp);
      assert(RegNum != Traits::RegisterSet::Reg_rbp);
    }

    switch (T->getType()) {
    default:
      llvm::report_fatal_error("Mem pointer should be a 32-bit GPR.");
    case IceType_i64:
      // Even though "default:" would also catch T.Type == IceType_i64, an
      // explicit 'case IceType_i64' shows that memory operands are always
      // supposed to be 32-bits.
      llvm::report_fatal_error("Mem pointer should not be a 64-bit GPR.");
    case IceType_i32: {
      Variable *T64 = makeReg(IceType_i64, RegNum);
      auto *Movzx = _movzx(T64, T);
      if (!NeedsLea) {
        // This movzx is only needed when Mem does not need to be lea'd into a
        // temporary. If an lea is going to be emitted, then eliding this movzx
        // is safe because the emitted lea will write a 32-bit result --
        // implicitly zero-extended to 64-bit.
        Movzx->setMustKeep();
      }
      T = T64;
    } break;
    }
  }

  if (NeedsLea) {
    Variable *NewT = makeReg(IceType_i32, RegNum32);
    Variable *Base = T;
    Variable *Index = T;
    static constexpr bool NotRebased = false;
    if (Shift == 0) {
      Index = nullptr;
    } else {
      Base = nullptr;
    }
    _lea(NewT, Traits::X86OperandMem::create(
                   Func, Mem->getType(), Base, Offset, Index, Shift,
                   Traits::X86OperandMem::DefaultSegment, NotRebased));

    T = makeReg(IceType_i64, RegNum);
    _movzx(T, NewT);
    Shift = 0;
    Offset = nullptr;
  }

  static constexpr bool IsRebased = true;
  return Traits::X86OperandMem::create(
      Func, Mem->getType(), ZeroReg, Offset, T, Shift,
      Traits::X86OperandMem::DefaultSegment, IsRebased);
}

void TargetX8664::_sub_sp(Operand *Adjustment) {
  Variable *rsp =
      getPhysicalRegister(Traits::RegisterSet::Reg_rsp, Traits::WordType);

  if (NeedSandboxing) {
    Variable *esp =
        getPhysicalRegister(Traits::RegisterSet::Reg_esp, IceType_i32);
    Variable *r15 =
        getPhysicalRegister(Traits::RegisterSet::Reg_r15, IceType_i64);

    // .bundle_start
    // sub Adjustment, %esp
    // add %r15, %rsp
    // .bundle_end
    AutoBundle _(this);
    _redefined(Context.insert<InstFakeDef>(esp, rsp));
    _sub(esp, Adjustment);
    _redefined(Context.insert<InstFakeDef>(rsp, esp));
    _add(rsp, r15);
  } else {
    _sub(rsp, Adjustment);
  }

  // Add a fake use of the stack pointer, to prevent the stack pointer adustment
  // from being dead-code eliminated in a function that doesn't return.
  Context.insert<InstFakeUse>(rsp);
}

void TargetX8664::initRebasePtr() {
  switch (SandboxingType) {
  case ST_Nonsfi:
    // Probably no implementation is needed, but error to be safe for now.
    llvm::report_fatal_error(
        "initRebasePtr() is not yet implemented on x32-nonsfi.");
  case ST_NaCl:
    RebasePtr = getPhysicalRegister(Traits::RegisterSet::Reg_r15, IceType_i64);
    break;
  case ST_None:
    // nothing.
    break;
  }
}

void TargetX8664::initSandbox() {
  assert(SandboxingType == ST_NaCl);
  Context.init(Func->getEntryNode());
  Context.setInsertPoint(Context.getCur());
  Variable *r15 =
      getPhysicalRegister(Traits::RegisterSet::Reg_r15, IceType_i64);
  Context.insert<InstFakeDef>(r15);
  Context.insert<InstFakeUse>(r15);
}

namespace {
bool isRematerializable(const Variable *Var) {
  return Var != nullptr && Var->isRematerializable();
}
} // end of anonymous namespace

bool TargetX8664::legalizeOptAddrForSandbox(OptAddr *Addr) {
  if (SandboxingType == ST_Nonsfi) {
    llvm::report_fatal_error("Nonsfi not yet implemented for x8664.");
  }

  if (isRematerializable(Addr->Base)) {
    if (Addr->Index == RebasePtr) {
      Addr->Index = nullptr;
      Addr->Shift = 0;
    }
    return true;
  }

  if (isRematerializable(Addr->Index)) {
    if (Addr->Base == RebasePtr) {
      Addr->Base = nullptr;
    }
    return true;
  }

  assert(Addr->Base != RebasePtr && Addr->Index != RebasePtr);

  if (Addr->Base == nullptr) {
    return true;
  }

  if (Addr->Index == nullptr) {
    return true;
  }

  return false;
}

void TargetX8664::lowerIndirectJump(Variable *JumpTarget) {
  std::unique_ptr<AutoBundle> Bundler;

  if (!NeedSandboxing) {
    if (JumpTarget->getType() != IceType_i64) {
      Variable *T = makeReg(IceType_i64);
      _movzx(T, JumpTarget);
      JumpTarget = T;
    }
  } else {
    Variable *T = makeReg(IceType_i32);
    Variable *T64 = makeReg(IceType_i64);
    Variable *r15 =
        getPhysicalRegister(Traits::RegisterSet::Reg_r15, IceType_i64);

    _mov(T, JumpTarget);
    Bundler = makeUnique<AutoBundle>(this);
    const SizeT BundleSize =
        1 << Func->getAssembler<>()->getBundleAlignLog2Bytes();
    _and(T, Ctx->getConstantInt32(~(BundleSize - 1)));
    _movzx(T64, T);
    _add(T64, r15);
    JumpTarget = T64;
  }

  _jmp(JumpTarget);
}

Inst *TargetX8664::emitCallToTarget(Operand *CallTarget, Variable *ReturnReg) {
  Inst *NewCall = nullptr;
  auto *CallTargetR = llvm::dyn_cast<Variable>(CallTarget);
  if (NeedSandboxing) {
    // In NaCl sandbox, calls are replaced by a push/jmp pair:
    //
    //     push .after_call
    //     jmp CallTarget
    //     .align bundle_size
    // after_call:
    //
    // In order to emit this sequence, we need a temporary label ("after_call",
    // in this example.)
    //
    // The operand to push is a ConstantRelocatable. The easy way to implement
    // this sequence is to create a ConstantRelocatable(0, "after_call"), but
    // this ends up creating more relocations for the linker to resolve.
    // Therefore, we create a ConstantRelocatable from the name of the function
    // being compiled (i.e., ConstantRelocatable(after_call - Func, Func).
    //
    // By default, ConstantRelocatables are emitted (in textual output) as
    //
    //  ConstantName + Offset
    //
    // ReturnReloc has an offset that is only known during binary emission.
    // Therefore, we set a custom emit string for ReturnReloc that will be
    // used instead. In this particular case, the code will be emitted as
    //
    //  push .after_call
    InstX86Label *ReturnAddress = InstX86Label::create(Func, this);
    auto *ReturnRelocOffset = RelocOffset::create(Func->getAssembler());
    ReturnAddress->setRelocOffset(ReturnRelocOffset);
    constexpr RelocOffsetT NoFixedOffset = 0;
    const std::string EmitString =
        BuildDefs::dump() ? ReturnAddress->getLabelName().toString() : "";
    auto *ReturnReloc = ConstantRelocatable::create(
        Func->getAssembler(), IceType_i32,
        RelocatableTuple(NoFixedOffset, {ReturnRelocOffset},
                         Func->getFunctionName(), EmitString));
    /* AutoBundle scoping */ {
      std::unique_ptr<AutoBundle> Bundler;
      if (CallTargetR == nullptr) {
        Bundler = makeUnique<AutoBundle>(this, InstBundleLock::Opt_PadToEnd);
        _push(ReturnReloc);
      } else {
        Variable *T = makeReg(IceType_i32);
        Variable *T64 = makeReg(IceType_i64);
        Variable *r15 =
            getPhysicalRegister(Traits::RegisterSet::Reg_r15, IceType_i64);

        _mov(T, CallTargetR);
        Bundler = makeUnique<AutoBundle>(this, InstBundleLock::Opt_PadToEnd);
        _push(ReturnReloc);
        const SizeT BundleSize =
            1 << Func->getAssembler<>()->getBundleAlignLog2Bytes();
        _and(T, Ctx->getConstantInt32(~(BundleSize - 1)));
        _movzx(T64, T);
        _add(T64, r15);
        CallTarget = T64;
      }

      NewCall = Context.insert<Traits::Insts::Jmp>(CallTarget);
    }
    if (ReturnReg != nullptr) {
      Context.insert<InstFakeDef>(ReturnReg);
    }

    Context.insert(ReturnAddress);
  } else {
    if (CallTargetR != nullptr) {
      // x86-64 in Subzero is ILP32. Therefore, CallTarget is i32, but the
      // emitted call needs a i64 register (for textual asm.)
      Variable *T = makeReg(IceType_i64);
      _movzx(T, CallTargetR);
      CallTarget = T;
    }
    NewCall = Context.insert<Traits::Insts::Call>(ReturnReg, CallTarget);
  }
  return NewCall;
}

Variable *TargetX8664::moveReturnValueToRegister(Operand *Value,
                                                 Type ReturnType) {
  if (isVectorType(ReturnType) || isScalarFloatingType(ReturnType)) {
    return legalizeToReg(Value, Traits::RegisterSet::Reg_xmm0);
  } else {
    assert(ReturnType == IceType_i32 || ReturnType == IceType_i64);
    Variable *Reg = nullptr;
    _mov(Reg, Value,
         Traits::getGprForType(ReturnType, Traits::RegisterSet::Reg_rax));
    return Reg;
  }
}

void TargetX8664::emitSandboxedReturn() {
  Variable *T_rcx = makeReg(IceType_i64, Traits::RegisterSet::Reg_rcx);
  Variable *T_ecx = makeReg(IceType_i32, Traits::RegisterSet::Reg_ecx);
  _pop(T_rcx);
  _mov(T_ecx, T_rcx);
  // lowerIndirectJump(T_ecx);
  Variable *r15 =
      getPhysicalRegister(Traits::RegisterSet::Reg_r15, IceType_i64);

  /* AutoBundle scoping */ {
    AutoBundle _(this);
    const SizeT BundleSize =
        1 << Func->getAssembler<>()->getBundleAlignLog2Bytes();
    _and(T_ecx, Ctx->getConstantInt32(~(BundleSize - 1)));
    Context.insert<InstFakeDef>(T_rcx, T_ecx);
    _add(T_rcx, r15);

    _jmp(T_rcx);
  }
}

// In some cases, there are x-macros tables for both high-level and low-level
// instructions/operands that use the same enum key value. The tables are kept
// separate to maintain a proper separation between abstraction layers. There
// is a risk that the tables could get out of sync if enum values are reordered
// or if entries are added or deleted. The following dummy namespaces use
// static_asserts to ensure everything is kept in sync.

namespace {
// Validate the enum values in FCMPX8664_TABLE.
namespace dummy1 {
// Define a temporary set of enum values based on low-level table entries.
enum _tmp_enum {
#define X(val, dflt, swapS, C1, C2, swapV, pred) _tmp_##val,
  FCMPX8664_TABLE
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
      "Inconsistency between FCMPX8664_TABLE and ICEINSTFCMP_TABLE");
FCMPX8664_TABLE
#undef X
// Repeat the static asserts with respect to the high-level table entries in
// case the high-level table has extra entries.
#define X(tag, str)                                                            \
  static_assert(                                                               \
      _table1_##tag == _table2_##tag,                                          \
      "Inconsistency between FCMPX8664_TABLE and ICEINSTFCMP_TABLE");
ICEINSTFCMP_TABLE
#undef X
} // end of namespace dummy1

// Validate the enum values in ICMPX8664_TABLE.
namespace dummy2 {
// Define a temporary set of enum values based on low-level table entries.
enum _tmp_enum {
#define X(val, C_32, C1_64, C2_64, C3_64) _tmp_##val,
  ICMPX8664_TABLE
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
      "Inconsistency between ICMPX8664_TABLE and ICEINSTICMP_TABLE");
ICMPX8664_TABLE
#undef X
// Repeat the static asserts with respect to the high-level table entries in
// case the high-level table has extra entries.
#define X(tag, reverse, str)                                                   \
  static_assert(                                                               \
      _table1_##tag == _table2_##tag,                                          \
      "Inconsistency between ICMPX8664_TABLE and ICEINSTICMP_TABLE");
ICEINSTICMP_TABLE
#undef X
} // end of namespace dummy2

// Validate the enum values in ICETYPEX8664_TABLE.
namespace dummy3 {
// Define a temporary set of enum values based on low-level table entries.
enum _tmp_enum {
#define X(tag, elty, cvt, sdss, pdps, spsd, int_, unpack, pack, width, fld)    \
  _tmp_##tag,
  ICETYPEX8664_TABLE
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
                "Inconsistency between ICETYPEX8664_TABLE and ICETYPE_TABLE");
ICETYPEX8664_TABLE
#undef X
// Repeat the static asserts with respect to the high-level table entries in
// case the high-level table has extra entries.
#define X(tag, sizeLog2, align, elts, elty, str, rcstr)                        \
  static_assert(_table1_##tag == _table2_##tag,                                \
                "Inconsistency between ICETYPEX8664_TABLE and ICETYPE_TABLE");
ICETYPE_TABLE
#undef X
} // end of namespace dummy3
} // end of anonymous namespace

} // end of namespace X8664
} // end of namespace Ice
