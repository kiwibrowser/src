/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_MIPS_INST_CLASSES_H
#define NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_MIPS_INST_CLASSES_H

#include <stdint.h>
#include "native_client/src/trusted/validator_mips/model.h"
#include "native_client/src/include/portability.h"


/*
 * Models the "instruction classes" that the decoder produces.
 */
namespace nacl_mips_dec {

/*
 * Used to describe whether an instruction is safe, and if not, what the issue
 * is.  Only instructions that MAY_BE_SAFE should be allowed in untrusted code,
 * and even those may be rejected by the validator.
 */
enum SafetyLevel {
  /*
   * The initial value of uninitialized SafetyLevels -- treat as unsafe.
   */
  UNKNOWN = 0,
  /*
   * This instruction is forbidden by our SFI model.
   */
  FORBIDDEN,
  /*
   * This instruction may be safe in untrusted code: in isolation it contains
   * nothing scary, but the validator may overrule this during global analysis.
   */
  MAY_BE_SAFE
};


// Function (op)codes.
uint32_t const kBitwiseLogicalAnd = 0x24;  //  b100100.


/*
 * Decodes a class of instructions.  Does spooky undefined things if handed
 * instructions that don't belong to its class.  Who defines which instructions
 * these are?  Why, the generated decoder, of course.
 *
 * This is an abstract base class intended to be overridden with the details of
 * particular instruction-classes.
 *
 * ClassDecoders should be stateless, and should provide a no-arg constructor
 * for use by the generated decoder.
 */
class ClassDecoder {
 public:
  /*
   * Checks how safe this instruction is, in isolation.
   * This will detect any violation in the MIPS spec -- undefined encodings,
   * use of registers that are unpredictable -- and the most basic constraints
   * in our SFI model.  Because ClassDecoders are referentially-transparent and
   * cannot touch global state, this will not check things that may vary with
   * ABI version.
   *
   * The most positive result this can return is called MAY_BE_SAFE because it
   * is necessary, but not sufficient: the validator has the final say.
   */
  virtual SafetyLevel safety(const Instruction instr) const = 0;

  /*
   * For instructions that perform 'masking', this function will return whether
   * this is true or not for the given instruction.
   *
   * The result is useful only for Arithm3 'and' instruction.
   */
  virtual bool IsMask(const Instruction instr,
                      const nacl_mips_dec::Register dest,
                      const nacl_mips_dec::Register mask) const {
    UNREFERENCED_PARAMETER(instr);
    UNREFERENCED_PARAMETER(dest);
    UNREFERENCED_PARAMETER(mask);
    return false;
  }

  /*
   * The gpr register altered by the instruction.
   */
  virtual Register DestGprReg(const Instruction instr) const {
    UNREFERENCED_PARAMETER(instr);
    return Register::None();
  }

  /*
   * May be used for instr's with immediate operand; like addiu or jal.
   */
  virtual uint32_t GetImm(const Instruction instr) const {
    UNREFERENCED_PARAMETER(instr);
    return -1;
  }

  /*
   * For direct jumps (j, jal, branch instructions).
   */
  virtual bool IsDirectJump() const {
    return false;
  }

  /*
   * For jump and link (jal, jalr, bal).
   */
  virtual bool IsJal() const {
    return false;
  }

  /*
   * For jump register instructions (jr, jalr).
   */
  virtual bool IsJmpReg() const {
    return false;
  }

  /*
   * For the instructions that are followed by a delay slot.
   */
  virtual bool HasDelaySlot() const {
    return IsDirectJump() || IsJmpReg();
  }

  /*
   * For load and store instructions.
   */
  virtual bool IsLoadStore() const {
    return false;
  }

  /*
   * For lw instruction.
   */
  virtual bool IsLoadWord() const {
    return false;
  }

  /*
   * For direct jumps, returning the destination address.
   */
  virtual uint32_t DestAddr(const Instruction instr, uint32_t addr) const {
    UNREFERENCED_PARAMETER(instr);
    UNREFERENCED_PARAMETER(addr);
    return 0;
  }

