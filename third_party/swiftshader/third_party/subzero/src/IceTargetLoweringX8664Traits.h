//===- subzero/src/IceTargetLoweringX8664Traits.h - x86-64 traits -*- C++ -*-=//
//
//                        The Subzero Code Generator
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// \brief Declares the X8664 Target Lowering Traits.
///
//===----------------------------------------------------------------------===//

#ifndef SUBZERO_SRC_ICETARGETLOWERINGX8664TRAITS_H
#define SUBZERO_SRC_ICETARGETLOWERINGX8664TRAITS_H

#include "IceAssembler.h"
#include "IceConditionCodesX8664.h"
#include "IceDefs.h"
#include "IceInst.h"
#include "IceInstX8664.def"
#include "IceOperand.h"
#include "IceRegistersX8664.h"
#include "IceTargetLowering.h"
#include "IceTargetLoweringX8664.def"
#include "IceTargetLoweringX86RegClass.h"

#include <array>
#include <initializer_list>

namespace Ice {

namespace X8664 {
using namespace ::Ice::X86;

template <class TraitsType> class AssemblerX86Base;
template <class TraitsType> struct Insts;
template <class TraitsType> class TargetX86Base;

class TargetX8664;

struct TargetX8664Traits {
  //----------------------------------------------------------------------------
  //     ______  ______  __    __
  //    /\  __ \/\  ___\/\ "-./  \
  //    \ \  __ \ \___  \ \ \-./\ \
  //     \ \_\ \_\/\_____\ \_\ \ \_\
  //      \/_/\/_/\/_____/\/_/  \/_/
  //
  //----------------------------------------------------------------------------
  static constexpr ::Ice::Assembler::AssemblerKind AsmKind =
      ::Ice::Assembler::Asm_X8664;

  static constexpr bool Is64Bit = true;
  static constexpr bool HasPopa = false;
  static constexpr bool HasPusha = false;
  static constexpr bool UsesX87 = false;
  static constexpr ::Ice::RegX8664::GPRRegister Last8BitGPR =
      ::Ice::RegX8664::GPRRegister::Encoded_Reg_r15d;

  enum ScaleFactor { TIMES_1 = 0, TIMES_2 = 1, TIMES_4 = 2, TIMES_8 = 3 };

  using GPRRegister = ::Ice::RegX8664::GPRRegister;
  using ByteRegister = ::Ice::RegX8664::ByteRegister;
  using XmmRegister = ::Ice::RegX8664::XmmRegister;

  using Cond = ::Ice::CondX8664;

  using RegisterSet = ::Ice::RegX8664;
  static constexpr RegisterSet::AllRegisters StackPtr = RegX8664::Reg_rsp;
  static constexpr RegisterSet::AllRegisters FramePtr = RegX8664::Reg_rbp;
  static constexpr GPRRegister Encoded_Reg_Accumulator =
      RegX8664::Encoded_Reg_eax;
  static constexpr GPRRegister Encoded_Reg_Counter = RegX8664::Encoded_Reg_ecx;
  static constexpr FixupKind FK_PcRel = llvm::ELF::R_X86_64_PC32;
  static constexpr FixupKind FK_Abs = llvm::ELF::R_X86_64_32S;
  static constexpr FixupKind FK_Gotoff = llvm::ELF::R_X86_64_GOTOFF64;
  static constexpr FixupKind FK_GotPC = llvm::ELF::R_X86_64_GOTPC32;

  class Operand {
  public:
    enum RexBits {
      RexNone = 0x00,
      RexBase = 0x40,
      RexW = RexBase | (1 << 3),
      RexR = RexBase | (1 << 2),
      RexX = RexBase | (1 << 1),
      RexB = RexBase | (1 << 0),
    };

  protected:
    // Needed by subclass Address.
    Operand() = default;

  public:
    Operand(const Operand &) = default;
    Operand(Operand &&) = default;
    Operand &operator=(const Operand &) = default;
    Operand &operator=(Operand &&) = default;

    uint8_t mod() const { return (encoding_at(0) >> 6) & 3; }

    uint8_t rexX() const { return (rex_ & RexX) != RexX ? RexNone : RexX; }
    uint8_t rexB() const { return (rex_ & RexB) != RexB ? RexNone : RexB; }

    GPRRegister rm() const {
      return static_cast<GPRRegister>((rexB() != 0 ? 0x08 : 0) |
                                      (encoding_at(0) & 7));
    }

    ScaleFactor scale() const {
      return static_cast<ScaleFactor>((encoding_at(1) >> 6) & 3);
    }

    GPRRegister index() const {
      return static_cast<GPRRegister>((rexX() != 0 ? 0x08 : 0) |
                                      ((encoding_at(1) >> 3) & 7));
    }

    GPRRegister base() const {
      return static_cast<GPRRegister>((rexB() != 0 ? 0x08 : 0) |
                                      (encoding_at(1) & 7));
    }

    int8_t disp8() const {
      assert(length_ >= 2);
      return static_cast<int8_t>(encoding_[length_ - 1]);
    }

    AssemblerFixup *fixup() const { return fixup_; }

  protected:
    void SetModRM(int mod, GPRRegister rm) {
      assert((mod & ~3) == 0);
      encoding_[0] = (mod << 6) | (rm & 0x07);
      rex_ = (rm & 0x08) ? RexB : RexNone;
      length_ = 1;
    }

    void SetSIB(ScaleFactor scale, GPRRegister index, GPRRegister base) {
      assert(length_ == 1);
      assert((scale & ~3) == 0);
      encoding_[1] = (scale << 6) | ((index & 0x07) << 3) | (base & 0x07);
      rex_ =
          ((base & 0x08) ? RexB : RexNone) | ((index & 0x08) ? RexX : RexNone);
      length_ = 2;
    }

    void SetDisp8(int8_t disp) {
      assert(length_ == 1 || length_ == 2);
      encoding_[length_++] = static_cast<uint8_t>(disp);
    }

