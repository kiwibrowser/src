//===- subzero/src/IceAssemblerX86Base.h - base x86 assembler -*- C++ -*---===//
//
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
/// \brief Defines the AssemblerX86 template class for x86, the base of all X86
/// assemblers.
//
//===----------------------------------------------------------------------===//

#ifndef SUBZERO_SRC_ICEASSEMBLERX86BASE_H
#define SUBZERO_SRC_ICEASSEMBLERX86BASE_H

#include "IceAssembler.h"
#include "IceDefs.h"
#include "IceOperand.h"
#include "IceTypes.h"
#include "IceUtils.h"

namespace Ice {

#ifndef X86NAMESPACE
#error "You must define the X86 Target namespace."
#endif

namespace X86NAMESPACE {

template <typename TraitsType>
class AssemblerX86Base : public ::Ice::Assembler {
  AssemblerX86Base(const AssemblerX86Base &) = delete;
  AssemblerX86Base &operator=(const AssemblerX86Base &) = delete;

protected:
  explicit AssemblerX86Base(
      bool EmitAddrSizeOverridePrefix = TraitsType::Is64Bit)
      : Assembler(Traits::AsmKind),
        EmitAddrSizeOverridePrefix(EmitAddrSizeOverridePrefix) {
    assert(Traits::Is64Bit || !EmitAddrSizeOverridePrefix);
  }

public:
  using Traits = TraitsType;
  using Address = typename Traits::Address;
  using ByteRegister = typename Traits::ByteRegister;
  using BrCond = typename Traits::Cond::BrCond;
  using CmppsCond = typename Traits::Cond::CmppsCond;
  using GPRRegister = typename Traits::GPRRegister;
  using Operand = typename Traits::Operand;
  using XmmRegister = typename Traits::XmmRegister;

  static constexpr int MAX_NOP_SIZE = 8;

  static bool classof(const Assembler *Asm) {
    return Asm->getKind() == Traits::AsmKind;
  }

  class Immediate {
    Immediate(const Immediate &) = delete;
    Immediate &operator=(const Immediate &) = delete;

  public:
    explicit Immediate(int32_t value) : value_(value) {}

    explicit Immediate(AssemblerFixup *fixup) : fixup_(fixup) {}

    int32_t value() const { return value_; }
    AssemblerFixup *fixup() const { return fixup_; }

    bool is_int8() const {
      // We currently only allow 32-bit fixups, and they usually have value = 0,
      // so if fixup_ != nullptr, it shouldn't be classified as int8/16.
      return fixup_ == nullptr && Utils::IsInt(8, value_);
    }
    bool is_uint8() const {
      return fixup_ == nullptr && Utils::IsUint(8, value_);
    }
    bool is_uint16() const {
      return fixup_ == nullptr && Utils::IsUint(16, value_);
    }

  private:
    const int32_t value_ = 0;
    AssemblerFixup *fixup_ = nullptr;
  };

  /// X86 allows near and far jumps.
  class Label final : public Ice::Label {
    Label(const Label &) = delete;
    Label &operator=(const Label &) = delete;

  public:
    Label() = default;
    ~Label() = default;

    void finalCheck() const override {
      Ice::Label::finalCheck();
      assert(!hasNear());
    }

    /// Returns the position of an earlier branch instruction which assumes that
    /// this label is "near", and bumps iterator to the next near position.
    intptr_t getNearPosition() {
      assert(hasNear());
      intptr_t Pos = UnresolvedNearPositions.back();
      UnresolvedNearPositions.pop_back();
      return Pos;
    }

    bool hasNear() const { return !UnresolvedNearPositions.empty(); }
    bool isUnused() const override {
      return Ice::Label::isUnused() && !hasNear();
    }

  private:
    friend class AssemblerX86Base<TraitsType>;

    void nearLinkTo(const Assembler &Asm, intptr_t position) {
      if (Asm.getPreliminary())
        return;
      assert(!isBound());
      UnresolvedNearPositions.push_back(position);
    }

    llvm::SmallVector<intptr_t, 20> UnresolvedNearPositions;
  };

public:
  ~AssemblerX86Base() override;

  static const bool kNearJump = true;
  static const bool kFarJump = false;

  void alignFunction() override;

  SizeT getBundleAlignLog2Bytes() const override { return 5; }

  const char *getAlignDirective() const override { return ".p2align"; }

  llvm::ArrayRef<uint8_t> getNonExecBundlePadding() const override {
    static const uint8_t Padding[] = {0xF4};
    return llvm::ArrayRef<uint8_t>(Padding, 1);
  }

  void padWithNop(intptr_t Padding) override {
    while (Padding > MAX_NOP_SIZE) {
      nop(MAX_NOP_SIZE);
      Padding -= MAX_NOP_SIZE;
    }
    if (Padding)
      nop(Padding);
  }

  Ice::Label *getCfgNodeLabel(SizeT NodeNumber) override;
  void bindCfgNodeLabel(const CfgNode *Node) override;
  Label *getOrCreateCfgNodeLabel(SizeT Number);
  Label *getOrCreateLocalLabel(SizeT Number);
  void bindLocalLabel(SizeT Number);

  bool fixupIsPCRel(FixupKind Kind) const override {
    // Currently assuming this is the only PC-rel relocation type used.
    // TODO(jpp): Traits.PcRelTypes.count(Kind) != 0
    return Kind == Traits::FK_PcRel;
  }

