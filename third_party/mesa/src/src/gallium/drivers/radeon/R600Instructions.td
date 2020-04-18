//===-- R600Instructions.td - R600 Instruction defs  -------*- tablegen -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// R600 Tablegen instruction definitions
//
//===----------------------------------------------------------------------===//

include "R600Intrinsics.td"

class InstR600 <bits<32> inst, dag outs, dag ins, string asm, list<dag> pattern,
                InstrItinClass itin>
    : AMDGPUInst <outs, ins, asm, pattern> {

  field bits<32> Inst;
  bit Trig = 0;
  bit Op3 = 0;
  bit isVector = 0;
  bits<2> FlagOperandIdx = 0;

  let Inst = inst;
  let Namespace = "AMDGPU";
  let OutOperandList = outs;
  let InOperandList = ins;
  let AsmString = asm;
  let Pattern = pattern;
  let Itinerary = itin;

  let TSFlags{4} = Trig;
  let TSFlags{5} = Op3;

  // Vector instructions are instructions that must fill all slots in an
  // instruction group
  let TSFlags{6} = isVector;
  let TSFlags{8-7} = FlagOperandIdx;
}

class InstR600ISA <dag outs, dag ins, string asm, list<dag> pattern> :
    AMDGPUInst <outs, ins, asm, pattern>
{
  field bits<64> Inst;

  let Namespace = "AMDGPU";
}

def MEMxi : Operand<iPTR> {
  let MIOperandInfo = (ops R600_TReg32_X:$ptr, i32imm:$index);
}

def MEMrr : Operand<iPTR> {
  let MIOperandInfo = (ops R600_Reg32:$ptr, R600_Reg32:$index);
}

def ADDRParam : ComplexPattern<i32, 2, "SelectADDRParam", [], []>;
def ADDRDWord : ComplexPattern<i32, 1, "SelectADDRDWord", [], []>;
def ADDRVTX_READ : ComplexPattern<i32, 2, "SelectADDRVTX_READ", [], []>;

class R600_ALU {

  bits<7> DST_GPR = 0;
  bits<9> SRC0_SEL = 0;
  bits<1> SRC0_NEG = 0;
  bits<9> SRC1_SEL = 0;
  bits<1> SRC1_NEG = 0;
  bits<1> CLAMP = 0;
  
}

def R600_Pred : PredicateOperand<i32, (ops R600_Predicate),
                                     (ops PRED_SEL_OFF)>;


class R600_1OP <bits<32> inst, string opName, list<dag> pattern,
                InstrItinClass itin = AnyALU> :
  InstR600 <inst,
          (outs R600_Reg32:$dst),
          (ins R600_Reg32:$src, R600_Pred:$p, variable_ops),
          !strconcat(opName, " $dst, $src ($p)"),
          pattern,
          itin
  >;

class R600_2OP <bits<32> inst, string opName, list<dag> pattern,
                InstrItinClass itin = AnyALU> :
  InstR600 <inst,
          (outs R600_Reg32:$dst),
          (ins R600_Reg32:$src0, R600_Reg32:$src1,R600_Pred:$p, variable_ops),
          !strconcat(opName, " $dst, $src0, $src1"),
          pattern,
          itin
  >;

class R600_3OP <bits<32> inst, string opName, list<dag> pattern,
                InstrItinClass itin = AnyALU> :
  InstR600 <inst,
          (outs R600_Reg32:$dst),
          (ins R600_Reg32:$src0, R600_Reg32:$src1, R600_Reg32:$src2,R600_Pred:$p, variable_ops),
          !strconcat(opName, " $dst, $src0, $src1, $src2"),
          pattern,
          itin>{

    let Op3 = 1;
  }



def PRED_X : InstR600 <0, (outs R600_Predicate_Bit:$dst),
           (ins R600_Reg32:$src0, i32imm:$src1, i32imm:$flags),
           "PRED $dst, $src0, $src1",
           [], NullALU>
{
  let DisableEncoding = "$src0";
  field bits<32> Inst;
  bits<32> src1;

  let Inst = src1;
  let FlagOperandIdx = 3;
}

let isTerminator = 1, isBranch = 1, isPseudo = 1 in {
def JUMP : InstR600 <0x10,
          (outs),
          (ins brtarget:$target, R600_Pred:$p),
          "JUMP $target ($p)",
          [], AnyALU
  >;
}

class R600_REDUCTION <bits<32> inst, dag ins, string asm, list<dag> pattern,
                      InstrItinClass itin = VecALU> :
  InstR600 <inst,
          (outs R600_Reg32:$dst),
          ins,
          asm,
          pattern,
          itin

  >;

class R600_TEX <bits<32> inst, string opName, list<dag> pattern,
                InstrItinClass itin = AnyALU> :
  InstR600 <inst,
          (outs R600_Reg128:$dst),
          (ins R600_Reg128:$src0, i32imm:$src1, i32imm:$src2),
          !strconcat(opName, "$dst, $src0, $src1, $src2"),
          pattern,
          itin
  >;

def TEX_SHADOW : PatLeaf<
  (imm),
  [{uint32_t TType = (uint32_t)N->getZExtValue();
    return (TType >= 6 && TType <= 8) || TType == 11 || TType == 12;
  }]
>;

class EG_CF_RAT <bits <8> cf_inst, bits <6> rat_inst, bits<4> rat_id, dag outs,
                 dag ins, string asm, list<dag> pattern> :
    InstR600ISA <outs, ins, asm, pattern>
{
  bits<7>  RW_GPR;
  bits<7>  INDEX_GPR;

  bits<2>  RIM;
  bits<2>  TYPE;
  bits<1>  RW_REL;
  bits<2>  ELEM_SIZE;

  bits<12> ARRAY_SIZE;
  bits<4>  COMP_MASK;
  bits<4>  BURST_COUNT;
  bits<1>  VPM;
  bits<1>  eop;
  bits<1>  MARK;
  bits<1>  BARRIER;

  // CF_ALLOC_EXPORT_WORD0_RAT
  let Inst{3-0}   = rat_id;
  let Inst{9-4}   = rat_inst;
  let Inst{10}    = 0; // Reserved
  let Inst{12-11} = RIM;
  let Inst{14-13} = TYPE;
  let Inst{21-15} = RW_GPR;
  let Inst{22}    = RW_REL;
  let Inst{29-23} = INDEX_GPR;
  let Inst{31-30} = ELEM_SIZE;

  // CF_ALLOC_EXPORT_WORD1_BUF
  let Inst{43-32} = ARRAY_SIZE;
  let Inst{47-44} = COMP_MASK;
  let Inst{51-48} = BURST_COUNT;
  let Inst{52}    = VPM;
  let Inst{53}    = eop;
  let Inst{61-54} = cf_inst;
  let Inst{62}    = MARK;
  let Inst{63}    = BARRIER;
}

