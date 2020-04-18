//===- subzero/src/IceInstX8664.cpp - X86-64 instruction implementation ---===//
//
//                        The Subzero Code Generator
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// \brief This file defines X8664 specific data related to X8664 Instructions
/// and Instruction traits.
///
/// These are declared in the IceTargetLoweringX8664Traits.h header file.
///
/// This file also defines X8664 operand specific methods (dump and emit.)
///
//===----------------------------------------------------------------------===//
#include "IceInstX8664.h"

#include "IceAssemblerX8664.h"
#include "IceCfg.h"
#include "IceCfgNode.h"
#include "IceConditionCodesX8664.h"
#include "IceInst.h"
#include "IceRegistersX8664.h"
#include "IceTargetLoweringX8664.h"
#include "IceOperand.h"

namespace Ice {

namespace X8664 {

const TargetX8664Traits::InstBrAttributesType
    TargetX8664Traits::InstBrAttributes[] = {
#define X(val, encode, opp, dump, emit)                                        \
  { X8664::Traits::Cond::opp, dump, emit }                                     \
  ,
        ICEINSTX8664BR_TABLE
#undef X
};

const TargetX8664Traits::InstCmppsAttributesType
    TargetX8664Traits::InstCmppsAttributes[] = {
#define X(val, emit)                                                           \
  { emit }                                                                     \
  ,
        ICEINSTX8664CMPPS_TABLE
#undef X
};

const TargetX8664Traits::TypeAttributesType
    TargetX8664Traits::TypeAttributes[] = {
#define X(tag, elty, cvt, sdss, pdps, spsd, int_, unpack, pack, width, fld)    \
  { cvt, sdss, pdps, spsd, int_, unpack, pack, width, fld }                    \
  ,
        ICETYPEX8664_TABLE
#undef X
};

void TargetX8664Traits::X86Operand::dump(const Cfg *, Ostream &Str) const {
  if (BuildDefs::dump())
    Str << "<OperandX8664>";
}

TargetX8664Traits::X86OperandMem::X86OperandMem(Cfg *Func, Type Ty,
                                                Variable *Base,
                                                Constant *Offset,
                                                Variable *Index, uint16_t Shift,
                                                bool IsRebased)
    : X86Operand(kMem, Ty), Base(Base), Offset(Offset), Index(Index),
      Shift(Shift), IsRebased(IsRebased) {
  assert(Shift <= 3);
  Vars = nullptr;
  NumVars = 0;
  if (Base)
    ++NumVars;
  if (Index)
    ++NumVars;
  if (NumVars) {
    Vars = Func->allocateArrayOf<Variable *>(NumVars);
    SizeT I = 0;
    if (Base)
      Vars[I++] = Base;
    if (Index)
      Vars[I++] = Index;
    assert(I == NumVars);
  }
}

namespace {
int32_t getRematerializableOffset(Variable *Var,
                                  const ::Ice::X8664::TargetX8664 *Target) {
  int32_t Disp = Var->getStackOffset();
  const auto RegNum = Var->getRegNum();
  if (RegNum == Target->getFrameReg()) {
    Disp += Target->getFrameFixedAllocaOffset();
  } else if (RegNum != Target->getStackReg()) {
    llvm::report_fatal_error("Unexpected rematerializable register type");
  }
  return Disp;
}
} // end of anonymous namespace

void TargetX8664Traits::X86OperandMem::emit(const Cfg *Func) const {
  if (!BuildDefs::dump())
    return;
  const auto *Target =
      static_cast<const ::Ice::X8664::TargetX8664 *>(Func->getTarget());
  // If the base is rematerializable, we need to replace it with the correct
  // physical register (stack or base pointer), and update the Offset.
  const bool NeedSandboxing = Target->needSandboxing();
  int32_t Disp = 0;
  if (getBase() && getBase()->isRematerializable()) {
    Disp += getRematerializableOffset(getBase(), Target);
  }
  // The index should never be rematerializable.  But if we ever allow it, then
  // we should make sure the rematerialization offset is shifted by the Shift
  // value.
  if (getIndex())
    assert(!getIndex()->isRematerializable());
  Ostream &Str = Func->getContext()->getStrEmit();
  // Emit as Offset(Base,Index,1<<Shift). Offset is emitted without the leading
  // '$'. Omit the (Base,Index,1<<Shift) part if Base==nullptr.
  if (getOffset() == nullptr && Disp == 0) {
    // No offset, emit nothing.
  } else if (getOffset() == nullptr && Disp != 0) {
    Str << Disp;
  } else if (const auto *CI = llvm::dyn_cast<ConstantInteger32>(Offset)) {
    if (Base == nullptr || CI->getValue() || Disp != 0)
      // Emit a non-zero offset without a leading '$'.
      Str << CI->getValue() + Disp;
  } else if (const auto *CR = llvm::dyn_cast<ConstantRelocatable>(Offset)) {
    // TODO(sehr): ConstantRelocatable still needs updating for
    // rematerializable base/index and Disp.
    assert(Disp == 0);
    const bool UseNonsfi = getFlags().getUseNonsfi();
    CR->emitWithoutPrefix(Target, UseNonsfi ? "@GOTOFF" : "");
    assert(!UseNonsfi);
    if (Base == nullptr && Index == nullptr) {
      // rip-relative addressing.
      if (NeedSandboxing) {
        Str << "(%rip)";
      } else {
        Str << "(%eip)";
      }
    }
  } else {
    llvm_unreachable("Invalid offset type for x86 mem operand");
  }

  if (Base == nullptr && Index == nullptr) {
    return;
  }

  Str << "(";
  if (Base != nullptr) {
    const Variable *B = Base;
    if (!NeedSandboxing) {
      // TODO(jpp): stop abusing the operand's type to identify LEAs.
      const Type MemType = getType();
      if (Base->getType() != IceType_i32 && MemType != IceType_void) {
        // X86-64 is ILP32, but %rsp and %rbp are accessed as 64-bit registers.
        // For filetype=asm, they need to be emitted as their 32-bit siblings.
        assert(Base->getType() == IceType_i64);
        assert(getEncodedGPR(Base->getRegNum()) == RegX8664::Encoded_Reg_rsp ||
               getEncodedGPR(Base->getRegNum()) == RegX8664::Encoded_Reg_rbp ||
               getType() == IceType_void);
        B = B->asType(Func, IceType_i32, X8664::Traits::getGprForType(
                                             IceType_i32, Base->getRegNum()));
      }
    }

    B->emit(Func);
  }

  if (Index != nullptr) {
    Variable *I = Index;
    Str << ",";
    I->emit(Func);
    if (Shift)
      Str << "," << (1u << Shift);
  }

  Str << ")";
}

void TargetX8664Traits::X86OperandMem::dump(const Cfg *Func,
                                            Ostream &Str) const {
  if (!BuildDefs::dump())
    return;
  bool Dumped = false;
  Str << "[";
  int32_t Disp = 0;
  const auto *Target =
      static_cast<const ::Ice::X8664::TargetX8664 *>(Func->getTarget());
  if (getBase() && getBase()->isRematerializable()) {
    Disp += getRematerializableOffset(getBase(), Target);
  }
  if (Base) {
    if (Func)
      Base->dump(Func);
    else
      Base->dump(Str);
    Dumped = true;
  }
  if (Index) {
    if (Base)
      Str << "+";
    if (Shift > 0)
      Str << (1u << Shift) << "*";
    if (Func)
      Index->dump(Func);
    else
      Index->dump(Str);
    Dumped = true;
  }
  if (Disp) {
    if (Disp > 0)
      Str << "+";
    Str << Disp;
    Dumped = true;
  }
  // Pretty-print the Offset.
  bool OffsetIsZero = false;
  bool OffsetIsNegative = false;
  if (!Offset) {
    OffsetIsZero = true;
  } else if (const auto *CI = llvm::dyn_cast<ConstantInteger32>(Offset)) {
    OffsetIsZero = (CI->getValue() == 0);
    OffsetIsNegative = (static_cast<int32_t>(CI->getValue()) < 0);
  } else {
    assert(llvm::isa<ConstantRelocatable>(Offset));
  }
  if (Dumped) {
    if (!OffsetIsZero) {     // Suppress if Offset is known to be 0
      if (!OffsetIsNegative) // Suppress if Offset is known to be negative
        Str << "+";
      Offset->dump(Func, Str);
    }
  } else {
    // There is only the offset.
    Offset->dump(Func, Str);
  }
  Str << "]";
}

TargetX8664Traits::Address TargetX8664Traits::X86OperandMem::toAsmAddress(
    TargetX8664Traits::Assembler *Asm,
    const Ice::TargetLowering *TargetLowering, bool IsLeaAddr) const {
  (void)IsLeaAddr;
  const auto *Target =
      static_cast<const ::Ice::X8664::TargetX8664 *>(TargetLowering);
  int32_t Disp = 0;
  if (getBase() && getBase()->isRematerializable()) {
    Disp += getRematerializableOffset(getBase(), Target);
  }
  if (getIndex() != nullptr) {
    assert(!getIndex()->isRematerializable());
  }

  AssemblerFixup *Fixup = nullptr;
  // Determine the offset (is it relocatable?)
  if (getOffset() != nullptr) {
    if (const auto *CI = llvm::dyn_cast<ConstantInteger32>(getOffset())) {
      Disp += static_cast<int32_t>(CI->getValue());
    } else if (const auto *CR =
                   llvm::dyn_cast<ConstantRelocatable>(getOffset())) {
      const auto FixupKind =
          (getBase() != nullptr || getIndex() != nullptr) ? FK_Abs : FK_PcRel;
      const RelocOffsetT DispAdjustment = FixupKind == FK_PcRel ? 4 : 0;
      Fixup = Asm->createFixup(FixupKind, CR);
      Fixup->set_addend(-DispAdjustment);
      Disp = CR->getOffset();
    } else {
      llvm_unreachable("Unexpected offset type");
    }
  }

  // Now convert to the various possible forms.
  if (getBase() && getIndex()) {
    const bool NeedSandboxing = Target->needSandboxing();
    (void)NeedSandboxing;
    assert(!NeedSandboxing || IsLeaAddr ||
           (getBase()->getRegNum() == Traits::RegisterSet::Reg_r15) ||
           (getBase()->getRegNum() == Traits::RegisterSet::Reg_rsp) ||
           (getBase()->getRegNum() == Traits::RegisterSet::Reg_rbp));
    return X8664::Traits::Address(getEncodedGPR(getBase()->getRegNum()),
                                  getEncodedGPR(getIndex()->getRegNum()),
                                  X8664::Traits::ScaleFactor(getShift()), Disp,
                                  Fixup);
  }

  if (getBase()) {
    return X8664::Traits::Address(getEncodedGPR(getBase()->getRegNum()), Disp,
                                  Fixup);
  }

  if (getIndex()) {
    return X8664::Traits::Address(getEncodedGPR(getIndex()->getRegNum()),
                                  X8664::Traits::ScaleFactor(getShift()), Disp,
                                  Fixup);
  }

  if (Fixup == nullptr) {
    // Absolute addresses are not allowed in Nexes -- they must be rebased
    // w.r.t. %r15.
    // Exception: LEAs are fine because they do not touch memory.
    assert(!Target->needSandboxing() || IsLeaAddr);
    return X8664::Traits::Address::Absolute(Disp);
  }

  return X8664::Traits::Address::RipRelative(Disp, Fixup);
}

TargetX8664Traits::Address
TargetX8664Traits::VariableSplit::toAsmAddress(const Cfg *Func) const {
  assert(!Var->hasReg());
  const ::Ice::TargetLowering *Target = Func->getTarget();
  int32_t Offset = Var->getStackOffset() + getOffset();
  return X8664::Traits::Address(getEncodedGPR(Target->getFrameOrStackReg()),
                                Offset, AssemblerFixup::NoFixup);
}

void TargetX8664Traits::VariableSplit::emit(const Cfg *Func) const {
  if (!BuildDefs::dump())
    return;
  Ostream &Str = Func->getContext()->getStrEmit();
  assert(!Var->hasReg());
  // The following is copied/adapted from TargetX8664::emitVariable().
  const ::Ice::TargetLowering *Target = Func->getTarget();
  constexpr Type Ty = IceType_i32;
  int32_t Offset = Var->getStackOffset() + getOffset();
  if (Offset)
    Str << Offset;
  Str << "(%" << Target->getRegName(Target->getFrameOrStackReg(), Ty) << ")";
}

void TargetX8664Traits::VariableSplit::dump(const Cfg *Func,
                                            Ostream &Str) const {
  if (!BuildDefs::dump())
    return;
  switch (Part) {
  case Low:
    Str << "low";
    break;
  case High:
    Str << "high";
    break;
  }
  Str << "(";
  if (Func)
    Var->dump(Func);
  else
    Var->dump(Str);
  Str << ")";
}

} // namespace X8664
} // end of namespace Ice

X86INSTS_DEFINE_STATIC_DATA(X8664, X8664::Traits)