    void SetDisp32(int32_t disp) {
      assert(length_ == 1 || length_ == 2);
      intptr_t disp_size = sizeof(disp);
      memmove(&encoding_[length_], &disp, disp_size);
      length_ += disp_size;
    }

    void SetFixup(AssemblerFixup *fixup) { fixup_ = fixup; }

  private:
    AssemblerFixup *fixup_ = nullptr;
    uint8_t rex_ = 0;
    uint8_t encoding_[6];
    uint8_t length_ = 0;

    explicit Operand(GPRRegister reg) : fixup_(nullptr) { SetModRM(3, reg); }

    /// Get the operand encoding byte at the given index.
    uint8_t encoding_at(intptr_t index) const {
      assert(index >= 0 && index < length_);
      return encoding_[index];
    }

    /// Returns whether or not this operand is really the given register in
    /// disguise. Used from the assembler to generate better encodings.
    bool IsRegister(GPRRegister reg) const {
      return ((encoding_[0] & 0xF8) ==
              0xC0) // Addressing mode is register only.
             &&
             (rm() == reg); // Register codes match.
    }

    friend class AssemblerX86Base<TargetX8664Traits>;
  };

  class Address : public Operand {
    Address() = default;

  public:
    Address(const Address &) = default;
    Address(Address &&) = default;
    Address &operator=(const Address &) = default;
    Address &operator=(Address &&) = default;

    Address(GPRRegister Base, int32_t Disp, AssemblerFixup *Fixup) {
      if (Fixup == nullptr && Disp == 0 &&
          (Base & 7) != RegX8664::Encoded_Reg_rbp) {
        SetModRM(0, Base);
        if ((Base & 7) == RegX8664::Encoded_Reg_rsp)
          SetSIB(TIMES_1, RegX8664::Encoded_Reg_rsp, Base);
      } else if (Fixup == nullptr && Utils::IsInt(8, Disp)) {
        SetModRM(1, Base);
        if ((Base & 7) == RegX8664::Encoded_Reg_rsp)
          SetSIB(TIMES_1, RegX8664::Encoded_Reg_rsp, Base);
        SetDisp8(Disp);
      } else {
        SetModRM(2, Base);
        if ((Base & 7) == RegX8664::Encoded_Reg_rsp)
          SetSIB(TIMES_1, RegX8664::Encoded_Reg_rsp, Base);
        SetDisp32(Disp);
        if (Fixup)
          SetFixup(Fixup);
      }
    }

    Address(GPRRegister Index, ScaleFactor Scale, int32_t Disp,
            AssemblerFixup *Fixup) {
      assert(Index != RegX8664::Encoded_Reg_rsp); // Illegal addressing mode.
      SetModRM(0, RegX8664::Encoded_Reg_rsp);
      SetSIB(Scale, Index, RegX8664::Encoded_Reg_rbp);
      SetDisp32(Disp);
      if (Fixup)
        SetFixup(Fixup);
    }

    Address(GPRRegister Base, GPRRegister Index, ScaleFactor Scale,
            int32_t Disp, AssemblerFixup *Fixup) {
      assert(Index != RegX8664::Encoded_Reg_rsp); // Illegal addressing mode.
      if (Fixup == nullptr && Disp == 0 &&
          (Base & 7) != RegX8664::Encoded_Reg_rbp) {
        SetModRM(0, RegX8664::Encoded_Reg_rsp);
        SetSIB(Scale, Index, Base);
      } else if (Fixup == nullptr && Utils::IsInt(8, Disp)) {
        SetModRM(1, RegX8664::Encoded_Reg_rsp);
        SetSIB(Scale, Index, Base);
        SetDisp8(Disp);
      } else {
        SetModRM(2, RegX8664::Encoded_Reg_rsp);
        SetSIB(Scale, Index, Base);
        SetDisp32(Disp);
        if (Fixup)
          SetFixup(Fixup);
      }
    }

    /// Generate a RIP-relative address expression on x86-64.
    static Address RipRelative(RelocOffsetT Offset, AssemblerFixup *Fixup) {
      assert(Fixup != nullptr);
      assert(Fixup->kind() == FK_PcRel);
      Address NewAddress;
      NewAddress.SetModRM(0x0, RegX8664::Encoded_Reg_rbp);

      // Use the Offset in the displacement for now. If we decide to process
      // fixups later, we'll need to patch up the emitted displacement.
      NewAddress.SetDisp32(Offset);
      if (Fixup)
        NewAddress.SetFixup(Fixup);

      return NewAddress;
    }

    /// Generate an absolute address.
    static Address Absolute(RelocOffsetT Addr) {
      Address NewAddress;
      NewAddress.SetModRM(0x0, RegX8664::Encoded_Reg_rsp);
      static constexpr ScaleFactor NoScale = TIMES_1;
      NewAddress.SetSIB(NoScale, RegX8664::Encoded_Reg_rsp,
                        RegX8664::Encoded_Reg_rbp);
      NewAddress.SetDisp32(Addr);
      return NewAddress;
    }

    static Address ofConstPool(Assembler *Asm, const Constant *Imm) {
      // TODO(jpp): ???
      AssemblerFixup *Fixup = Asm->createFixup(FK_Abs, Imm);
      const RelocOffsetT Offset = 4;
      return Address::RipRelative(Offset, Fixup);
    }
  };

  //----------------------------------------------------------------------------
  //     __      ______  __     __  ______  ______  __  __   __  ______
  //    /\ \    /\  __ \/\ \  _ \ \/\  ___\/\  == \/\ \/\ "-.\ \/\  ___\
  //    \ \ \___\ \ \/\ \ \ \/ ".\ \ \  __\\ \  __<\ \ \ \ \-.  \ \ \__ \
  //     \ \_____\ \_____\ \__/".~\_\ \_____\ \_\ \_\ \_\ \_\\"\_\ \_____\
  //      \/_____/\/_____/\/_/   \/_/\/_____/\/_/ /_/\/_/\/_/ \/_/\/_____/
  //
  //----------------------------------------------------------------------------
  enum InstructionSet {
    Begin,
    // SSE2 is the PNaCl baseline instruction set.
    SSE2 = Begin,
    SSE4_1,
    End
  };