  // Operations to emit GPR instructions (and dispatch on operand type).
  using TypedEmitGPR = void (AssemblerX86Base::*)(Type, GPRRegister);
  using TypedEmitAddr = void (AssemblerX86Base::*)(Type, const Address &);
  struct GPREmitterOneOp {
    TypedEmitGPR Reg;
    TypedEmitAddr Addr;
  };

  using TypedEmitGPRGPR = void (AssemblerX86Base::*)(Type, GPRRegister,
                                                     GPRRegister);
  using TypedEmitGPRAddr = void (AssemblerX86Base::*)(Type, GPRRegister,
                                                      const Address &);
  using TypedEmitGPRImm = void (AssemblerX86Base::*)(Type, GPRRegister,
                                                     const Immediate &);
  struct GPREmitterRegOp {
    TypedEmitGPRGPR GPRGPR;
    TypedEmitGPRAddr GPRAddr;
    TypedEmitGPRImm GPRImm;
  };

  struct GPREmitterShiftOp {
    // Technically, Addr/GPR and Addr/Imm are also allowed, but */Addr are
    // not. In practice, we always normalize the Dest to a Register first.
    TypedEmitGPRGPR GPRGPR;
    TypedEmitGPRImm GPRImm;
  };

  using TypedEmitGPRGPRImm = void (AssemblerX86Base::*)(Type, GPRRegister,
                                                        GPRRegister,
                                                        const Immediate &);
  struct GPREmitterShiftD {
    // Technically AddrGPR and AddrGPRImm are also allowed, but in practice we
    // always normalize Dest to a Register first.
    TypedEmitGPRGPR GPRGPR;
    TypedEmitGPRGPRImm GPRGPRImm;
  };

  using TypedEmitAddrGPR = void (AssemblerX86Base::*)(Type, const Address &,
                                                      GPRRegister);
  using TypedEmitAddrImm = void (AssemblerX86Base::*)(Type, const Address &,
                                                      const Immediate &);
  struct GPREmitterAddrOp {
    TypedEmitAddrGPR AddrGPR;
    TypedEmitAddrImm AddrImm;
  };

  // Operations to emit XMM instructions (and dispatch on operand type).
  using TypedEmitXmmXmm = void (AssemblerX86Base::*)(Type, XmmRegister,
                                                     XmmRegister);
  using TypedEmitXmmAddr = void (AssemblerX86Base::*)(Type, XmmRegister,
                                                      const Address &);
  struct XmmEmitterRegOp {
    TypedEmitXmmXmm XmmXmm;
    TypedEmitXmmAddr XmmAddr;
  };

  using EmitXmmXmm = void (AssemblerX86Base::*)(XmmRegister, XmmRegister);
  using EmitXmmAddr = void (AssemblerX86Base::*)(XmmRegister, const Address &);
  using EmitAddrXmm = void (AssemblerX86Base::*)(const Address &, XmmRegister);
  struct XmmEmitterMovOps {
    EmitXmmXmm XmmXmm;
    EmitXmmAddr XmmAddr;
    EmitAddrXmm AddrXmm;
  };

  using TypedEmitXmmImm = void (AssemblerX86Base::*)(Type, XmmRegister,
                                                     const Immediate &);

  struct XmmEmitterShiftOp {
    TypedEmitXmmXmm XmmXmm;
    TypedEmitXmmAddr XmmAddr;
    TypedEmitXmmImm XmmImm;
  };

  // Cross Xmm/GPR cast instructions.
  template <typename DReg_t, typename SReg_t> struct CastEmitterRegOp {
    using TypedEmitRegs = void (AssemblerX86Base::*)(Type, DReg_t, Type,
                                                     SReg_t);
    using TypedEmitAddr = void (AssemblerX86Base::*)(Type, DReg_t, Type,
                                                     const Address &);

    TypedEmitRegs RegReg;
    TypedEmitAddr RegAddr;
  };

  // Three operand (potentially) cross Xmm/GPR instructions. The last operand
  // must be an immediate.
  template <typename DReg_t, typename SReg_t> struct ThreeOpImmEmitter {
    using TypedEmitRegRegImm = void (AssemblerX86Base::*)(Type, DReg_t, SReg_t,
                                                          const Immediate &);
    using TypedEmitRegAddrImm = void (AssemblerX86Base::*)(Type, DReg_t,
                                                           const Address &,
                                                           const Immediate &);

    TypedEmitRegRegImm RegRegImm;
    TypedEmitRegAddrImm RegAddrImm;
  };

  /*
   * Emit Machine Instructions.
   */
  void call(GPRRegister reg);
  void call(const Address &address);
  void call(const ConstantRelocatable *label); // not testable.
  void call(const Immediate &abs_address);

  static const intptr_t kCallExternalLabelSize = 5;

  void pushl(GPRRegister reg);
  void pushl(const Immediate &Imm);
  void pushl(const ConstantRelocatable *Label);

  void popl(GPRRegister reg);
  void popl(const Address &address);

  template <typename T = Traits,
            typename = typename std::enable_if<T::HasPusha>::type>
  void pushal();
  template <typename T = Traits,
            typename = typename std::enable_if<T::HasPopa>::type>
  void popal();