def load_param : PatFrag<(ops node:$ptr),
                         (load node:$ptr),
                          [{
                           const Value *Src = cast<LoadSDNode>(N)->getSrcValue();
                           if (Src) {
                                PointerType * PT = dyn_cast<PointerType>(Src->getType());
                                return PT && PT->getAddressSpace() == AMDGPUAS::PARAM_I_ADDRESS;
                           }
                           return false;
                          }]>;

def isR600 : Predicate<"Subtarget.device()"
                            "->getGeneration() == AMDGPUDeviceInfo::HD4XXX">;
def isR700 : Predicate<"Subtarget.device()"
                            "->getGeneration() == AMDGPUDeviceInfo::HD4XXX &&"
                            "Subtarget.device()->getDeviceFlag()"
                            ">= OCL_DEVICE_RV710">;
def isEG : Predicate<"Subtarget.device()"
                            "->getGeneration() >= AMDGPUDeviceInfo::HD5XXX && "
                            "Subtarget.device()->getDeviceFlag() != OCL_DEVICE_CAYMAN">;
def isCayman : Predicate<"Subtarget.device()"
                            "->getDeviceFlag() == OCL_DEVICE_CAYMAN">;
def isEGorCayman : Predicate<"Subtarget.device()"
                            "->getGeneration() == AMDGPUDeviceInfo::HD5XXX"
			    "|| Subtarget.device()->getGeneration() =="
			    "AMDGPUDeviceInfo::HD6XXX">;

def isR600toCayman : Predicate<
                     "Subtarget.device()->getGeneration() <= AMDGPUDeviceInfo::HD6XXX">;