  static const char *TargetName;
  static constexpr Type WordType = IceType_i64;

  static const char *getRegName(RegNumT RegNum) {
    static const char *const RegNames[RegisterSet::Reg_NUM] = {
#define X(val, encode, name, base, scratch, preserved, stackptr, frameptr,     \
          sboxres, isGPR, is64, is32, is16, is8, isXmm, is64To8, is32To8,      \
          is16To8, isTrunc8Rcvr, isAhRcvr, aliases)                            \
  name,
        REGX8664_TABLE
#undef X
    };
    RegNum.assertIsValid();
    return RegNames[RegNum];
  }

  static GPRRegister getEncodedGPR(RegNumT RegNum) {
    static const GPRRegister GPRRegs[RegisterSet::Reg_NUM] = {
#define X(val, encode, name, base, scratch, preserved, stackptr, frameptr,     \
          sboxres, isGPR, is64, is32, is16, is8, isXmm, is64To8, is32To8,      \
          is16To8, isTrunc8Rcvr, isAhRcvr, aliases)                            \
  GPRRegister(isGPR ? encode : GPRRegister::Encoded_Not_GPR),
        REGX8664_TABLE
#undef X
    };
    RegNum.assertIsValid();
    assert(GPRRegs[RegNum] != GPRRegister::Encoded_Not_GPR);
    return GPRRegs[RegNum];
  }

  static ByteRegister getEncodedByteReg(RegNumT RegNum) {
    static const ByteRegister ByteRegs[RegisterSet::Reg_NUM] = {
#define X(val, encode, name, base, scratch, preserved, stackptr, frameptr,     \
          sboxres, isGPR, is64, is32, is16, is8, isXmm, is64To8, is32To8,      \
          is16To8, isTrunc8Rcvr, isAhRcvr, aliases)                            \
  ByteRegister(is8 ? encode : ByteRegister::Encoded_Not_ByteReg),
        REGX8664_TABLE
#undef X
    };
    RegNum.assertIsValid();
    assert(ByteRegs[RegNum] != ByteRegister::Encoded_Not_ByteReg);
    return ByteRegs[RegNum];
  }

  static XmmRegister getEncodedXmm(RegNumT RegNum) {
    static const XmmRegister XmmRegs[RegisterSet::Reg_NUM] = {
#define X(val, encode, name, base, scratch, preserved, stackptr, frameptr,     \
          sboxres, isGPR, is64, is32, is16, is8, isXmm, is64To8, is32To8,      \
          is16To8, isTrunc8Rcvr, isAhRcvr, aliases)                            \
  XmmRegister(isXmm ? encode : XmmRegister::Encoded_Not_Xmm),
        REGX8664_TABLE
#undef X
    };
    RegNum.assertIsValid();
    assert(XmmRegs[RegNum] != XmmRegister::Encoded_Not_Xmm);
    return XmmRegs[RegNum];
  }

  static uint32_t getEncoding(RegNumT RegNum) {
    static const uint32_t Encoding[RegisterSet::Reg_NUM] = {
#define X(val, encode, name, base, scratch, preserved, stackptr, frameptr,     \
          sboxres, isGPR, is64, is32, is16, is8, isXmm, is64To8, is32To8,      \
          is16To8, isTrunc8Rcvr, isAhRcvr, aliases)                            \
  encode,
        REGX8664_TABLE
#undef X
    };
    RegNum.assertIsValid();
    return Encoding[RegNum];
  }

  static inline RegNumT getBaseReg(RegNumT RegNum) {
    static const RegNumT BaseRegs[RegisterSet::Reg_NUM] = {
#define X(val, encode, name, base, scratch, preserved, stackptr, frameptr,     \
          sboxres, isGPR, is64, is32, is16, is8, isXmm, is64To8, is32To8,      \
          is16To8, isTrunc8Rcvr, isAhRcvr, aliases)                            \
  RegisterSet::base,
        REGX8664_TABLE
#undef X
    };
    RegNum.assertIsValid();
    return BaseRegs[RegNum];
  }

private:
  static RegNumT getFirstGprForType(Type Ty) {
    switch (Ty) {
    default:
      llvm_unreachable("Invalid type for GPR.");
    case IceType_i1:
    case IceType_i8:
      return RegisterSet::Reg_al;
    case IceType_i16:
      return RegisterSet::Reg_ax;
    case IceType_i32:
      return RegisterSet::Reg_eax;
    case IceType_i64:
      return RegisterSet::Reg_rax;
    }
  }