  void setcc(BrCond condition, ByteRegister dst);
  void setcc(BrCond condition, const Address &address);

  void mov(Type Ty, GPRRegister dst, const Immediate &src);
  void mov(Type Ty, GPRRegister dst, GPRRegister src);
  void mov(Type Ty, GPRRegister dst, const Address &src);
  void mov(Type Ty, const Address &dst, GPRRegister src);
  void mov(Type Ty, const Address &dst, const Immediate &imm);

  template <typename T = Traits>
  typename std::enable_if<T::Is64Bit, void>::type movabs(const GPRRegister Dst,
                                                         uint64_t Imm64);
  template <typename T = Traits>
  typename std::enable_if<!T::Is64Bit, void>::type movabs(const GPRRegister,
                                                          uint64_t) {
    llvm::report_fatal_error("movabs is only supported in 64-bit x86 targets.");
  }

  void movzx(Type Ty, GPRRegister dst, GPRRegister src);
  void movzx(Type Ty, GPRRegister dst, const Address &src);
  void movsx(Type Ty, GPRRegister dst, GPRRegister src);
  void movsx(Type Ty, GPRRegister dst, const Address &src);

  void lea(Type Ty, GPRRegister dst, const Address &src);

  void cmov(Type Ty, BrCond cond, GPRRegister dst, GPRRegister src);
  void cmov(Type Ty, BrCond cond, GPRRegister dst, const Address &src);

  void rep_movsb();

  void movss(Type Ty, XmmRegister dst, const Address &src);
  void movss(Type Ty, const Address &dst, XmmRegister src);
  void movss(Type Ty, XmmRegister dst, XmmRegister src);

  void movd(Type SrcTy, XmmRegister dst, GPRRegister src);
  void movd(Type SrcTy, XmmRegister dst, const Address &src);
  void movd(Type DestTy, GPRRegister dst, XmmRegister src);
  void movd(Type DestTy, const Address &dst, XmmRegister src);

  void movq(XmmRegister dst, XmmRegister src);
  void movq(const Address &dst, XmmRegister src);
  void movq(XmmRegister dst, const Address &src);

  void addss(Type Ty, XmmRegister dst, XmmRegister src);
  void addss(Type Ty, XmmRegister dst, const Address &src);
  void subss(Type Ty, XmmRegister dst, XmmRegister src);
  void subss(Type Ty, XmmRegister dst, const Address &src);
  void mulss(Type Ty, XmmRegister dst, XmmRegister src);
  void mulss(Type Ty, XmmRegister dst, const Address &src);
  void divss(Type Ty, XmmRegister dst, XmmRegister src);
  void divss(Type Ty, XmmRegister dst, const Address &src);

  void movaps(XmmRegister dst, XmmRegister src);

  void movups(XmmRegister dst, XmmRegister src);
  void movups(XmmRegister dst, const Address &src);
  void movups(const Address &dst, XmmRegister src);

  void padd(Type Ty, XmmRegister dst, XmmRegister src);
  void padd(Type Ty, XmmRegister dst, const Address &src);
  void padds(Type Ty, XmmRegister dst, XmmRegister src);
  void padds(Type Ty, XmmRegister dst, const Address &src);
  void paddus(Type Ty, XmmRegister dst, XmmRegister src);
  void paddus(Type Ty, XmmRegister dst, const Address &src);
  void pand(Type Ty, XmmRegister dst, XmmRegister src);
  void pand(Type Ty, XmmRegister dst, const Address &src);
  void pandn(Type Ty, XmmRegister dst, XmmRegister src);
  void pandn(Type Ty, XmmRegister dst, const Address &src);
  void pmull(Type Ty, XmmRegister dst, XmmRegister src);
  void pmull(Type Ty, XmmRegister dst, const Address &src);
  void pmulhw(Type Ty, XmmRegister dst, XmmRegister src);
  void pmulhw(Type Ty, XmmRegister dst, const Address &src);
  void pmulhuw(Type Ty, XmmRegister dst, XmmRegister src);
  void pmulhuw(Type Ty, XmmRegister dst, const Address &src);
  void pmaddwd(Type Ty, XmmRegister dst, XmmRegister src);
  void pmaddwd(Type Ty, XmmRegister dst, const Address &src);
  void pmuludq(Type Ty, XmmRegister dst, XmmRegister src);
  void pmuludq(Type Ty, XmmRegister dst, const Address &src);
  void por(Type Ty, XmmRegister dst, XmmRegister src);
  void por(Type Ty, XmmRegister dst, const Address &src);
  void psub(Type Ty, XmmRegister dst, XmmRegister src);
  void psub(Type Ty, XmmRegister dst, const Address &src);
  void psubs(Type Ty, XmmRegister dst, XmmRegister src);
  void psubs(Type Ty, XmmRegister dst, const Address &src);
  void psubus(Type Ty, XmmRegister dst, XmmRegister src);
  void psubus(Type Ty, XmmRegister dst, const Address &src);
  void pxor(Type Ty, XmmRegister dst, XmmRegister src);
  void pxor(Type Ty, XmmRegister dst, const Address &src);

  void psll(Type Ty, XmmRegister dst, XmmRegister src);
  void psll(Type Ty, XmmRegister dst, const Address &src);
  void psll(Type Ty, XmmRegister dst, const Immediate &src);

