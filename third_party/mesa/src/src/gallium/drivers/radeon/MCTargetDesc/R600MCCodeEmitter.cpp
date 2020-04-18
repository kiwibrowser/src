//===- R600MCCodeEmitter.cpp - Code Emitter for R600->Cayman GPU families -===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This code emitters outputs bytecode that is understood by the r600g driver
// in the Mesa [1] project.  The bytecode is very similar to the hardware's ISA,
// except that the size of the instruction fields are rounded up to the
// nearest byte.
//
// [1] http://www.mesa3d.org/
//
//===----------------------------------------------------------------------===//

#include "R600Defines.h"
#include "MCTargetDesc/AMDGPUMCTargetDesc.h"
#include "MCTargetDesc/AMDGPUMCCodeEmitter.h"
#include "llvm/MC/MCCodeEmitter.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Support/raw_ostream.h"

#include <stdio.h>

#define SRC_BYTE_COUNT 11
#define DST_BYTE_COUNT 5

using namespace llvm;

namespace {

class R600MCCodeEmitter : public AMDGPUMCCodeEmitter {
  R600MCCodeEmitter(const R600MCCodeEmitter &); // DO NOT IMPLEMENT
  void operator=(const R600MCCodeEmitter &); // DO NOT IMPLEMENT
  const MCInstrInfo &MCII;
  const MCSubtargetInfo &STI;
  MCContext &Ctx;

public:

  R600MCCodeEmitter(const MCInstrInfo &mcii, const MCSubtargetInfo &sti,
                    MCContext &ctx)
    : MCII(mcii), STI(sti), Ctx(ctx) { }

  /// EncodeInstruction - Encode the instruction and write it to the OS.
  virtual void EncodeInstruction(const MCInst &MI, raw_ostream &OS,
                         SmallVectorImpl<MCFixup> &Fixups) const;

  /// getMachineOpValue - Reutrn the encoding for an MCOperand.
  virtual uint64_t getMachineOpValue(const MCInst &MI, const MCOperand &MO,
                                     SmallVectorImpl<MCFixup> &Fixups) const;
private:

  void EmitALUInstr(const MCInst &MI, SmallVectorImpl<MCFixup> &Fixups,
                    raw_ostream &OS) const;
  void EmitSrc(const MCInst &MI, unsigned OpIdx, raw_ostream &OS) const;
  void EmitDst(const MCInst &MI, raw_ostream &OS) const;
  void EmitALU(const MCInst &MI, unsigned numSrc,
               SmallVectorImpl<MCFixup> &Fixups,
               raw_ostream &OS) const;
  void EmitTexInstr(const MCInst &MI, SmallVectorImpl<MCFixup> &Fixups,
                    raw_ostream &OS) const;
  void EmitFCInstr(const MCInst &MI, raw_ostream &OS) const;

  void EmitNullBytes(unsigned int byteCount, raw_ostream &OS) const;

  void EmitByte(unsigned int byte, raw_ostream &OS) const;

  void EmitTwoBytes(uint32_t bytes, raw_ostream &OS) const;

  void Emit(uint32_t value, raw_ostream &OS) const;
  void Emit(uint64_t value, raw_ostream &OS) const;

  unsigned getHWRegIndex(unsigned reg) const;
  unsigned getHWRegChan(unsigned reg) const;
  unsigned getHWReg(unsigned regNo) const;

  bool isFCOp(unsigned opcode) const;
  bool isTexOp(unsigned opcode) const;
  bool isFlagSet(const MCInst &MI, unsigned Operand, unsigned Flag) const;

  /// getHWRegIndexGen - Get the register's hardware index.  Implemented in
  /// R600HwRegInfo.include.
  unsigned getHWRegIndexGen(unsigned int Reg) const;

  /// getHWRegChanGen - Get the register's channel.  Implemented in
  /// R600HwRegInfo.include.
  unsigned getHWRegChanGen(unsigned int Reg) const;
};

} // End anonymous namespace

enum RegElement {
  ELEMENT_X = 0,
  ELEMENT_Y,
  ELEMENT_Z,
  ELEMENT_W
};

enum InstrTypes {
  INSTR_ALU = 0,
  INSTR_TEX,
  INSTR_FC,
  INSTR_NATIVE,
  INSTR_VTX
};

