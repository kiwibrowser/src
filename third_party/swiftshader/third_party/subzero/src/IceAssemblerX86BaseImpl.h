//===- subzero/src/IceAssemblerX86BaseImpl.h - base x86 assembler -*- C++ -*-=//
// Copyright (c) 2013, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.
//
// Modified by the Subzero authors.
//
//===----------------------------------------------------------------------===//
//
//                        The Subzero Code Generator
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
/// \file
/// \brief Implements the AssemblerX86Base template class, which is the base
/// Assembler class for X86 assemblers.
//
//===----------------------------------------------------------------------===//

#include "IceAssemblerX86Base.h"

#include "IceCfg.h"
#include "IceCfgNode.h"
#include "IceOperand.h"

namespace Ice {
namespace X86NAMESPACE {

template <typename TraitsType>
AssemblerX86Base<TraitsType>::~AssemblerX86Base() {
  if (BuildDefs::asserts()) {
    for (const Label *Label : CfgNodeLabels) {
      Label->finalCheck();
    }
    for (const Label *Label : LocalLabels) {
      Label->finalCheck();
    }
  }
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::alignFunction() {
  const SizeT Align = 1 << getBundleAlignLog2Bytes();
  SizeT BytesNeeded = Utils::OffsetToAlignment(Buffer.getPosition(), Align);
  constexpr SizeT HltSize = 1;
  while (BytesNeeded > 0) {
    hlt();
    BytesNeeded -= HltSize;
  }
}

template <typename TraitsType>
typename AssemblerX86Base<TraitsType>::Label *
AssemblerX86Base<TraitsType>::getOrCreateLabel(SizeT Number,
                                               LabelVector &Labels) {
  Label *L = nullptr;
  if (Number == Labels.size()) {
    L = new (this->allocate<Label>()) Label();
    Labels.push_back(L);
    return L;
  }
  if (Number > Labels.size()) {
    Utils::reserveAndResize(Labels, Number + 1);
  }
  L = Labels[Number];
  if (!L) {
    L = new (this->allocate<Label>()) Label();
    Labels[Number] = L;
  }
  return L;
}

template <typename TraitsType>
Ice::Label *AssemblerX86Base<TraitsType>::getCfgNodeLabel(SizeT NodeNumber) {
  assert(NodeNumber < CfgNodeLabels.size());
  return CfgNodeLabels[NodeNumber];
}

template <typename TraitsType>
typename AssemblerX86Base<TraitsType>::Label *
AssemblerX86Base<TraitsType>::getOrCreateCfgNodeLabel(SizeT NodeNumber) {
  return getOrCreateLabel(NodeNumber, CfgNodeLabels);
}

template <typename TraitsType>
typename AssemblerX86Base<TraitsType>::Label *
AssemblerX86Base<TraitsType>::getOrCreateLocalLabel(SizeT Number) {
  return getOrCreateLabel(Number, LocalLabels);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::bindCfgNodeLabel(const CfgNode *Node) {
  assert(!getPreliminary());
  Label *L = getOrCreateCfgNodeLabel(Node->getIndex());
  this->bind(L);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::bindLocalLabel(SizeT Number) {
  Label *L = getOrCreateLocalLabel(Number);
  if (!getPreliminary())
    this->bind(L);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::call(GPRRegister reg) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitRexB(RexTypeIrrelevant, reg);
  emitUint8(0xFF);
  emitRegisterOperand(2, gprEncoding(reg));
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::call(const Address &address) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitAddrSizeOverridePrefix();
  emitRex(RexTypeIrrelevant, address, RexRegIrrelevant);
  emitUint8(0xFF);
  emitOperand(2, address);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::call(const ConstantRelocatable *label) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  intptr_t call_start = Buffer.getPosition();
  emitUint8(0xE8);
  auto *Fixup = this->createFixup(Traits::FK_PcRel, label);
  Fixup->set_addend(-4);
  emitFixup(Fixup);
  emitInt32(0);
  assert((Buffer.getPosition() - call_start) == kCallExternalLabelSize);
  (void)call_start;
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::call(const Immediate &abs_address) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  intptr_t call_start = Buffer.getPosition();
  emitUint8(0xE8);
  auto *Fixup = this->createFixup(Traits::FK_PcRel, AssemblerFixup::NullSymbol);
  Fixup->set_addend(abs_address.value() - 4);
  emitFixup(Fixup);
  emitInt32(0);
  assert((Buffer.getPosition() - call_start) == kCallExternalLabelSize);
  (void)call_start;
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::pushl(GPRRegister reg) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitRexB(RexTypeIrrelevant, reg);
  emitUint8(0x50 + gprEncoding(reg));
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::pushl(const Immediate &Imm) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0x68);
  emitInt32(Imm.value());
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::pushl(const ConstantRelocatable *Label) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0x68);
  emitFixup(this->createFixup(Traits::FK_Abs, Label));
  // In x86-32, the emitted value is an addend to the relocation. Therefore, we
  // must emit a 0 (because we're pushing an absolute relocation.)
  // In x86-64, the emitted value does not matter (the addend lives in the
  // relocation record as an extra field.)
  emitInt32(0);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::popl(GPRRegister reg) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  // Any type that would not force a REX prefix to be emitted can be provided
  // here.
  emitRexB(RexTypeIrrelevant, reg);
  emitUint8(0x58 + gprEncoding(reg));
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::popl(const Address &address) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitAddrSizeOverridePrefix();
  emitRex(RexTypeIrrelevant, address, RexRegIrrelevant);
  emitUint8(0x8F);
  emitOperand(0, address);
}

template <typename TraitsType>
template <typename, typename>
void AssemblerX86Base<TraitsType>::pushal() {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0x60);
}

template <typename TraitsType>
template <typename, typename>
void AssemblerX86Base<TraitsType>::popal() {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0x61);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::setcc(BrCond condition, ByteRegister dst) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitRexB(IceType_i8, dst);
  emitUint8(0x0F);
  emitUint8(0x90 + condition);
  emitUint8(0xC0 + gprEncoding(dst));
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::setcc(BrCond condition,
                                         const Address &address) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitAddrSizeOverridePrefix();
  emitRex(RexTypeIrrelevant, address, RexRegIrrelevant);
  emitUint8(0x0F);
  emitUint8(0x90 + condition);
  emitOperand(0, address);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::mov(Type Ty, GPRRegister dst,
                                       const Immediate &imm) {
  assert(Ty != IceType_i64 && "i64 not supported yet.");
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  if (Ty == IceType_i16)
    emitOperandSizeOverride();
  emitRexB(Ty, dst);
  if (isByteSizedType(Ty)) {
    emitUint8(0xB0 + gprEncoding(dst));
    emitUint8(imm.value() & 0xFF);
  } else {
    // TODO(jpp): When removing the assertion above ensure that in x86-64 we
    // emit a 64-bit immediate.
    emitUint8(0xB8 + gprEncoding(dst));
    emitImmediate(Ty, imm);
  }
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::mov(Type Ty, GPRRegister dst,
                                       GPRRegister src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  if (Ty == IceType_i16)
    emitOperandSizeOverride();
  emitRexRB(Ty, src, dst);
  if (isByteSizedType(Ty)) {
    emitUint8(0x88);
  } else {
    emitUint8(0x89);
  }
  emitRegisterOperand(gprEncoding(src), gprEncoding(dst));
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::mov(Type Ty, GPRRegister dst,
                                       const Address &src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  if (Ty == IceType_i16)
    emitOperandSizeOverride();
  emitAddrSizeOverridePrefix();
  emitRex(Ty, src, dst);
  if (isByteSizedType(Ty)) {
    emitUint8(0x8A);
  } else {
    emitUint8(0x8B);
  }
  emitOperand(gprEncoding(dst), src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::mov(Type Ty, const Address &dst,
                                       GPRRegister src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  if (Ty == IceType_i16)
    emitOperandSizeOverride();
  emitAddrSizeOverridePrefix();
  emitRex(Ty, dst, src);
  if (isByteSizedType(Ty)) {
    emitUint8(0x88);
  } else {
    emitUint8(0x89);
  }
  emitOperand(gprEncoding(src), dst);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::mov(Type Ty, const Address &dst,
                                       const Immediate &imm) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  if (Ty == IceType_i16)
    emitOperandSizeOverride();
  emitAddrSizeOverridePrefix();
  emitRex(Ty, dst, RexRegIrrelevant);
  if (isByteSizedType(Ty)) {
    emitUint8(0xC6);
    static constexpr RelocOffsetT OffsetFromNextInstruction = 1;
    emitOperand(0, dst, OffsetFromNextInstruction);
    emitUint8(imm.value() & 0xFF);
  } else {
    emitUint8(0xC7);
    const uint8_t OffsetFromNextInstruction = Ty == IceType_i16 ? 2 : 4;
    emitOperand(0, dst, OffsetFromNextInstruction);
    emitImmediate(Ty, imm);
  }
}

template <typename TraitsType>
template <typename T>
typename std::enable_if<T::Is64Bit, void>::type
AssemblerX86Base<TraitsType>::movabs(const GPRRegister Dst, uint64_t Imm64) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  const bool NeedsRexW = (Imm64 & ~0xFFFFFFFFull) != 0;
  const Type RexType = NeedsRexW ? RexTypeForceRexW : RexTypeIrrelevant;
  emitRexB(RexType, Dst);
  emitUint8(0xB8 | gprEncoding(Dst));
  // When emitting Imm64, we don't have to mask out the upper 32 bits for
  // emitInt32 will/should only emit a 32-bit constant. In reality, we are
  // paranoid, so we go ahead an mask the upper bits out anyway.
  emitInt32(Imm64 & 0xFFFFFFFF);
  if (NeedsRexW)
    emitInt32((Imm64 >> 32) & 0xFFFFFFFF);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::movzx(Type SrcTy, GPRRegister dst,
                                         GPRRegister src) {
  if (Traits::Is64Bit && SrcTy == IceType_i32) {
    // 32-bit mov clears the upper 32 bits, hence zero-extending the 32-bit
    // operand to 64-bit.
    mov(IceType_i32, dst, src);
    return;
  }

  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  bool ByteSized = isByteSizedType(SrcTy);
  assert(ByteSized || SrcTy == IceType_i16);
  emitRexRB(RexTypeIrrelevant, dst, SrcTy, src);
  emitUint8(0x0F);
  emitUint8(ByteSized ? 0xB6 : 0xB7);
  emitRegisterOperand(gprEncoding(dst), gprEncoding(src));
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::movzx(Type SrcTy, GPRRegister dst,
                                         const Address &src) {
  if (Traits::Is64Bit && SrcTy == IceType_i32) {
    // 32-bit mov clears the upper 32 bits, hence zero-extending the 32-bit
    // operand to 64-bit.
    mov(IceType_i32, dst, src);
    return;
  }

  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  bool ByteSized = isByteSizedType(SrcTy);
  assert(ByteSized || SrcTy == IceType_i16);
  emitAddrSizeOverridePrefix();
  emitRex(SrcTy, src, RexTypeIrrelevant, dst);
  emitUint8(0x0F);
  emitUint8(ByteSized ? 0xB6 : 0xB7);
  emitOperand(gprEncoding(dst), src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::movsx(Type SrcTy, GPRRegister dst,
                                         GPRRegister src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  bool ByteSized = isByteSizedType(SrcTy);
  emitRexRB(RexTypeForceRexW, dst, SrcTy, src);
  if (ByteSized || SrcTy == IceType_i16) {
    emitUint8(0x0F);
    emitUint8(ByteSized ? 0xBE : 0xBF);
  } else {
    assert(Traits::Is64Bit && SrcTy == IceType_i32);
    emitUint8(0x63);
  }
  emitRegisterOperand(gprEncoding(dst), gprEncoding(src));
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::movsx(Type SrcTy, GPRRegister dst,
                                         const Address &src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  bool ByteSized = isByteSizedType(SrcTy);
  emitAddrSizeOverridePrefix();
  emitRex(SrcTy, src, RexTypeForceRexW, dst);
  if (ByteSized || SrcTy == IceType_i16) {
    emitUint8(0x0F);
    emitUint8(ByteSized ? 0xBE : 0xBF);
  } else {
    assert(Traits::Is64Bit && SrcTy == IceType_i32);
    emitUint8(0x63);
  }
  emitOperand(gprEncoding(dst), src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::lea(Type Ty, GPRRegister dst,
                                       const Address &src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  assert(Ty == IceType_i16 || Ty == IceType_i32 ||
         (Traits::Is64Bit && Ty == IceType_i64));
  if (Ty == IceType_i16)
    emitOperandSizeOverride();
  emitAddrSizeOverridePrefix();
  emitRex(Ty, src, dst);
  emitUint8(0x8D);
  emitOperand(gprEncoding(dst), src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::cmov(Type Ty, BrCond cond, GPRRegister dst,
                                        GPRRegister src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  if (Ty == IceType_i16)
    emitOperandSizeOverride();
  else
    assert(Ty == IceType_i32 || (Traits::Is64Bit && Ty == IceType_i64));
  emitRexRB(Ty, dst, src);
  emitUint8(0x0F);
  emitUint8(0x40 + cond);
  emitRegisterOperand(gprEncoding(dst), gprEncoding(src));
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::cmov(Type Ty, BrCond cond, GPRRegister dst,
                                        const Address &src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  if (Ty == IceType_i16)
    emitOperandSizeOverride();
  else
    assert(Ty == IceType_i32 || (Traits::Is64Bit && Ty == IceType_i64));
  emitAddrSizeOverridePrefix();
  emitRex(Ty, src, dst);
  emitUint8(0x0F);
  emitUint8(0x40 + cond);
  emitOperand(gprEncoding(dst), src);
}

template <typename TraitsType> void AssemblerX86Base<TraitsType>::rep_movsb() {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0xF3);
  emitUint8(0xA4);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::movss(Type Ty, XmmRegister dst,
                                         const Address &src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(isFloat32Asserting32Or64(Ty) ? 0xF3 : 0xF2);
  emitAddrSizeOverridePrefix();
  emitRex(RexTypeIrrelevant, src, dst);
  emitUint8(0x0F);
  emitUint8(0x10);
  emitOperand(gprEncoding(dst), src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::movss(Type Ty, const Address &dst,
                                         XmmRegister src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(isFloat32Asserting32Or64(Ty) ? 0xF3 : 0xF2);
  emitAddrSizeOverridePrefix();
  emitRex(RexTypeIrrelevant, dst, src);
  emitUint8(0x0F);
  emitUint8(0x11);
  emitOperand(gprEncoding(src), dst);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::movss(Type Ty, XmmRegister dst,
                                         XmmRegister src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(isFloat32Asserting32Or64(Ty) ? 0xF3 : 0xF2);
  emitRexRB(RexTypeIrrelevant, src, dst);
  emitUint8(0x0F);
  emitUint8(0x11);
  emitXmmRegisterOperand(src, dst);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::movd(Type SrcTy, XmmRegister dst,
                                        GPRRegister src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0x66);
  emitRexRB(SrcTy, dst, src);
  emitUint8(0x0F);
  emitUint8(0x6E);
  emitRegisterOperand(gprEncoding(dst), gprEncoding(src));
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::movd(Type SrcTy, XmmRegister dst,
                                        const Address &src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0x66);
  emitAddrSizeOverridePrefix();
  emitRex(SrcTy, src, dst);
  emitUint8(0x0F);
  emitUint8(0x6E);
  emitOperand(gprEncoding(dst), src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::movd(Type DestTy, GPRRegister dst,
                                        XmmRegister src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0x66);
  emitRexRB(DestTy, src, dst);
  emitUint8(0x0F);
  emitUint8(0x7E);
  emitRegisterOperand(gprEncoding(src), gprEncoding(dst));
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::movd(Type DestTy, const Address &dst,
                                        XmmRegister src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0x66);
  emitAddrSizeOverridePrefix();
  emitRex(DestTy, dst, src);
  emitUint8(0x0F);
  emitUint8(0x7E);
  emitOperand(gprEncoding(src), dst);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::movq(XmmRegister dst, XmmRegister src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0xF3);
  emitRexRB(RexTypeIrrelevant, dst, src);
  emitUint8(0x0F);
  emitUint8(0x7E);
  emitXmmRegisterOperand(dst, src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::movq(const Address &dst, XmmRegister src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0x66);
  emitAddrSizeOverridePrefix();
  emitRex(RexTypeIrrelevant, dst, src);
  emitUint8(0x0F);
  emitUint8(0xD6);
  emitOperand(gprEncoding(src), dst);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::movq(XmmRegister dst, const Address &src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0xF3);
  emitAddrSizeOverridePrefix();
  emitRex(RexTypeIrrelevant, src, dst);
  emitUint8(0x0F);
  emitUint8(0x7E);
  emitOperand(gprEncoding(dst), src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::addss(Type Ty, XmmRegister dst,
                                         XmmRegister src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(isFloat32Asserting32Or64(Ty) ? 0xF3 : 0xF2);
  emitRexRB(RexTypeIrrelevant, dst, src);
  emitUint8(0x0F);
  emitUint8(0x58);
  emitXmmRegisterOperand(dst, src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::addss(Type Ty, XmmRegister dst,
                                         const Address &src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(isFloat32Asserting32Or64(Ty) ? 0xF3 : 0xF2);
  emitAddrSizeOverridePrefix();
  emitRex(RexTypeIrrelevant, src, dst);
  emitUint8(0x0F);
  emitUint8(0x58);
  emitOperand(gprEncoding(dst), src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::subss(Type Ty, XmmRegister dst,
                                         XmmRegister src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(isFloat32Asserting32Or64(Ty) ? 0xF3 : 0xF2);
  emitRexRB(RexTypeIrrelevant, dst, src);
  emitUint8(0x0F);
  emitUint8(0x5C);
  emitXmmRegisterOperand(dst, src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::subss(Type Ty, XmmRegister dst,
                                         const Address &src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(isFloat32Asserting32Or64(Ty) ? 0xF3 : 0xF2);
  emitAddrSizeOverridePrefix();
  emitRex(RexTypeIrrelevant, src, dst);
  emitUint8(0x0F);
  emitUint8(0x5C);
  emitOperand(gprEncoding(dst), src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::mulss(Type Ty, XmmRegister dst,
                                         XmmRegister src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(isFloat32Asserting32Or64(Ty) ? 0xF3 : 0xF2);
  emitRexRB(RexTypeIrrelevant, dst, src);
  emitUint8(0x0F);
  emitUint8(0x59);
  emitXmmRegisterOperand(dst, src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::mulss(Type Ty, XmmRegister dst,
                                         const Address &src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(isFloat32Asserting32Or64(Ty) ? 0xF3 : 0xF2);
  emitAddrSizeOverridePrefix();
  emitRex(RexTypeIrrelevant, src, dst);
  emitUint8(0x0F);
  emitUint8(0x59);
  emitOperand(gprEncoding(dst), src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::divss(Type Ty, XmmRegister dst,
                                         XmmRegister src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(isFloat32Asserting32Or64(Ty) ? 0xF3 : 0xF2);
  emitRexRB(RexTypeIrrelevant, dst, src);
  emitUint8(0x0F);
  emitUint8(0x5E);
  emitXmmRegisterOperand(dst, src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::divss(Type Ty, XmmRegister dst,
                                         const Address &src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(isFloat32Asserting32Or64(Ty) ? 0xF3 : 0xF2);
  emitAddrSizeOverridePrefix();
  emitRex(RexTypeIrrelevant, src, dst);
  emitUint8(0x0F);
  emitUint8(0x5E);
  emitOperand(gprEncoding(dst), src);
}

template <typename TraitsType>
template <typename T, typename>
void AssemblerX86Base<TraitsType>::fld(Type Ty,
                                       const typename T::Address &src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitAddrSizeOverridePrefix();
  emitUint8(isFloat32Asserting32Or64(Ty) ? 0xD9 : 0xDD);
  emitOperand(0, src);
}

template <typename TraitsType>
template <typename T, typename>
void AssemblerX86Base<TraitsType>::fstp(Type Ty,
                                        const typename T::Address &dst) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitAddrSizeOverridePrefix();
  emitUint8(isFloat32Asserting32Or64(Ty) ? 0xD9 : 0xDD);
  emitOperand(3, dst);
}

template <typename TraitsType>
template <typename T, typename>
void AssemblerX86Base<TraitsType>::fstp(typename T::X87STRegister st) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0xDD);
  emitUint8(0xD8 + st);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::movaps(XmmRegister dst, XmmRegister src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitRexRB(RexTypeIrrelevant, dst, src);
  emitUint8(0x0F);
  emitUint8(0x28);
  emitXmmRegisterOperand(dst, src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::movups(XmmRegister dst, XmmRegister src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitRexRB(RexTypeIrrelevant, dst, src);
  emitUint8(0x0F);
  emitUint8(0x10);
  emitXmmRegisterOperand(dst, src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::movups(XmmRegister dst, const Address &src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitAddrSizeOverridePrefix();
  emitRex(RexTypeIrrelevant, src, dst);
  emitUint8(0x0F);
  emitUint8(0x10);
  emitOperand(gprEncoding(dst), src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::movups(const Address &dst, XmmRegister src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitAddrSizeOverridePrefix();
  emitRex(RexTypeIrrelevant, dst, src);
  emitUint8(0x0F);
  emitUint8(0x11);
  emitOperand(gprEncoding(src), dst);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::padd(Type Ty, XmmRegister dst,
                                        XmmRegister src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0x66);
  emitRexRB(RexTypeIrrelevant, dst, src);
  emitUint8(0x0F);
  if (isByteSizedArithType(Ty)) {
    emitUint8(0xFC);
  } else if (Ty == IceType_i16) {
    emitUint8(0xFD);
  } else {
    emitUint8(0xFE);
  }
  emitXmmRegisterOperand(dst, src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::padd(Type Ty, XmmRegister dst,
                                        const Address &src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0x66);
  emitAddrSizeOverridePrefix();
  emitRex(RexTypeIrrelevant, src, dst);
  emitUint8(0x0F);
  if (isByteSizedArithType(Ty)) {
    emitUint8(0xFC);
  } else if (Ty == IceType_i16) {
    emitUint8(0xFD);
  } else {
    emitUint8(0xFE);
  }
  emitOperand(gprEncoding(dst), src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::padds(Type Ty, XmmRegister dst,
                                         XmmRegister src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0x66);
  emitRexRB(RexTypeIrrelevant, dst, src);
  emitUint8(0x0F);
  if (isByteSizedArithType(Ty)) {
    emitUint8(0xEC);
  } else if (Ty == IceType_i16) {
    emitUint8(0xED);
  } else {
    assert(false && "Unexpected padds operand type");
  }
  emitXmmRegisterOperand(dst, src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::padds(Type Ty, XmmRegister dst,
                                         const Address &src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0x66);
  emitAddrSizeOverridePrefix();
  emitRex(RexTypeIrrelevant, src, dst);
  emitUint8(0x0F);
  if (isByteSizedArithType(Ty)) {
    emitUint8(0xEC);
  } else if (Ty == IceType_i16) {
    emitUint8(0xED);
  } else {
    assert(false && "Unexpected padds operand type");
  }
  emitOperand(gprEncoding(dst), src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::paddus(Type Ty, XmmRegister dst,
                                          XmmRegister src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0x66);
  emitRexRB(RexTypeIrrelevant, dst, src);
  emitUint8(0x0F);
  if (isByteSizedArithType(Ty)) {
    emitUint8(0xDC);
  } else if (Ty == IceType_i16) {
    emitUint8(0xDD);
  } else {
    assert(false && "Unexpected paddus operand type");
  }
  emitXmmRegisterOperand(dst, src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::paddus(Type Ty, XmmRegister dst,
                                          const Address &src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0x66);
  emitAddrSizeOverridePrefix();
  emitRex(RexTypeIrrelevant, src, dst);
  emitUint8(0x0F);
  if (isByteSizedArithType(Ty)) {
    emitUint8(0xDC);
  } else if (Ty == IceType_i16) {
    emitUint8(0xDD);
  } else {
    assert(false && "Unexpected paddus operand type");
  }
  emitOperand(gprEncoding(dst), src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::pand(Type /* Ty */, XmmRegister dst,
                                        XmmRegister src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0x66);
  emitRexRB(RexTypeIrrelevant, dst, src);
  emitUint8(0x0F);
  emitUint8(0xDB);
  emitXmmRegisterOperand(dst, src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::pand(Type /* Ty */, XmmRegister dst,
                                        const Address &src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0x66);
  emitAddrSizeOverridePrefix();
  emitRex(RexTypeIrrelevant, src, dst);
  emitUint8(0x0F);
  emitUint8(0xDB);
  emitOperand(gprEncoding(dst), src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::pandn(Type /* Ty */, XmmRegister dst,
                                         XmmRegister src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0x66);
  emitRexRB(RexTypeIrrelevant, dst, src);
  emitUint8(0x0F);
  emitUint8(0xDF);
  emitXmmRegisterOperand(dst, src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::pandn(Type /* Ty */, XmmRegister dst,
                                         const Address &src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0x66);
  emitAddrSizeOverridePrefix();
  emitRex(RexTypeIrrelevant, src, dst);
  emitUint8(0x0F);
  emitUint8(0xDF);
  emitOperand(gprEncoding(dst), src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::pmull(Type Ty, XmmRegister dst,
                                         XmmRegister src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0x66);
  emitRexRB(RexTypeIrrelevant, dst, src);
  emitUint8(0x0F);
  if (Ty == IceType_i16) {
    emitUint8(0xD5);
  } else {
    assert(Ty == IceType_i32);
    emitUint8(0x38);
    emitUint8(0x40);
  }
  emitXmmRegisterOperand(dst, src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::pmull(Type Ty, XmmRegister dst,
                                         const Address &src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0x66);
  emitAddrSizeOverridePrefix();
  emitRex(RexTypeIrrelevant, src, dst);
  emitUint8(0x0F);
  if (Ty == IceType_i16) {
    emitUint8(0xD5);
  } else {
    assert(Ty == IceType_i32);
    emitUint8(0x38);
    emitUint8(0x40);
  }
  emitOperand(gprEncoding(dst), src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::pmulhw(Type Ty, XmmRegister dst,
                                          XmmRegister src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0x66);
  emitRexRB(RexTypeIrrelevant, dst, src);
  emitUint8(0x0F);
  assert(Ty == IceType_v8i16);
  (void)Ty;
  emitUint8(0xE5);
  emitXmmRegisterOperand(dst, src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::pmulhw(Type Ty, XmmRegister dst,
                                          const Address &src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0x66);
  emitAddrSizeOverridePrefix();
  emitRex(RexTypeIrrelevant, src, dst);
  emitUint8(0x0F);
  assert(Ty == IceType_v8i16);
  (void)Ty;
  emitUint8(0xE5);
  emitOperand(gprEncoding(dst), src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::pmulhuw(Type Ty, XmmRegister dst,
                                           XmmRegister src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0x66);
  emitRexRB(RexTypeIrrelevant, dst, src);
  emitUint8(0x0F);
  assert(Ty == IceType_v8i16);
  (void)Ty;
  emitUint8(0xE4);
  emitXmmRegisterOperand(dst, src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::pmulhuw(Type Ty, XmmRegister dst,
                                           const Address &src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0x66);
  emitAddrSizeOverridePrefix();
  emitRex(RexTypeIrrelevant, src, dst);
  emitUint8(0x0F);
  assert(Ty == IceType_v8i16);
  (void)Ty;
  emitUint8(0xE4);
  emitOperand(gprEncoding(dst), src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::pmaddwd(Type Ty, XmmRegister dst,
                                           XmmRegister src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0x66);
  emitRexRB(RexTypeIrrelevant, dst, src);
  emitUint8(0x0F);
  assert(Ty == IceType_v8i16);
  (void)Ty;
  emitUint8(0xF5);
  emitXmmRegisterOperand(dst, src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::pmaddwd(Type Ty, XmmRegister dst,
                                           const Address &src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0x66);
  emitAddrSizeOverridePrefix();
  emitRex(RexTypeIrrelevant, src, dst);
  emitUint8(0x0F);
  assert(Ty == IceType_v8i16);
  (void)Ty;
  emitUint8(0xF5);
  emitOperand(gprEncoding(dst), src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::pmuludq(Type /* Ty */, XmmRegister dst,
                                           XmmRegister src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0x66);
  emitRexRB(RexTypeIrrelevant, dst, src);
  emitUint8(0x0F);
  emitUint8(0xF4);
  emitXmmRegisterOperand(dst, src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::pmuludq(Type /* Ty */, XmmRegister dst,
                                           const Address &src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0x66);
  emitAddrSizeOverridePrefix();
  emitRex(RexTypeIrrelevant, src, dst);
  emitUint8(0x0F);
  emitUint8(0xF4);
  emitOperand(gprEncoding(dst), src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::por(Type /* Ty */, XmmRegister dst,
                                       XmmRegister src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0x66);
  emitRexRB(RexTypeIrrelevant, dst, src);
  emitUint8(0x0F);
  emitUint8(0xEB);
  emitXmmRegisterOperand(dst, src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::por(Type /* Ty */, XmmRegister dst,
                                       const Address &src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0x66);
  emitAddrSizeOverridePrefix();
  emitRex(RexTypeIrrelevant, src, dst);
  emitUint8(0x0F);
  emitUint8(0xEB);
  emitOperand(gprEncoding(dst), src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::psub(Type Ty, XmmRegister dst,
                                        XmmRegister src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0x66);
  emitRexRB(RexTypeIrrelevant, dst, src);
  emitUint8(0x0F);
  if (isByteSizedArithType(Ty)) {
    emitUint8(0xF8);
  } else if (Ty == IceType_i16) {
    emitUint8(0xF9);
  } else {
    emitUint8(0xFA);
  }
  emitXmmRegisterOperand(dst, src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::psub(Type Ty, XmmRegister dst,
                                        const Address &src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0x66);
  emitAddrSizeOverridePrefix();
  emitRex(RexTypeIrrelevant, src, dst);
  emitUint8(0x0F);
  if (isByteSizedArithType(Ty)) {
    emitUint8(0xF8);
  } else if (Ty == IceType_i16) {
    emitUint8(0xF9);
  } else {
    emitUint8(0xFA);
  }
  emitOperand(gprEncoding(dst), src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::psubs(Type Ty, XmmRegister dst,
                                         XmmRegister src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0x66);
  emitRexRB(RexTypeIrrelevant, dst, src);
  emitUint8(0x0F);
  if (isByteSizedArithType(Ty)) {
    emitUint8(0xE8);
  } else if (Ty == IceType_i16) {
    emitUint8(0xE9);
  } else {
    assert(false && "Unexpected psubs operand type");
  }
  emitXmmRegisterOperand(dst, src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::psubs(Type Ty, XmmRegister dst,
                                         const Address &src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0x66);
  emitAddrSizeOverridePrefix();
  emitRex(RexTypeIrrelevant, src, dst);
  emitUint8(0x0F);
  if (isByteSizedArithType(Ty)) {
    emitUint8(0xE8);
  } else if (Ty == IceType_i16) {
    emitUint8(0xE9);
  } else {
    assert(false && "Unexpected psubs operand type");
  }
  emitOperand(gprEncoding(dst), src);
}
template <typename TraitsType>
void AssemblerX86Base<TraitsType>::psubus(Type Ty, XmmRegister dst,
                                          XmmRegister src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0x66);
  emitRexRB(RexTypeIrrelevant, dst, src);
  emitUint8(0x0F);
  if (isByteSizedArithType(Ty)) {
    emitUint8(0xD8);
  } else if (Ty == IceType_i16) {
    emitUint8(0xD9);
  } else {
    assert(false && "Unexpected psubus operand type");
  }
  emitXmmRegisterOperand(dst, src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::psubus(Type Ty, XmmRegister dst,
                                          const Address &src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0x66);
  emitAddrSizeOverridePrefix();
  emitRex(RexTypeIrrelevant, src, dst);
  emitUint8(0x0F);
  if (isByteSizedArithType(Ty)) {
    emitUint8(0xD8);
  } else if (Ty == IceType_i16) {
    emitUint8(0xD9);
  } else {
    assert(false && "Unexpected psubus operand type");
  }
  emitOperand(gprEncoding(dst), src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::pxor(Type /* Ty */, XmmRegister dst,
                                        XmmRegister src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0x66);
  emitRexRB(RexTypeIrrelevant, dst, src);
  emitUint8(0x0F);
  emitUint8(0xEF);
  emitXmmRegisterOperand(dst, src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::pxor(Type /* Ty */, XmmRegister dst,
                                        const Address &src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0x66);
  emitAddrSizeOverridePrefix();
  emitRex(RexTypeIrrelevant, src, dst);
  emitUint8(0x0F);
  emitUint8(0xEF);
  emitOperand(gprEncoding(dst), src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::psll(Type Ty, XmmRegister dst,
                                        XmmRegister src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0x66);
  emitRexRB(RexTypeIrrelevant, dst, src);
  emitUint8(0x0F);
  if (Ty == IceType_i16) {
    emitUint8(0xF1);
  } else {
    assert(Ty == IceType_i32);
    emitUint8(0xF2);
  }
  emitXmmRegisterOperand(dst, src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::psll(Type Ty, XmmRegister dst,
                                        const Address &src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0x66);
  emitAddrSizeOverridePrefix();
  emitRex(RexTypeIrrelevant, src, dst);
  emitUint8(0x0F);
  if (Ty == IceType_i16) {
    emitUint8(0xF1);
  } else {
    assert(Ty == IceType_i32);
    emitUint8(0xF2);
  }
  emitOperand(gprEncoding(dst), src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::psll(Type Ty, XmmRegister dst,
                                        const Immediate &imm) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  assert(imm.is_int8());
  emitUint8(0x66);
  emitRexB(RexTypeIrrelevant, dst);
  emitUint8(0x0F);
  if (Ty == IceType_i16) {
    emitUint8(0x71);
  } else {
    assert(Ty == IceType_i32);
    emitUint8(0x72);
  }
  emitRegisterOperand(6, gprEncoding(dst));
  emitUint8(imm.value() & 0xFF);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::psra(Type Ty, XmmRegister dst,
                                        XmmRegister src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0x66);
  emitRexRB(RexTypeIrrelevant, dst, src);
  emitUint8(0x0F);
  if (Ty == IceType_i16) {
    emitUint8(0xE1);
  } else {
    assert(Ty == IceType_i32);
    emitUint8(0xE2);
  }
  emitXmmRegisterOperand(dst, src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::psra(Type Ty, XmmRegister dst,
                                        const Address &src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0x66);
  emitAddrSizeOverridePrefix();
  emitRex(RexTypeIrrelevant, src, dst);
  emitUint8(0x0F);
  if (Ty == IceType_i16) {
    emitUint8(0xE1);
  } else {
    assert(Ty == IceType_i32);
    emitUint8(0xE2);
  }
  emitOperand(gprEncoding(dst), src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::psra(Type Ty, XmmRegister dst,
                                        const Immediate &imm) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  assert(imm.is_int8());
  emitUint8(0x66);
  emitRexB(RexTypeIrrelevant, dst);
  emitUint8(0x0F);
  if (Ty == IceType_i16) {
    emitUint8(0x71);
  } else {
    assert(Ty == IceType_i32);
    emitUint8(0x72);
  }
  emitRegisterOperand(4, gprEncoding(dst));
  emitUint8(imm.value() & 0xFF);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::psrl(Type Ty, XmmRegister dst,
                                        XmmRegister src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0x66);
  emitRexRB(RexTypeIrrelevant, dst, src);
  emitUint8(0x0F);
  if (Ty == IceType_i16) {
    emitUint8(0xD1);
  } else if (Ty == IceType_f64) {
    emitUint8(0xD3);
  } else {
    assert(Ty == IceType_i32 || Ty == IceType_f32 || Ty == IceType_v4f32);
    emitUint8(0xD2);
  }
  emitXmmRegisterOperand(dst, src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::psrl(Type Ty, XmmRegister dst,
                                        const Address &src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0x66);
  emitAddrSizeOverridePrefix();
  emitRex(RexTypeIrrelevant, src, dst);
  emitUint8(0x0F);
  if (Ty == IceType_i16) {
    emitUint8(0xD1);
  } else if (Ty == IceType_f64) {
    emitUint8(0xD3);
  } else {
    assert(Ty == IceType_i32 || Ty == IceType_f32 || Ty == IceType_v4f32);
    emitUint8(0xD2);
  }
  emitOperand(gprEncoding(dst), src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::psrl(Type Ty, XmmRegister dst,
                                        const Immediate &imm) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  assert(imm.is_int8());
  emitUint8(0x66);
  emitRexB(RexTypeIrrelevant, dst);
  emitUint8(0x0F);
  if (Ty == IceType_i16) {
    emitUint8(0x71);
  } else if (Ty == IceType_f64) {
    emitUint8(0x73);
  } else {
    assert(Ty == IceType_i32 || Ty == IceType_f32 || Ty == IceType_v4f32);
    emitUint8(0x72);
  }
  emitRegisterOperand(2, gprEncoding(dst));
  emitUint8(imm.value() & 0xFF);
}

// {add,sub,mul,div}ps are given a Ty parameter for consistency with
// {add,sub,mul,div}ss. In the future, when the PNaCl ABI allows addpd, etc.,
// we can use the Ty parameter to decide on adding a 0x66 prefix.
template <typename TraitsType>
void AssemblerX86Base<TraitsType>::addps(Type /* Ty */, XmmRegister dst,
                                         XmmRegister src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitRexRB(RexTypeIrrelevant, dst, src);
  emitUint8(0x0F);
  emitUint8(0x58);
  emitXmmRegisterOperand(dst, src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::addps(Type /* Ty */, XmmRegister dst,
                                         const Address &src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitAddrSizeOverridePrefix();
  emitRex(RexTypeIrrelevant, src, dst);
  emitUint8(0x0F);
  emitUint8(0x58);
  emitOperand(gprEncoding(dst), src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::subps(Type /* Ty */, XmmRegister dst,
                                         XmmRegister src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitRexRB(RexTypeIrrelevant, dst, src);
  emitUint8(0x0F);
  emitUint8(0x5C);
  emitXmmRegisterOperand(dst, src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::subps(Type /* Ty */, XmmRegister dst,
                                         const Address &src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitAddrSizeOverridePrefix();
  emitRex(RexTypeIrrelevant, src, dst);
  emitUint8(0x0F);
  emitUint8(0x5C);
  emitOperand(gprEncoding(dst), src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::divps(Type /* Ty */, XmmRegister dst,
                                         XmmRegister src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitRexRB(RexTypeIrrelevant, dst, src);
  emitUint8(0x0F);
  emitUint8(0x5E);
  emitXmmRegisterOperand(dst, src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::divps(Type /* Ty */, XmmRegister dst,
                                         const Address &src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitAddrSizeOverridePrefix();
  emitRex(RexTypeIrrelevant, src, dst);
  emitUint8(0x0F);
  emitUint8(0x5E);
  emitOperand(gprEncoding(dst), src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::mulps(Type /* Ty */, XmmRegister dst,
                                         XmmRegister src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitRexRB(RexTypeIrrelevant, dst, src);
  emitUint8(0x0F);
  emitUint8(0x59);
  emitXmmRegisterOperand(dst, src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::mulps(Type /* Ty */, XmmRegister dst,
                                         const Address &src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitAddrSizeOverridePrefix();
  emitRex(RexTypeIrrelevant, src, dst);
  emitUint8(0x0F);
  emitUint8(0x59);
  emitOperand(gprEncoding(dst), src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::minps(Type Ty, XmmRegister dst,
                                         XmmRegister src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  if (!isFloat32Asserting32Or64(Ty))
    emitUint8(0x66);
  emitRexRB(RexTypeIrrelevant, dst, src);
  emitUint8(0x0F);
  emitUint8(0x5D);
  emitXmmRegisterOperand(dst, src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::minps(Type Ty, XmmRegister dst,
                                         const Address &src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  if (!isFloat32Asserting32Or64(Ty))
    emitUint8(0x66);
  emitAddrSizeOverridePrefix();
  emitRex(RexTypeIrrelevant, src, dst);
  emitUint8(0x0F);
  emitUint8(0x5D);
  emitOperand(gprEncoding(dst), src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::minss(Type Ty, XmmRegister dst,
                                         XmmRegister src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(isFloat32Asserting32Or64(Ty) ? 0xF3 : 0xF2);
  emitRexRB(RexTypeIrrelevant, dst, src);
  emitUint8(0x0F);
  emitUint8(0x5D);
  emitXmmRegisterOperand(dst, src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::minss(Type Ty, XmmRegister dst,
                                         const Address &src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(isFloat32Asserting32Or64(Ty) ? 0xF3 : 0xF2);
  emitAddrSizeOverridePrefix();
  emitRex(RexTypeIrrelevant, src, dst);
  emitUint8(0x0F);
  emitUint8(0x5D);
  emitOperand(gprEncoding(dst), src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::maxps(Type Ty, XmmRegister dst,
                                         XmmRegister src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  if (!isFloat32Asserting32Or64(Ty))
    emitUint8(0x66);
  emitRexRB(RexTypeIrrelevant, dst, src);
  emitUint8(0x0F);
  emitUint8(0x5F);
  emitXmmRegisterOperand(dst, src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::maxps(Type Ty, XmmRegister dst,
                                         const Address &src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  if (!isFloat32Asserting32Or64(Ty))
    emitUint8(0x66);
  emitAddrSizeOverridePrefix();
  emitRex(RexTypeIrrelevant, src, dst);
  emitUint8(0x0F);
  emitUint8(0x5F);
  emitOperand(gprEncoding(dst), src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::maxss(Type Ty, XmmRegister dst,
                                         XmmRegister src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(isFloat32Asserting32Or64(Ty) ? 0xF3 : 0xF2);
  emitRexRB(RexTypeIrrelevant, dst, src);
  emitUint8(0x0F);
  emitUint8(0x5F);
  emitXmmRegisterOperand(dst, src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::maxss(Type Ty, XmmRegister dst,
                                         const Address &src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(isFloat32Asserting32Or64(Ty) ? 0xF3 : 0xF2);
  emitAddrSizeOverridePrefix();
  emitRex(RexTypeIrrelevant, src, dst);
  emitUint8(0x0F);
  emitUint8(0x5F);
  emitOperand(gprEncoding(dst), src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::andnps(Type Ty, XmmRegister dst,
                                          XmmRegister src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  if (!isFloat32Asserting32Or64(Ty))
    emitUint8(0x66);
  emitRexRB(RexTypeIrrelevant, dst, src);
  emitUint8(0x0F);
  emitUint8(0x55);
  emitXmmRegisterOperand(dst, src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::andnps(Type Ty, XmmRegister dst,
                                          const Address &src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  if (!isFloat32Asserting32Or64(Ty))
    emitUint8(0x66);
  emitAddrSizeOverridePrefix();
  emitRex(RexTypeIrrelevant, src, dst);
  emitUint8(0x0F);
  emitUint8(0x55);
  emitOperand(gprEncoding(dst), src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::andps(Type Ty, XmmRegister dst,
                                         XmmRegister src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  if (!isFloat32Asserting32Or64(Ty))
    emitUint8(0x66);
  emitRexRB(RexTypeIrrelevant, dst, src);
  emitUint8(0x0F);
  emitUint8(0x54);
  emitXmmRegisterOperand(dst, src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::andps(Type Ty, XmmRegister dst,
                                         const Address &src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  if (!isFloat32Asserting32Or64(Ty))
    emitUint8(0x66);
  emitAddrSizeOverridePrefix();
  emitRex(RexTypeIrrelevant, src, dst);
  emitUint8(0x0F);
  emitUint8(0x54);
  emitOperand(gprEncoding(dst), src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::orps(Type Ty, XmmRegister dst,
                                        XmmRegister src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  if (!isFloat32Asserting32Or64(Ty))
    emitUint8(0x66);
  emitRexRB(RexTypeIrrelevant, dst, src);
  emitUint8(0x0F);
  emitUint8(0x56);
  emitXmmRegisterOperand(dst, src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::orps(Type Ty, XmmRegister dst,
                                        const Address &src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  if (!isFloat32Asserting32Or64(Ty))
    emitUint8(0x66);
  emitAddrSizeOverridePrefix();
  emitRex(RexTypeIrrelevant, src, dst);
  emitUint8(0x0F);
  emitUint8(0x56);
  emitOperand(gprEncoding(dst), src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::blendvps(Type /* Ty */, XmmRegister dst,
                                            XmmRegister src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0x66);
  emitRexRB(RexTypeIrrelevant, dst, src);
  emitUint8(0x0F);
  emitUint8(0x38);
  emitUint8(0x14);
  emitXmmRegisterOperand(dst, src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::blendvps(Type /* Ty */, XmmRegister dst,
                                            const Address &src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0x66);
  emitAddrSizeOverridePrefix();
  emitRex(RexTypeIrrelevant, src, dst);
  emitUint8(0x0F);
  emitUint8(0x38);
  emitUint8(0x14);
  emitOperand(gprEncoding(dst), src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::pblendvb(Type /* Ty */, XmmRegister dst,
                                            XmmRegister src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0x66);
  emitRexRB(RexTypeIrrelevant, dst, src);
  emitUint8(0x0F);
  emitUint8(0x38);
  emitUint8(0x10);
  emitXmmRegisterOperand(dst, src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::pblendvb(Type /* Ty */, XmmRegister dst,
                                            const Address &src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0x66);
  emitAddrSizeOverridePrefix();
  emitRex(RexTypeIrrelevant, src, dst);
  emitUint8(0x0F);
  emitUint8(0x38);
  emitUint8(0x10);
  emitOperand(gprEncoding(dst), src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::cmpps(Type Ty, XmmRegister dst,
                                         XmmRegister src,
                                         CmppsCond CmpCondition) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  if (Ty == IceType_f64)
    emitUint8(0x66);
  emitRexRB(RexTypeIrrelevant, dst, src);
  emitUint8(0x0F);
  emitUint8(0xC2);
  emitXmmRegisterOperand(dst, src);
  emitUint8(CmpCondition);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::cmpps(Type Ty, XmmRegister dst,
                                         const Address &src,
                                         CmppsCond CmpCondition) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  if (Ty == IceType_f64)
    emitUint8(0x66);
  emitAddrSizeOverridePrefix();
  emitRex(RexTypeIrrelevant, src, dst);
  emitUint8(0x0F);
  emitUint8(0xC2);
  static constexpr RelocOffsetT OffsetFromNextInstruction = 1;
  emitOperand(gprEncoding(dst), src, OffsetFromNextInstruction);
  emitUint8(CmpCondition);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::sqrtps(XmmRegister dst) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitRexRB(RexTypeIrrelevant, dst, dst);
  emitUint8(0x0F);
  emitUint8(0x51);
  emitXmmRegisterOperand(dst, dst);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::rsqrtps(XmmRegister dst) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitRexRB(RexTypeIrrelevant, dst, dst);
  emitUint8(0x0F);
  emitUint8(0x52);
  emitXmmRegisterOperand(dst, dst);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::reciprocalps(XmmRegister dst) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitRexRB(RexTypeIrrelevant, dst, dst);
  emitUint8(0x0F);
  emitUint8(0x53);
  emitXmmRegisterOperand(dst, dst);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::movhlps(XmmRegister dst, XmmRegister src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitRexRB(RexTypeIrrelevant, dst, src);
  emitUint8(0x0F);
  emitUint8(0x12);
  emitXmmRegisterOperand(dst, src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::movlhps(XmmRegister dst, XmmRegister src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitRexRB(RexTypeIrrelevant, dst, src);
  emitUint8(0x0F);
  emitUint8(0x16);
  emitXmmRegisterOperand(dst, src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::unpcklps(XmmRegister dst, XmmRegister src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitRexRB(RexTypeIrrelevant, dst, src);
  emitUint8(0x0F);
  emitUint8(0x14);
  emitXmmRegisterOperand(dst, src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::unpckhps(XmmRegister dst, XmmRegister src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitRexRB(RexTypeIrrelevant, dst, src);
  emitUint8(0x0F);
  emitUint8(0x15);
  emitXmmRegisterOperand(dst, src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::unpcklpd(XmmRegister dst, XmmRegister src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0x66);
  emitRexRB(RexTypeIrrelevant, dst, src);
  emitUint8(0x0F);
  emitUint8(0x14);
  emitXmmRegisterOperand(dst, src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::unpckhpd(XmmRegister dst, XmmRegister src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0x66);
  emitRexRB(RexTypeIrrelevant, dst, src);
  emitUint8(0x0F);
  emitUint8(0x15);
  emitXmmRegisterOperand(dst, src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::set1ps(XmmRegister dst, GPRRegister tmp1,
                                          const Immediate &imm) {
  // Load 32-bit immediate value into tmp1.
  mov(IceType_i32, tmp1, imm);
  // Move value from tmp1 into dst.
  movd(IceType_i32, dst, tmp1);
  // Broadcast low lane into other three lanes.
  shufps(RexTypeIrrelevant, dst, dst, Immediate(0x0));
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::pshufb(Type /* Ty */, XmmRegister dst,
                                          XmmRegister src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0x66);
  emitRexRB(RexTypeIrrelevant, dst, src);
  emitUint8(0x0F);
  emitUint8(0x38);
  emitUint8(0x00);
  emitXmmRegisterOperand(dst, src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::pshufb(Type /* Ty */, XmmRegister dst,
                                          const Address &src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0x66);
  emitAddrSizeOverridePrefix();
  emitRex(RexTypeIrrelevant, src, dst);
  emitUint8(0x0F);
  emitUint8(0x38);
  emitUint8(0x00);
  emitOperand(gprEncoding(dst), src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::pshufd(Type /* Ty */, XmmRegister dst,
                                          XmmRegister src,
                                          const Immediate &imm) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0x66);
  emitRexRB(RexTypeIrrelevant, dst, src);
  emitUint8(0x0F);
  emitUint8(0x70);
  emitXmmRegisterOperand(dst, src);
  assert(imm.is_uint8());
  emitUint8(imm.value());
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::pshufd(Type /* Ty */, XmmRegister dst,
                                          const Address &src,
                                          const Immediate &imm) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0x66);
  emitAddrSizeOverridePrefix();
  emitRex(RexTypeIrrelevant, src, dst);
  emitUint8(0x0F);
  emitUint8(0x70);
  static constexpr RelocOffsetT OffsetFromNextInstruction = 1;
  emitOperand(gprEncoding(dst), src, OffsetFromNextInstruction);
  assert(imm.is_uint8());
  emitUint8(imm.value());
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::punpckl(Type Ty, XmmRegister Dst,
                                           XmmRegister Src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0x66);
  emitRexRB(RexTypeIrrelevant, Dst, Src);
  emitUint8(0x0F);
  if (Ty == IceType_v4i32 || Ty == IceType_v4f32) {
    emitUint8(0x62);
  } else if (Ty == IceType_v8i16) {
    emitUint8(0x61);
  } else if (Ty == IceType_v16i8) {
    emitUint8(0x60);
  } else {
    assert(false && "Unexpected vector unpack operand type");
  }
  emitXmmRegisterOperand(Dst, Src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::punpckl(Type Ty, XmmRegister Dst,
                                           const Address &Src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0x66);
  emitAddrSizeOverridePrefix();
  emitRex(RexTypeIrrelevant, Src, Dst);
  emitUint8(0x0F);
  if (Ty == IceType_v4i32 || Ty == IceType_v4f32) {
    emitUint8(0x62);
  } else if (Ty == IceType_v8i16) {
    emitUint8(0x61);
  } else if (Ty == IceType_v16i8) {
    emitUint8(0x60);
  } else {
    assert(false && "Unexpected vector unpack operand type");
  }
  emitOperand(gprEncoding(Dst), Src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::punpckh(Type Ty, XmmRegister Dst,
                                           XmmRegister Src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0x66);
  emitRexRB(RexTypeIrrelevant, Dst, Src);
  emitUint8(0x0F);
  if (Ty == IceType_v4i32 || Ty == IceType_v4f32) {
    emitUint8(0x6A);
  } else if (Ty == IceType_v8i16) {
    emitUint8(0x69);
  } else if (Ty == IceType_v16i8) {
    emitUint8(0x68);
  } else {
    assert(false && "Unexpected vector unpack operand type");
  }
  emitXmmRegisterOperand(Dst, Src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::punpckh(Type Ty, XmmRegister Dst,
                                           const Address &Src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0x66);
  emitAddrSizeOverridePrefix();
  emitRex(RexTypeIrrelevant, Src, Dst);
  emitUint8(0x0F);
  if (Ty == IceType_v4i32 || Ty == IceType_v4f32) {
    emitUint8(0x6A);
  } else if (Ty == IceType_v8i16) {
    emitUint8(0x69);
  } else if (Ty == IceType_v16i8) {
    emitUint8(0x68);
  } else {
    assert(false && "Unexpected vector unpack operand type");
  }
  emitOperand(gprEncoding(Dst), Src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::packss(Type Ty, XmmRegister Dst,
                                          XmmRegister Src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0x66);
  emitRexRB(RexTypeIrrelevant, Dst, Src);
  emitUint8(0x0F);
  if (Ty == IceType_v4i32 || Ty == IceType_v4f32) {
    emitUint8(0x6B);
  } else if (Ty == IceType_v8i16) {
    emitUint8(0x63);
  } else {
    assert(false && "Unexpected vector pack operand type");
  }
  emitXmmRegisterOperand(Dst, Src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::packss(Type Ty, XmmRegister Dst,
                                          const Address &Src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0x66);
  emitAddrSizeOverridePrefix();
  emitRex(RexTypeIrrelevant, Src, Dst);
  emitUint8(0x0F);
  if (Ty == IceType_v4i32 || Ty == IceType_v4f32) {
    emitUint8(0x6B);
  } else if (Ty == IceType_v8i16) {
    emitUint8(0x63);
  } else {
    assert(false && "Unexpected vector pack operand type");
  }
  emitOperand(gprEncoding(Dst), Src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::packus(Type Ty, XmmRegister Dst,
                                          XmmRegister Src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0x66);
  emitRexRB(RexTypeIrrelevant, Dst, Src);
  emitUint8(0x0F);
  if (Ty == IceType_v4i32 || Ty == IceType_v4f32) {
    emitUint8(0x38);
    emitUint8(0x2B);
  } else if (Ty == IceType_v8i16) {
    emitUint8(0x67);
  } else {
    assert(false && "Unexpected vector pack operand type");
  }
  emitXmmRegisterOperand(Dst, Src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::packus(Type Ty, XmmRegister Dst,
                                          const Address &Src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0x66);
  emitAddrSizeOverridePrefix();
  emitRex(RexTypeIrrelevant, Src, Dst);
  emitUint8(0x0F);
  if (Ty == IceType_v4i32 || Ty == IceType_v4f32) {
    emitUint8(0x38);
    emitUint8(0x2B);
  } else if (Ty == IceType_v8i16) {
    emitUint8(0x67);
  } else {
    assert(false && "Unexpected vector pack operand type");
  }
  emitOperand(gprEncoding(Dst), Src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::shufps(Type /* Ty */, XmmRegister dst,
                                          XmmRegister src,
                                          const Immediate &imm) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitRexRB(RexTypeIrrelevant, dst, src);
  emitUint8(0x0F);
  emitUint8(0xC6);
  emitXmmRegisterOperand(dst, src);
  assert(imm.is_uint8());
  emitUint8(imm.value());
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::shufps(Type /* Ty */, XmmRegister dst,
                                          const Address &src,
                                          const Immediate &imm) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitAddrSizeOverridePrefix();
  emitRex(RexTypeIrrelevant, src, dst);
  emitUint8(0x0F);
  emitUint8(0xC6);
  static constexpr RelocOffsetT OffsetFromNextInstruction = 1;
  emitOperand(gprEncoding(dst), src, OffsetFromNextInstruction);
  assert(imm.is_uint8());
  emitUint8(imm.value());
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::sqrtpd(XmmRegister dst) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0x66);
  emitRexRB(RexTypeIrrelevant, dst, dst);
  emitUint8(0x0F);
  emitUint8(0x51);
  emitXmmRegisterOperand(dst, dst);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::cvtdq2ps(Type /* Ignore */, XmmRegister dst,
                                            XmmRegister src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitRexRB(RexTypeIrrelevant, dst, src);
  emitUint8(0x0F);
  emitUint8(0x5B);
  emitXmmRegisterOperand(dst, src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::cvtdq2ps(Type /* Ignore */, XmmRegister dst,
                                            const Address &src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitAddrSizeOverridePrefix();
  emitRex(RexTypeIrrelevant, src, dst);
  emitUint8(0x0F);
  emitUint8(0x5B);
  emitOperand(gprEncoding(dst), src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::cvttps2dq(Type /* Ignore */, XmmRegister dst,
                                             XmmRegister src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0xF3);
  emitRexRB(RexTypeIrrelevant, dst, src);
  emitUint8(0x0F);
  emitUint8(0x5B);
  emitXmmRegisterOperand(dst, src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::cvttps2dq(Type /* Ignore */, XmmRegister dst,
                                             const Address &src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0xF3);
  emitAddrSizeOverridePrefix();
  emitRex(RexTypeIrrelevant, src, dst);
  emitUint8(0x0F);
  emitUint8(0x5B);
  emitOperand(gprEncoding(dst), src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::cvtps2dq(Type /* Ignore */, XmmRegister dst,
                                            XmmRegister src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0x66);
  emitRexRB(RexTypeIrrelevant, dst, src);
  emitUint8(0x0F);
  emitUint8(0x5B);
  emitXmmRegisterOperand(dst, src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::cvtps2dq(Type /* Ignore */, XmmRegister dst,
                                            const Address &src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0x66);
  emitAddrSizeOverridePrefix();
  emitRex(RexTypeIrrelevant, src, dst);
  emitUint8(0x0F);
  emitUint8(0x5B);
  emitOperand(gprEncoding(dst), src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::cvtsi2ss(Type DestTy, XmmRegister dst,
                                            Type SrcTy, GPRRegister src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(isFloat32Asserting32Or64(DestTy) ? 0xF3 : 0xF2);
  emitRexRB(SrcTy, dst, src);
  emitUint8(0x0F);
  emitUint8(0x2A);
  emitXmmRegisterOperand(dst, src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::cvtsi2ss(Type DestTy, XmmRegister dst,
                                            Type SrcTy, const Address &src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(isFloat32Asserting32Or64(DestTy) ? 0xF3 : 0xF2);
  emitAddrSizeOverridePrefix();
  emitRex(SrcTy, src, dst);
  emitUint8(0x0F);
  emitUint8(0x2A);
  emitOperand(gprEncoding(dst), src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::cvtfloat2float(Type SrcTy, XmmRegister dst,
                                                  XmmRegister src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  // ss2sd or sd2ss
  emitUint8(isFloat32Asserting32Or64(SrcTy) ? 0xF3 : 0xF2);
  emitRexRB(RexTypeIrrelevant, dst, src);
  emitUint8(0x0F);
  emitUint8(0x5A);
  emitXmmRegisterOperand(dst, src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::cvtfloat2float(Type SrcTy, XmmRegister dst,
                                                  const Address &src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(isFloat32Asserting32Or64(SrcTy) ? 0xF3 : 0xF2);
  emitAddrSizeOverridePrefix();
  emitRex(RexTypeIrrelevant, src, dst);
  emitUint8(0x0F);
  emitUint8(0x5A);
  emitOperand(gprEncoding(dst), src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::cvttss2si(Type DestTy, GPRRegister dst,
                                             Type SrcTy, XmmRegister src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(isFloat32Asserting32Or64(SrcTy) ? 0xF3 : 0xF2);
  emitRexRB(DestTy, dst, src);
  emitUint8(0x0F);
  emitUint8(0x2C);
  emitXmmRegisterOperand(dst, src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::cvttss2si(Type DestTy, GPRRegister dst,
                                             Type SrcTy, const Address &src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(isFloat32Asserting32Or64(SrcTy) ? 0xF3 : 0xF2);
  emitAddrSizeOverridePrefix();
  emitRex(DestTy, src, dst);
  emitUint8(0x0F);
  emitUint8(0x2C);
  emitOperand(gprEncoding(dst), src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::cvtss2si(Type DestTy, GPRRegister dst,
                                            Type SrcTy, XmmRegister src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(isFloat32Asserting32Or64(SrcTy) ? 0xF3 : 0xF2);
  emitRexRB(DestTy, dst, src);
  emitUint8(0x0F);
  emitUint8(0x2D);
  emitXmmRegisterOperand(dst, src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::cvtss2si(Type DestTy, GPRRegister dst,
                                            Type SrcTy, const Address &src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(isFloat32Asserting32Or64(SrcTy) ? 0xF3 : 0xF2);
  emitAddrSizeOverridePrefix();
  emitRex(DestTy, src, dst);
  emitUint8(0x0F);
  emitUint8(0x2D);
  emitOperand(gprEncoding(dst), src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::ucomiss(Type Ty, XmmRegister a,
                                           XmmRegister b) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  if (Ty == IceType_f64)
    emitUint8(0x66);
  emitRexRB(RexTypeIrrelevant, a, b);
  emitUint8(0x0F);
  emitUint8(0x2E);
  emitXmmRegisterOperand(a, b);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::ucomiss(Type Ty, XmmRegister a,
                                           const Address &b) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  if (Ty == IceType_f64)
    emitUint8(0x66);
  emitAddrSizeOverridePrefix();
  emitRex(RexTypeIrrelevant, b, a);
  emitUint8(0x0F);
  emitUint8(0x2E);
  emitOperand(gprEncoding(a), b);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::movmsk(Type Ty, GPRRegister dst,
                                          XmmRegister src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  if (Ty == IceType_v16i8) {
    emitUint8(0x66);
  } else if (Ty == IceType_v4f32 || Ty == IceType_v4i32) {
    // No operand size prefix
  } else {
    assert(false && "Unexpected movmsk operand type");
  }
  emitRexRB(RexTypeIrrelevant, dst, src);
  emitUint8(0x0F);
  if (Ty == IceType_v16i8) {
    emitUint8(0xD7);
  } else if (Ty == IceType_v4f32 || Ty == IceType_v4i32) {
    emitUint8(0x50);
  } else {
    assert(false && "Unexpected movmsk operand type");
  }
  emitXmmRegisterOperand(dst, src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::sqrt(Type Ty, XmmRegister dst,
                                        const Address &src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  if (isScalarFloatingType(Ty))
    emitUint8(isFloat32Asserting32Or64(Ty) ? 0xF3 : 0xF2);
  emitAddrSizeOverridePrefix();
  emitRex(RexTypeIrrelevant, src, dst);
  emitUint8(0x0F);
  emitUint8(0x51);
  emitOperand(gprEncoding(dst), src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::sqrt(Type Ty, XmmRegister dst,
                                        XmmRegister src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  if (isScalarFloatingType(Ty))
    emitUint8(isFloat32Asserting32Or64(Ty) ? 0xF3 : 0xF2);
  emitRexRB(RexTypeIrrelevant, dst, src);
  emitUint8(0x0F);
  emitUint8(0x51);
  emitXmmRegisterOperand(dst, src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::xorps(Type Ty, XmmRegister dst,
                                         const Address &src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  if (!isFloat32Asserting32Or64(Ty))
    emitUint8(0x66);
  emitAddrSizeOverridePrefix();
  emitRex(RexTypeIrrelevant, src, dst);
  emitUint8(0x0F);
  emitUint8(0x57);
  emitOperand(gprEncoding(dst), src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::xorps(Type Ty, XmmRegister dst,
                                         XmmRegister src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  if (!isFloat32Asserting32Or64(Ty))
    emitUint8(0x66);
  emitRexRB(RexTypeIrrelevant, dst, src);
  emitUint8(0x0F);
  emitUint8(0x57);
  emitXmmRegisterOperand(dst, src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::insertps(Type Ty, XmmRegister dst,
                                            XmmRegister src,
                                            const Immediate &imm) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  assert(imm.is_uint8());
  assert(isVectorFloatingType(Ty));
  (void)Ty;
  emitUint8(0x66);
  emitRexRB(RexTypeIrrelevant, dst, src);
  emitUint8(0x0F);
  emitUint8(0x3A);
  emitUint8(0x21);
  emitXmmRegisterOperand(dst, src);
  emitUint8(imm.value());
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::insertps(Type Ty, XmmRegister dst,
                                            const Address &src,
                                            const Immediate &imm) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  assert(imm.is_uint8());
  assert(isVectorFloatingType(Ty));
  (void)Ty;
  emitUint8(0x66);
  emitAddrSizeOverridePrefix();
  emitRex(RexTypeIrrelevant, src, dst);
  emitUint8(0x0F);
  emitUint8(0x3A);
  emitUint8(0x21);
  static constexpr RelocOffsetT OffsetFromNextInstruction = 1;
  emitOperand(gprEncoding(dst), src, OffsetFromNextInstruction);
  emitUint8(imm.value());
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::pinsr(Type Ty, XmmRegister dst,
                                         GPRRegister src,
                                         const Immediate &imm) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  assert(imm.is_uint8());
  emitUint8(0x66);
  emitRexRB(Ty, dst, src);
  emitUint8(0x0F);
  if (Ty == IceType_i16) {
    emitUint8(0xC4);
  } else {
    emitUint8(0x3A);
    emitUint8(isByteSizedType(Ty) ? 0x20 : 0x22);
  }
  emitXmmRegisterOperand(dst, src);
  emitUint8(imm.value());
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::pinsr(Type Ty, XmmRegister dst,
                                         const Address &src,
                                         const Immediate &imm) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  assert(imm.is_uint8());
  emitUint8(0x66);
  emitAddrSizeOverridePrefix();
  emitRex(RexTypeIrrelevant, src, dst);
  emitUint8(0x0F);
  if (Ty == IceType_i16) {
    emitUint8(0xC4);
  } else {
    emitUint8(0x3A);
    emitUint8(isByteSizedType(Ty) ? 0x20 : 0x22);
  }
  static constexpr RelocOffsetT OffsetFromNextInstruction = 1;
  emitOperand(gprEncoding(dst), src, OffsetFromNextInstruction);
  emitUint8(imm.value());
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::pextr(Type Ty, GPRRegister dst,
                                         XmmRegister src,
                                         const Immediate &imm) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  assert(imm.is_uint8());
  if (Ty == IceType_i16) {
    emitUint8(0x66);
    emitRexRB(Ty, dst, src);
    emitUint8(0x0F);
    emitUint8(0xC5);
    emitXmmRegisterOperand(dst, src);
    emitUint8(imm.value());
  } else {
    emitUint8(0x66);
    emitRexRB(Ty, src, dst);
    emitUint8(0x0F);
    emitUint8(0x3A);
    emitUint8(isByteSizedType(Ty) ? 0x14 : 0x16);
    // SSE 4.1 versions are "MRI" because dst can be mem, while pextrw (SSE2)
    // is RMI because dst must be reg.
    emitXmmRegisterOperand(src, dst);
    emitUint8(imm.value());
  }
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::pmovsxdq(XmmRegister dst, XmmRegister src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0x66);
  emitRexRB(RexTypeIrrelevant, dst, src);
  emitUint8(0x0F);
  emitUint8(0x38);
  emitUint8(0x25);
  emitXmmRegisterOperand(dst, src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::pcmpeq(Type Ty, XmmRegister dst,
                                          XmmRegister src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0x66);
  emitRexRB(RexTypeIrrelevant, dst, src);
  emitUint8(0x0F);
  if (isByteSizedArithType(Ty)) {
    emitUint8(0x74);
  } else if (Ty == IceType_i16) {
    emitUint8(0x75);
  } else {
    emitUint8(0x76);
  }
  emitXmmRegisterOperand(dst, src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::pcmpeq(Type Ty, XmmRegister dst,
                                          const Address &src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0x66);
  emitAddrSizeOverridePrefix();
  emitRex(RexTypeIrrelevant, src, dst);
  emitUint8(0x0F);
  if (isByteSizedArithType(Ty)) {
    emitUint8(0x74);
  } else if (Ty == IceType_i16) {
    emitUint8(0x75);
  } else {
    emitUint8(0x76);
  }
  emitOperand(gprEncoding(dst), src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::pcmpgt(Type Ty, XmmRegister dst,
                                          XmmRegister src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0x66);
  emitRexRB(RexTypeIrrelevant, dst, src);
  emitUint8(0x0F);
  if (isByteSizedArithType(Ty)) {
    emitUint8(0x64);
  } else if (Ty == IceType_i16) {
    emitUint8(0x65);
  } else {
    emitUint8(0x66);
  }
  emitXmmRegisterOperand(dst, src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::pcmpgt(Type Ty, XmmRegister dst,
                                          const Address &src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0x66);
  emitAddrSizeOverridePrefix();
  emitRex(RexTypeIrrelevant, src, dst);
  emitUint8(0x0F);
  if (isByteSizedArithType(Ty)) {
    emitUint8(0x64);
  } else if (Ty == IceType_i16) {
    emitUint8(0x65);
  } else {
    emitUint8(0x66);
  }
  emitOperand(gprEncoding(dst), src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::round(Type Ty, XmmRegister dst,
                                         XmmRegister src,
                                         const Immediate &mode) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0x66);
  emitRexRB(RexTypeIrrelevant, dst, src);
  emitUint8(0x0F);
  emitUint8(0x3A);
  switch (Ty) {
  case IceType_v4f32:
    emitUint8(0x08);
    break;
  case IceType_f32:
    emitUint8(0x0A);
    break;
  case IceType_f64:
    emitUint8(0x0B);
    break;
  default:
    assert(false && "Unsupported round operand type");
  }
  emitXmmRegisterOperand(dst, src);
  // Mask precision exeption.
  emitUint8(static_cast<uint8_t>(mode.value()) | 0x8);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::round(Type Ty, XmmRegister dst,
                                         const Address &src,
                                         const Immediate &mode) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0x66);
  emitAddrSizeOverridePrefix();
  emitRex(RexTypeIrrelevant, src, dst);
  emitUint8(0x0F);
  emitUint8(0x3A);
  switch (Ty) {
  case IceType_v4f32:
    emitUint8(0x08);
    break;
  case IceType_f32:
    emitUint8(0x0A);
    break;
  case IceType_f64:
    emitUint8(0x0B);
    break;
  default:
    assert(false && "Unsupported round operand type");
  }
  emitOperand(gprEncoding(dst), src);
  // Mask precision exeption.
  emitUint8(static_cast<uint8_t>(mode.value()) | 0x8);
}

template <typename TraitsType>
template <typename T, typename>
void AssemblerX86Base<TraitsType>::fnstcw(const typename T::Address &dst) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitAddrSizeOverridePrefix();
  emitUint8(0xD9);
  emitOperand(7, dst);
}

template <typename TraitsType>
template <typename T, typename>
void AssemblerX86Base<TraitsType>::fldcw(const typename T::Address &src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitAddrSizeOverridePrefix();
  emitUint8(0xD9);
  emitOperand(5, src);
}

template <typename TraitsType>
template <typename T, typename>
void AssemblerX86Base<TraitsType>::fistpl(const typename T::Address &dst) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitAddrSizeOverridePrefix();
  emitUint8(0xDF);
  emitOperand(7, dst);
}

template <typename TraitsType>
template <typename T, typename>
void AssemblerX86Base<TraitsType>::fistps(const typename T::Address &dst) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitAddrSizeOverridePrefix();
  emitUint8(0xDB);
  emitOperand(3, dst);
}

template <typename TraitsType>
template <typename T, typename>
void AssemblerX86Base<TraitsType>::fildl(const typename T::Address &src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitAddrSizeOverridePrefix();
  emitUint8(0xDF);
  emitOperand(5, src);
}

template <typename TraitsType>
template <typename T, typename>
void AssemblerX86Base<TraitsType>::filds(const typename T::Address &src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitAddrSizeOverridePrefix();
  emitUint8(0xDB);
  emitOperand(0, src);
}

template <typename TraitsType>
template <typename, typename>
void AssemblerX86Base<TraitsType>::fincstp() {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0xD9);
  emitUint8(0xF7);
}

template <typename TraitsType>
template <uint32_t Tag>
void AssemblerX86Base<TraitsType>::arith_int(Type Ty, GPRRegister reg,
                                             const Immediate &imm) {
  static_assert(Tag < 8, "Tag must be between 0..7");
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  if (Ty == IceType_i16)
    emitOperandSizeOverride();
  emitRexB(Ty, reg);
  if (isByteSizedType(Ty)) {
    emitComplexI8(Tag, Operand(reg), imm);
  } else {
    emitComplex(Ty, Tag, Operand(reg), imm);
  }
}

template <typename TraitsType>
template <uint32_t Tag>
void AssemblerX86Base<TraitsType>::arith_int(Type Ty, GPRRegister reg0,
                                             GPRRegister reg1) {
  static_assert(Tag < 8, "Tag must be between 0..7");
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  if (Ty == IceType_i16)
    emitOperandSizeOverride();
  emitRexRB(Ty, reg0, reg1);
  if (isByteSizedType(Ty))
    emitUint8(Tag * 8 + 2);
  else
    emitUint8(Tag * 8 + 3);
  emitRegisterOperand(gprEncoding(reg0), gprEncoding(reg1));
}

template <typename TraitsType>
template <uint32_t Tag>
void AssemblerX86Base<TraitsType>::arith_int(Type Ty, GPRRegister reg,
                                             const Address &address) {
  static_assert(Tag < 8, "Tag must be between 0..7");
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  if (Ty == IceType_i16)
    emitOperandSizeOverride();
  emitAddrSizeOverridePrefix();
  emitRex(Ty, address, reg);
  if (isByteSizedType(Ty))
    emitUint8(Tag * 8 + 2);
  else
    emitUint8(Tag * 8 + 3);
  emitOperand(gprEncoding(reg), address);
}

template <typename TraitsType>
template <uint32_t Tag>
void AssemblerX86Base<TraitsType>::arith_int(Type Ty, const Address &address,
                                             GPRRegister reg) {
  static_assert(Tag < 8, "Tag must be between 0..7");
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  if (Ty == IceType_i16)
    emitOperandSizeOverride();
  emitAddrSizeOverridePrefix();
  emitRex(Ty, address, reg);
  if (isByteSizedType(Ty))
    emitUint8(Tag * 8 + 0);
  else
    emitUint8(Tag * 8 + 1);
  emitOperand(gprEncoding(reg), address);
}

template <typename TraitsType>
template <uint32_t Tag>
void AssemblerX86Base<TraitsType>::arith_int(Type Ty, const Address &address,
                                             const Immediate &imm) {
  static_assert(Tag < 8, "Tag must be between 0..7");
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  if (Ty == IceType_i16)
    emitOperandSizeOverride();
  emitAddrSizeOverridePrefix();
  emitRex(Ty, address, RexRegIrrelevant);
  if (isByteSizedType(Ty)) {
    emitComplexI8(Tag, address, imm);
  } else {
    emitComplex(Ty, Tag, address, imm);
  }
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::cmp(Type Ty, GPRRegister reg,
                                       const Immediate &imm) {
  arith_int<7>(Ty, reg, imm);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::cmp(Type Ty, GPRRegister reg0,
                                       GPRRegister reg1) {
  arith_int<7>(Ty, reg0, reg1);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::cmp(Type Ty, GPRRegister reg,
                                       const Address &address) {
  arith_int<7>(Ty, reg, address);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::cmp(Type Ty, const Address &address,
                                       GPRRegister reg) {
  arith_int<7>(Ty, address, reg);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::cmp(Type Ty, const Address &address,
                                       const Immediate &imm) {
  arith_int<7>(Ty, address, imm);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::test(Type Ty, GPRRegister reg1,
                                        GPRRegister reg2) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  if (Ty == IceType_i16)
    emitOperandSizeOverride();
  emitRexRB(Ty, reg1, reg2);
  if (isByteSizedType(Ty))
    emitUint8(0x84);
  else
    emitUint8(0x85);
  emitRegisterOperand(gprEncoding(reg1), gprEncoding(reg2));
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::test(Type Ty, const Address &addr,
                                        GPRRegister reg) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  if (Ty == IceType_i16)
    emitOperandSizeOverride();
  emitAddrSizeOverridePrefix();
  emitRex(Ty, addr, reg);
  if (isByteSizedType(Ty))
    emitUint8(0x84);
  else
    emitUint8(0x85);
  emitOperand(gprEncoding(reg), addr);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::test(Type Ty, GPRRegister reg,
                                        const Immediate &immediate) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  // For registers that have a byte variant (EAX, EBX, ECX, and EDX) we only
  // test the byte register to keep the encoding short. This is legal even if
  // the register had high bits set since this only sets flags registers based
  // on the "AND" of the two operands, and the immediate had zeros at those
  // high bits.
  if (immediate.is_uint8() && reg <= Traits::Last8BitGPR) {
    // Use zero-extended 8-bit immediate.
    emitRexB(Ty, reg);
    if (reg == Traits::Encoded_Reg_Accumulator) {
      emitUint8(0xA8);
    } else {
      emitUint8(0xF6);
      emitUint8(0xC0 + gprEncoding(reg));
    }
    emitUint8(immediate.value() & 0xFF);
  } else if (reg == Traits::Encoded_Reg_Accumulator) {
    // Use short form if the destination is EAX.
    if (Ty == IceType_i16)
      emitOperandSizeOverride();
    emitUint8(0xA9);
    emitImmediate(Ty, immediate);
  } else {
    if (Ty == IceType_i16)
      emitOperandSizeOverride();
    emitRexB(Ty, reg);
    emitUint8(0xF7);
    emitRegisterOperand(0, gprEncoding(reg));
    emitImmediate(Ty, immediate);
  }
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::test(Type Ty, const Address &addr,
                                        const Immediate &immediate) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  // If the immediate is short, we only test the byte addr to keep the encoding
  // short.
  if (immediate.is_uint8()) {
    // Use zero-extended 8-bit immediate.
    emitAddrSizeOverridePrefix();
    emitRex(Ty, addr, RexRegIrrelevant);
    emitUint8(0xF6);
    static constexpr RelocOffsetT OffsetFromNextInstruction = 1;
    emitOperand(0, addr, OffsetFromNextInstruction);
    emitUint8(immediate.value() & 0xFF);
  } else {
    if (Ty == IceType_i16)
      emitOperandSizeOverride();
    emitAddrSizeOverridePrefix();
    emitRex(Ty, addr, RexRegIrrelevant);
    emitUint8(0xF7);
    const uint8_t OffsetFromNextInstruction = Ty == IceType_i16 ? 2 : 4;
    emitOperand(0, addr, OffsetFromNextInstruction);
    emitImmediate(Ty, immediate);
  }
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::And(Type Ty, GPRRegister dst,
                                       GPRRegister src) {
  arith_int<4>(Ty, dst, src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::And(Type Ty, GPRRegister dst,
                                       const Address &address) {
  arith_int<4>(Ty, dst, address);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::And(Type Ty, GPRRegister dst,
                                       const Immediate &imm) {
  arith_int<4>(Ty, dst, imm);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::And(Type Ty, const Address &address,
                                       GPRRegister reg) {
  arith_int<4>(Ty, address, reg);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::And(Type Ty, const Address &address,
                                       const Immediate &imm) {
  arith_int<4>(Ty, address, imm);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::Or(Type Ty, GPRRegister dst,
                                      GPRRegister src) {
  arith_int<1>(Ty, dst, src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::Or(Type Ty, GPRRegister dst,
                                      const Address &address) {
  arith_int<1>(Ty, dst, address);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::Or(Type Ty, GPRRegister dst,
                                      const Immediate &imm) {
  arith_int<1>(Ty, dst, imm);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::Or(Type Ty, const Address &address,
                                      GPRRegister reg) {
  arith_int<1>(Ty, address, reg);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::Or(Type Ty, const Address &address,
                                      const Immediate &imm) {
  arith_int<1>(Ty, address, imm);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::Xor(Type Ty, GPRRegister dst,
                                       GPRRegister src) {
  arith_int<6>(Ty, dst, src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::Xor(Type Ty, GPRRegister dst,
                                       const Address &address) {
  arith_int<6>(Ty, dst, address);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::Xor(Type Ty, GPRRegister dst,
                                       const Immediate &imm) {
  arith_int<6>(Ty, dst, imm);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::Xor(Type Ty, const Address &address,
                                       GPRRegister reg) {
  arith_int<6>(Ty, address, reg);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::Xor(Type Ty, const Address &address,
                                       const Immediate &imm) {
  arith_int<6>(Ty, address, imm);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::add(Type Ty, GPRRegister dst,
                                       GPRRegister src) {
  arith_int<0>(Ty, dst, src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::add(Type Ty, GPRRegister reg,
                                       const Address &address) {
  arith_int<0>(Ty, reg, address);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::add(Type Ty, GPRRegister reg,
                                       const Immediate &imm) {
  arith_int<0>(Ty, reg, imm);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::add(Type Ty, const Address &address,
                                       GPRRegister reg) {
  arith_int<0>(Ty, address, reg);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::add(Type Ty, const Address &address,
                                       const Immediate &imm) {
  arith_int<0>(Ty, address, imm);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::adc(Type Ty, GPRRegister dst,
                                       GPRRegister src) {
  arith_int<2>(Ty, dst, src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::adc(Type Ty, GPRRegister dst,
                                       const Address &address) {
  arith_int<2>(Ty, dst, address);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::adc(Type Ty, GPRRegister reg,
                                       const Immediate &imm) {
  arith_int<2>(Ty, reg, imm);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::adc(Type Ty, const Address &address,
                                       GPRRegister reg) {
  arith_int<2>(Ty, address, reg);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::adc(Type Ty, const Address &address,
                                       const Immediate &imm) {
  arith_int<2>(Ty, address, imm);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::sub(Type Ty, GPRRegister dst,
                                       GPRRegister src) {
  arith_int<5>(Ty, dst, src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::sub(Type Ty, GPRRegister reg,
                                       const Address &address) {
  arith_int<5>(Ty, reg, address);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::sub(Type Ty, GPRRegister reg,
                                       const Immediate &imm) {
  arith_int<5>(Ty, reg, imm);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::sub(Type Ty, const Address &address,
                                       GPRRegister reg) {
  arith_int<5>(Ty, address, reg);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::sub(Type Ty, const Address &address,
                                       const Immediate &imm) {
  arith_int<5>(Ty, address, imm);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::sbb(Type Ty, GPRRegister dst,
                                       GPRRegister src) {
  arith_int<3>(Ty, dst, src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::sbb(Type Ty, GPRRegister dst,
                                       const Address &address) {
  arith_int<3>(Ty, dst, address);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::sbb(Type Ty, GPRRegister reg,
                                       const Immediate &imm) {
  arith_int<3>(Ty, reg, imm);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::sbb(Type Ty, const Address &address,
                                       GPRRegister reg) {
  arith_int<3>(Ty, address, reg);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::sbb(Type Ty, const Address &address,
                                       const Immediate &imm) {
  arith_int<3>(Ty, address, imm);
}

template <typename TraitsType> void AssemblerX86Base<TraitsType>::cbw() {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitOperandSizeOverride();
  emitUint8(0x98);
}

template <typename TraitsType> void AssemblerX86Base<TraitsType>::cwd() {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitOperandSizeOverride();
  emitUint8(0x99);
}

template <typename TraitsType> void AssemblerX86Base<TraitsType>::cdq() {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0x99);
}

template <typename TraitsType>
template <typename T>
typename std::enable_if<T::Is64Bit, void>::type
AssemblerX86Base<TraitsType>::cqo() {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitRexB(RexTypeForceRexW, RexRegIrrelevant);
  emitUint8(0x99);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::div(Type Ty, GPRRegister reg) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  if (Ty == IceType_i16)
    emitOperandSizeOverride();
  emitRexB(Ty, reg);
  if (isByteSizedArithType(Ty))
    emitUint8(0xF6);
  else
    emitUint8(0xF7);
  emitRegisterOperand(6, gprEncoding(reg));
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::div(Type Ty, const Address &addr) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  if (Ty == IceType_i16)
    emitOperandSizeOverride();
  emitAddrSizeOverridePrefix();
  emitRex(Ty, addr, RexRegIrrelevant);
  if (isByteSizedArithType(Ty))
    emitUint8(0xF6);
  else
    emitUint8(0xF7);
  emitOperand(6, addr);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::idiv(Type Ty, GPRRegister reg) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  if (Ty == IceType_i16)
    emitOperandSizeOverride();
  emitRexB(Ty, reg);
  if (isByteSizedArithType(Ty))
    emitUint8(0xF6);
  else
    emitUint8(0xF7);
  emitRegisterOperand(7, gprEncoding(reg));
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::idiv(Type Ty, const Address &addr) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  if (Ty == IceType_i16)
    emitOperandSizeOverride();
  emitAddrSizeOverridePrefix();
  emitRex(Ty, addr, RexRegIrrelevant);
  if (isByteSizedArithType(Ty))
    emitUint8(0xF6);
  else
    emitUint8(0xF7);
  emitOperand(7, addr);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::imul(Type Ty, GPRRegister dst,
                                        GPRRegister src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  assert(Ty == IceType_i16 || Ty == IceType_i32 ||
         (Traits::Is64Bit && Ty == IceType_i64));
  if (Ty == IceType_i16)
    emitOperandSizeOverride();
  emitRexRB(Ty, dst, src);
  emitUint8(0x0F);
  emitUint8(0xAF);
  emitRegisterOperand(gprEncoding(dst), gprEncoding(src));
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::imul(Type Ty, GPRRegister reg,
                                        const Address &address) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  assert(Ty == IceType_i16 || Ty == IceType_i32 ||
         (Traits::Is64Bit && Ty == IceType_i64));
  if (Ty == IceType_i16)
    emitOperandSizeOverride();
  emitAddrSizeOverridePrefix();
  emitRex(Ty, address, reg);
  emitUint8(0x0F);
  emitUint8(0xAF);
  emitOperand(gprEncoding(reg), address);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::imul(Type Ty, GPRRegister reg,
                                        const Immediate &imm) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  assert(Ty == IceType_i16 || Ty == IceType_i32 || Ty == IceType_i64);
  if (Ty == IceType_i16)
    emitOperandSizeOverride();
  emitRexRB(Ty, reg, reg);
  if (imm.is_int8()) {
    emitUint8(0x6B);
    emitRegisterOperand(gprEncoding(reg), gprEncoding(reg));
    emitUint8(imm.value() & 0xFF);
  } else {
    emitUint8(0x69);
    emitRegisterOperand(gprEncoding(reg), gprEncoding(reg));
    emitImmediate(Ty, imm);
  }
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::imul(Type Ty, GPRRegister reg) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  if (Ty == IceType_i16)
    emitOperandSizeOverride();
  emitRexB(Ty, reg);
  if (isByteSizedArithType(Ty))
    emitUint8(0xF6);
  else
    emitUint8(0xF7);
  emitRegisterOperand(5, gprEncoding(reg));
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::imul(Type Ty, const Address &address) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  if (Ty == IceType_i16)
    emitOperandSizeOverride();
  emitAddrSizeOverridePrefix();
  emitRex(Ty, address, RexRegIrrelevant);
  if (isByteSizedArithType(Ty))
    emitUint8(0xF6);
  else
    emitUint8(0xF7);
  emitOperand(5, address);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::imul(Type Ty, GPRRegister dst,
                                        GPRRegister src, const Immediate &imm) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  assert(Ty == IceType_i16 || Ty == IceType_i32);
  if (Ty == IceType_i16)
    emitOperandSizeOverride();
  emitRexRB(Ty, dst, src);
  if (imm.is_int8()) {
    emitUint8(0x6B);
    emitRegisterOperand(gprEncoding(dst), gprEncoding(src));
    emitUint8(imm.value() & 0xFF);
  } else {
    emitUint8(0x69);
    emitRegisterOperand(gprEncoding(dst), gprEncoding(src));
    emitImmediate(Ty, imm);
  }
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::imul(Type Ty, GPRRegister dst,
                                        const Address &address,
                                        const Immediate &imm) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  assert(Ty == IceType_i16 || Ty == IceType_i32);
  if (Ty == IceType_i16)
    emitOperandSizeOverride();
  emitAddrSizeOverridePrefix();
  emitRex(Ty, address, dst);
  if (imm.is_int8()) {
    emitUint8(0x6B);
    static constexpr RelocOffsetT OffsetFromNextInstruction = 1;
    emitOperand(gprEncoding(dst), address, OffsetFromNextInstruction);
    emitUint8(imm.value() & 0xFF);
  } else {
    emitUint8(0x69);
    const uint8_t OffsetFromNextInstruction = Ty == IceType_i16 ? 2 : 4;
    emitOperand(gprEncoding(dst), address, OffsetFromNextInstruction);
    emitImmediate(Ty, imm);
  }
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::mul(Type Ty, GPRRegister reg) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  if (Ty == IceType_i16)
    emitOperandSizeOverride();
  emitRexB(Ty, reg);
  if (isByteSizedArithType(Ty))
    emitUint8(0xF6);
  else
    emitUint8(0xF7);
  emitRegisterOperand(4, gprEncoding(reg));
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::mul(Type Ty, const Address &address) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  if (Ty == IceType_i16)
    emitOperandSizeOverride();
  emitAddrSizeOverridePrefix();
  emitRex(Ty, address, RexRegIrrelevant);
  if (isByteSizedArithType(Ty))
    emitUint8(0xF6);
  else
    emitUint8(0xF7);
  emitOperand(4, address);
}

template <typename TraitsType>
template <typename, typename>
void AssemblerX86Base<TraitsType>::incl(GPRRegister reg) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0x40 + reg);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::incl(const Address &address) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitAddrSizeOverridePrefix();
  emitRex(IceType_i32, address, RexRegIrrelevant);
  emitUint8(0xFF);
  emitOperand(0, address);
}

template <typename TraitsType>
template <typename, typename>
void AssemblerX86Base<TraitsType>::decl(GPRRegister reg) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0x48 + reg);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::decl(const Address &address) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitAddrSizeOverridePrefix();
  emitRex(IceType_i32, address, RexRegIrrelevant);
  emitUint8(0xFF);
  emitOperand(1, address);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::rol(Type Ty, GPRRegister reg,
                                       const Immediate &imm) {
  emitGenericShift(0, Ty, reg, imm);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::rol(Type Ty, GPRRegister operand,
                                       GPRRegister shifter) {
  emitGenericShift(0, Ty, Operand(operand), shifter);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::rol(Type Ty, const Address &operand,
                                       GPRRegister shifter) {
  emitGenericShift(0, Ty, operand, shifter);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::shl(Type Ty, GPRRegister reg,
                                       const Immediate &imm) {
  emitGenericShift(4, Ty, reg, imm);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::shl(Type Ty, GPRRegister operand,
                                       GPRRegister shifter) {
  emitGenericShift(4, Ty, Operand(operand), shifter);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::shl(Type Ty, const Address &operand,
                                       GPRRegister shifter) {
  emitGenericShift(4, Ty, operand, shifter);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::shr(Type Ty, GPRRegister reg,
                                       const Immediate &imm) {
  emitGenericShift(5, Ty, reg, imm);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::shr(Type Ty, GPRRegister operand,
                                       GPRRegister shifter) {
  emitGenericShift(5, Ty, Operand(operand), shifter);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::shr(Type Ty, const Address &operand,
                                       GPRRegister shifter) {
  emitGenericShift(5, Ty, operand, shifter);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::sar(Type Ty, GPRRegister reg,
                                       const Immediate &imm) {
  emitGenericShift(7, Ty, reg, imm);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::sar(Type Ty, GPRRegister operand,
                                       GPRRegister shifter) {
  emitGenericShift(7, Ty, Operand(operand), shifter);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::sar(Type Ty, const Address &address,
                                       GPRRegister shifter) {
  emitGenericShift(7, Ty, address, shifter);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::shld(Type Ty, GPRRegister dst,
                                        GPRRegister src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  assert(Ty == IceType_i16 || Ty == IceType_i32);
  if (Ty == IceType_i16)
    emitOperandSizeOverride();
  emitRexRB(Ty, src, dst);
  emitUint8(0x0F);
  emitUint8(0xA5);
  emitRegisterOperand(gprEncoding(src), gprEncoding(dst));
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::shld(Type Ty, GPRRegister dst,
                                        GPRRegister src, const Immediate &imm) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  assert(Ty == IceType_i16 || Ty == IceType_i32);
  assert(imm.is_int8());
  if (Ty == IceType_i16)
    emitOperandSizeOverride();
  emitRexRB(Ty, src, dst);
  emitUint8(0x0F);
  emitUint8(0xA4);
  emitRegisterOperand(gprEncoding(src), gprEncoding(dst));
  emitUint8(imm.value() & 0xFF);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::shld(Type Ty, const Address &operand,
                                        GPRRegister src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  assert(Ty == IceType_i16 || Ty == IceType_i32);
  if (Ty == IceType_i16)
    emitOperandSizeOverride();
  emitAddrSizeOverridePrefix();
  emitRex(Ty, operand, src);
  emitUint8(0x0F);
  emitUint8(0xA5);
  emitOperand(gprEncoding(src), operand);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::shrd(Type Ty, GPRRegister dst,
                                        GPRRegister src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  assert(Ty == IceType_i16 || Ty == IceType_i32);
  if (Ty == IceType_i16)
    emitOperandSizeOverride();
  emitRexRB(Ty, src, dst);
  emitUint8(0x0F);
  emitUint8(0xAD);
  emitRegisterOperand(gprEncoding(src), gprEncoding(dst));
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::shrd(Type Ty, GPRRegister dst,
                                        GPRRegister src, const Immediate &imm) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  assert(Ty == IceType_i16 || Ty == IceType_i32);
  assert(imm.is_int8());
  if (Ty == IceType_i16)
    emitOperandSizeOverride();
  emitRexRB(Ty, src, dst);
  emitUint8(0x0F);
  emitUint8(0xAC);
  emitRegisterOperand(gprEncoding(src), gprEncoding(dst));
  emitUint8(imm.value() & 0xFF);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::shrd(Type Ty, const Address &dst,
                                        GPRRegister src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  assert(Ty == IceType_i16 || Ty == IceType_i32);
  if (Ty == IceType_i16)
    emitOperandSizeOverride();
  emitAddrSizeOverridePrefix();
  emitRex(Ty, dst, src);
  emitUint8(0x0F);
  emitUint8(0xAD);
  emitOperand(gprEncoding(src), dst);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::neg(Type Ty, GPRRegister reg) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  if (Ty == IceType_i16)
    emitOperandSizeOverride();
  emitRexB(Ty, reg);
  if (isByteSizedArithType(Ty))
    emitUint8(0xF6);
  else
    emitUint8(0xF7);
  emitRegisterOperand(3, gprEncoding(reg));
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::neg(Type Ty, const Address &addr) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  if (Ty == IceType_i16)
    emitOperandSizeOverride();
  emitAddrSizeOverridePrefix();
  emitRex(Ty, addr, RexRegIrrelevant);
  if (isByteSizedArithType(Ty))
    emitUint8(0xF6);
  else
    emitUint8(0xF7);
  emitOperand(3, addr);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::notl(GPRRegister reg) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitRexB(IceType_i32, reg);
  emitUint8(0xF7);
  emitUint8(0xD0 | gprEncoding(reg));
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::bswap(Type Ty, GPRRegister reg) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  assert(Ty == IceType_i32 || (Traits::Is64Bit && Ty == IceType_i64));
  emitRexB(Ty, reg);
  emitUint8(0x0F);
  emitUint8(0xC8 | gprEncoding(reg));
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::bsf(Type Ty, GPRRegister dst,
                                       GPRRegister src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  assert(Ty == IceType_i16 || Ty == IceType_i32 ||
         (Traits::Is64Bit && Ty == IceType_i64));
  if (Ty == IceType_i16)
    emitOperandSizeOverride();
  emitRexRB(Ty, dst, src);
  emitUint8(0x0F);
  emitUint8(0xBC);
  emitRegisterOperand(gprEncoding(dst), gprEncoding(src));
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::bsf(Type Ty, GPRRegister dst,
                                       const Address &src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  assert(Ty == IceType_i16 || Ty == IceType_i32 ||
         (Traits::Is64Bit && Ty == IceType_i64));
  if (Ty == IceType_i16)
    emitOperandSizeOverride();
  emitAddrSizeOverridePrefix();
  emitRex(Ty, src, dst);
  emitUint8(0x0F);
  emitUint8(0xBC);
  emitOperand(gprEncoding(dst), src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::bsr(Type Ty, GPRRegister dst,
                                       GPRRegister src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  assert(Ty == IceType_i16 || Ty == IceType_i32 ||
         (Traits::Is64Bit && Ty == IceType_i64));
  if (Ty == IceType_i16)
    emitOperandSizeOverride();
  emitRexRB(Ty, dst, src);
  emitUint8(0x0F);
  emitUint8(0xBD);
  emitRegisterOperand(gprEncoding(dst), gprEncoding(src));
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::bsr(Type Ty, GPRRegister dst,
                                       const Address &src) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  assert(Ty == IceType_i16 || Ty == IceType_i32 ||
         (Traits::Is64Bit && Ty == IceType_i64));
  if (Ty == IceType_i16)
    emitOperandSizeOverride();
  emitAddrSizeOverridePrefix();
  emitRex(Ty, src, dst);
  emitUint8(0x0F);
  emitUint8(0xBD);
  emitOperand(gprEncoding(dst), src);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::bt(GPRRegister base, GPRRegister offset) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitRexRB(IceType_i32, offset, base);
  emitUint8(0x0F);
  emitUint8(0xA3);
  emitRegisterOperand(gprEncoding(offset), gprEncoding(base));
}

template <typename TraitsType> void AssemblerX86Base<TraitsType>::ret() {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0xC3);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::ret(const Immediate &imm) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0xC2);
  assert(imm.is_uint16());
  emitUint8(imm.value() & 0xFF);
  emitUint8((imm.value() >> 8) & 0xFF);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::nop(int size) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  // There are nops up to size 15, but for now just provide up to size 8.
  assert(0 < size && size <= MAX_NOP_SIZE);
  switch (size) {
  case 1:
    emitUint8(0x90);
    break;
  case 2:
    emitUint8(0x66);
    emitUint8(0x90);
    break;
  case 3:
    emitUint8(0x0F);
    emitUint8(0x1F);
    emitUint8(0x00);
    break;
  case 4:
    emitUint8(0x0F);
    emitUint8(0x1F);
    emitUint8(0x40);
    emitUint8(0x00);
    break;
  case 5:
    emitUint8(0x0F);
    emitUint8(0x1F);
    emitUint8(0x44);
    emitUint8(0x00);
    emitUint8(0x00);
    break;
  case 6:
    emitUint8(0x66);
    emitUint8(0x0F);
    emitUint8(0x1F);
    emitUint8(0x44);
    emitUint8(0x00);
    emitUint8(0x00);
    break;
  case 7:
    emitUint8(0x0F);
    emitUint8(0x1F);
    emitUint8(0x80);
    emitUint8(0x00);
    emitUint8(0x00);
    emitUint8(0x00);
    emitUint8(0x00);
    break;
  case 8:
    emitUint8(0x0F);
    emitUint8(0x1F);
    emitUint8(0x84);
    emitUint8(0x00);
    emitUint8(0x00);
    emitUint8(0x00);
    emitUint8(0x00);
    emitUint8(0x00);
    break;
  default:
    llvm_unreachable("Unimplemented");
  }
}

template <typename TraitsType> void AssemblerX86Base<TraitsType>::int3() {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0xCC);
}

template <typename TraitsType> void AssemblerX86Base<TraitsType>::hlt() {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0xF4);
}

template <typename TraitsType> void AssemblerX86Base<TraitsType>::ud2() {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0x0F);
  emitUint8(0x0B);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::j(BrCond condition, Label *label,
                                     bool near) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  if (label->isBound()) {
    static const int kShortSize = 2;
    static const int kLongSize = 6;
    intptr_t offset = label->getPosition() - Buffer.size();
    assert(offset <= 0);
    if (Utils::IsInt(8, offset - kShortSize)) {
      // TODO(stichnot): Here and in jmp(), we may need to be more
      // conservative about the backward branch distance if the branch
      // instruction is within a bundle_lock sequence, because the
      // distance may increase when padding is added. This isn't an issue for
      // branches outside a bundle_lock, because if padding is added, the retry
      // may change it to a long backward branch without affecting any of the
      // bookkeeping.
      emitUint8(0x70 + condition);
      emitUint8((offset - kShortSize) & 0xFF);
    } else {
      emitUint8(0x0F);
      emitUint8(0x80 + condition);
      emitInt32(offset - kLongSize);
    }
  } else if (near) {
    emitUint8(0x70 + condition);
    emitNearLabelLink(label);
  } else {
    emitUint8(0x0F);
    emitUint8(0x80 + condition);
    emitLabelLink(label);
  }
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::j(BrCond condition,
                                     const ConstantRelocatable *label) {
  llvm::report_fatal_error("Untested - please verify and then reenable.");
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0x0F);
  emitUint8(0x80 + condition);
  auto *Fixup = this->createFixup(Traits::FK_PcRel, label);
  Fixup->set_addend(-4);
  emitFixup(Fixup);
  emitInt32(0);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::jmp(GPRRegister reg) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitRexB(RexTypeIrrelevant, reg);
  emitUint8(0xFF);
  emitRegisterOperand(4, gprEncoding(reg));
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::jmp(Label *label, bool near) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  if (label->isBound()) {
    static const int kShortSize = 2;
    static const int kLongSize = 5;
    intptr_t offset = label->getPosition() - Buffer.size();
    assert(offset <= 0);
    if (Utils::IsInt(8, offset - kShortSize)) {
      emitUint8(0xEB);
      emitUint8((offset - kShortSize) & 0xFF);
    } else {
      emitUint8(0xE9);
      emitInt32(offset - kLongSize);
    }
  } else if (near) {
    emitUint8(0xEB);
    emitNearLabelLink(label);
  } else {
    emitUint8(0xE9);
    emitLabelLink(label);
  }
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::jmp(const ConstantRelocatable *label) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0xE9);
  auto *Fixup = this->createFixup(Traits::FK_PcRel, label);
  Fixup->set_addend(-4);
  emitFixup(Fixup);
  emitInt32(0);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::jmp(const Immediate &abs_address) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0xE9);
  AssemblerFixup *Fixup =
      createFixup(Traits::FK_PcRel, AssemblerFixup::NullSymbol);
  Fixup->set_addend(abs_address.value() - 4);
  emitFixup(Fixup);
  emitInt32(0);
}

template <typename TraitsType> void AssemblerX86Base<TraitsType>::mfence() {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0x0F);
  emitUint8(0xAE);
  emitUint8(0xF0);
}

template <typename TraitsType> void AssemblerX86Base<TraitsType>::lock() {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0xF0);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::cmpxchg(Type Ty, const Address &address,
                                           GPRRegister reg, bool Locked) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  if (Ty == IceType_i16)
    emitOperandSizeOverride();
  if (Locked)
    emitUint8(0xF0);
  emitAddrSizeOverridePrefix();
  emitRex(Ty, address, reg);
  emitUint8(0x0F);
  if (isByteSizedArithType(Ty))
    emitUint8(0xB0);
  else
    emitUint8(0xB1);
  emitOperand(gprEncoding(reg), address);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::cmpxchg8b(const Address &address,
                                             bool Locked) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  if (Locked)
    emitUint8(0xF0);
  emitAddrSizeOverridePrefix();
  emitRex(IceType_i32, address, RexRegIrrelevant);
  emitUint8(0x0F);
  emitUint8(0xC7);
  emitOperand(1, address);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::xadd(Type Ty, const Address &addr,
                                        GPRRegister reg, bool Locked) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  if (Ty == IceType_i16)
    emitOperandSizeOverride();
  if (Locked)
    emitUint8(0xF0);
  emitAddrSizeOverridePrefix();
  emitRex(Ty, addr, reg);
  emitUint8(0x0F);
  if (isByteSizedArithType(Ty))
    emitUint8(0xC0);
  else
    emitUint8(0xC1);
  emitOperand(gprEncoding(reg), addr);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::xchg(Type Ty, GPRRegister reg0,
                                        GPRRegister reg1) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  if (Ty == IceType_i16)
    emitOperandSizeOverride();
  // Use short form if either register is EAX.
  if (reg0 == Traits::Encoded_Reg_Accumulator) {
    emitRexB(Ty, reg1);
    emitUint8(0x90 + gprEncoding(reg1));
  } else if (reg1 == Traits::Encoded_Reg_Accumulator) {
    emitRexB(Ty, reg0);
    emitUint8(0x90 + gprEncoding(reg0));
  } else {
    emitRexRB(Ty, reg0, reg1);
    if (isByteSizedArithType(Ty))
      emitUint8(0x86);
    else
      emitUint8(0x87);
    emitRegisterOperand(gprEncoding(reg0), gprEncoding(reg1));
  }
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::xchg(Type Ty, const Address &addr,
                                        GPRRegister reg) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  if (Ty == IceType_i16)
    emitOperandSizeOverride();
  emitAddrSizeOverridePrefix();
  emitRex(Ty, addr, reg);
  if (isByteSizedArithType(Ty))
    emitUint8(0x86);
  else
    emitUint8(0x87);
  emitOperand(gprEncoding(reg), addr);
}

template <typename TraitsType> void AssemblerX86Base<TraitsType>::iaca_start() {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(0x0F);
  emitUint8(0x0B);

  // mov $111, ebx
  constexpr GPRRegister dst = Traits::GPRRegister::Encoded_Reg_ebx;
  constexpr Type Ty = IceType_i32;
  emitRexB(Ty, dst);
  emitUint8(0xB8 + gprEncoding(dst));
  emitImmediate(Ty, Immediate(111));

  emitUint8(0x64);
  emitUint8(0x67);
  emitUint8(0x90);
}

template <typename TraitsType> void AssemblerX86Base<TraitsType>::iaca_end() {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);

  // mov $222, ebx
  constexpr GPRRegister dst = Traits::GPRRegister::Encoded_Reg_ebx;
  constexpr Type Ty = IceType_i32;
  emitRexB(Ty, dst);
  emitUint8(0xB8 + gprEncoding(dst));
  emitImmediate(Ty, Immediate(222));

  emitUint8(0x64);
  emitUint8(0x67);
  emitUint8(0x90);

  emitUint8(0x0F);
  emitUint8(0x0B);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::emitSegmentOverride(uint8_t prefix) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  emitUint8(prefix);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::align(intptr_t alignment, intptr_t offset) {
  assert(llvm::isPowerOf2_32(alignment));
  intptr_t pos = offset + Buffer.getPosition();
  intptr_t mod = pos & (alignment - 1);
  if (mod == 0) {
    return;
  }
  intptr_t bytes_needed = alignment - mod;
  while (bytes_needed > MAX_NOP_SIZE) {
    nop(MAX_NOP_SIZE);
    bytes_needed -= MAX_NOP_SIZE;
  }
  if (bytes_needed) {
    nop(bytes_needed);
  }
  assert(((offset + Buffer.getPosition()) & (alignment - 1)) == 0);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::bind(Label *L) {
  const intptr_t Bound = Buffer.size();
  assert(!L->isBound()); // Labels can only be bound once.
  while (L->isLinked()) {
    const intptr_t Position = L->getLinkPosition();
    const intptr_t Next = Buffer.load<int32_t>(Position);
    const intptr_t Offset = Bound - (Position + 4);
    Buffer.store<int32_t>(Position, Offset);
    L->Position = Next;
  }
  while (L->hasNear()) {
    intptr_t Position = L->getNearPosition();
    const intptr_t Offset = Bound - (Position + 1);
    assert(Utils::IsInt(8, Offset));
    Buffer.store<int8_t>(Position, Offset);
  }
  L->bindTo(Bound);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::emitOperand(int rm, const Operand &operand,
                                               RelocOffsetT Addend) {
  assert(rm >= 0 && rm < 8);
  const intptr_t length = operand.length_;
  assert(length > 0);
  intptr_t displacement_start = 1;
  // Emit the ModRM byte updated with the given RM value.
  assert((operand.encoding_[0] & 0x38) == 0);
  emitUint8(operand.encoding_[0] + (rm << 3));
  // Whenever the addressing mode is not register indirect, using esp == 0x4
  // as the register operation indicates an SIB byte follows.
  if (((operand.encoding_[0] & 0xc0) != 0xc0) &&
      ((operand.encoding_[0] & 0x07) == 0x04)) {
    emitUint8(operand.encoding_[1]);
    displacement_start = 2;
  }

  AssemblerFixup *Fixup = operand.fixup();
  if (Fixup == nullptr) {
    for (intptr_t i = displacement_start; i < length; i++) {
      emitUint8(operand.encoding_[i]);
    }
    return;
  }

  // Emit the fixup, and a dummy 4-byte immediate. Note that the Disp32 in
  // operand.encoding_[i, i+1, i+2, i+3] is part of the constant relocatable
  // used to create the fixup, so there's no need to add it to the addend.
  assert(length - displacement_start == 4);
  if (fixupIsPCRel(Fixup->kind())) {
    Fixup->set_addend(Fixup->get_addend() - Addend);
  } else {
    Fixup->set_addend(Fixup->get_addend());
  }
  emitFixup(Fixup);
  emitInt32(0);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::emitImmediate(Type Ty,
                                                 const Immediate &imm) {
  auto *const Fixup = imm.fixup();
  if (Ty == IceType_i16) {
    assert(Fixup == nullptr);
    emitInt16(imm.value());
    return;
  }

  if (Fixup == nullptr) {
    emitInt32(imm.value());
    return;
  }

  Fixup->set_addend(Fixup->get_addend() + imm.value());
  emitFixup(Fixup);
  emitInt32(0);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::emitComplexI8(int rm, const Operand &operand,
                                                 const Immediate &immediate) {
  assert(rm >= 0 && rm < 8);
  assert(immediate.is_int8());
  if (operand.IsRegister(Traits::Encoded_Reg_Accumulator)) {
    // Use short form if the destination is al.
    emitUint8(0x04 + (rm << 3));
    emitUint8(immediate.value() & 0xFF);
  } else {
    // Use sign-extended 8-bit immediate.
    emitUint8(0x80);
    static constexpr RelocOffsetT OffsetFromNextInstruction = 1;
    emitOperand(rm, operand, OffsetFromNextInstruction);
    emitUint8(immediate.value() & 0xFF);
  }
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::emitComplex(Type Ty, int rm,
                                               const Operand &operand,
                                               const Immediate &immediate) {
  assert(rm >= 0 && rm < 8);
  if (immediate.is_int8()) {
    // Use sign-extended 8-bit immediate.
    emitUint8(0x83);
    static constexpr RelocOffsetT OffsetFromNextInstruction = 1;
    emitOperand(rm, operand, OffsetFromNextInstruction);
    emitUint8(immediate.value() & 0xFF);
  } else if (operand.IsRegister(Traits::Encoded_Reg_Accumulator)) {
    // Use short form if the destination is eax.
    emitUint8(0x05 + (rm << 3));
    emitImmediate(Ty, immediate);
  } else {
    emitUint8(0x81);
    const uint8_t OffsetFromNextInstruction = Ty == IceType_i16 ? 2 : 4;
    emitOperand(rm, operand, OffsetFromNextInstruction);
    emitImmediate(Ty, immediate);
  }
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::emitLabel(Label *label,
                                             intptr_t instruction_size) {
  if (label->isBound()) {
    intptr_t offset = label->getPosition() - Buffer.size();
    assert(offset <= 0);
    emitInt32(offset - instruction_size);
  } else {
    emitLabelLink(label);
  }
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::emitLabelLink(Label *Label) {
  assert(!Label->isBound());
  intptr_t Position = Buffer.size();
  emitInt32(Label->Position);
  Label->linkTo(*this, Position);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::emitNearLabelLink(Label *Label) {
  assert(!Label->isBound());
  intptr_t Position = Buffer.size();
  emitUint8(0);
  Label->nearLinkTo(*this, Position);
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::emitGenericShift(int rm, Type Ty,
                                                    GPRRegister reg,
                                                    const Immediate &imm) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  // We don't assert that imm fits into 8 bits; instead, it gets masked below.
  // Note that we don't mask it further (e.g. to 5 bits) because we want the
  // same processor behavior regardless of whether it's an immediate (masked to
  // 8 bits) or in register cl (essentially ecx masked to 8 bits).
  if (Ty == IceType_i16)
    emitOperandSizeOverride();
  emitRexB(Ty, reg);
  if (imm.value() == 1) {
    emitUint8(isByteSizedArithType(Ty) ? 0xD0 : 0xD1);
    emitOperand(rm, Operand(reg));
  } else {
    emitUint8(isByteSizedArithType(Ty) ? 0xC0 : 0xC1);
    static constexpr RelocOffsetT OffsetFromNextInstruction = 1;
    emitOperand(rm, Operand(reg), OffsetFromNextInstruction);
    emitUint8(imm.value() & 0xFF);
  }
}

template <typename TraitsType>
void AssemblerX86Base<TraitsType>::emitGenericShift(int rm, Type Ty,
                                                    const Operand &operand,
                                                    GPRRegister shifter) {
  AssemblerBuffer::EnsureCapacity ensured(&Buffer);
  assert(shifter == Traits::Encoded_Reg_Counter);
  (void)shifter;
  if (Ty == IceType_i16)
    emitOperandSizeOverride();
  emitRexB(Ty, operand.rm());
  emitUint8(isByteSizedArithType(Ty) ? 0xD2 : 0xD3);
  emitOperand(rm, operand);
}

} // end of namespace X86NAMESPACE
} // end of namespace Ice