  void psra(Type Ty, XmmRegister dst, XmmRegister src);
  void psra(Type Ty, XmmRegister dst, const Address &src);
  void psra(Type Ty, XmmRegister dst, const Immediate &src);
  void psrl(Type Ty, XmmRegister dst, XmmRegister src);
  void psrl(Type Ty, XmmRegister dst, const Address &src);
  void psrl(Type Ty, XmmRegister dst, const Immediate &src);

  void addps(Type Ty, XmmRegister dst, XmmRegister src);
  void addps(Type Ty, XmmRegister dst, const Address &src);
  void subps(Type Ty, XmmRegister dst, XmmRegister src);
  void subps(Type Ty, XmmRegister dst, const Address &src);
  void divps(Type Ty, XmmRegister dst, XmmRegister src);
  void divps(Type Ty, XmmRegister dst, const Address &src);
  void mulps(Type Ty, XmmRegister dst, XmmRegister src);
  void mulps(Type Ty, XmmRegister dst, const Address &src);
  void minps(Type Ty, XmmRegister dst, const Address &src);
  void minps(Type Ty, XmmRegister dst, XmmRegister src);
  void minss(Type Ty, XmmRegister dst, const Address &src);
  void minss(Type Ty, XmmRegister dst, XmmRegister src);
  void maxps(Type Ty, XmmRegister dst, const Address &src);
  void maxps(Type Ty, XmmRegister dst, XmmRegister src);
  void maxss(Type Ty, XmmRegister dst, const Address &src);
  void maxss(Type Ty, XmmRegister dst, XmmRegister src);
  void andnps(Type Ty, XmmRegister dst, const Address &src);
  void andnps(Type Ty, XmmRegister dst, XmmRegister src);
  void andps(Type Ty, XmmRegister dst, const Address &src);
  void andps(Type Ty, XmmRegister dst, XmmRegister src);
  void orps(Type Ty, XmmRegister dst, const Address &src);
  void orps(Type Ty, XmmRegister dst, XmmRegister src);

  void blendvps(Type Ty, XmmRegister dst, XmmRegister src);
  void blendvps(Type Ty, XmmRegister dst, const Address &src);
  void pblendvb(Type Ty, XmmRegister dst, XmmRegister src);
  void pblendvb(Type Ty, XmmRegister dst, const Address &src);

  void cmpps(Type Ty, XmmRegister dst, XmmRegister src, CmppsCond CmpCondition);
  void cmpps(Type Ty, XmmRegister dst, const Address &src,
             CmppsCond CmpCondition);

  void sqrtps(XmmRegister dst);
  void rsqrtps(XmmRegister dst);
  void reciprocalps(XmmRegister dst);

  void movhlps(XmmRegister dst, XmmRegister src);
  void movlhps(XmmRegister dst, XmmRegister src);
  void unpcklps(XmmRegister dst, XmmRegister src);
  void unpckhps(XmmRegister dst, XmmRegister src);
  void unpcklpd(XmmRegister dst, XmmRegister src);
  void unpckhpd(XmmRegister dst, XmmRegister src);

  void set1ps(XmmRegister dst, GPRRegister tmp, const Immediate &imm);

  void sqrtpd(XmmRegister dst);

  void pshufb(Type Ty, XmmRegister dst, XmmRegister src);
  void pshufb(Type Ty, XmmRegister dst, const Address &src);
  void pshufd(Type Ty, XmmRegister dst, XmmRegister src, const Immediate &mask);
  void pshufd(Type Ty, XmmRegister dst, const Address &src,
              const Immediate &mask);
  void punpckl(Type Ty, XmmRegister Dst, XmmRegister Src);
  void punpckl(Type Ty, XmmRegister Dst, const Address &Src);
  void punpckh(Type Ty, XmmRegister Dst, XmmRegister Src);
  void punpckh(Type Ty, XmmRegister Dst, const Address &Src);
  void packss(Type Ty, XmmRegister Dst, XmmRegister Src);
  void packss(Type Ty, XmmRegister Dst, const Address &Src);
  void packus(Type Ty, XmmRegister Dst, XmmRegister Src);
  void packus(Type Ty, XmmRegister Dst, const Address &Src);
  void shufps(Type Ty, XmmRegister dst, XmmRegister src, const Immediate &mask);
  void shufps(Type Ty, XmmRegister dst, const Address &src,
              const Immediate &mask);

  void cvtdq2ps(Type, XmmRegister dst, XmmRegister src);
  void cvtdq2ps(Type, XmmRegister dst, const Address &src);

  void cvttps2dq(Type, XmmRegister dst, XmmRegister src);
  void cvttps2dq(Type, XmmRegister dst, const Address &src);

  void cvtps2dq(Type, XmmRegister dst, XmmRegister src);
  void cvtps2dq(Type, XmmRegister dst, const Address &src);

  void cvtsi2ss(Type DestTy, XmmRegister dst, Type SrcTy, GPRRegister src);
  void cvtsi2ss(Type DestTy, XmmRegister dst, Type SrcTy, const Address &src);

  void cvtfloat2float(Type SrcTy, XmmRegister dst, XmmRegister src);
  void cvtfloat2float(Type SrcTy, XmmRegister dst, const Address &src);

  void cvttss2si(Type DestTy, GPRRegister dst, Type SrcTy, XmmRegister src);
  void cvttss2si(Type DestTy, GPRRegister dst, Type SrcTy, const Address &src);