enum FCInstr {
  FC_IF = 0,
  FC_IF_INT,
  FC_ELSE,
  FC_ENDIF,
  FC_BGNLOOP,
  FC_ENDLOOP,
  FC_BREAK,
  FC_BREAK_NZ_INT,
  FC_CONTINUE,
  FC_BREAK_Z_INT,
  FC_BREAK_NZ
};

enum TextureTypes {
  TEXTURE_1D = 1,
  TEXTURE_2D,
  TEXTURE_3D,
  TEXTURE_CUBE,
  TEXTURE_RECT,
  TEXTURE_SHADOW1D,
  TEXTURE_SHADOW2D,
  TEXTURE_SHADOWRECT,
  TEXTURE_1D_ARRAY,
  TEXTURE_2D_ARRAY,
  TEXTURE_SHADOW1D_ARRAY,
  TEXTURE_SHADOW2D_ARRAY
};

MCCodeEmitter *llvm::createR600MCCodeEmitter(const MCInstrInfo &MCII,
                                           const MCSubtargetInfo &STI,
                                           MCContext &Ctx) {
  return new R600MCCodeEmitter(MCII, STI, Ctx);
}

void R600MCCodeEmitter::EncodeInstruction(const MCInst &MI, raw_ostream &OS,
                                       SmallVectorImpl<MCFixup> &Fixups) const {
  if (isTexOp(MI.getOpcode())) {
    EmitTexInstr(MI, Fixups, OS);
  } else if (isFCOp(MI.getOpcode())){
    EmitFCInstr(MI, OS);
  } else if (MI.getOpcode() == AMDGPU::RETURN ||
    MI.getOpcode() == AMDGPU::BUNDLE ||
    MI.getOpcode() == AMDGPU::KILL) {
    return;
  } else {
    switch(MI.getOpcode()) {
    case AMDGPU::RAT_WRITE_CACHELESS_eg:
      {
        uint64_t inst = getBinaryCodeForInstr(MI, Fixups);
        EmitByte(INSTR_NATIVE, OS);
        Emit(inst, OS);
        break;
      }
    case AMDGPU::VTX_READ_PARAM_i32_eg:
    case AMDGPU::VTX_READ_PARAM_f32_eg:
    case AMDGPU::VTX_READ_GLOBAL_i32_eg:
    case AMDGPU::VTX_READ_GLOBAL_f32_eg:
    case AMDGPU::VTX_READ_GLOBAL_v4i32_eg:
    case AMDGPU::VTX_READ_GLOBAL_v4f32_eg:
      {
        uint64_t InstWord01 = getBinaryCodeForInstr(MI, Fixups);
        uint32_t InstWord2 = MI.getOperand(2).getImm(); // Offset

        EmitByte(INSTR_VTX, OS);
        Emit(InstWord01, OS);
        Emit(InstWord2, OS);
        break;
      }

    default:
      EmitALUInstr(MI, Fixups, OS);
      break;
    }
  }
}

void R600MCCodeEmitter::EmitALUInstr(const MCInst &MI,
                                     SmallVectorImpl<MCFixup> &Fixups,
                                     raw_ostream &OS) const {
  const MCInstrDesc &MCDesc = MCII.get(MI.getOpcode());
  unsigned NumOperands = MI.getNumOperands();

  if(MCDesc.findFirstPredOperandIdx() > -1)
    NumOperands--;

  if (GET_FLAG_OPERAND_IDX(MCDesc.TSFlags) != 0)
    NumOperands--;

  if(MI.getOpcode() == AMDGPU::PRED_X)
    NumOperands = 2;

  // XXX Check if instruction writes a result
  if (NumOperands < 1) {
    return;
  }

  // Emit instruction type
  EmitByte(0, OS);

  unsigned int OpIndex;
  for (OpIndex = 1; OpIndex < NumOperands; OpIndex++) {
    // Literal constants are always stored as the last operand.
    if (MI.getOperand(OpIndex).isImm() || MI.getOperand(OpIndex).isFPImm()) {
      break;
    }
    EmitSrc(MI, OpIndex, OS);
  }

  // Emit zeros for unused sources
  for ( ; OpIndex < 4; OpIndex++) {
    EmitNullBytes(SRC_BYTE_COUNT, OS);
  }

  EmitDst(MI, OS);

  EmitALU(MI, NumOperands - 1, Fixups, OS);
}

