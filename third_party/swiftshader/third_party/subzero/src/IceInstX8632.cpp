//===- subzero/src/IceInstX8632.cpp - X86-32 instruction implementation ---===//
//
//                        The Subzero Code Generator
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// \brief Defines X8632 specific data related to X8632 Instructions and
/// Instruction traits.
///
/// These are declared in the IceTargetLoweringX8632Traits.h header file. This
/// file also defines X8632 operand specific methods (dump and emit.)
///
//===----------------------------------------------------------------------===//
#include "IceInstX8632.h"

#include "IceAssemblerX8632.h"
#include "IceCfg.h"
#include "IceCfgNode.h"
#include "IceConditionCodesX8632.h"
#include "IceInst.h"
#include "IceRegistersX8632.h"
#include "IceTargetLoweringX8632.h"
#include "IceOperand.h"

namespace Ice {

namespace X8632 {

const TargetX8632Traits::InstBrAttributesType
    TargetX8632Traits::InstBrAttributes[] = {
#define X(val, encode, opp, dump, emit)                                        \
  { X8632::Traits::Cond::opp, dump, emit }                                     \
  ,
        ICEINSTX8632BR_TABLE
#undef X
};

const TargetX8632Traits::InstCmppsAttributesType
    TargetX8632Traits::InstCmppsAttributes[] = {
#define X(val, emit)                                                           \
  { emit }                                                                     \
  ,
        ICEINSTX8632CMPPS_TABLE
#undef X
};

const TargetX8632Traits::TypeAttributesType
    TargetX8632Traits::TypeAttributes[] = {
#define X(tag, elty, cvt, sdss, pdps, spsd, int_, unpack, pack, width, fld)    \
  { cvt, sdss, pdps, spsd, int_, unpack, pack, width, fld }                    \
  ,
        ICETYPEX8632_TABLE
#undef X
};

const char *TargetX8632Traits::InstSegmentRegNames[] = {
#define X(val, name, prefix) name,
    SEG_REGX8632_TABLE
#undef X
};

uint8_t TargetX8632Traits::InstSegmentPrefixes[] = {
#define X(val, name, prefix) prefix,
    SEG_REGX8632_TABLE
#undef X
};

void TargetX8632Traits::X86Operand::dump(const Cfg *, Ostream &Str) const {
  if (BuildDefs::dump())
    Str << "<OperandX8632>";
}

TargetX8632Traits::X86OperandMem::X86OperandMem(
    Cfg *Func, Type Ty, Variable *Base, Constant *Offset, Variable *Index,
    uint16_t Shift, SegmentRegisters SegmentReg, bool IsRebased)
    : X86Operand(kMem, Ty), Base(Base), Offset(Offset), Index(Index),
      Shift(Shift), SegmentReg(SegmentReg), IsRebased(IsRebased) {
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
                                  const Ice::X8632::TargetX8632 *Target) {
  int32_t Disp = Var->getStackOffset();
  const auto RegNum = Var->getRegNum();
  if (RegNum == Target->getFrameReg()) {
    Disp += Target->getFrameFixedAllocaOffset();
  } else if (RegNum != Target->getStackReg()) {
    llvm::report_fatal_error("Unexpected rematerializable register type");
  }
  return Disp;
}

void validateMemOperandPIC(const TargetX8632Traits::X86OperandMem *Mem,
                           bool UseNonsfi) {
  if (!BuildDefs::asserts())
    return;
  const bool HasCR =
      Mem->getOffset() && llvm::isa<ConstantRelocatable>(Mem->getOffset());
  (void)HasCR;
  const bool IsRebased = Mem->getIsRebased();
  (void)IsRebased;
  if (UseNonsfi)
    assert(HasCR == IsRebased);
  else
    assert(!IsRebased);
}

} // end of anonymous namespace

void TargetX8632Traits::X86OperandMem::emit(const Cfg *Func) const {
  if (!BuildDefs::dump())
    return;
  const bool UseNonsfi = getFlags().getUseNonsfi();
  validateMemOperandPIC(this, UseNonsfi);
  const auto *Target =
      static_cast<const ::Ice::X8632::TargetX8632 *>(Func->getTarget());
  // If the base is rematerializable, we need to replace it with the correct
  // physical register (esp or ebp), and update the Offset.
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
  if (SegmentReg != DefaultSegment) {
    assert(SegmentReg >= 0 && SegmentReg < SegReg_NUM);
    Str << "%" << X8632::Traits::InstSegmentRegNames[SegmentReg] << ":";
  }
  // Emit as Offset(Base,Index,1<<Shift). Offset is emitted without the leading
  // '$'. Omit the (Base,Index,1<<Shift) part if Base==nullptr.
  if (getOffset() == nullptr && Disp == 0) {
    // No offset, emit nothing.
  } else if (getOffset() == nullptr && Disp != 0) {
    Str << Disp;
  } else if (const auto *CI = llvm::dyn_cast<ConstantInteger32>(getOffset())) {
    if (getBase() == nullptr || CI->getValue() || Disp != 0)
      // Emit a non-zero offset without a leading '$'.
      Str << CI->getValue() + Disp;
  } else if (const auto *CR =
                 llvm::dyn_cast<ConstantRelocatable>(getOffset())) {
    // TODO(sehr): ConstantRelocatable still needs updating for
    // rematerializable base/index and Disp.
    assert(Disp == 0);
    CR->emitWithoutPrefix(Target, UseNonsfi ? "@GOTOFF" : "");
  } else {
    llvm_unreachable("Invalid offset type for x86 mem operand");
  }

  if (getBase() || getIndex()) {
    Str << "(";
    if (getBase())
      getBase()->emit(Func);
    if (getIndex()) {
      Str << ",";
      getIndex()->emit(Func);
      if (getShift())
        Str << "," << (1u << getShift());
    }
    Str << ")";
  }
}

void TargetX8632Traits::X86OperandMem::dump(const Cfg *Func,
                                            Ostream &Str) const {
  if (!BuildDefs::dump())
    return;
  if (SegmentReg != DefaultSegment) {
    assert(SegmentReg >= 0 && SegmentReg < SegReg_NUM);
    Str << X8632::Traits::InstSegmentRegNames[SegmentReg] << ":";
  }
  bool Dumped = false;
  Str << "[";
  int32_t Disp = 0;
  const auto *Target =
      static_cast<const ::Ice::X8632::TargetX8632 *>(Func->getTarget());
  if (getBase() && getBase()->isRematerializable()) {
    Disp += getRematerializableOffset(getBase(), Target);
  }
  if (getBase()) {
    if (Func)
      getBase()->dump(Func);
    else
      getBase()->dump(Str);
    Dumped = true;
  }
  if (getIndex()) {
    assert(!getIndex()->isRematerializable());
    if (getBase())
      Str << "+";
    if (getShift() > 0)
      Str << (1u << getShift()) << "*";
    if (Func)
      getIndex()->dump(Func);
    else
      getIndex()->dump(Str);
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
  if (getOffset() == nullptr) {
    OffsetIsZero = true;
  } else if (const auto *CI = llvm::dyn_cast<ConstantInteger32>(getOffset())) {
    OffsetIsZero = (CI->getValue() == 0);
    OffsetIsNegative = (static_cast<int32_t>(CI->getValue()) < 0);
  } else {
    assert(llvm::isa<ConstantRelocatable>(getOffset()));
  }
  if (Dumped) {
    if (!OffsetIsZero) {     // Suppress if Offset is known to be 0
      if (!OffsetIsNegative) // Suppress if Offset is known to be negative
        Str << "+";
      getOffset()->dump(Func, Str);
    }
  } else {
    // There is only the offset.
    getOffset()->dump(Func, Str);
  }
  Str << "]";
}

void TargetX8632Traits::X86OperandMem::emitSegmentOverride(
    TargetX8632Traits::Assembler *Asm) const {
  if (SegmentReg != DefaultSegment) {
    assert(SegmentReg >= 0 && SegmentReg < SegReg_NUM);
    Asm->emitSegmentOverride(X8632::Traits::InstSegmentPrefixes[SegmentReg]);
  }
}

TargetX8632Traits::Address TargetX8632Traits::X86OperandMem::toAsmAddress(
    TargetX8632Traits::Assembler *Asm,
    const Ice::TargetLowering *TargetLowering, bool /*IsLeaAddr*/) const {
  const auto *Target =
      static_cast<const ::Ice::X8632::TargetX8632 *>(TargetLowering);
  const bool UseNonsfi = getFlags().getUseNonsfi();
  validateMemOperandPIC(this, UseNonsfi);
  int32_t Disp = 0;
  if (getBase() && getBase()->isRematerializable()) {
    Disp += getRematerializableOffset(getBase(), Target);
  }
  // The index should never be rematerializable.  But if we ever allow it, then
  // we should make sure the rematerialization offset is shifted by the Shift
  // value.
  if (getIndex())
    assert(!getIndex()->isRematerializable());
  AssemblerFixup *Fixup = nullptr;
  // Determine the offset (is it relocatable?)
  if (getOffset()) {
    if (const auto *CI = llvm::dyn_cast<ConstantInteger32>(getOffset())) {
      Disp += static_cast<int32_t>(CI->getValue());
    } else if (const auto CR =
                   llvm::dyn_cast<ConstantRelocatable>(getOffset())) {
      Disp += CR->getOffset();
      Fixup = Asm->createFixup(Target->getAbsFixup(), CR);
    } else {
      llvm_unreachable("Unexpected offset type");
    }
  }

  // Now convert to the various possible forms.
  if (getBase() && getIndex()) {
    return X8632::Traits::Address(getEncodedGPR(getBase()->getRegNum()),
                                  getEncodedGPR(getIndex()->getRegNum()),
                                  X8632::Traits::ScaleFactor(getShift()), Disp,
                                  Fixup);
  } else if (getBase()) {
    return X8632::Traits::Address(getEncodedGPR(getBase()->getRegNum()), Disp,
                                  Fixup);
  } else if (getIndex()) {
    return X8632::Traits::Address(getEncodedGPR(getIndex()->getRegNum()),
                                  X8632::Traits::ScaleFactor(getShift()), Disp,
                                  Fixup);
  } else {
    return X8632::Traits::Address(Disp, Fixup);
  }
}

TargetX8632Traits::Address
TargetX8632Traits::VariableSplit::toAsmAddress(const Cfg *Func) const {
  assert(!Var->hasReg());
  const ::Ice::TargetLowering *Target = Func->getTarget();
  int32_t Offset = Var->getStackOffset() + getOffset();
  return X8632::Traits::Address(getEncodedGPR(Target->getFrameOrStackReg()),
                                Offset, AssemblerFixup::NoFixup);
}

void TargetX8632Traits::VariableSplit::emit(const Cfg *Func) const {
  if (!BuildDefs::dump())
    return;
  Ostream &Str = Func->getContext()->getStrEmit();
  assert(!Var->hasReg());
  // The following is copied/adapted from TargetX8632::emitVariable().
  const ::Ice::TargetLowering *Target = Func->getTarget();
  constexpr Type Ty = IceType_i32;
  int32_t Offset = Var->getStackOffset() + getOffset();
  if (Offset)
    Str << Offset;
  Str << "(%" << Target->getRegName(Target->getFrameOrStackReg(), Ty) << ")";
}

void TargetX8632Traits::VariableSplit::dump(const Cfg *Func,
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

} // namespace X8632
} // end of namespace Ice

X86INSTS_DEFINE_STATIC_DATA(X8632, X8632::Traits)