  void cvtss2si(Type DestTy, GPRRegister dst, Type SrcTy, XmmRegister src);
  void cvtss2si(Type DestTy, GPRRegister dst, Type SrcTy, const Address &src);

  void ucomiss(Type Ty, XmmRegister a, XmmRegister b);
  void ucomiss(Type Ty, XmmRegister a, const Address &b);

  void movmsk(Type Ty, GPRRegister dst, XmmRegister src);

  void sqrt(Type Ty, XmmRegister dst, const Address &src);
  void sqrt(Type Ty, XmmRegister dst, XmmRegister src);

  void xorps(Type Ty, XmmRegister dst, const Address &src);
  void xorps(Type Ty, XmmRegister dst, XmmRegister src);

  void insertps(Type Ty, XmmRegister dst, XmmRegister src,
                const Immediate &imm);
  void insertps(Type Ty, XmmRegister dst, const Address &src,
                const Immediate &imm);

  void pinsr(Type Ty, XmmRegister dst, GPRRegister src, const Immediate &imm);
  void pinsr(Type Ty, XmmRegister dst, const Address &src,
             const Immediate &imm);

  void pextr(Type Ty, GPRRegister dst, XmmRegister src, const Immediate &imm);

  void pmovsxdq(XmmRegister dst, XmmRegister src);

  void pcmpeq(Type Ty, XmmRegister dst, XmmRegister src);
  void pcmpeq(Type Ty, XmmRegister dst, const Address &src);
  void pcmpgt(Type Ty, XmmRegister dst, XmmRegister src);
  void pcmpgt(Type Ty, XmmRegister dst, const Address &src);

  enum RoundingMode {
    kRoundToNearest = 0x0,
    kRoundDown = 0x1,
    kRoundUp = 0x2,
    kRoundToZero = 0x3
  };
  void round(Type Ty, XmmRegister dst, XmmRegister src, const Immediate &mode);
  void round(Type Ty, XmmRegister dst, const Address &src,
             const Immediate &mode);

  //----------------------------------------------------------------------------
  //
  // Begin: X87 instructions. Only available when Traits::UsesX87.
  //
  //----------------------------------------------------------------------------
  template <typename T = Traits,
            typename = typename std::enable_if<T::UsesX87>::type>
  void fld(Type Ty, const typename T::Address &src);
  template <typename T = Traits,
            typename = typename std::enable_if<T::UsesX87>::type>
  void fstp(Type Ty, const typename T::Address &dst);
  template <typename T = Traits,
            typename = typename std::enable_if<T::UsesX87>::type>
  void fstp(typename T::X87STRegister st);

  template <typename T = Traits,
            typename = typename std::enable_if<T::UsesX87>::type>
  void fnstcw(const typename T::Address &dst);
  template <typename T = Traits,
            typename = typename std::enable_if<T::UsesX87>::type>
  void fldcw(const typename T::Address &src);

  template <typename T = Traits,
            typename = typename std::enable_if<T::UsesX87>::type>
  void fistpl(const typename T::Address &dst);
  template <typename T = Traits,
            typename = typename std::enable_if<T::UsesX87>::type>
  void fistps(const typename T::Address &dst);
  template <typename T = Traits,
            typename = typename std::enable_if<T::UsesX87>::type>
  void fildl(const typename T::Address &src);
  template <typename T = Traits,
            typename = typename std::enable_if<T::UsesX87>::type>
  void filds(const typename T::Address &src);

  template <typename T = Traits,
            typename = typename std::enable_if<T::UsesX87>::type>
  void fincstp();
  //----------------------------------------------------------------------------
  //
  // End: X87 instructions.
  //
  //----------------------------------------------------------------------------

  void cmp(Type Ty, GPRRegister reg0, GPRRegister reg1);
  void cmp(Type Ty, GPRRegister reg, const Address &address);
  void cmp(Type Ty, GPRRegister reg, const Immediate &imm);
  void cmp(Type Ty, const Address &address, GPRRegister reg);
  void cmp(Type Ty, const Address &address, const Immediate &imm);

  void test(Type Ty, GPRRegister reg0, GPRRegister reg1);
  void test(Type Ty, GPRRegister reg, const Immediate &imm);
  void test(Type Ty, const Address &address, GPRRegister reg);
  void test(Type Ty, const Address &address, const Immediate &imm);

  void And(Type Ty, GPRRegister dst, GPRRegister src);
  void And(Type Ty, GPRRegister dst, const Address &address);
  void And(Type Ty, GPRRegister dst, const Immediate &imm);
  void And(Type Ty, const Address &address, GPRRegister reg);
  void And(Type Ty, const Address &address, const Immediate &imm);

  void Or(Type Ty, GPRRegister dst, GPRRegister src);
  void Or(Type Ty, GPRRegister dst, const Address &address);
  void Or(Type Ty, GPRRegister dst, const Immediate &imm);
  void Or(Type Ty, const Address &address, GPRRegister reg);
  void Or(Type Ty, const Address &address, const Immediate &imm);

  void Xor(Type Ty, GPRRegister dst, GPRRegister src);
  void Xor(Type Ty, GPRRegister dst, const Address &address);
  void Xor(Type Ty, GPRRegister dst, const Immediate &imm);
  void Xor(Type Ty, const Address &address, GPRRegister reg);
  void Xor(Type Ty, const Address &address, const Immediate &imm);