public:
  static RegNumT getGprForType(Type Ty, RegNumT RegNum) {
    assert(RegNum.hasValue());

    if (!isScalarIntegerType(Ty)) {
      return RegNum;
    }

    assert(Ty == IceType_i1 || Ty == IceType_i8 || Ty == IceType_i16 ||
           Ty == IceType_i32 || Ty == IceType_i64);

    if (RegNum == RegisterSet::Reg_ah) {
      assert(Ty == IceType_i8);
      return RegNum;
    }

    assert(RegNum != RegisterSet::Reg_bh);
    assert(RegNum != RegisterSet::Reg_ch);
    assert(RegNum != RegisterSet::Reg_dh);

    const RegNumT FirstGprForType = getFirstGprForType(Ty);

    switch (RegNum) {
    default:
      llvm::report_fatal_error("Unknown register.");
#define X(val, encode, name, base, scratch, preserved, stackptr, frameptr,     \
          sboxres, isGPR, is64, is32, is16, is8, isXmm, is64To8, is32To8,      \
          is16To8, isTrunc8Rcvr, isAhRcvr, aliases)                            \
  case RegisterSet::val: {                                                     \
    if (!isGPR)                                                                \
      return RegisterSet::val;                                                 \
    assert((is64) || (is32) || (is16) || (is8) ||                              \
           getBaseReg(RegisterSet::val) == RegisterSet::Reg_rsp);              \
    constexpr RegisterSet::AllRegisters FirstGprWithRegNumSize =               \
        ((is64) || RegisterSet::val == RegisterSet::Reg_rsp)                   \
            ? RegisterSet::Reg_rax                                             \
            : (((is32) || RegisterSet::val == RegisterSet::Reg_esp)            \
                   ? RegisterSet::Reg_eax                                      \
                   : (((is16) || RegisterSet::val == RegisterSet::Reg_sp)      \
                          ? RegisterSet::Reg_ax                                \
                          : RegisterSet::Reg_al));                             \
    const auto NewRegNum =                                                     \
        RegNumT::fixme(RegNum - FirstGprWithRegNumSize + FirstGprForType);     \
    assert(getBaseReg(RegNum) == getBaseReg(NewRegNum) &&                      \
           "Error involving " #val);                                           \
    return NewRegNum;                                                          \
  }
      REGX8664_TABLE
#undef X
    }
  }

private:
  /// SizeOf is used to obtain the size of an initializer list as a constexpr
  /// expression. This is only needed until our C++ library is updated to
  /// C++ 14 -- which defines constexpr members to std::initializer_list.
  class SizeOf {
    SizeOf(const SizeOf &) = delete;
    SizeOf &operator=(const SizeOf &) = delete;

  public:
    constexpr SizeOf() : Size(0) {}
    template <typename... T>
    explicit constexpr SizeOf(T...)
        : Size(length<T...>::value) {}
    constexpr SizeT size() const { return Size; }

  private:
    template <typename T, typename... U> struct length {
      static constexpr std::size_t value = 1 + length<U...>::value;
    };

    template <typename T> struct length<T> {
      static constexpr std::size_t value = 1;
    };

    const std::size_t Size;
  };