void R600MCCodeEmitter::EmitSrc(const MCInst &MI, unsigned OpIdx,
                                raw_ostream &OS) const {
  const MCOperand &MO = MI.getOperand(OpIdx);
  union {
    float f;
    uint32_t i;
  } Value;
  Value.i = 0;
  // Emit the source select (2 bytes).  For GPRs, this is the register index.
  // For other potential instruction operands, (e.g. constant registers) the
  // value of the source select is defined in the r600isa docs.
  if (MO.isReg()) {
    unsigned reg = MO.getReg();
    EmitTwoBytes(getHWReg(reg), OS);
    if (reg == AMDGPU::ALU_LITERAL_X) {
      unsigned ImmOpIndex = MI.getNumOperands() - 1;
      MCOperand ImmOp = MI.getOperand(ImmOpIndex);
      if (ImmOp.isFPImm()) {
        Value.f = ImmOp.getFPImm();
      } else {
        assert(ImmOp.isImm());
        Value.i = ImmOp.getImm();
      }
    }
  } else {
    // XXX: Handle other operand types.
    EmitTwoBytes(0, OS);
  }

  // Emit the source channel (1 byte)
  if (MO.isReg()) {
    EmitByte(getHWRegChan(MO.getReg()), OS);
  } else {
    EmitByte(0, OS);
  }

  // XXX: Emit isNegated (1 byte)
  if ((!(isFlagSet(MI, OpIdx, MO_FLAG_ABS)))
      && (isFlagSet(MI, OpIdx, MO_FLAG_NEG) ||
     (MO.isReg() &&
      (MO.getReg() == AMDGPU::NEG_ONE || MO.getReg() == AMDGPU::NEG_HALF)))){
    EmitByte(1, OS);
  } else {
    EmitByte(0, OS);
  }

  // Emit isAbsolute (1 byte)
  if (isFlagSet(MI, OpIdx, MO_FLAG_ABS)) {
    EmitByte(1, OS);
  } else {
    EmitByte(0, OS);
  }

  // XXX: Emit relative addressing mode (1 byte)
  EmitByte(0, OS);

  // Emit kc_bank, This will be adjusted later by r600_asm
  EmitByte(0, OS);

  // Emit the literal value, if applicable (4 bytes).
  Emit(Value.i, OS);

}

void R600MCCodeEmitter::EmitDst(const MCInst &MI, raw_ostream &OS) const {

  const MCOperand &MO = MI.getOperand(0);
  if (MO.isReg() && MO.getReg() != AMDGPU::PREDICATE_BIT) {
    // Emit the destination register index (1 byte)
    EmitByte(getHWReg(MO.getReg()), OS);

    // Emit the element of the destination register (1 byte)
    EmitByte(getHWRegChan(MO.getReg()), OS);

    // Emit isClamped (1 byte)
    if (isFlagSet(MI, 0, MO_FLAG_CLAMP)) {
      EmitByte(1, OS);
    } else {
      EmitByte(0, OS);
    }

    // Emit writemask (1 byte).
    if (isFlagSet(MI, 0, MO_FLAG_MASK)) {
      EmitByte(0, OS);
    } else {
      EmitByte(1, OS);
    }

    // XXX: Emit relative addressing mode
    EmitByte(0, OS);
  } else {
    // XXX: Handle other operand types.  Are there any for destination regs?
    EmitNullBytes(DST_BYTE_COUNT, OS);
  }
}