  void add(Type Ty, GPRRegister dst, GPRRegister src);
  void add(Type Ty, GPRRegister reg, const Address &address);
  void add(Type Ty, GPRRegister reg, const Immediate &imm);
  void add(Type Ty, const Address &address, GPRRegister reg);
  void add(Type Ty, const Address &address, const Immediate &imm);

  void adc(Type Ty, GPRRegister dst, GPRRegister src);
  void adc(Type Ty, GPRRegister dst, const Address &address);
  void adc(Type Ty, GPRRegister reg, const Immediate &imm);
  void adc(Type Ty, const Address &address, GPRRegister reg);
  void adc(Type Ty, const Address &address, const Immediate &imm);

  void sub(Type Ty, GPRRegister dst, GPRRegister src);
  void sub(Type Ty, GPRRegister reg, const Address &address);
  void sub(Type Ty, GPRRegister reg, const Immediate &imm);
  void sub(Type Ty, const Address &address, GPRRegister reg);
  void sub(Type Ty, const Address &address, const Immediate &imm);

  void sbb(Type Ty, GPRRegister dst, GPRRegister src);
  void sbb(Type Ty, GPRRegister reg, const Address &address);
  void sbb(Type Ty, GPRRegister reg, const Immediate &imm);
  void sbb(Type Ty, const Address &address, GPRRegister reg);
  void sbb(Type Ty, const Address &address, const Immediate &imm);

  void cbw();
  void cwd();
  void cdq();
  template <typename T = Traits>
  typename std::enable_if<T::Is64Bit, void>::type cqo();
  template <typename T = Traits>
  typename std::enable_if<!T::Is64Bit, void>::type cqo() {
    llvm::report_fatal_error("CQO is only available in 64-bit x86 backends.");
  }

  void div(Type Ty, GPRRegister reg);
  void div(Type Ty, const Address &address);

  void idiv(Type Ty, GPRRegister reg);
  void idiv(Type Ty, const Address &address);

  void imul(Type Ty, GPRRegister dst, GPRRegister src);
  void imul(Type Ty, GPRRegister reg, const Immediate &imm);
  void imul(Type Ty, GPRRegister reg, const Address &address);

  void imul(Type Ty, GPRRegister reg);
  void imul(Type Ty, const Address &address);

  void imul(Type Ty, GPRRegister dst, GPRRegister src, const Immediate &imm);
  void imul(Type Ty, GPRRegister dst, const Address &address,
            const Immediate &imm);

  void mul(Type Ty, GPRRegister reg);
  void mul(Type Ty, const Address &address);

  template <class T = Traits,
            typename = typename std::enable_if<!T::Is64Bit>::type>
  void incl(GPRRegister reg);
  void incl(const Address &address);

  template <class T = Traits,
            typename = typename std::enable_if<!T::Is64Bit>::type>
  void decl(GPRRegister reg);
  void decl(const Address &address);

  void rol(Type Ty, GPRRegister reg, const Immediate &imm);
  void rol(Type Ty, GPRRegister operand, GPRRegister shifter);
  void rol(Type Ty, const Address &operand, GPRRegister shifter);

  void shl(Type Ty, GPRRegister reg, const Immediate &imm);
  void shl(Type Ty, GPRRegister operand, GPRRegister shifter);
  void shl(Type Ty, const Address &operand, GPRRegister shifter);

  void shr(Type Ty, GPRRegister reg, const Immediate &imm);
  void shr(Type Ty, GPRRegister operand, GPRRegister shifter);
  void shr(Type Ty, const Address &operand, GPRRegister shifter);

  void sar(Type Ty, GPRRegister reg, const Immediate &imm);
  void sar(Type Ty, GPRRegister operand, GPRRegister shifter);
  void sar(Type Ty, const Address &address, GPRRegister shifter);

  void shld(Type Ty, GPRRegister dst, GPRRegister src);
  void shld(Type Ty, GPRRegister dst, GPRRegister src, const Immediate &imm);
  void shld(Type Ty, const Address &operand, GPRRegister src);
  void shrd(Type Ty, GPRRegister dst, GPRRegister src);
  void shrd(Type Ty, GPRRegister dst, GPRRegister src, const Immediate &imm);
  void shrd(Type Ty, const Address &dst, GPRRegister src);

  void neg(Type Ty, GPRRegister reg);
  void neg(Type Ty, const Address &addr);
  void notl(GPRRegister reg);

  void bsf(Type Ty, GPRRegister dst, GPRRegister src);
  void bsf(Type Ty, GPRRegister dst, const Address &src);
  void bsr(Type Ty, GPRRegister dst, GPRRegister src);
  void bsr(Type Ty, GPRRegister dst, const Address &src);

  void bswap(Type Ty, GPRRegister reg);

  void bt(GPRRegister base, GPRRegister offset);

  void ret();
  void ret(const Immediate &imm);

  // 'size' indicates size in bytes and must be in the range 1..8.
  void nop(int size = 1);
  void int3();
  void hlt();
  void ud2();

  // j(Label) is fully tested.
  void j(BrCond condition, Label *label, bool near = kFarJump);
  void j(BrCond condition, const ConstantRelocatable *label); // not testable.