public:
  static void initRegisterSet(
      const ::Ice::ClFlags &Flags,
      std::array<SmallBitVector, RCX86_NUM> *TypeToRegisterSet,
      std::array<SmallBitVector, RegisterSet::Reg_NUM> *RegisterAliases) {
    SmallBitVector IntegerRegistersI64(RegisterSet::Reg_NUM);
    SmallBitVector IntegerRegistersI32(RegisterSet::Reg_NUM);
    SmallBitVector IntegerRegistersI16(RegisterSet::Reg_NUM);
    SmallBitVector IntegerRegistersI8(RegisterSet::Reg_NUM);
    SmallBitVector FloatRegisters(RegisterSet::Reg_NUM);
    SmallBitVector VectorRegisters(RegisterSet::Reg_NUM);
    SmallBitVector Trunc64To8Registers(RegisterSet::Reg_NUM);
    SmallBitVector Trunc32To8Registers(RegisterSet::Reg_NUM);
    SmallBitVector Trunc16To8Registers(RegisterSet::Reg_NUM);
    SmallBitVector Trunc8RcvrRegisters(RegisterSet::Reg_NUM);
    SmallBitVector AhRcvrRegisters(RegisterSet::Reg_NUM);
    SmallBitVector InvalidRegisters(RegisterSet::Reg_NUM);

    static constexpr struct {
      uint16_t Val;
      unsigned IsReservedWhenSandboxing : 1;
      unsigned Is64 : 1;
      unsigned Is32 : 1;
      unsigned Is16 : 1;
      unsigned Is8 : 1;
      unsigned IsXmm : 1;
      unsigned Is64To8 : 1;
      unsigned Is32To8 : 1;
      unsigned Is16To8 : 1;
      unsigned IsTrunc8Rcvr : 1;
      unsigned IsAhRcvr : 1;
#define NUM_ALIASES_BITS 2
      SizeT NumAliases : (NUM_ALIASES_BITS + 1);
      uint16_t Aliases[1 << NUM_ALIASES_BITS];
#undef NUM_ALIASES_BITS
    } X8664RegTable[RegisterSet::Reg_NUM] = {
#define X(val, encode, name, base, scratch, preserved, stackptr, frameptr,     \
          sboxres, isGPR, is64, is32, is16, is8, isXmm, is64To8, is32To8,      \
          is16To8, isTrunc8Rcvr, isAhRcvr, aliases)                            \
  {                                                                            \
    RegisterSet::val, sboxres, is64, is32, is16, is8, isXmm, is64To8, is32To8, \
        is16To8, isTrunc8Rcvr, isAhRcvr, (SizeOf aliases).size(), aliases,     \
  }                                                                            \
  ,
        REGX8664_TABLE
#undef X
    };

    const bool NeedSandboxing = Flags.getUseSandboxing();
    for (SizeT ii = 0; ii < llvm::array_lengthof(X8664RegTable); ++ii) {
      const auto &Entry = X8664RegTable[ii];
      // Even though the register is disabled for register allocation, it might
      // still be used by the Target Lowering (e.g., base pointer), so the
      // register alias table still needs to be defined.
      (*RegisterAliases)[Entry.Val].resize(RegisterSet::Reg_NUM);
      for (int J = 0; J < Entry.NumAliases; ++J) {
        SizeT Alias = Entry.Aliases[J];
        assert(!(*RegisterAliases)[Entry.Val][Alias] && "Duplicate alias");
        (*RegisterAliases)[Entry.Val].set(Alias);
      }

      (*RegisterAliases)[Entry.Val].set(Entry.Val);
      const bool DisabledRegister =
          NeedSandboxing && Entry.IsReservedWhenSandboxing;
      if (DisabledRegister) {
        continue;
      }
      (IntegerRegistersI64)[Entry.Val] = Entry.Is64;
      (IntegerRegistersI32)[Entry.Val] = Entry.Is32;
      (IntegerRegistersI16)[Entry.Val] = Entry.Is16;
      (IntegerRegistersI8)[Entry.Val] = Entry.Is8;
      (FloatRegisters)[Entry.Val] = Entry.IsXmm;
      (VectorRegisters)[Entry.Val] = Entry.IsXmm;
      (Trunc64To8Registers)[Entry.Val] = Entry.Is64To8;
      (Trunc32To8Registers)[Entry.Val] = Entry.Is32To8;
      (Trunc16To8Registers)[Entry.Val] = Entry.Is16To8;
      (Trunc8RcvrRegisters)[Entry.Val] = Entry.IsTrunc8Rcvr;
      (AhRcvrRegisters)[Entry.Val] = Entry.IsAhRcvr;
    }

    (*TypeToRegisterSet)[RC_void] = InvalidRegisters;
    (*TypeToRegisterSet)[RC_i1] = IntegerRegistersI8;
    (*TypeToRegisterSet)[RC_i8] = IntegerRegistersI8;
    (*TypeToRegisterSet)[RC_i16] = IntegerRegistersI16;
    (*TypeToRegisterSet)[RC_i32] = IntegerRegistersI32;
    (*TypeToRegisterSet)[RC_i64] = IntegerRegistersI64;
    (*TypeToRegisterSet)[RC_f32] = FloatRegisters;
    (*TypeToRegisterSet)[RC_f64] = FloatRegisters;
    (*TypeToRegisterSet)[RC_v4i1] = VectorRegisters;
    (*TypeToRegisterSet)[RC_v8i1] = VectorRegisters;
    (*TypeToRegisterSet)[RC_v16i1] = VectorRegisters;
    (*TypeToRegisterSet)[RC_v16i8] = VectorRegisters;
    (*TypeToRegisterSet)[RC_v8i16] = VectorRegisters;
    (*TypeToRegisterSet)[RC_v4i32] = VectorRegisters;
    (*TypeToRegisterSet)[RC_v4f32] = VectorRegisters;
    (*TypeToRegisterSet)[RCX86_Is64To8] = Trunc64To8Registers;
    (*TypeToRegisterSet)[RCX86_Is32To8] = Trunc32To8Registers;
    (*TypeToRegisterSet)[RCX86_Is16To8] = Trunc16To8Registers;
    (*TypeToRegisterSet)[RCX86_IsTrunc8Rcvr] = Trunc8RcvrRegisters;
    (*TypeToRegisterSet)[RCX86_IsAhRcvr] = AhRcvrRegisters;
  }

  static SmallBitVector getRegisterSet(const ::Ice::ClFlags &Flags,
                                       TargetLowering::RegSetMask Include,
                                       TargetLowering::RegSetMask Exclude) {
    SmallBitVector Registers(RegisterSet::Reg_NUM);

    const bool NeedSandboxing = Flags.getUseSandboxing();
#define X(val, encode, name, base, scratch, preserved, stackptr, frameptr,     \
          sboxres, isGPR, is64, is32, is16, is8, isXmm, is64To8, is32To8,      \
          is16To8, isTrunc8Rcvr, isAhRcvr, aliases)                            \
  if (!NeedSandboxing || !(sboxres)) {                                         \
    if (scratch && (Include & ::Ice::TargetLowering::RegSet_CallerSave))       \
      Registers[RegisterSet::val] = true;                                      \
    if (preserved && (Include & ::Ice::TargetLowering::RegSet_CalleeSave))     \
      Registers[RegisterSet::val] = true;                                      \
    if (stackptr && (Include & ::Ice::TargetLowering::RegSet_StackPointer))    \
      Registers[RegisterSet::val] = true;                                      \
    if (frameptr && (Include & ::Ice::TargetLowering::RegSet_FramePointer))    \
      Registers[RegisterSet::val] = true;                                      \
    if (scratch && (Exclude & ::Ice::TargetLowering::RegSet_CallerSave))       \
      Registers[RegisterSet::val] = false;                                     \
    if (preserved && (Exclude & ::Ice::TargetLowering::RegSet_CalleeSave))     \
      Registers[RegisterSet::val] = false;                                     \
    if (stackptr && (Exclude & ::Ice::TargetLowering::RegSet_StackPointer))    \
      Registers[RegisterSet::val] = false;                                     \
    if (frameptr && (Exclude & ::Ice::TargetLowering::RegSet_FramePointer))    \
      Registers[RegisterSet::val] = false;                                     \
  }

    REGX8664_TABLE

#undef X

    return Registers;
  }

  static void makeRandomRegisterPermutation(
      Cfg *Func, llvm::SmallVectorImpl<RegNumT> &Permutation,
      const SmallBitVector &ExcludeRegisters, uint64_t Salt) {
    // TODO(stichnot): Declaring Permutation this way loses type/size
    // information. Fix this in conjunction with the caller-side TODO.
    assert(Permutation.size() >= RegisterSet::Reg_NUM);
    // Expected upper bound on the number of registers in a single equivalence
    // class.  For x86-64, this would comprise the 16 XMM registers. This is
    // for performance, not correctness.
    static const unsigned MaxEquivalenceClassSize = 8;
    using RegisterList = llvm::SmallVector<RegNumT, MaxEquivalenceClassSize>;
    using EquivalenceClassMap = std::map<uint32_t, RegisterList>;
    EquivalenceClassMap EquivalenceClasses;
    SizeT NumShuffled = 0, NumPreserved = 0;

// Build up the equivalence classes of registers by looking at the register
// properties as well as whether the registers should be explicitly excluded
// from shuffling.
#define X(val, encode, name, base, scratch, preserved, stackptr, frameptr,     \
          sboxres, isGPR, is64, is32, is16, is8, isXmm, is64To8, is32To8,      \
          is16To8, isTrunc8Rcvr, isAhRcvr, aliases)                            \
  if (ExcludeRegisters[RegisterSet::val]) {                                    \
    /* val stays the same in the resulting permutation. */                     \
    Permutation[RegisterSet::val] = RegisterSet::val;                          \
    ++NumPreserved;                                                            \
  } else {                                                                     \
    uint32_t AttrKey = 0;                                                      \
    uint32_t Index = 0;                                                        \
    /* Combine relevant attributes into an equivalence class key. */           \
    Index |= (scratch << (AttrKey++));                                         \
    Index |= (preserved << (AttrKey++));                                       \
    Index |= (is8 << (AttrKey++));                                             \
    Index |= (is16 << (AttrKey++));                                            \
    Index |= (is32 << (AttrKey++));                                            \
    Index |= (is64 << (AttrKey++));                                            \
    Index |= (isXmm << (AttrKey++));                                           \
    Index |= (is16To8 << (AttrKey++));                                         \
    Index |= (is32To8 << (AttrKey++));                                         \
    Index |= (is64To8 << (AttrKey++));                                         \
    Index |= (isTrunc8Rcvr << (AttrKey++));                                    \
    /* val is assigned to an equivalence class based on its properties. */     \
    EquivalenceClasses[Index].push_back(RegisterSet::val);                     \
  }
    REGX8664_TABLE