void R600MCCodeEmitter::EmitALU(const MCInst &MI, unsigned numSrc,
                                SmallVectorImpl<MCFixup> &Fixups,
                                raw_ostream &OS) const {
  const MCInstrDesc &MCDesc = MCII.get(MI.getOpcode());

  // Emit the instruction (2 bytes)
  EmitTwoBytes(getBinaryCodeForInstr(MI, Fixups), OS);

  // Emit IsLast (for this instruction group) (1 byte)
  if (isFlagSet(MI, 0, MO_FLAG_NOT_LAST)) {
    EmitByte(0, OS);
  } else {
    EmitByte(1, OS);
  }

  // Emit isOp3 (1 byte)
  if (numSrc == 3) {
    EmitByte(1, OS);
  } else {
    EmitByte(0, OS);
  }

  // XXX: Emit push modifier
    if(isFlagSet(MI, 1,  MO_FLAG_PUSH)) {
    EmitByte(1, OS);
  } else {
    EmitByte(0, OS);
  }

    // XXX: Emit predicate (1 byte)
  int PredIdx = MCDesc.findFirstPredOperandIdx();
  if (PredIdx > -1)
    switch(MI.getOperand(PredIdx).getReg()) {
    case AMDGPU::PRED_SEL_ZERO:
      EmitByte(2, OS);
      break;
    case AMDGPU::PRED_SEL_ONE:
      EmitByte(3, OS);
      break;
    default:
      EmitByte(0, OS);
      break;
    }
  else {
    EmitByte(0, OS);
  }


  // XXX: Emit bank swizzle. (1 byte)  Do we need this?  It looks like
  // r600_asm.c sets it.
  EmitByte(0, OS);

  // XXX: Emit bank_swizzle_force (1 byte) Not sure what this is for.
  EmitByte(0, OS);

  // XXX: Emit OMOD (1 byte) Not implemented.
  EmitByte(0, OS);

  // XXX: Emit index_mode.  I think this is for indirect addressing, so we
  // don't need to worry about it.
  EmitByte(0, OS);
}

void R600MCCodeEmitter::EmitTexInstr(const MCInst &MI,
                                     SmallVectorImpl<MCFixup> &Fixups,
                                     raw_ostream &OS) const {

  unsigned opcode = MI.getOpcode();
  bool hasOffsets = (opcode == AMDGPU::TEX_LD);
  unsigned op_offset = hasOffsets ? 3 : 0;
  int64_t sampler = MI.getOperand(op_offset+2).getImm();
  int64_t textureType = MI.getOperand(op_offset+3).getImm();
  unsigned srcSelect[4] = {0, 1, 2, 3};

  // Emit instruction type
  EmitByte(1, OS);

  // Emit instruction
  EmitByte(getBinaryCodeForInstr(MI, Fixups), OS);

  // XXX: Emit resource id r600_shader.c uses sampler + 1.  Why?
  EmitByte(sampler + 1 + 1, OS);

  // Emit source register
  EmitByte(getHWReg(MI.getOperand(1).getReg()), OS);

  // XXX: Emit src isRelativeAddress
  EmitByte(0, OS);

  // Emit destination register
  EmitByte(getHWReg(MI.getOperand(0).getReg()), OS);

  // XXX: Emit dst isRealtiveAddress
  EmitByte(0, OS);

  // XXX: Emit dst select
  EmitByte(0, OS); // X
  EmitByte(1, OS); // Y
  EmitByte(2, OS); // Z
  EmitByte(3, OS); // W

  // XXX: Emit lod bias
  EmitByte(0, OS);

  // XXX: Emit coord types
  unsigned coordType[4] = {1, 1, 1, 1};

  if (textureType == TEXTURE_RECT
      || textureType == TEXTURE_SHADOWRECT) {
    coordType[ELEMENT_X] = 0;
    coordType[ELEMENT_Y] = 0;
  }

  if (textureType == TEXTURE_1D_ARRAY
      || textureType == TEXTURE_SHADOW1D_ARRAY) {
    if (opcode == AMDGPU::TEX_SAMPLE_C_L || opcode == AMDGPU::TEX_SAMPLE_C_LB) {
      coordType[ELEMENT_Y] = 0;
    } else {
      coordType[ELEMENT_Z] = 0;
      srcSelect[ELEMENT_Z] = ELEMENT_Y;
    }
  } else if (textureType == TEXTURE_2D_ARRAY
             || textureType == TEXTURE_SHADOW2D_ARRAY) {
    coordType[ELEMENT_Z] = 0;
  }

  for (unsigned i = 0; i < 4; i++) {
    EmitByte(coordType[i], OS);
  }

  // XXX: Emit offsets
  if (hasOffsets)
	  for (unsigned i = 2; i < 5; i++)
		  EmitByte(MI.getOperand(i).getImm()<<1, OS);
  else
	  EmitNullBytes(3, OS);

  // Emit sampler id
  EmitByte(sampler, OS);

  // XXX:Emit source select
  if ((textureType == TEXTURE_SHADOW1D
      || textureType == TEXTURE_SHADOW2D
      || textureType == TEXTURE_SHADOWRECT
      || textureType == TEXTURE_SHADOW1D_ARRAY)
      && opcode != AMDGPU::TEX_SAMPLE_C_L
      && opcode != AMDGPU::TEX_SAMPLE_C_LB) {
    srcSelect[ELEMENT_W] = ELEMENT_Z;
  }

  for (unsigned i = 0; i < 4; i++) {
    EmitByte(srcSelect[i], OS);
  }
}