  void jmp(GPRRegister reg);
  void jmp(Label *label, bool near = kFarJump);
  void jmp(const ConstantRelocatable *label); // not testable.
  void jmp(const Immediate &abs_address);

  void mfence();

  void lock();
  void cmpxchg(Type Ty, const Address &address, GPRRegister reg, bool Locked);
  void cmpxchg8b(const Address &address, bool Locked);
  void xadd(Type Ty, const Address &address, GPRRegister reg, bool Locked);
  void xchg(Type Ty, GPRRegister reg0, GPRRegister reg1);
  void xchg(Type Ty, const Address &address, GPRRegister reg);

  /// \name Intel Architecture Code Analyzer markers.
  /// @{
  void iaca_start();
  void iaca_end();
  /// @}

  void emitSegmentOverride(uint8_t prefix);

  intptr_t preferredLoopAlignment() { return 16; }
  void align(intptr_t alignment, intptr_t offset);
  void bind(Label *label);

  intptr_t CodeSize() const { return Buffer.size(); }

protected:
  inline void emitUint8(uint8_t value);

private:
  ENABLE_MAKE_UNIQUE;

  // EmidAddrSizeOverridePrefix directs the emission of the 0x67 prefix to
  // force 32-bit registers when accessing memory. This is only used in native
  // 64-bit.
  const bool EmitAddrSizeOverridePrefix;

  static constexpr Type RexTypeIrrelevant = IceType_i32;
  static constexpr Type RexTypeForceRexW = IceType_i64;
  static constexpr GPRRegister RexRegIrrelevant =
      Traits::GPRRegister::Encoded_Reg_eax;

  inline void emitInt16(int16_t value);
  inline void emitInt32(int32_t value);
  inline void emitRegisterOperand(int rm, int reg);
  template <typename RegType, typename RmType>
  inline void emitXmmRegisterOperand(RegType reg, RmType rm);
  inline void emitOperandSizeOverride();

  void emitOperand(int rm, const Operand &operand, RelocOffsetT Addend = 0);
  void emitImmediate(Type ty, const Immediate &imm);
  void emitComplexI8(int rm, const Operand &operand,
                     const Immediate &immediate);
  void emitComplex(Type Ty, int rm, const Operand &operand,
                   const Immediate &immediate);
  void emitLabel(Label *label, intptr_t instruction_size);
  void emitLabelLink(Label *label);
  void emitNearLabelLink(Label *label);

  void emitGenericShift(int rm, Type Ty, GPRRegister reg, const Immediate &imm);
  void emitGenericShift(int rm, Type Ty, const Operand &operand,
                        GPRRegister shifter);

  using LabelVector = std::vector<Label *>;
  // A vector of pool-allocated x86 labels for CFG nodes.
  LabelVector CfgNodeLabels;
  // A vector of pool-allocated x86 labels for Local labels.
  LabelVector LocalLabels;

  Label *getOrCreateLabel(SizeT Number, LabelVector &Labels);

  void emitAddrSizeOverridePrefix() {
    if (!Traits::Is64Bit || !EmitAddrSizeOverridePrefix) {
      return;
    }
    static constexpr uint8_t AddrSizeOverridePrefix = 0x67;
    emitUint8(AddrSizeOverridePrefix);
  }

  // The arith_int() methods factor out the commonality between the encodings
  // of add(), Or(), adc(), sbb(), And(), sub(), Xor(), and cmp(). The Tag
  // parameter is statically asserted to be less than 8.
  template <uint32_t Tag>
  void arith_int(Type Ty, GPRRegister reg, const Immediate &imm);

  template <uint32_t Tag>
  void arith_int(Type Ty, GPRRegister reg0, GPRRegister reg1);

  template <uint32_t Tag>
  void arith_int(Type Ty, GPRRegister reg, const Address &address);

  template <uint32_t Tag>
  void arith_int(Type Ty, const Address &address, GPRRegister reg);

  template <uint32_t Tag>
  void arith_int(Type Ty, const Address &address, const Immediate &imm);

  // gprEncoding returns Reg encoding for operand emission. For x86-64 we mask
  // out the 4th bit as it is encoded in the REX.[RXB] bits. No other bits are
  // touched because we don't want to mask errors.
  template <typename RegType, typename T = Traits>
  typename std::enable_if<T::Is64Bit, typename T::GPRRegister>::type
  gprEncoding(const RegType Reg) {
    return static_cast<GPRRegister>(static_cast<uint8_t>(Reg) & ~0x08);
  }

  template <typename RegType, typename T = Traits>
  typename std::enable_if<!T::Is64Bit, typename T::GPRRegister>::type
  gprEncoding(const RegType Reg) {
    return static_cast<typename T::GPRRegister>(Reg);
  }

  template <typename RegType>
  bool is8BitRegisterRequiringRex(const Type Ty, const RegType Reg) {
    static constexpr bool IsGPR =
        std::is_same<typename std::decay<RegType>::type, ByteRegister>::value ||
        std::is_same<typename std::decay<RegType>::type, GPRRegister>::value;

    // At this point in the assembler, we have encoded regs, so it is not
    // possible to distinguish between the "new" low byte registers introduced
    // in x86-64 and the legacy [abcd]h registers. Because x86, we may still
    // see ah (div) in the assembler, so we whitelist it here.
    //
    // The "local" uint32_t Encoded_Reg_ah is needed because RegType is an
    // enum that is not necessarily the same type of
    // Traits::RegisterSet::Encoded_Reg_ah.
    constexpr uint32_t Encoded_Reg_ah = Traits::RegisterSet::Encoded_Reg_ah;
    return IsGPR && (Reg & 0x04) != 0 && (Reg & 0x08) == 0 &&
           isByteSizedType(Ty) && (Reg != Encoded_Reg_ah);
  }