#undef X

    // Create a random number generator for regalloc randomization.
    RandomNumberGenerator RNG(getFlags().getRandomSeed(),
                              RPE_RegAllocRandomization, Salt);
    RandomNumberGeneratorWrapper RNGW(RNG);

    // Shuffle the resulting equivalence classes.
    for (auto I : EquivalenceClasses) {
      const RegisterList &List = I.second;
      RegisterList Shuffled(List);
      RandomShuffle(Shuffled.begin(), Shuffled.end(), RNGW);
      for (size_t SI = 0, SE = Shuffled.size(); SI < SE; ++SI) {
        Permutation[List[SI]] = Shuffled[SI];
        ++NumShuffled;
      }
    }

    assert(NumShuffled + NumPreserved == RegisterSet::Reg_NUM);

    if (Func->isVerbose(IceV_Random)) {
      OstreamLocker L(Func->getContext());
      Ostream &Str = Func->getContext()->getStrDump();
      Str << "Register equivalence classes:\n";
      for (auto I : EquivalenceClasses) {
        Str << "{";
        const RegisterList &List = I.second;
        bool First = true;
        for (RegNumT Register : List) {
          if (!First)
            Str << " ";
          First = false;
          Str << getRegName(Register);
        }
        Str << "}\n";
      }
    }
  }

  static RegNumT getRaxOrDie() { return RegisterSet::Reg_rax; }

  static RegNumT getRdxOrDie() { return RegisterSet::Reg_rdx; }

#if defined(SUBZERO_USE_MICROSOFT_ABI)
  // Microsoft x86-64 calling convention:
  //
  // * The first four arguments of vector/fp type, regardless of their
  // position relative to the other arguments in the argument list, are placed
  // in registers %xmm0 - %xmm3.
  //
  // * The first four arguments of integer types, regardless of their position
  // relative to the other arguments in the argument list, are placed in
  // registers %rcx, %rdx, %r8, and %r9.

  /// The maximum number of arguments to pass in XMM registers
  static constexpr uint32_t X86_MAX_XMM_ARGS = 4;
  /// The maximum number of arguments to pass in GPR registers
  static constexpr uint32_t X86_MAX_GPR_ARGS = 4;
  static RegNumT getRegisterForGprArgNum(Type Ty, uint32_t ArgNum) {
    if (ArgNum >= X86_MAX_GPR_ARGS) {
      return RegNumT();
    }
    static const RegisterSet::AllRegisters GprForArgNum[] = {
        RegisterSet::Reg_rcx, RegisterSet::Reg_rdx, RegisterSet::Reg_r8,
        RegisterSet::Reg_r9,
    };
    static_assert(llvm::array_lengthof(GprForArgNum) == X86_MAX_GPR_ARGS,
                  "Mismatch between MAX_GPR_ARGS and GprForArgNum.");
    assert(Ty == IceType_i64 || Ty == IceType_i32);
    return getGprForType(Ty, GprForArgNum[ArgNum]);
  }
#else
  // System V x86-64 calling convention:
  //
  // * The first eight arguments of vector/fp type, regardless of their
  // position relative to the other arguments in the argument list, are placed
  // in registers %xmm0 - %xmm7.
  //
  // * The first six arguments of integer types, regardless of their position
  // relative to the other arguments in the argument list, are placed in
  // registers %rdi, %rsi, %rdx, %rcx, %r8, and %r9.
  //
  // This intends to match the section "Function Calling Sequence" of the
  // document "System V Application Binary Interface."

  /// The maximum number of arguments to pass in XMM registers
  static constexpr uint32_t X86_MAX_XMM_ARGS = 8;
  /// The maximum number of arguments to pass in GPR registers
  static constexpr uint32_t X86_MAX_GPR_ARGS = 6;
  /// Get the register for a given argument slot in the GPRs.
  static RegNumT getRegisterForGprArgNum(Type Ty, uint32_t ArgNum) {
    if (ArgNum >= X86_MAX_GPR_ARGS) {
      return RegNumT();
    }
    static const RegisterSet::AllRegisters GprForArgNum[] = {
        RegisterSet::Reg_rdi, RegisterSet::Reg_rsi, RegisterSet::Reg_rdx,
        RegisterSet::Reg_rcx, RegisterSet::Reg_r8,  RegisterSet::Reg_r9,
    };
    static_assert(llvm::array_lengthof(GprForArgNum) == X86_MAX_GPR_ARGS,
                  "Mismatch between MAX_GPR_ARGS and GprForArgNum.");
    assert(Ty == IceType_i64 || Ty == IceType_i32);
    return getGprForType(Ty, GprForArgNum[ArgNum]);
  }