void R600MCCodeEmitter::EmitFCInstr(const MCInst &MI, raw_ostream &OS) const {

  // Emit instruction type
  EmitByte(INSTR_FC, OS);

  // Emit SRC
  unsigned NumOperands = MI.getNumOperands();
  if (NumOperands > 0) {
    assert(NumOperands == 1);
    EmitSrc(MI, 0, OS);
  } else {
    EmitNullBytes(SRC_BYTE_COUNT, OS);
  }

  // Emit FC Instruction
  enum FCInstr instr;
  switch (MI.getOpcode()) {
  case AMDGPU::BREAK_LOGICALZ_f32:
    instr = FC_BREAK;
    break;
  case AMDGPU::BREAK_LOGICALNZ_f32:
    instr = FC_BREAK_NZ;
    break;
  case AMDGPU::BREAK_LOGICALNZ_i32:
    instr = FC_BREAK_NZ_INT;
    break;
  case AMDGPU::BREAK_LOGICALZ_i32:
    instr = FC_BREAK_Z_INT;
    break;
  case AMDGPU::CONTINUE_LOGICALNZ_f32:
  case AMDGPU::CONTINUE_LOGICALNZ_i32:
    instr = FC_CONTINUE;
    break;
  case AMDGPU::IF_LOGICALNZ_f32:
    instr = FC_IF;
  case AMDGPU::IF_LOGICALNZ_i32:
    instr = FC_IF_INT;
    break;
  case AMDGPU::IF_LOGICALZ_f32:
    abort();
    break;
  case AMDGPU::ELSE:
    instr = FC_ELSE;
    break;
  case AMDGPU::ENDIF:
    instr = FC_ENDIF;
    break;
  case AMDGPU::ENDLOOP:
    instr = FC_ENDLOOP;
    break;
  case AMDGPU::WHILELOOP:
    instr = FC_BGNLOOP;
    break;
  default:
    abort();
    break;
  }
  EmitByte(instr, OS);
}

void R600MCCodeEmitter::EmitNullBytes(unsigned int ByteCount,
                                      raw_ostream &OS) const {

  for (unsigned int i = 0; i < ByteCount; i++) {
    EmitByte(0, OS);
  }
}

void R600MCCodeEmitter::EmitByte(unsigned int Byte, raw_ostream &OS) const {
  OS.write((uint8_t) Byte & 0xff);
}

void R600MCCodeEmitter::EmitTwoBytes(unsigned int Bytes,
                                     raw_ostream &OS) const {
  OS.write((uint8_t) (Bytes & 0xff));
  OS.write((uint8_t) ((Bytes >> 8) & 0xff));
}

void R600MCCodeEmitter::Emit(uint32_t Value, raw_ostream &OS) const {
  for (unsigned i = 0; i < 4; i++) {
    OS.write((uint8_t) ((Value >> (8 * i)) & 0xff));
  }
}

void R600MCCodeEmitter::Emit(uint64_t Value, raw_ostream &OS) const {
  for (unsigned i = 0; i < 8; i++) {
    EmitByte((Value >> (8 * i)) & 0xff, OS);
  }
}

unsigned R600MCCodeEmitter::getHWRegIndex(unsigned reg) const {
  switch(reg) {
  case AMDGPU::ZERO: return 248;
  case AMDGPU::ONE:
  case AMDGPU::NEG_ONE: return 249;
  case AMDGPU::ONE_INT: return 250;
  case AMDGPU::HALF:
  case AMDGPU::NEG_HALF: return 252;
  case AMDGPU::ALU_LITERAL_X: return 253;
  case AMDGPU::PREDICATE_BIT:
  case AMDGPU::PRED_SEL_OFF:
  case AMDGPU::PRED_SEL_ZERO:
  case AMDGPU::PRED_SEL_ONE:
    return 0;
  default: return getHWRegIndexGen(reg);
  }
}