  // assembleAndEmitRex is used for determining which (if any) rex prefix
  // should be emitted for the current instruction. It allows different types
  // for Reg and Rm because they could be of different types (e.g., in
  // mov[sz]x instructions.) If Addr is not nullptr, then Rm is ignored, and
  // Rex.B is determined by Addr instead. TyRm is still used to determine
  // Addr's size.
  template <typename RegType, typename RmType, typename T = Traits>
  typename std::enable_if<T::Is64Bit, void>::type
  assembleAndEmitRex(const Type TyReg, const RegType Reg, const Type TyRm,
                     const RmType Rm,
                     const typename T::Address *Addr = nullptr) {
    const uint8_t W = (TyReg == IceType_i64 || TyRm == IceType_i64)
                          ? T::Operand::RexW
                          : T::Operand::RexNone;
    const uint8_t R = (Reg & 0x08) ? T::Operand::RexR : T::Operand::RexNone;
    const uint8_t X = (Addr != nullptr)
                          ? (typename T::Operand::RexBits)Addr->rexX()
                          : T::Operand::RexNone;
    const uint8_t B =
        (Addr != nullptr)
            ? (typename T::Operand::RexBits)Addr->rexB()
            : (Rm & 0x08) ? T::Operand::RexB : T::Operand::RexNone;
    const uint8_t Prefix = W | R | X | B;
    if (Prefix != T::Operand::RexNone) {
      emitUint8(Prefix);
    } else if (is8BitRegisterRequiringRex(TyReg, Reg) ||
               (Addr == nullptr && is8BitRegisterRequiringRex(TyRm, Rm))) {
      emitUint8(T::Operand::RexBase);
    }
  }

  template <typename RegType, typename RmType, typename T = Traits>
  typename std::enable_if<!T::Is64Bit, void>::type
  assembleAndEmitRex(const Type, const RegType, const Type, const RmType,
                     const typename T::Address * = nullptr) {}

  // emitRexRB is used for emitting a Rex prefix instructions with two
  // explicit register operands in its mod-rm byte.
  template <typename RegType, typename RmType>
  void emitRexRB(const Type Ty, const RegType Reg, const RmType Rm) {
    assembleAndEmitRex(Ty, Reg, Ty, Rm);
  }

  template <typename RegType, typename RmType>
  void emitRexRB(const Type TyReg, const RegType Reg, const Type TyRm,
                 const RmType Rm) {
    assembleAndEmitRex(TyReg, Reg, TyRm, Rm);
  }

  // emitRexB is used for emitting a Rex prefix if one is needed on encoding
  // the Reg field in an x86 instruction. It is invoked by the template when
  // Reg is the single register operand in the instruction (e.g., push Reg.)
  template <typename RmType> void emitRexB(const Type Ty, const RmType Rm) {
    emitRexRB(Ty, RexRegIrrelevant, Ty, Rm);
  }

  // emitRex is used for emitting a Rex prefix for an address and a GPR. The
  // address may contain zero, one, or two registers.
  template <typename RegType>
  void emitRex(const Type Ty, const Address &Addr, const RegType Reg) {
    assembleAndEmitRex(Ty, Reg, Ty, RexRegIrrelevant, &Addr);
  }

  template <typename RegType>
  void emitRex(const Type AddrTy, const Address &Addr, const Type TyReg,
               const RegType Reg) {
    assembleAndEmitRex(TyReg, Reg, AddrTy, RexRegIrrelevant, &Addr);
  }
};

template <typename TraitsType>
inline void AssemblerX86Base<TraitsType>::emitUint8(uint8_t value) {
  Buffer.emit<uint8_t>(value);
}

template <typename TraitsType>
inline void AssemblerX86Base<TraitsType>::emitInt16(int16_t value) {
  Buffer.emit<int16_t>(value);
}

template <typename TraitsType>
inline void AssemblerX86Base<TraitsType>::emitInt32(int32_t value) {
  Buffer.emit<int32_t>(value);
}

template <typename TraitsType>
inline void AssemblerX86Base<TraitsType>::emitRegisterOperand(int reg, int rm) {
  assert(reg >= 0 && reg < 8);
  assert(rm >= 0 && rm < 8);
  Buffer.emit<uint8_t>(0xC0 + (reg << 3) + rm);
}

template <typename TraitsType>
template <typename RegType, typename RmType>
inline void AssemblerX86Base<TraitsType>::emitXmmRegisterOperand(RegType reg,
                                                                 RmType rm) {
  emitRegisterOperand(gprEncoding(reg), gprEncoding(rm));
}

template <typename TraitsType>
inline void AssemblerX86Base<TraitsType>::emitOperandSizeOverride() {
  emitUint8(0x66);
}

} // end of namespace X86NAMESPACE

} // end of namespace Ice

#include "IceAssemblerX86BaseImpl.h"

#endif // SUBZERO_SRC_ICEASSEMBLERX86BASE_H