#endif

  /// Whether scalar floating point arguments are passed in XMM registers
  static constexpr bool X86_PASS_SCALAR_FP_IN_XMM = true;
  /// Get the register for a given argument slot in the XMM registers.
  static RegNumT getRegisterForXmmArgNum(uint32_t ArgNum) {
    // TODO(sehr): Change to use the CCArg technique used in ARM32.
    static_assert(RegisterSet::Reg_xmm0 + 1 == RegisterSet::Reg_xmm1,
                  "Inconsistency between XMM register numbers and ordinals");
    if (ArgNum >= X86_MAX_XMM_ARGS) {
      return RegNumT();
    }
    return RegNumT::fixme(RegisterSet::Reg_xmm0 + ArgNum);
  }

  /// The number of bits in a byte
  static constexpr uint32_t X86_CHAR_BIT = 8;
  /// Stack alignment. This is defined in IceTargetLoweringX8664.cpp because it
  /// is used as an argument to std::max(), and the default std::less<T> has an
  /// operator(T const&, T const&) which requires this member to have an
  /// address.
  static const uint32_t X86_STACK_ALIGNMENT_BYTES;
  /// Size of the return address on the stack
  static constexpr uint32_t X86_RET_IP_SIZE_BYTES = 8;
  /// The number of different NOP instructions
  static constexpr uint32_t X86_NUM_NOP_VARIANTS = 5;

  /// \name Limits for unrolling memory intrinsics.
  /// @{
  static constexpr uint32_t MEMCPY_UNROLL_LIMIT = 8;
  static constexpr uint32_t MEMMOVE_UNROLL_LIMIT = 8;
  static constexpr uint32_t MEMSET_UNROLL_LIMIT = 8;
  /// @}

  /// Value is in bytes. Return Value adjusted to the next highest multiple of
  /// the stack alignment.
  static uint32_t applyStackAlignment(uint32_t Value) {
    return Utils::applyAlignment(Value, X86_STACK_ALIGNMENT_BYTES);
  }

  /// Return the type which the elements of the vector have in the X86
  /// representation of the vector.
  static Type getInVectorElementType(Type Ty) {
    assert(isVectorType(Ty));
    assert(Ty < TableTypeX8664AttributesSize);
    return TableTypeX8664Attributes[Ty].InVectorElementType;
  }

  // Note: The following data structures are defined in
  // IceTargetLoweringX8664.cpp.

  /// The following table summarizes the logic for lowering the fcmp
  /// instruction. There is one table entry for each of the 16 conditions.
  ///
  /// The first four columns describe the case when the operands are floating
  /// point scalar values. A comment in lowerFcmp() describes the lowering
  /// template. In the most general case, there is a compare followed by two
  /// conditional branches, because some fcmp conditions don't map to a single
  /// x86 conditional branch. However, in many cases it is possible to swap the
  /// operands in the comparison and have a single conditional branch. Since
  /// it's quite tedious to validate the table by hand, good execution tests are
  /// helpful.
  ///
  /// The last two columns describe the case when the operands are vectors of
  /// floating point values. For most fcmp conditions, there is a clear mapping
  /// to a single x86 cmpps instruction variant. Some fcmp conditions require
  /// special code to handle and these are marked in the table with a
  /// Cmpps_Invalid predicate.
  /// {@
  static const struct TableFcmpType {
    uint32_t Default;
    bool SwapScalarOperands;
    Cond::BrCond C1, C2;
    bool SwapVectorOperands;
    Cond::CmppsCond Predicate;
  } TableFcmp[];
  static const size_t TableFcmpSize;
  /// @}

  /// The following table summarizes the logic for lowering the icmp instruction
  /// for i32 and narrower types. Each icmp condition has a clear mapping to an
  /// x86 conditional branch instruction.
  /// {@
  static const struct TableIcmp32Type { Cond::BrCond Mapping; } TableIcmp32[];
  static const size_t TableIcmp32Size;
  /// @}

  /// The following table summarizes the logic for lowering the icmp instruction
  /// for the i64 type. For Eq and Ne, two separate 32-bit comparisons and
  /// conditional branches are needed. For the other conditions, three separate
  /// conditional branches are needed.
  /// {@
  static const struct TableIcmp64Type {
    Cond::BrCond C1, C2, C3;
  } TableIcmp64[];
  static const size_t TableIcmp64Size;
  /// @}

  static Cond::BrCond getIcmp32Mapping(InstIcmp::ICond Cond) {
    assert(Cond < TableIcmp32Size);
    return TableIcmp32[Cond].Mapping;
  }

  static const struct TableTypeX8664AttributesType {
    Type InVectorElementType;
  } TableTypeX8664Attributes[];
  static const size_t TableTypeX8664AttributesSize;

  //----------------------------------------------------------------------------
  //      __  __   __  ______  ______
  //    /\ \/\ "-.\ \/\  ___\/\__  _\
  //    \ \ \ \ \-.  \ \___  \/_/\ \/
  //     \ \_\ \_\\"\_\/\_____\ \ \_\
  //      \/_/\/_/ \/_/\/_____/  \/_/
  //
  //----------------------------------------------------------------------------
  using Traits = TargetX8664Traits;
  using Insts = ::Ice::X8664::Insts<Traits>;

  using TargetLowering = ::Ice::X8664::TargetX86Base<Traits>;
  using ConcreteTarget = ::Ice::X8664::TargetX8664;
  using Assembler = ::Ice::X8664::AssemblerX86Base<Traits>;

  /// X86Operand extends the Operand hierarchy. Its subclasses are X86OperandMem
  /// and VariableSplit.
  class X86Operand : public ::Ice::Operand {
    X86Operand() = delete;
    X86Operand(const X86Operand &) = delete;
    X86Operand &operator=(const X86Operand &) = delete;

  public:
    enum OperandKindX8664 { k__Start = ::Ice::Operand::kTarget, kMem, kSplit };
    using ::Ice::Operand::dump;

    void dump(const Cfg *, Ostream &Str) const override;

  protected:
    X86Operand(OperandKindX8664 Kind, Type Ty)
        : Operand(static_cast<::Ice::Operand::OperandKind>(Kind), Ty) {}
  };

  /// X86OperandMem represents the m64 addressing mode, with optional base and
  /// index registers, a constant offset, and a fixed shift value for the index
  /// register.
  class X86OperandMem : public X86Operand {
    X86OperandMem() = delete;
    X86OperandMem(const X86OperandMem &) = delete;
    X86OperandMem &operator=(const X86OperandMem &) = delete;

  public:
    enum SegmentRegisters { DefaultSegment = -1, SegReg_NUM };
    static X86OperandMem *
    create(Cfg *Func, Type Ty, Variable *Base, Constant *Offset,
           Variable *Index = nullptr, uint16_t Shift = 0,
           SegmentRegisters SegmentRegister = DefaultSegment,
           bool IsRebased = false) {
      assert(SegmentRegister == DefaultSegment);
      (void)SegmentRegister;
      return new (Func->allocate<X86OperandMem>())
          X86OperandMem(Func, Ty, Base, Offset, Index, Shift, IsRebased);
    }
    static X86OperandMem *create(Cfg *Func, Type Ty, Variable *Base,
                                 Constant *Offset, bool IsRebased) {
      constexpr Variable *NoIndex = nullptr;
      constexpr uint16_t NoShift = 0;
      return new (Func->allocate<X86OperandMem>())
          X86OperandMem(Func, Ty, Base, Offset, NoIndex, NoShift, IsRebased);
    }
    Variable *getBase() const { return Base; }
    Constant *getOffset() const { return Offset; }
    Variable *getIndex() const { return Index; }
    uint16_t getShift() const { return Shift; }
    SegmentRegisters getSegmentRegister() const { return DefaultSegment; }
    void emitSegmentOverride(Assembler *) const {}
    bool getIsRebased() const { return IsRebased; }
    Address toAsmAddress(Assembler *Asm, const Ice::TargetLowering *Target,
                         bool IsLeaAddr = false) const;

    void emit(const Cfg *Func) const override;
    using X86Operand::dump;
    void dump(const Cfg *Func, Ostream &Str) const override;

    static bool classof(const Operand *Operand) {
      return Operand->getKind() == static_cast<OperandKind>(kMem);
    }

    void setRandomized(bool R) { Randomized = R; }

    bool getRandomized() const { return Randomized; }

  private:
    X86OperandMem(Cfg *Func, Type Ty, Variable *Base, Constant *Offset,
                  Variable *Index, uint16_t Shift, bool IsRebased);

    Variable *const Base;
    Constant *const Offset;
    Variable *const Index;
    const uint16_t Shift;
    const bool IsRebased;
    /// A flag to show if this memory operand is a randomized one. Randomized
    /// memory operands are generated in
    /// TargetX86Base::randomizeOrPoolImmediate()
    bool Randomized = false;
  };

  /// VariableSplit is a way to treat an f64 memory location as a pair of i32
  /// locations (Low and High). This is needed for some cases of the Bitcast
  /// instruction. Since it's not possible for integer registers to access the
  /// XMM registers and vice versa, the lowering forces the f64 to be spilled to
  /// the stack and then accesses through the VariableSplit.
  // TODO(jpp): remove references to VariableSplit from IceInstX86Base as 64bit
  // targets can natively handle these.
  class VariableSplit : public X86Operand {
    VariableSplit() = delete;
    VariableSplit(const VariableSplit &) = delete;
    VariableSplit &operator=(const VariableSplit &) = delete;

  public:
    enum Portion { Low, High };
    static VariableSplit *create(Cfg *Func, Variable *Var, Portion Part) {
      return new (Func->allocate<VariableSplit>())
          VariableSplit(Func, Var, Part);
    }
    int32_t getOffset() const { return Part == High ? 4 : 0; }

    Address toAsmAddress(const Cfg *Func) const;
    void emit(const Cfg *Func) const override;
    using X86Operand::dump;
    void dump(const Cfg *Func, Ostream &Str) const override;

    static bool classof(const Operand *Operand) {
      return Operand->getKind() == static_cast<OperandKind>(kSplit);
    }

  private:
    VariableSplit(Cfg *Func, Variable *Var, Portion Part)
        : X86Operand(kSplit, IceType_i32), Var(Var), Part(Part) {
      assert(Var->getType() == IceType_f64);
      Vars = Func->allocateArrayOf<Variable *>(1);
      Vars[0] = Var;
      NumVars = 1;
    }

    Variable *Var;
    Portion Part;
  };

  // Note: The following data structures are defined in IceInstX8664.cpp.

  static const struct InstBrAttributesType {
    Cond::BrCond Opposite;
    const char *DisplayString;
    const char *EmitString;
  } InstBrAttributes[];

  static const struct InstCmppsAttributesType {
    const char *EmitString;
  } InstCmppsAttributes[];

  static const struct TypeAttributesType {
    const char *CvtString;      // i (integer), s (single FP), d (double FP)
    const char *SdSsString;     // ss, sd, or <blank>
    const char *PdPsString;     // ps, pd, or <blank>
    const char *SpSdString;     // ss, sd, ps, pd, or <blank>
    const char *IntegralString; // b, w, d, or <blank>
    const char *UnpackString;   // bw, wd, dq, or <blank>
    const char *PackString;     // wb, dw, or <blank>
    const char *WidthString;    // b, w, l, q, or <blank>
    const char *FldString;      // s, l, or <blank>
  } TypeAttributes[];
};

using Traits = ::Ice::X8664::TargetX8664Traits;
} // end of namespace X8664

} // end of namespace Ice

#endif // SUBZERO_SRC_ICETARGETLOWERINGX8664TRAITS_H