unsigned R600MCCodeEmitter::getHWRegChan(unsigned reg) const {
  switch(reg) {
  case AMDGPU::ZERO:
  case AMDGPU::ONE:
  case AMDGPU::ONE_INT:
  case AMDGPU::NEG_ONE:
  case AMDGPU::HALF:
  case AMDGPU::NEG_HALF:
  case AMDGPU::ALU_LITERAL_X:
  case AMDGPU::PREDICATE_BIT:
  case AMDGPU::PRED_SEL_OFF:
  case AMDGPU::PRED_SEL_ZERO:
  case AMDGPU::PRED_SEL_ONE:
    return 0;
  default: return getHWRegChanGen(reg);
  }
}
unsigned R600MCCodeEmitter::getHWReg(unsigned RegNo) const {
  unsigned HWReg;

  HWReg = getHWRegIndex(RegNo);
  if (AMDGPUMCRegisterClasses[AMDGPU::R600_CReg32RegClassID].contains(RegNo)) {
    HWReg += 512;
  }
  return HWReg;
}

uint64_t R600MCCodeEmitter::getMachineOpValue(const MCInst &MI,
                                              const MCOperand &MO,
                                        SmallVectorImpl<MCFixup> &Fixup) const {
  if (MO.isReg()) {
    return getHWReg(MO.getReg());
  } else {
    return MO.getImm();
  }
}

//===----------------------------------------------------------------------===//
// Encoding helper functions
//===----------------------------------------------------------------------===//

bool R600MCCodeEmitter::isFCOp(unsigned opcode) const {
  switch(opcode) {
  default: return false;
  case AMDGPU::BREAK_LOGICALZ_f32:
  case AMDGPU::BREAK_LOGICALNZ_i32:
  case AMDGPU::BREAK_LOGICALZ_i32:
  case AMDGPU::BREAK_LOGICALNZ_f32:
  case AMDGPU::CONTINUE_LOGICALNZ_f32:
  case AMDGPU::IF_LOGICALNZ_i32:
  case AMDGPU::IF_LOGICALZ_f32:
  case AMDGPU::ELSE:
  case AMDGPU::ENDIF:
  case AMDGPU::ENDLOOP:
  case AMDGPU::IF_LOGICALNZ_f32:
  case AMDGPU::WHILELOOP:
    return true;
  }
}

bool R600MCCodeEmitter::isTexOp(unsigned opcode) const {
  switch(opcode) {
  default: return false;
  case AMDGPU::TEX_LD:
  case AMDGPU::TEX_GET_TEXTURE_RESINFO:
  case AMDGPU::TEX_SAMPLE:
  case AMDGPU::TEX_SAMPLE_C:
  case AMDGPU::TEX_SAMPLE_L:
  case AMDGPU::TEX_SAMPLE_C_L:
  case AMDGPU::TEX_SAMPLE_LB:
  case AMDGPU::TEX_SAMPLE_C_LB:
  case AMDGPU::TEX_SAMPLE_G:
  case AMDGPU::TEX_SAMPLE_C_G:
  case AMDGPU::TEX_GET_GRADIENTS_H:
  case AMDGPU::TEX_GET_GRADIENTS_V:
  case AMDGPU::TEX_SET_GRADIENTS_H:
  case AMDGPU::TEX_SET_GRADIENTS_V:
    return true;
  }
}

bool R600MCCodeEmitter::isFlagSet(const MCInst &MI, unsigned Operand,
                                  unsigned Flag) const {
  const MCInstrDesc &MCDesc = MCII.get(MI.getOpcode());
  unsigned FlagIndex = GET_FLAG_OPERAND_IDX(MCDesc.TSFlags);
  if (FlagIndex == 0) {
    return false;
  }
  assert(MI.getOperand(FlagIndex).isImm());
  return !!((MI.getOperand(FlagIndex).getImm() >>
            (NUM_MO_FLAGS * Operand)) & Flag);
}
#define R600RegisterInfo R600MCCodeEmitter
#include "R600HwRegInfo.include"
#undef R600RegisterInfo

#include "AMDGPUGenMCCodeEmitter.inc"