  /*
   * Used by jump register instructions; returns the register that holds the
   * address to jump to.
   */
  virtual Register TargetReg(const Instruction instr) const {
    UNREFERENCED_PARAMETER(instr);
    return Register::None();
  }

  /*
   * Base address register, for load and store instructions.
   */
  virtual Register BaseAddressRegister(const Instruction instr) const {
    UNREFERENCED_PARAMETER(instr);
    return Register::None();
  }


 protected:
  ClassDecoder() {}
  virtual ~ClassDecoder() {}
};

/*
 * Current MIPS NaCl halt (break).
 */
class NaClHalt : public ClassDecoder {
 public:
  virtual ~NaClHalt() {}
  virtual SafetyLevel safety(const Instruction instr) const {
    UNREFERENCED_PARAMETER(instr);
    return MAY_BE_SAFE;
  }
};

/*
 * Represents an instruction that is forbidden under all circumstances, so we
 * didn't bother decoding it further.
 */
class Forbidden : public ClassDecoder {
 public:
  virtual ~Forbidden() {}
  virtual SafetyLevel safety(const Instruction instr) const {
    UNREFERENCED_PARAMETER(instr);
    return FORBIDDEN;
  }
};

/*
 * Instructions with 2 registers and an immediate value, where bits 20-16
 * contain the destination gpr register.
 */
class Arithm2 : public ClassDecoder {
 public:
  virtual ~Arithm2() {}
  virtual Register DestGprReg(const Instruction instr) const {
    return instr.Reg(20, 16);
  }
  virtual SafetyLevel safety(const Instruction instr) const {
    UNREFERENCED_PARAMETER(instr);
    return MAY_BE_SAFE;
  }
};

/*
 * Instruction with 3 registers, with bits 15-11 containing the destination gpr
 * register.
 */
class Arithm3 : public ClassDecoder {
 public:
  virtual ~Arithm3() {}
  virtual Register DestGprReg(const Instruction instr) const {
    return instr.Reg(15, 11);
  }
  virtual SafetyLevel safety(const Instruction instr) const {
    UNREFERENCED_PARAMETER(instr);
    return MAY_BE_SAFE;
  }
  virtual bool IsMask(const Instruction instr,
                      const nacl_mips_dec::Register dest,
                      const nacl_mips_dec::Register mask) const {
    return ((instr.Bits(5, 0) == kBitwiseLogicalAnd)
            && instr.Reg(15, 11).Equals(dest)
            && instr.Reg(25, 21).Equals(dest)
            && instr.Reg(20, 16).Equals(mask));
  }
};

/*
 * Direct jump class, subclassed by Branch and JmpImm.
 */
class DirectJump : public ClassDecoder {
 public:
  virtual ~DirectJump() {}
  virtual SafetyLevel safety(const Instruction instr) const {
    UNREFERENCED_PARAMETER(instr);
    return MAY_BE_SAFE;
  }
  virtual bool IsDirectJump() const {
    return true;
  }
};

/*
 * Branch instructions.
 */
class Branch : public DirectJump {
 public:
  virtual ~Branch() {}
  virtual uint32_t GetImm(const Instruction instr) const {
    return instr.Bits(15, 0);
  }
  virtual uint32_t DestAddr(const Instruction instr, uint32_t addr) const {
    return ((addr + kInstrSize) +  ((int16_t)GetImm(instr) << 2));
  }
};

/*
 * Branch and link instructions (bal, bgezal, bltzal, bgezall, bltzall).
 */
class BranchAndLink : public Branch {
 public:
  virtual ~BranchAndLink() {}
  virtual bool IsJal() const {
    return true;
  }
};

/*
 * Load and store instructions.
 */
class AbstractLoadStore : public ClassDecoder {
 public:
  virtual bool IsLoadStore() const {
    return true;
  }
  virtual ~AbstractLoadStore() {}
  virtual SafetyLevel safety(const Instruction instr) const {
    UNREFERENCED_PARAMETER(instr);
    return MAY_BE_SAFE;
  }
  virtual Register BaseAddressRegister(const Instruction instr) const {
    return instr.Reg(25, 21);
  }
  virtual uint32_t GetImm(const Instruction instr) const {
    return instr.Bits(15, 0);
  }
};

/*
 * Store instructions.
 */
class Store : public AbstractLoadStore {
 public:
  virtual ~Store() {}
};

/*
 * Load instructions, which alter the destination register.
 */
class Load : public AbstractLoadStore {
 public:
  virtual ~Load() {}
  virtual Register DestGprReg(const Instruction instr) const {
    return instr.Reg(20, 16);
  }
};

/*
 * Load word instruction, for loading thread pointer.
 */
class LoadWord : public Load {
 public:
  virtual ~LoadWord() {}
  virtual bool IsLoadWord() const {
    return true;
  }
};