let Predicates = [isR600toCayman] in { 

//===----------------------------------------------------------------------===//
// Common Instructions R600, R700, Evergreen, Cayman
//===----------------------------------------------------------------------===//

def ADD : R600_2OP <
  0x0, "ADD",
  [(set R600_Reg32:$dst, (fadd R600_Reg32:$src0, R600_Reg32:$src1))]
>;

// Non-IEEE MUL: 0 * anything = 0
def MUL : R600_2OP <
  0x1, "MUL NON-IEEE",
  [(set R600_Reg32:$dst, (int_AMDGPU_mul R600_Reg32:$src0, R600_Reg32:$src1))]
>;

def MUL_IEEE : R600_2OP <
  0x2, "MUL_IEEE",
  [(set R600_Reg32:$dst, (fmul R600_Reg32:$src0, R600_Reg32:$src1))]
>;

def MAX : R600_2OP <
  0x3, "MAX",
  [(set R600_Reg32:$dst, (AMDGPUfmax R600_Reg32:$src0, R600_Reg32:$src1))]
>;

def MIN : R600_2OP <
  0x4, "MIN",
  [(set R600_Reg32:$dst, (AMDGPUfmin R600_Reg32:$src0, R600_Reg32:$src1))]
>;

// For the SET* instructions there is a naming conflict in TargetSelectionDAG.td,
// so some of the instruction names don't match the asm string.
// XXX: Use the defs in TargetSelectionDAG.td instead of intrinsics.

def SETE : R600_2OP <
  0x08, "SETE",
  [(set R600_Reg32:$dst,
   (selectcc (f32 R600_Reg32:$src0), R600_Reg32:$src1, FP_ONE, FP_ZERO,
             COND_EQ))]
>;

def SGT : R600_2OP <
  0x09, "SETGT",
  [(set R600_Reg32:$dst,
   (selectcc (f32 R600_Reg32:$src0), R600_Reg32:$src1, FP_ONE, FP_ZERO,
              COND_GT))]
>;

def SGE : R600_2OP <
  0xA, "SETGE",
  [(set R600_Reg32:$dst,
   (selectcc (f32 R600_Reg32:$src0), R600_Reg32:$src1, FP_ONE, FP_ZERO,
              COND_GE))]
>;

def SNE : R600_2OP <
  0xB, "SETNE",
  [(set R600_Reg32:$dst,
   (selectcc (f32 R600_Reg32:$src0), R600_Reg32:$src1, FP_ONE, FP_ZERO,
    COND_NE))]
>;

def FRACT : R600_1OP <
  0x10, "FRACT",
  [(set R600_Reg32:$dst, (AMDGPUfract R600_Reg32:$src))]
>;

def TRUNC : R600_1OP <
  0x11, "TRUNC",
  [(set R600_Reg32:$dst, (int_AMDGPU_trunc R600_Reg32:$src))]
>;

def CEIL : R600_1OP <
  0x12, "CEIL",
  [(set R600_Reg32:$dst, (fceil R600_Reg32:$src))]
>;

def RNDNE : R600_1OP <
  0x13, "RNDNE",
  [(set R600_Reg32:$dst, (frint R600_Reg32:$src))]
>;

def FLOOR : R600_1OP <
  0x14, "FLOOR",
  [(set R600_Reg32:$dst, (int_AMDGPU_floor R600_Reg32:$src))]
>;

def MOV : InstR600 <0x19, (outs R600_Reg32:$dst),
                          (ins R600_Reg32:$src0, i32imm:$flags,
                               R600_Pred:$p),
                          "MOV $dst, $src0", [], AnyALU> {
  let FlagOperandIdx = 2;
}

class MOV_IMM <ValueType vt, Operand immType> : InstR600 <0x19,
  (outs R600_Reg32:$dst),
  (ins R600_Reg32:$alu_literal, R600_Pred:$p, immType:$imm),
  "MOV_IMM $dst, $imm",
  [], AnyALU
>;

def MOV_IMM_I32 : MOV_IMM<i32, i32imm>;
def : Pat <
  (imm:$val),
  (MOV_IMM_I32 (i32 ALU_LITERAL_X), imm:$val)
>;

def MOV_IMM_F32 : MOV_IMM<f32, f32imm>;
def : Pat <
  (fpimm:$val),
  (MOV_IMM_F32 (i32 ALU_LITERAL_X), fpimm:$val)
>;

def KILLGT : InstR600 <0x2D,
          (outs R600_Reg32:$dst),
          (ins R600_Reg32:$src0, R600_Reg32:$src1, i32imm:$flags, R600_Pred:$p,
               variable_ops),
          "KILLGT $dst, $src0, $src1, $flags ($p)",
          [],
          NullALU>{
  let FlagOperandIdx = 3;
}

def AND_INT : R600_2OP <
  0x30, "AND_INT",
  [(set R600_Reg32:$dst, (and R600_Reg32:$src0, R600_Reg32:$src1))]
>;

def OR_INT : R600_2OP <
  0x31, "OR_INT",
  [(set R600_Reg32:$dst, (or R600_Reg32:$src0, R600_Reg32:$src1))]
>;

def XOR_INT : R600_2OP <
  0x32, "XOR_INT",
  [(set R600_Reg32:$dst, (xor R600_Reg32:$src0, R600_Reg32:$src1))]
>;

def NOT_INT : R600_1OP <
  0x33, "NOT_INT",
  [(set R600_Reg32:$dst, (not R600_Reg32:$src))]
>;

def ADD_INT : R600_2OP <
  0x34, "ADD_INT",
  [(set R600_Reg32:$dst, (add R600_Reg32:$src0, R600_Reg32:$src1))]
>;

def SUB_INT : R600_2OP <
	0x35, "SUB_INT",
	[(set R600_Reg32:$dst, (sub R600_Reg32:$src0, R600_Reg32:$src1))]
>;

def MAX_INT : R600_2OP <
  0x36, "MAX_INT",
  [(set R600_Reg32:$dst, (AMDGPUsmax R600_Reg32:$src0, R600_Reg32:$src1))]>;

def MIN_INT : R600_2OP <
  0x37, "MIN_INT",
  [(set R600_Reg32:$dst, (AMDGPUsmin R600_Reg32:$src0, R600_Reg32:$src1))]>;

def MAX_UINT : R600_2OP <
  0x38, "MAX_UINT",
  [(set R600_Reg32:$dst, (AMDGPUsmax R600_Reg32:$src0, R600_Reg32:$src1))]
>;

def MIN_UINT : R600_2OP <
  0x39, "MIN_UINT",
  [(set R600_Reg32:$dst, (AMDGPUumin R600_Reg32:$src0, R600_Reg32:$src1))]
>;

def SETE_INT : R600_2OP <
  0x3A, "SETE_INT",
  [(set (i32 R600_Reg32:$dst),
   (selectcc (i32 R600_Reg32:$src0), R600_Reg32:$src1, -1, 0, SETEQ))]
>;

def SETGT_INT : R600_2OP <
  0x3B, "SGT_INT",
  [(set (i32 R600_Reg32:$dst),
   (selectcc (i32 R600_Reg32:$src0), R600_Reg32:$src1, -1, 0, SETGT))]
>;

def SETGE_INT : R600_2OP <
	0x3C, "SETGE_INT",
	[(set (i32 R600_Reg32:$dst),
   (selectcc (i32 R600_Reg32:$src0), R600_Reg32:$src1, -1, 0, SETGE))]
>;

def SETNE_INT : R600_2OP <
  0x3D, "SETNE_INT",
  [(set (i32 R600_Reg32:$dst),
   (selectcc (i32 R600_Reg32:$src0), R600_Reg32:$src1, -1, 0, SETNE))]
>;

def SETGT_UINT : R600_2OP <
  0x3E, "SETGT_UINT",
  [(set (i32 R600_Reg32:$dst),
   (selectcc (i32 R600_Reg32:$src0), R600_Reg32:$src1, -1, 0, SETUGT))]
>;

def SETGE_UINT : R600_2OP <
  0x3F, "SETGE_UINT",
  [(set (i32 R600_Reg32:$dst),
    (selectcc (i32 R600_Reg32:$src0), R600_Reg32:$src1, -1, 0, SETUGE))]
>;

def CNDE_INT : R600_3OP <
	0x1C, "CNDE_INT",
  [(set (i32 R600_Reg32:$dst),
   (select R600_Reg32:$src0, R600_Reg32:$src2, R600_Reg32:$src1))]
>;

//===----------------------------------------------------------------------===//
// Texture instructions
//===----------------------------------------------------------------------===//

def TEX_LD : R600_TEX <
  0x03, "TEX_LD",
  [(set R600_Reg128:$dst, (int_AMDGPU_txf R600_Reg128:$src0, imm:$src1, imm:$src2, imm:$src3, imm:$src4, imm:$src5))]
> {
let AsmString = "TEX_LD $dst, $src0, $src1, $src2, $src3, $src4, $src5";
let InOperandList = (ins R600_Reg128:$src0, i32imm:$src1, i32imm:$src2, i32imm:$src3, i32imm:$src4, i32imm:$src5);
}

def TEX_GET_TEXTURE_RESINFO : R600_TEX <
  0x04, "TEX_GET_TEXTURE_RESINFO",
  [(set R600_Reg128:$dst, (int_AMDGPU_txq R600_Reg128:$src0, imm:$src1, imm:$src2))]
>;

def TEX_GET_GRADIENTS_H : R600_TEX <
  0x07, "TEX_GET_GRADIENTS_H",
  [(set R600_Reg128:$dst, (int_AMDGPU_ddx R600_Reg128:$src0, imm:$src1, imm:$src2))]
>;

def TEX_GET_GRADIENTS_V : R600_TEX <
  0x08, "TEX_GET_GRADIENTS_V",
  [(set R600_Reg128:$dst, (int_AMDGPU_ddy R600_Reg128:$src0, imm:$src1, imm:$src2))]
>;

def TEX_SET_GRADIENTS_H : R600_TEX <
  0x0B, "TEX_SET_GRADIENTS_H",
  []
>;

def TEX_SET_GRADIENTS_V : R600_TEX <
  0x0C, "TEX_SET_GRADIENTS_V",
  []
>;

def TEX_SAMPLE : R600_TEX <
  0x10, "TEX_SAMPLE",
  [(set R600_Reg128:$dst, (int_AMDGPU_tex R600_Reg128:$src0, imm:$src1, imm:$src2))]
>;

def TEX_SAMPLE_C : R600_TEX <
  0x18, "TEX_SAMPLE_C",
  [(set R600_Reg128:$dst, (int_AMDGPU_tex R600_Reg128:$src0, imm:$src1, TEX_SHADOW:$src2))]
>;

def TEX_SAMPLE_L : R600_TEX <
  0x11, "TEX_SAMPLE_L",
  [(set R600_Reg128:$dst, (int_AMDGPU_txl R600_Reg128:$src0, imm:$src1, imm:$src2))]
>;

def TEX_SAMPLE_C_L : R600_TEX <
  0x19, "TEX_SAMPLE_C_L",
  [(set R600_Reg128:$dst, (int_AMDGPU_txl R600_Reg128:$src0, imm:$src1, TEX_SHADOW:$src2))]
>;

def TEX_SAMPLE_LB : R600_TEX <
  0x12, "TEX_SAMPLE_LB",
  [(set R600_Reg128:$dst, (int_AMDGPU_txb R600_Reg128:$src0, imm:$src1, imm:$src2))]
>;

def TEX_SAMPLE_C_LB : R600_TEX <
  0x1A, "TEX_SAMPLE_C_LB",
  [(set R600_Reg128:$dst, (int_AMDGPU_txb R600_Reg128:$src0, imm:$src1, TEX_SHADOW:$src2))]
>;

def TEX_SAMPLE_G : R600_TEX <
  0x14, "TEX_SAMPLE_G",
  []
>;

def TEX_SAMPLE_C_G : R600_TEX <
  0x1C, "TEX_SAMPLE_C_G",
  []
>;

//===----------------------------------------------------------------------===//
// Helper classes for common instructions
//===----------------------------------------------------------------------===//

class MUL_LIT_Common <bits<32> inst> : R600_3OP <
  inst, "MUL_LIT",
  []
>;

class MULADD_Common <bits<32> inst> : R600_3OP <
  inst, "MULADD",
  [(set (f32 R600_Reg32:$dst),
   (IL_mad R600_Reg32:$src0, R600_Reg32:$src1, R600_Reg32:$src2))]
>;

class CNDE_Common <bits<32> inst> : R600_3OP <
  inst, "CNDE",
  [(set (f32 R600_Reg32:$dst),
   (select (i32 (fp_to_sint (fneg R600_Reg32:$src0))), (f32 R600_Reg32:$src2), (f32 R600_Reg32:$src1)))]
>;

class CNDGT_Common <bits<32> inst> : R600_3OP <
  inst, "CNDGT",
  []
>;
  
class CNDGE_Common <bits<32> inst> : R600_3OP <
  inst, "CNDGE",
  [(set R600_Reg32:$dst, (int_AMDGPU_cndlt R600_Reg32:$src0, R600_Reg32:$src2, R600_Reg32:$src1))]
>;

class DOT4_Common <bits<32> inst> : R600_REDUCTION <
  inst,
  (ins R600_Reg128:$src0, R600_Reg128:$src1, i32imm:$flags),
  "DOT4 $dst $src0, $src1",
  []
  > {
  let FlagOperandIdx = 3;
}

class DOT4_Pat <Instruction dot4> : Pat <
  (int_AMDGPU_dp4 R600_Reg128:$src0, R600_Reg128:$src1),
  (dot4 R600_Reg128:$src0, R600_Reg128:$src1, 0)
>;

multiclass CUBE_Common <bits<32> inst> {

  def _pseudo : InstR600 <
    inst,
    (outs R600_Reg128:$dst),
    (ins R600_Reg128:$src),
    "CUBE $dst $src",
    [(set R600_Reg128:$dst, (int_AMDGPU_cube R600_Reg128:$src))],
    VecALU
  >;

  def _real : InstR600 <
    inst,
    (outs R600_Reg32:$dst),
    (ins R600_Reg32:$src0, R600_Reg32:$src1, i32imm:$flags),
    "CUBE $dst, $src0, $src1",
    [], VecALU
  >{
    let FlagOperandIdx = 3;
  }
}

class EXP_IEEE_Common <bits<32> inst> : R600_1OP <
  inst, "EXP_IEEE",
  [(set R600_Reg32:$dst, (fexp2 R600_Reg32:$src))]
>;

class FLT_TO_INT_Common <bits<32> inst> : R600_1OP <
  inst, "FLT_TO_INT",
  [(set R600_Reg32:$dst, (fp_to_sint R600_Reg32:$src))]
>;

class INT_TO_FLT_Common <bits<32> inst> : R600_1OP <
  inst, "INT_TO_FLT",
  [(set R600_Reg32:$dst, (sint_to_fp R600_Reg32:$src))]
>;

class FLT_TO_UINT_Common <bits<32> inst> : R600_1OP <
  inst, "FLT_TO_UINT",
  [(set R600_Reg32:$dst, (fp_to_uint R600_Reg32:$src))]
>;

class UINT_TO_FLT_Common <bits<32> inst> : R600_1OP <
  inst, "UINT_TO_FLT",
  [(set R600_Reg32:$dst, (uint_to_fp R600_Reg32:$src))]
>;

class LOG_CLAMPED_Common <bits<32> inst> : R600_1OP <
  inst, "LOG_CLAMPED",
  []
>;

class LOG_IEEE_Common <bits<32> inst> : R600_1OP <
  inst, "LOG_IEEE",
  [(set R600_Reg32:$dst, (int_AMDIL_log R600_Reg32:$src))]
>;

class LSHL_Common <bits<32> inst> : R600_2OP <
  inst, "LSHL $dst, $src0, $src1",
  [(set R600_Reg32:$dst, (shl R600_Reg32:$src0, R600_Reg32:$src1))]
>;

class LSHR_Common <bits<32> inst> : R600_2OP <
  inst, "LSHR $dst, $src0, $src1",
  [(set R600_Reg32:$dst, (srl R600_Reg32:$src0, R600_Reg32:$src1))]
>;

class ASHR_Common <bits<32> inst> : R600_2OP <
  inst, "ASHR $dst, $src0, $src1",
  [(set R600_Reg32:$dst, (sra R600_Reg32:$src0, R600_Reg32:$src1))]
>;

class MULHI_INT_Common <bits<32> inst> : R600_2OP <
  inst, "MULHI_INT $dst, $src0, $src1",
  [(set R600_Reg32:$dst, (mulhs R600_Reg32:$src0, R600_Reg32:$src1))]
>;

class MULHI_UINT_Common <bits<32> inst> : R600_2OP <
  inst, "MULHI $dst, $src0, $src1",
  [(set R600_Reg32:$dst, (mulhu R600_Reg32:$src0, R600_Reg32:$src1))]
>;

class MULLO_INT_Common <bits<32> inst> : R600_2OP <
  inst, "MULLO_INT $dst, $src0, $src1",
  [(set R600_Reg32:$dst, (mul R600_Reg32:$src0, R600_Reg32:$src1))]
>;

class MULLO_UINT_Common <bits<32> inst> : R600_2OP <
  inst, "MULLO_UINT $dst, $src0, $src1",
  []
>;

class RECIP_CLAMPED_Common <bits<32> inst> : R600_1OP <
  inst, "RECIP_CLAMPED",
  []
>;

class RECIP_IEEE_Common <bits<32> inst> : R600_1OP <
  inst, "RECIP_IEEE",
  [(set R600_Reg32:$dst, (int_AMDGPU_rcp R600_Reg32:$src))]
>;

class RECIP_UINT_Common <bits<32> inst> : R600_1OP <
  inst, "RECIP_INT $dst, $src",
  [(set R600_Reg32:$dst, (AMDGPUurecip R600_Reg32:$src))]
>;

class RECIPSQRT_CLAMPED_Common <bits<32> inst> : R600_1OP <
  inst, "RECIPSQRT_CLAMPED",
  [(set R600_Reg32:$dst, (int_AMDGPU_rsq R600_Reg32:$src))]
>;

class RECIPSQRT_IEEE_Common <bits<32> inst> : R600_1OP <
  inst, "RECIPSQRT_IEEE",
  []
>;

class SIN_Common <bits<32> inst> : R600_1OP <
  inst, "SIN", []>{
  let Trig = 1;
}

class COS_Common <bits<32> inst> : R600_1OP <
  inst, "COS", []> {
  let Trig = 1;
}

//===----------------------------------------------------------------------===//
// Helper patterns for complex intrinsics
//===----------------------------------------------------------------------===//

class DIV_Common <InstR600 recip_ieee> : Pat<
  (int_AMDGPU_div R600_Reg32:$src0, R600_Reg32:$src1),
  (MUL R600_Reg32:$src0, (recip_ieee R600_Reg32:$src1))
>;

class SSG_Common <InstR600 cndgt, InstR600 cndge> : Pat <
  (int_AMDGPU_ssg R600_Reg32:$src),
  (cndgt R600_Reg32:$src, (f32 ONE), (cndge R600_Reg32:$src, (f32 ZERO), (f32 NEG_ONE)))
>;

class TGSI_LIT_Z_Common <InstR600 mul_lit, InstR600 log_clamped, InstR600 exp_ieee> : Pat <
  (int_TGSI_lit_z R600_Reg32:$src_x, R600_Reg32:$src_y, R600_Reg32:$src_w),
  (exp_ieee (mul_lit (log_clamped (MAX R600_Reg32:$src_y, (f32 ZERO))), R600_Reg32:$src_w, R600_Reg32:$src_x))
>;

//===----------------------------------------------------------------------===//
// R600 / R700 Instructions
//===----------------------------------------------------------------------===//

let Predicates = [isR600] in {

  def MUL_LIT_r600 : MUL_LIT_Common<0x0C>;
  def MULADD_r600 : MULADD_Common<0x10>;
  def CNDE_r600 : CNDE_Common<0x18>;
  def CNDGT_r600 : CNDGT_Common<0x19>;
  def CNDGE_r600 : CNDGE_Common<0x1A>;
  def DOT4_r600 : DOT4_Common<0x50>;
  def : DOT4_Pat <DOT4_r600>;
  defm CUBE_r600 : CUBE_Common<0x52>;
  def EXP_IEEE_r600 : EXP_IEEE_Common<0x61>;
  def LOG_CLAMPED_r600 : LOG_CLAMPED_Common<0x62>;
  def LOG_IEEE_r600 : LOG_IEEE_Common<0x63>;
  def RECIP_CLAMPED_r600 : RECIP_CLAMPED_Common<0x64>;
  def RECIP_IEEE_r600 : RECIP_IEEE_Common<0x66>;
  def RECIPSQRT_CLAMPED_r600 : RECIPSQRT_CLAMPED_Common<0x67>;
  def RECIPSQRT_IEEE_r600 : RECIPSQRT_IEEE_Common<0x69>;
  def FLT_TO_INT_r600 : FLT_TO_INT_Common<0x6b>;
  def INT_TO_FLT_r600 : INT_TO_FLT_Common<0x6c>;
  def FLT_TO_UINT_r600 : FLT_TO_UINT_Common<0x79>;
  def UINT_TO_FLT_r600 : UINT_TO_FLT_Common<0x6d>;
  def SIN_r600 : SIN_Common<0x6E>;
  def COS_r600 : COS_Common<0x6F>;
  def ASHR_r600 : ASHR_Common<0x70>;
  def LSHR_r600 : LSHR_Common<0x71>;
  def LSHL_r600 : LSHL_Common<0x72>;
  def MULLO_INT_r600 : MULLO_INT_Common<0x73>;
  def MULHI_INT_r600 : MULHI_INT_Common<0x74>;
  def MULLO_UINT_r600 : MULLO_UINT_Common<0x75>;
  def MULHI_UINT_r600 : MULHI_UINT_Common<0x76>;
  def RECIP_UINT_r600 : RECIP_UINT_Common <0x78>;

  def DIV_r600 : DIV_Common<RECIP_IEEE_r600>;
  def POW_r600 : POW_Common<LOG_IEEE_r600, EXP_IEEE_r600, MUL, GPRF32>;
  def SSG_r600 : SSG_Common<CNDGT_r600, CNDGE_r600>;
  def TGSI_LIT_Z_r600 : TGSI_LIT_Z_Common<MUL_LIT_r600, LOG_CLAMPED_r600, EXP_IEEE_r600>;

}

// Helper pattern for normalizing inputs to triginomic instructions for R700+
// cards.
class TRIG_eg <InstR600 trig, Intrinsic intr> : Pat<
  (intr R600_Reg32:$src),
  (trig (MUL (MOV_IMM_I32 (i32 ALU_LITERAL_X), CONST.TWO_PI_INV), R600_Reg32:$src))
>;

//===----------------------------------------------------------------------===//
// R700 Only instructions
//===----------------------------------------------------------------------===//

let Predicates = [isR700] in {
  def SIN_r700 : SIN_Common<0x6E>;
  def COS_r700 : COS_Common<0x6F>;

  // R700 normalizes inputs to SIN/COS the same as EG
  def : TRIG_eg <SIN_r700, int_AMDGPU_sin>;
  def : TRIG_eg <COS_r700, int_AMDGPU_cos>;
}

//===----------------------------------------------------------------------===//
// Evergreen Only instructions
//===----------------------------------------------------------------------===//

let Predicates = [isEG] in {
  
def RECIP_IEEE_eg : RECIP_IEEE_Common<0x86>;

def MULLO_INT_eg : MULLO_INT_Common<0x8F>;
def MULHI_INT_eg : MULHI_INT_Common<0x90>;
def MULLO_UINT_eg : MULLO_UINT_Common<0x91>;
def MULHI_UINT_eg : MULHI_UINT_Common<0x92>;
def RECIP_UINT_eg : RECIP_UINT_Common<0x94>;

} // End Predicates = [isEG]

//===----------------------------------------------------------------------===//
// Evergreen / Cayman Instructions
//===----------------------------------------------------------------------===//

let Predicates = [isEGorCayman] in {

  // BFE_UINT - bit_extract, an optimization for mask and shift
  // Src0 = Input
  // Src1 = Offset
  // Src2 = Width
  //
  // bit_extract = (Input << (32 - Offset - Width)) >> (32 - Width)
  //
  // Example Usage:
  // (Offset, Width)
  //
  // (0, 8)           = (Input << 24) >> 24  = (Input &  0xff)       >> 0
  // (8, 8)           = (Input << 16) >> 24  = (Input &  0xffff)     >> 8
  // (16,8)           = (Input <<  8) >> 24  = (Input &  0xffffff)   >> 16
  // (24,8)           = (Input <<  0) >> 24  = (Input &  0xffffffff) >> 24
  def BFE_UINT_eg : R600_3OP <0x4, "BFE_UINT",
    [(set R600_Reg32:$dst, (int_AMDIL_bit_extract_u32 R600_Reg32:$src0,
                                                      R600_Reg32:$src1,
                                                      R600_Reg32:$src2))],
    VecALU
  >;

  def BIT_ALIGN_INT_eg : R600_3OP <0xC, "BIT_ALIGN_INT",
    [(set R600_Reg32:$dst, (AMDGPUbitalign R600_Reg32:$src0, R600_Reg32:$src1,
                                          R600_Reg32:$src2))],
    VecALU
  >;

  def MULADD_eg : MULADD_Common<0x14>;
  def ASHR_eg : ASHR_Common<0x15>;
  def LSHR_eg : LSHR_Common<0x16>;
  def LSHL_eg : LSHL_Common<0x17>;
  def CNDE_eg : CNDE_Common<0x19>;
  def CNDGT_eg : CNDGT_Common<0x1A>;
  def CNDGE_eg : CNDGE_Common<0x1B>;
  def MUL_LIT_eg : MUL_LIT_Common<0x1F>;
  def EXP_IEEE_eg : EXP_IEEE_Common<0x81>;
  def LOG_CLAMPED_eg : LOG_CLAMPED_Common<0x82>;
  def LOG_IEEE_eg : LOG_IEEE_Common<0x83>;
  def RECIP_CLAMPED_eg : RECIP_CLAMPED_Common<0x84>;
  def RECIPSQRT_CLAMPED_eg : RECIPSQRT_CLAMPED_Common<0x87>;
  def RECIPSQRT_IEEE_eg : RECIPSQRT_IEEE_Common<0x89>;
  def SIN_eg : SIN_Common<0x8D>;
  def COS_eg : COS_Common<0x8E>;
  def DOT4_eg : DOT4_Common<0xBE>;
  def : DOT4_Pat <DOT4_eg>;
  defm CUBE_eg : CUBE_Common<0xC0>;

  def DIV_eg : DIV_Common<RECIP_IEEE_eg>;
  def POW_eg : POW_Common<LOG_IEEE_eg, EXP_IEEE_eg, MUL, GPRF32>;
  def SSG_eg : SSG_Common<CNDGT_eg, CNDGE_eg>;
  def TGSI_LIT_Z_eg : TGSI_LIT_Z_Common<MUL_LIT_eg, LOG_CLAMPED_eg, EXP_IEEE_eg>;

  def : TRIG_eg <SIN_eg, int_AMDGPU_sin>;
  def : TRIG_eg <COS_eg, int_AMDGPU_cos>;

  def FLT_TO_INT_eg : FLT_TO_INT_Common<0x50> {
    let Pattern = [];
  }

  def INT_TO_FLT_eg : INT_TO_FLT_Common<0x9B>;

  def FLT_TO_UINT_eg : FLT_TO_UINT_Common<0x9A> {
    let Pattern = [];
  }

  def UINT_TO_FLT_eg : UINT_TO_FLT_Common<0x9C>;

  def : Pat<(fp_to_sint R600_Reg32:$src),
    (FLT_TO_INT_eg (TRUNC R600_Reg32:$src))>;

  def : Pat<(fp_to_uint R600_Reg32:$src),
    (FLT_TO_UINT_eg (TRUNC R600_Reg32:$src))>;

//===----------------------------------------------------------------------===//
// Memory read/write instructions
//===----------------------------------------------------------------------===//

let usesCustomInserter = 1 in {

def RAT_WRITE_CACHELESS_eg : EG_CF_RAT <0x57, 0x2, 0, (outs),
  (ins R600_TReg32_X:$rw_gpr, R600_TReg32_X:$index_gpr, i32imm:$eop),
  "RAT_WRITE_CACHELESS_eg $rw_gpr, $index_gpr, $eop",
  []>
{
  let RIM         = 0;
  // XXX: Have a separate instruction for non-indexed writes.
  let TYPE        = 1;
  let RW_REL      = 0;
  let ELEM_SIZE   = 0;

  let ARRAY_SIZE  = 0;
  let COMP_MASK   = 1;
  let BURST_COUNT = 0;
  let VPM         = 0;
  let MARK        = 0;
  let BARRIER     = 1;
}

} // End usesCustomInserter = 1

// i32 global_store
def : Pat <
  (global_store (i32 R600_TReg32_X:$val), R600_TReg32_X:$ptr),
  (RAT_WRITE_CACHELESS_eg R600_TReg32_X:$val, R600_TReg32_X:$ptr, 0)
>;

// Floating point global_store
def : Pat <
  (global_store (f32 R600_TReg32_X:$val), R600_TReg32_X:$ptr),
  (RAT_WRITE_CACHELESS_eg R600_TReg32_X:$val, R600_TReg32_X:$ptr, 0)
>;

class VTX_READ_eg <bits<8> buffer_id, dag outs, list<dag> pattern>
    : InstR600ISA <outs, (ins MEMxi:$ptr), "VTX_READ_eg $dst, $ptr", pattern> {

  // Operands
  bits<7> DST_GPR;
  bits<7> SRC_GPR;

  // Static fields
  bits<5> VC_INST = 0;
  bits<2> FETCH_TYPE = 2;
  bits<1> FETCH_WHOLE_QUAD = 0;
  bits<8> BUFFER_ID = buffer_id;
  bits<1> SRC_REL = 0;
  // XXX: We can infer this field based on the SRC_GPR.  This would allow us
  // to store vertex addresses in any channel, not just X.
  bits<2> SRC_SEL_X = 0;
  bits<6> MEGA_FETCH_COUNT;
  bits<1> DST_REL = 0;
  bits<3> DST_SEL_X;
  bits<3> DST_SEL_Y;
  bits<3> DST_SEL_Z;
  bits<3> DST_SEL_W;
  // The docs say that if this bit is set, then DATA_FORMAT, NUM_FORMAT_ALL,
  // FORMAT_COMP_ALL, SRF_MODE_ALL, and ENDIAN_SWAP fields will be ignored,
  // however, based on my testing if USE_CONST_FIELDS is set, then all
  // these fields need to be set to 0.
  bits<1> USE_CONST_FIELDS = 0;
  bits<6> DATA_FORMAT;
  bits<2> NUM_FORMAT_ALL = 1;
  bits<1> FORMAT_COMP_ALL = 0;
  bits<1> SRF_MODE_ALL = 0;

  // LLVM can only encode 64-bit instructions, so these fields are manually
  // encoded in R600CodeEmitter
  //
  // bits<16> OFFSET;
  // bits<2>  ENDIAN_SWAP = 0;
  // bits<1>  CONST_BUF_NO_STRIDE = 0;
  // bits<1>  MEGA_FETCH = 0;
  // bits<1>  ALT_CONST = 0;
  // bits<2>  BUFFER_INDEX_MODE = 0;

  // VTX_WORD0
  let Inst{4-0}   = VC_INST;
  let Inst{6-5}   = FETCH_TYPE;
  let Inst{7}     = FETCH_WHOLE_QUAD;
  let Inst{15-8}  = BUFFER_ID;
  let Inst{22-16} = SRC_GPR;
  let Inst{23}    = SRC_REL;
  let Inst{25-24} = SRC_SEL_X;
  let Inst{31-26} = MEGA_FETCH_COUNT;

  // VTX_WORD1_GPR
  let Inst{38-32} = DST_GPR;
  let Inst{39}    = DST_REL;
  let Inst{40}    = 0; // Reserved
  let Inst{43-41} = DST_SEL_X;
  let Inst{46-44} = DST_SEL_Y;
  let Inst{49-47} = DST_SEL_Z;
  let Inst{52-50} = DST_SEL_W;
  let Inst{53}    = USE_CONST_FIELDS;
  let Inst{59-54} = DATA_FORMAT;
  let Inst{61-60} = NUM_FORMAT_ALL;
  let Inst{62}    = FORMAT_COMP_ALL;
  let Inst{63}    = SRF_MODE_ALL;

  // VTX_WORD2 (LLVM can only encode 64-bit instructions, so WORD2 encoding
  // is done in R600CodeEmitter
  //
  // Inst{79-64} = OFFSET;
  // Inst{81-80} = ENDIAN_SWAP;
  // Inst{82}    = CONST_BUF_NO_STRIDE;
  // Inst{83}    = MEGA_FETCH;
  // Inst{84}    = ALT_CONST;
  // Inst{86-85} = BUFFER_INDEX_MODE;
  // Inst{95-86} = 0; Reserved

  // VTX_WORD3 (Padding)
  //
  // Inst{127-96} = 0;
}

class VTX_READ_32_eg <bits<8> buffer_id, list<dag> pattern>
    : VTX_READ_eg <buffer_id, (outs R600_TReg32_X:$dst), pattern> {

  let MEGA_FETCH_COUNT = 4;
  let DST_SEL_X        = 0;
  let DST_SEL_Y        = 7;   // Masked
  let DST_SEL_Z        = 7;   // Masked
  let DST_SEL_W        = 7;   // Masked
  let DATA_FORMAT      = 0xD; // COLOR_32

  // This is not really necessary, but there were some GPU hangs that appeared
  // to be caused by ALU instructions in the next instruction group that wrote
  // to the $ptr registers of the VTX_READ.  
  // e.g.
  // %T3_X<def> = VTX_READ_PARAM_i32_eg %T2_X<kill>, 24
  // %T2_X<def> = MOV %ZERO
  //Adding this constraint prevents this from happening.
  let Constraints = "$ptr.ptr = $dst";
}

class VTX_READ_128_eg <bits<8> buffer_id, list<dag> pattern>
    : VTX_READ_eg <buffer_id, (outs R600_Reg128:$dst), pattern> {

  let MEGA_FETCH_COUNT = 16;
  let DST_SEL_X        =  0;
  let DST_SEL_Y        =  1;
  let DST_SEL_Z        =  2;
  let DST_SEL_W        =  3;
  let DATA_FORMAT      =  0x22; // COLOR_32_32_32_32

  // XXX: Need to force VTX_READ_128 instructions to write to the same register
  // that holds its buffer address to avoid potential hangs.  We can't use
  // the same constraint as VTX_READ_32_eg, because the $ptr.ptr and $dst
  // registers are different sizes.
}

//===----------------------------------------------------------------------===//
// VTX Read from parameter memory space
//===----------------------------------------------------------------------===//

class VTX_READ_PARAM_32_eg <ValueType vt> : VTX_READ_32_eg <0,
  [(set (vt R600_TReg32_X:$dst), (load_param ADDRVTX_READ:$ptr))]
>;

def VTX_READ_PARAM_i32_eg : VTX_READ_PARAM_32_eg<i32>;
def VTX_READ_PARAM_f32_eg : VTX_READ_PARAM_32_eg<f32>;


//===----------------------------------------------------------------------===//
// VTX Read from global memory space
//===----------------------------------------------------------------------===//

// 32-bit reads

class VTX_READ_GLOBAL_eg <ValueType vt> : VTX_READ_32_eg <1,
  [(set (vt R600_TReg32_X:$dst), (global_load ADDRVTX_READ:$ptr))]
>;

def VTX_READ_GLOBAL_i32_eg : VTX_READ_GLOBAL_eg<i32>;
def VTX_READ_GLOBAL_f32_eg : VTX_READ_GLOBAL_eg<f32>;

// 128-bit reads

class VTX_READ_GLOBAL_128_eg <ValueType vt> : VTX_READ_128_eg <1,
  [(set (vt R600_Reg128:$dst), (global_load ADDRVTX_READ:$ptr))]
>;

def VTX_READ_GLOBAL_v4i32_eg : VTX_READ_GLOBAL_128_eg<v4i32>;
def VTX_READ_GLOBAL_v4f32_eg : VTX_READ_GLOBAL_128_eg<v4f32>;

}

let Predicates = [isCayman] in {

let isVector = 1 in { 

def RECIP_IEEE_cm : RECIP_IEEE_Common<0x86>;

def MULLO_INT_cm : MULLO_INT_Common<0x8F>;
def MULHI_INT_cm : MULHI_INT_Common<0x90>;
def MULLO_UINT_cm : MULLO_UINT_Common<0x91>;
def MULHI_UINT_cm : MULHI_UINT_Common<0x92>;

} // End isVector = 1

// RECIP_UINT emulation for Cayman
def : Pat <
  (AMDGPUurecip R600_Reg32:$src0),
  (FLT_TO_UINT_eg (MUL_IEEE (RECIP_IEEE_cm (UINT_TO_FLT_eg R600_Reg32:$src0)),
                            (MOV_IMM_I32 (i32 ALU_LITERAL_X), 0x4f800000)))
>;

} // End isCayman

let isCodeGenOnly = 1 in {

  def MULLIT : AMDGPUShaderInst <
    (outs R600_Reg128:$dst),
    (ins R600_Reg32:$src0, R600_Reg32:$src1, R600_Reg32:$src2),
    "MULLIT $dst, $src0, $src1",
    [(set R600_Reg128:$dst, (int_AMDGPU_mullit R600_Reg32:$src0, R600_Reg32:$src1, R600_Reg32:$src2))]
  >;

let usesCustomInserter = 1, isPseudo = 1 in {

class R600PreloadInst <string asm, Intrinsic intr> : AMDGPUInst <
  (outs R600_TReg32:$dst),
  (ins),
  asm,
  [(set R600_TReg32:$dst, (intr))]
>;

def R600_LOAD_CONST : AMDGPUShaderInst <
  (outs R600_Reg32:$dst),
  (ins i32imm:$src0),
  "R600_LOAD_CONST $dst, $src0",
  [(set R600_Reg32:$dst, (int_AMDGPU_load_const imm:$src0))]
>;

def RESERVE_REG : AMDGPUShaderInst <
  (outs),
  (ins i32imm:$src),
  "RESERVE_REG $src",
  [(int_AMDGPU_reserve_reg imm:$src)]
>;

def TXD: AMDGPUShaderInst <
  (outs R600_Reg128:$dst),
  (ins R600_Reg128:$src0, R600_Reg128:$src1, R600_Reg128:$src2, i32imm:$src3, i32imm:$src4),
  "TXD $dst, $src0, $src1, $src2, $src3, $src4",
  [(set R600_Reg128:$dst, (int_AMDGPU_txd R600_Reg128:$src0, R600_Reg128:$src1, R600_Reg128:$src2, imm:$src3, imm:$src4))]
>;

def TXD_SHADOW: AMDGPUShaderInst <
  (outs R600_Reg128:$dst),
  (ins R600_Reg128:$src0, R600_Reg128:$src1, R600_Reg128:$src2, i32imm:$src3, i32imm:$src4),
  "TXD_SHADOW $dst, $src0, $src1, $src2, $src3, $src4",
  [(set R600_Reg128:$dst, (int_AMDGPU_txd R600_Reg128:$src0, R600_Reg128:$src1, R600_Reg128:$src2, imm:$src3, TEX_SHADOW:$src4))]
>;

} // End usesCustomInserter = 1, isPseudo = 1

} // End isCodeGenOnly = 1

def CLAMP_R600 :  CLAMP <R600_Reg32>;
def FABS_R600 : FABS<R600_Reg32>;
def FNEG_R600 : FNEG<R600_Reg32>;

let usesCustomInserter = 1 in {

def MASK_WRITE : AMDGPUShaderInst <
    (outs),
    (ins R600_Reg32:$src),
    "MASK_WRITE $src",
    []
>;

} // End usesCustomInserter = 1

//===---------------------------------------------------------------------===//
// Return instruction
//===---------------------------------------------------------------------===//
let isTerminator = 1, isReturn = 1, isBarrier = 1, hasCtrlDep = 1 in {
  def RETURN          : ILFormat<(outs), (ins variable_ops),
      "RETURN", [(IL_retflag)]>;
}

//===----------------------------------------------------------------------===//
// ISel Patterns
//===----------------------------------------------------------------------===//

// KIL Patterns
def KILP : Pat <
  (int_AMDGPU_kilp),
  (MASK_WRITE (KILLGT (f32 ONE), (f32 ZERO), 0))
>;

def KIL : Pat <
  (int_AMDGPU_kill R600_Reg32:$src0),
  (MASK_WRITE (KILLGT (f32 ZERO), (f32 R600_Reg32:$src0), 0))
>;

// SGT Reverse args
def : Pat <
  (selectcc (f32 R600_Reg32:$src0), R600_Reg32:$src1, FP_ONE, FP_ZERO, COND_LT),
  (SGT R600_Reg32:$src1, R600_Reg32:$src0)
>;

// SGE Reverse args
def : Pat <
  (selectcc (f32 R600_Reg32:$src0), R600_Reg32:$src1, FP_ONE, FP_ZERO, COND_LE),
  (SGE R600_Reg32:$src1, R600_Reg32:$src0) 
>;

// SETGT_INT reverse args
def : Pat <
  (selectcc (i32 R600_Reg32:$src0), R600_Reg32:$src1, -1, 0, SETLT),
  (SETGT_INT R600_Reg32:$src1, R600_Reg32:$src0)
>;

// SETGE_INT reverse args
def : Pat <
  (selectcc (i32 R600_Reg32:$src0), R600_Reg32:$src1, -1, 0, SETLE),
  (SETGE_INT R600_Reg32:$src1, R600_Reg32:$src0)
>;

// SETGT_UINT reverse args
def : Pat <
  (selectcc (i32 R600_Reg32:$src0), R600_Reg32:$src1, -1, 0, SETULT),
  (SETGT_UINT R600_Reg32:$src1, R600_Reg32:$src0)
>;

// SETGE_UINT reverse args
def : Pat <
  (selectcc (i32 R600_Reg32:$src0), R600_Reg32:$src1, -1, 0, SETULE),
  (SETGE_UINT R600_Reg32:$src0, R600_Reg32:$src1)
>;

// The next two patterns are special cases for handling 'true if ordered' and
// 'true if unordered' conditionals.  The assumption here is that the behavior of
// SETE and SNE conforms to the Direct3D 10 rules for floating point values
// described here:
// http://msdn.microsoft.com/en-us/library/windows/desktop/cc308050.aspx#alpha_32_bit
// We assume that  SETE returns false when one of the operands is NAN and
// SNE returns true when on of the operands is NAN

//SETE - 'true if ordered'
def : Pat <
  (selectcc (f32 R600_Reg32:$src0), R600_Reg32:$src1, FP_ONE, FP_ZERO, SETO),
  (SETE R600_Reg32:$src0, R600_Reg32:$src1)
>;

//SNE - 'true if unordered'
def : Pat <
  (selectcc (f32 R600_Reg32:$src0), R600_Reg32:$src1, FP_ONE, FP_ZERO, SETUO),
  (SNE R600_Reg32:$src0, R600_Reg32:$src1)
>;

def : Extract_Element <f32, v4f32, R600_Reg128, 0, sel_x>;
def : Extract_Element <f32, v4f32, R600_Reg128, 1, sel_y>;
def : Extract_Element <f32, v4f32, R600_Reg128, 2, sel_z>;
def : Extract_Element <f32, v4f32, R600_Reg128, 3, sel_w>;

def : Insert_Element <f32, v4f32, R600_Reg32, R600_Reg128, 4, sel_x>;
def : Insert_Element <f32, v4f32, R600_Reg32, R600_Reg128, 5, sel_y>;
def : Insert_Element <f32, v4f32, R600_Reg32, R600_Reg128, 6, sel_z>;
def : Insert_Element <f32, v4f32, R600_Reg32, R600_Reg128, 7, sel_w>;

def : Extract_Element <i32, v4i32, R600_Reg128, 0, sel_x>;
def : Extract_Element <i32, v4i32, R600_Reg128, 1, sel_y>;
def : Extract_Element <i32, v4i32, R600_Reg128, 2, sel_z>;
def : Extract_Element <i32, v4i32, R600_Reg128, 3, sel_w>;

def : Insert_Element <i32, v4i32, R600_Reg32, R600_Reg128, 4, sel_x>;
def : Insert_Element <i32, v4i32, R600_Reg32, R600_Reg128, 5, sel_y>;
def : Insert_Element <i32, v4i32, R600_Reg32, R600_Reg128, 6, sel_z>;
def : Insert_Element <i32, v4i32, R600_Reg32, R600_Reg128, 7, sel_w>;

def : Vector_Build <v4f32, R600_Reg32>;
def : Vector_Build <v4i32, R600_Reg32>;

// bitconvert patterns

def : BitConvert <i32, f32, R600_Reg32>;
def : BitConvert <f32, i32, R600_Reg32>;
def : BitConvert <v4f32, v4i32, R600_Reg128>;

} // End isR600toCayman Predicate