  /*
 * Floating point load and store instructions.
 */
class FPLoadStore : public AbstractLoadStore {
 public:
  virtual ~FPLoadStore() {}
};

/*
 * Store Conditional class, containing the sc instruction,
 * which might alter the contents of the register which is the 1st operand.
 */
class StoreConditional : public Store {
 public:
  virtual ~StoreConditional() {}
  virtual Register DestGprReg(const Instruction instr) const {
    return instr.Reg(20, 16);
  }
};

/*
 * Direct jumps - j, jal.
 */
class JmpImm : public DirectJump {
 public:
  virtual ~JmpImm() {}
  virtual uint32_t GetImm(const Instruction instr) const {
    return instr.Bits(25, 0);
  }
  virtual uint32_t DestAddr(const Instruction instr, uint32_t addr) const {
    return ((addr + kInstrSize) & 0xf0000000) + (GetImm(instr) << 2);
  }
};

/*
 * Direct jump and link (jal).
 */
class JalImm : public JmpImm {
 public:
  virtual ~JalImm() {}
  virtual bool IsJal() const {
    return true;
  }
};

/*
 * Jump register instructions - jr, jalr.
 */
class JmpReg : public ClassDecoder {
 public:
  virtual ~JmpReg() {}
  virtual SafetyLevel safety(const Instruction instr) const {
    UNREFERENCED_PARAMETER(instr);
    return MAY_BE_SAFE;
  }
  virtual bool IsJmpReg() const {
    return true;
  }
  virtual Register TargetReg(const Instruction instr) const {
    return instr.Reg(25, 21);
  }
};

/*
 * Jump and link register - jalr.
 */
class JalReg : public JmpReg {
 public:
  virtual ~JalReg() {}
  virtual bool IsJal() const {
    return true;
  }
  virtual Register DestGprReg(const Instruction instr) const {
    return instr.Reg(15, 11);
  }
};

/*
 * ext and ins instructions.
 */
class ExtIns : public ClassDecoder {
 public:
  virtual ~ExtIns() {}
  virtual Register DestGprReg(const Instruction instr) const {
    return instr.Reg(20, 16);
  }
  virtual SafetyLevel safety(const Instruction instr) const {
    UNREFERENCED_PARAMETER(instr);
    return MAY_BE_SAFE;
  }
};
/*
 * The instructions that are safe under all circumstances.
 */
class Safe : public ClassDecoder {
 public:
  virtual ~Safe() {}
  virtual SafetyLevel safety(const Instruction instr) const {
    UNREFERENCED_PARAMETER(instr);
    return MAY_BE_SAFE;
  }
};

class Other : public ClassDecoder {
 public:
  virtual ~Other() {}
  virtual SafetyLevel safety(const Instruction instr) const {
    UNREFERENCED_PARAMETER(instr);
    return FORBIDDEN;
  }
};

/*
 * Unknown instructions, treated as forbidden.
 */
class Unrecognized : public ClassDecoder {
 public:
  virtual ~Unrecognized() {}
  virtual SafetyLevel safety(const Instruction instr) const {
    UNREFERENCED_PARAMETER(instr);
    return FORBIDDEN;
  }
};
}  // namespace nacl_mips_dec

#endif  // NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_MIPS_INST_CLASSES_H
