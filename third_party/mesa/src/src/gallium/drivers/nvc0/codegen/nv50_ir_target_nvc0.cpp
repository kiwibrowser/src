/*
 * Copyright 2011 Christoph Bumiller
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "nv50_ir_target_nvc0.h"

namespace nv50_ir {

Target *getTargetNVC0(unsigned int chipset)
{
   return new TargetNVC0(chipset);
}

TargetNVC0::TargetNVC0(unsigned int card) : Target(false, card >= 0xe4)
{
   chipset = card;
   initOpInfo();
}

// BULTINS / LIBRARY FUNCTIONS:

// lazyness -> will just hardcode everything for the time being

// Will probably make this nicer once we support subroutines properly,
// i.e. when we have an input IR that provides function declarations.

// TODO: separate version for nve4+ which doesn't like the 4-byte insn formats
static const uint32_t nvc0_builtin_code[] =
{
// DIV U32: slow unsigned integer division
//
// UNR recurrence (q = a / b):
// look for z such that 2^32 - b <= b * z < 2^32
// then q - 1 <= (a * z) / 2^32 <= q
//
// INPUT:   $r0: dividend, $r1: divisor
// OUTPUT:  $r0: result, $r1: modulus
// CLOBBER: $r2 - $r3, $p0 - $p1
// SIZE:    22 / 14 * 8 bytes
//
#if 1
   0x04009c03, 0x78000000,
   0x7c209c82, 0x38000000, // 0x7c209cdd,
   0x0400dde2, 0x18000000, // 0x0010dd18,
   0x08309c03, 0x60000000,
   0x05205d04, 0x1c000000, // 0x05605c18,
   0x0810dc03, 0x50000000, // 0x0810dc2a,
   0x0c209c43, 0x20040000,
   0x0810dc03, 0x50000000,
   0x0c209c43, 0x20040000,
   0x0810dc03, 0x50000000,
   0x0c209c43, 0x20040000,
   0x0810dc03, 0x50000000,
   0x0c209c43, 0x20040000,
   0x0810dc03, 0x50000000,
   0x0c209c43, 0x20040000,
   0x0000dde4, 0x28000000,
   0x08001c43, 0x50000000,
   0x05209d04, 0x1c000000, // 0x05609c18,
   0x00105c03, 0x20060000, // 0x0010430d,
   0x0811dc03, 0x1b0e0000,
   0x08104103, 0x48000000,
   0x04000002, 0x08000000,
   0x0811c003, 0x1b0e0000,
   0x08104103, 0x48000000,
   0x04000002, 0x08000000, // 0x040000ac,
   0x00001de7, 0x90000000, // 0x90001dff,
#else
   0x0401dc03, 0x1b0e0000,
   0x00008003, 0x78000000,
   0x0400c003, 0x78000000,
   0x0c20c103, 0x48000000,
   0x0c108003, 0x60000000,
   0x00005c28,
   0x00001d18,
   0x0031c023, 0x1b0ec000,
   0xb000a1e7, 0x40000000,
   0x04000003, 0x6000c000,
   0x0813dc03, 0x1b000000,
   0x0420446c,
   0x040004bd,
   0x04208003, 0x5800c000,
   0x0430c103, 0x4800c000,
   0x0ffc5dff,
   0x90001dff,
#endif

// DIV S32: slow signed integer division
//
// INPUT:   $r0: dividend, $r1: divisor
// OUTPUT:  $r0: result, $r1: modulus
// CLOBBER: $r2 - $r3, $p0 - $p3
// SIZE:    18 * 8 bytes
//
   0xfc05dc23, 0x188e0000,
   0xfc17dc23, 0x18c40000,
   0x01201ec4, 0x1c000000, // 0x03301e18,
   0x05205ec4, 0x1c000000, // 0x07305e18,
   0x0401dc03, 0x1b0e0000,
   0x00008003, 0x78000000,
   0x0400c003, 0x78000000,
   0x0c20c103, 0x48000000,
   0x0c108003, 0x60000000,
   0x00005de4, 0x28000000, // 0x00005c28,
   0x00001de2, 0x18000000, // 0x00001d18,
   0x0031c023, 0x1b0ec000,
   0xe000a1e7, 0x40000000, // 0xb000a1e7, 0x40000000,
   0x04000003, 0x6000c000,
   0x0813dc03, 0x1b000000,
   0x04204603, 0x48000000, // 0x0420446c,
   0x04000442, 0x38000000, // 0x040004bd,
   0x04208003, 0x5800c000,
   0x0430c103, 0x4800c000,
   0xe0001de7, 0x4003fffe, // 0x0ffc5dff,
   0x01200f84, 0x1c000000, // 0x01700e18,
   0x05204b84, 0x1c000000, // 0x05704a18,
   0x00001de7, 0x90000000, // 0x90001dff,

// RCP F64: Newton Raphson reciprocal(x): r_{i+1} = r_i * (2.0 - x * r_i)
//
// INPUT:   $r0d (x)
// OUTPUT:  $r0d (rcp(x))
// CLOBBER: $r2 - $r7
// SIZE:    9 * 8 bytes
//
   0x9810dc08,
   0x00009c28,
   0x4001df18,
   0x00019d18,
   0x08011e01, 0x200c0000,
   0x10209c01, 0x50000000,
   0x08011e01, 0x200c0000,
   0x10209c01, 0x50000000,
   0x08011e01, 0x200c0000,
   0x10201c01, 0x50000000,
   0x00001de7, 0x90000000,

// RSQ F64: Newton Raphson rsqrt(x): r_{i+1} = r_i * (1.5 - 0.5 * x * r_i * r_i)
//
// INPUT:   $r0d (x)
// OUTPUT:  $r0d (rsqrt(x))
// CLOBBER: $r2 - $r7
// SIZE:    14 * 8 bytes
//
   0x9c10dc08,
   0x00009c28,
   0x00019d18,
   0x3fe1df18,
   0x18001c01, 0x50000000,
   0x0001dde2, 0x18ffe000,
   0x08211c01, 0x50000000,
   0x10011e01, 0x200c0000,
   0x10209c01, 0x50000000,
   0x08211c01, 0x50000000,
   0x10011e01, 0x200c0000,
   0x10209c01, 0x50000000,
   0x08211c01, 0x50000000,
   0x10011e01, 0x200c0000,
   0x10201c01, 0x50000000,
   0x00001de7, 0x90000000,
};

static const uint16_t nvc0_builtin_offsets[NVC0_BUILTIN_COUNT] =
{
   0,
   8 * (26),
   8 * (26 + 23),
   8 * (26 + 23 + 9)
};

void
TargetNVC0::getBuiltinCode(const uint32_t **code, uint32_t *size) const
{
   *code = &nvc0_builtin_code[0];
   *size = sizeof(nvc0_builtin_code);
}

uint32_t
TargetNVC0::getBuiltinOffset(int builtin) const
{
   assert(builtin < NVC0_BUILTIN_COUNT);
   return nvc0_builtin_offsets[builtin];
}

struct opProperties
{
   operation op;
   unsigned int mNeg   : 4;
   unsigned int mAbs   : 4;
   unsigned int mNot   : 4;
   unsigned int mSat   : 4;
   unsigned int fConst : 3;
   unsigned int fImmd  : 4; // last bit indicates if full immediate is suppoted
};

static const struct opProperties _initProps[] =
{
   //           neg  abs  not  sat  c[]  imm
   { OP_ADD,    0x3, 0x3, 0x0, 0x8, 0x2, 0x2 | 0x8 },
   { OP_SUB,    0x3, 0x3, 0x0, 0x0, 0x2, 0x2 | 0x8 },
   { OP_MUL,    0x3, 0x0, 0x0, 0x8, 0x2, 0x2 | 0x8 },
   { OP_MAX,    0x3, 0x3, 0x0, 0x0, 0x2, 0x2 },
   { OP_MIN,    0x3, 0x3, 0x0, 0x0, 0x2, 0x2 },
   { OP_MAD,    0x7, 0x0, 0x0, 0x8, 0x6, 0x2 | 0x8 }, // special c[] constraint
   { OP_ABS,    0x0, 0x0, 0x0, 0x0, 0x1, 0x0 },
   { OP_NEG,    0x0, 0x1, 0x0, 0x0, 0x1, 0x0 },
   { OP_CVT,    0x1, 0x1, 0x0, 0x8, 0x1, 0x0 },
   { OP_CEIL,   0x1, 0x1, 0x0, 0x8, 0x1, 0x0 },
   { OP_FLOOR,  0x1, 0x1, 0x0, 0x8, 0x1, 0x0 },
   { OP_TRUNC,  0x1, 0x1, 0x0, 0x8, 0x1, 0x0 },
   { OP_AND,    0x0, 0x0, 0x3, 0x0, 0x2, 0x2 | 0x8 },
   { OP_OR,     0x0, 0x0, 0x3, 0x0, 0x2, 0x2 | 0x8 },
   { OP_XOR,    0x0, 0x0, 0x3, 0x0, 0x2, 0x2 | 0x8 },
   { OP_SHL,    0x0, 0x0, 0x0, 0x0, 0x2, 0x2 },
   { OP_SHR,    0x0, 0x0, 0x0, 0x0, 0x2, 0x2 },
   { OP_SET,    0x3, 0x3, 0x0, 0x0, 0x2, 0x2 },
   { OP_SLCT,   0x4, 0x0, 0x0, 0x0, 0x6, 0x2 }, // special c[] constraint
   { OP_PREEX2, 0x1, 0x1, 0x0, 0x0, 0x1, 0x1 },
   { OP_PRESIN, 0x1, 0x1, 0x0, 0x0, 0x1, 0x1 },
   { OP_COS,    0x1, 0x1, 0x0, 0x8, 0x0, 0x0 },
   { OP_SIN,    0x1, 0x1, 0x0, 0x8, 0x0, 0x0 },
   { OP_EX2,    0x1, 0x1, 0x0, 0x8, 0x0, 0x0 },
   { OP_LG2,    0x1, 0x1, 0x0, 0x8, 0x0, 0x0 },
   { OP_RCP,    0x1, 0x1, 0x0, 0x8, 0x0, 0x0 },
   { OP_RSQ,    0x1, 0x1, 0x0, 0x8, 0x0, 0x0 },
   { OP_DFDX,   0x1, 0x0, 0x0, 0x0, 0x0, 0x0 },
   { OP_DFDY,   0x1, 0x0, 0x0, 0x0, 0x0, 0x0 },
   { OP_CALL,   0x0, 0x0, 0x0, 0x0, 0x1, 0x0 },
   { OP_INSBF,  0x0, 0x0, 0x0, 0x0, 0x0, 0x4 },
   { OP_SET_AND, 0x3, 0x3, 0x0, 0x0, 0x2, 0x2 },
   { OP_SET_OR, 0x3, 0x3, 0x0, 0x0, 0x2, 0x2 },
   { OP_SET_XOR, 0x3, 0x3, 0x0, 0x0, 0x2, 0x2 },
   // saturate only:
   { OP_LINTERP, 0x0, 0x0, 0x0, 0x8, 0x0, 0x0 },
   { OP_PINTERP, 0x0, 0x0, 0x0, 0x8, 0x0, 0x0 },
};

void TargetNVC0::initOpInfo()
{
   unsigned int i, j;

   static const uint32_t commutative[(OP_LAST + 31) / 32] =
   {
      // ADD, MAD, MUL, AND, OR, XOR, MAX, MIN
      0x0670ca00, 0x0000003f, 0x00000000
   };

   static const uint32_t shortForm[(OP_LAST + 31) / 32] =
   {
      // ADD, MAD, MUL, AND, OR, XOR, PRESIN, PREEX2, SFN, CVT, PINTERP, MOV
      0x0670ca00, 0x00000000, 0x00000000
   };

   static const operation noDest[] =
   {
      OP_STORE, OP_WRSV, OP_EXPORT, OP_BRA, OP_CALL, OP_RET, OP_EXIT,
      OP_DISCARD, OP_CONT, OP_BREAK, OP_PRECONT, OP_PREBREAK, OP_PRERET,
      OP_JOIN, OP_JOINAT, OP_BRKPT, OP_MEMBAR, OP_EMIT, OP_RESTART,
      OP_QUADON, OP_QUADPOP, OP_TEXBAR
   };

   for (i = 0; i < DATA_FILE_COUNT; ++i)
      nativeFileMap[i] = (DataFile)i;
   nativeFileMap[FILE_ADDRESS] = FILE_GPR;

   for (i = 0; i < OP_LAST; ++i) {
      opInfo[i].variants = NULL;
      opInfo[i].op = (operation)i;
      opInfo[i].srcTypes = 1 << (int)TYPE_F32;
      opInfo[i].dstTypes = 1 << (int)TYPE_F32;
      opInfo[i].immdBits = 0;
      opInfo[i].srcNr = operationSrcNr[i];

      for (j = 0; j < opInfo[i].srcNr; ++j) {
         opInfo[i].srcMods[j] = 0;
         opInfo[i].srcFiles[j] = 1 << (int)FILE_GPR;
      }
      opInfo[i].dstMods = 0;
      opInfo[i].dstFiles = 1 << (int)FILE_GPR;

      opInfo[i].hasDest = 1;
      opInfo[i].vector = (i >= OP_TEX && i <= OP_TEXCSAA);
      opInfo[i].commutative = (commutative[i / 32] >> (i % 32)) & 1;
      opInfo[i].pseudo = (i < OP_MOV);
      opInfo[i].predicate = !opInfo[i].pseudo;
      opInfo[i].flow = (i >= OP_BRA && i <= OP_JOIN);
      opInfo[i].minEncSize = (shortForm[i / 32] & (1 << (i % 32))) ? 4 : 8;
   }
   for (i = 0; i < sizeof(noDest) / sizeof(noDest[0]); ++i)
      opInfo[noDest[i]].hasDest = 0;

   for (i = 0; i < sizeof(_initProps) / sizeof(_initProps[0]); ++i) {
      const struct opProperties *prop = &_initProps[i];

      for (int s = 0; s < 3; ++s) {
         if (prop->mNeg & (1 << s))
            opInfo[prop->op].srcMods[s] |= NV50_IR_MOD_NEG;
         if (prop->mAbs & (1 << s))
            opInfo[prop->op].srcMods[s] |= NV50_IR_MOD_ABS;
         if (prop->mNot & (1 << s))
            opInfo[prop->op].srcMods[s] |= NV50_IR_MOD_NOT;
         if (prop->fConst & (1 << s))
            opInfo[prop->op].srcFiles[s] |= 1 << (int)FILE_MEMORY_CONST;
         if (prop->fImmd & (1 << s))
            opInfo[prop->op].srcFiles[s] |= 1 << (int)FILE_IMMEDIATE;
         if (prop->fImmd & 8)
            opInfo[prop->op].immdBits = 0xffffffff;
      }
      if (prop->mSat & 8)
         opInfo[prop->op].dstMods = NV50_IR_MOD_SAT;
   }
}

unsigned int
TargetNVC0::getFileSize(DataFile file) const
{
   switch (file) {
   case FILE_NULL:          return 0;
   case FILE_GPR:           return 63;
   case FILE_PREDICATE:     return 7;
   case FILE_FLAGS:         return 1;
   case FILE_ADDRESS:       return 0;
   case FILE_IMMEDIATE:     return 0;
   case FILE_MEMORY_CONST:  return 65536;
   case FILE_SHADER_INPUT:  return 0x400;
   case FILE_SHADER_OUTPUT: return 0x400;
   case FILE_MEMORY_GLOBAL: return 0xffffffff;
   case FILE_MEMORY_SHARED: return 16 << 10;
   case FILE_MEMORY_LOCAL:  return 48 << 10;
   case FILE_SYSTEM_VALUE:  return 32;
   default:
      assert(!"invalid file");
      return 0;
   }
}

unsigned int
TargetNVC0::getFileUnit(DataFile file) const
{
   if (file == FILE_GPR || file == FILE_ADDRESS || file == FILE_SYSTEM_VALUE)
      return 2;
   return 0;
}

uint32_t
TargetNVC0::getSVAddress(DataFile shaderFile, const Symbol *sym) const
{
   const int idx = sym->reg.data.sv.index;
   const SVSemantic sv = sym->reg.data.sv.sv;

   const bool isInput = shaderFile == FILE_SHADER_INPUT;

   switch (sv) {
   case SV_POSITION:       return 0x070 + idx * 4;
   case SV_INSTANCE_ID:    return 0x2f8;
   case SV_VERTEX_ID:      return 0x2fc;
   case SV_PRIMITIVE_ID:   return isInput ? 0x060 : 0x040;
   case SV_LAYER:          return 0x064;
   case SV_VIEWPORT_INDEX: return 0x068;
   case SV_POINT_SIZE:     return 0x06c;
   case SV_CLIP_DISTANCE:  return 0x2c0 + idx * 4;
   case SV_POINT_COORD:    return 0x2e0 + idx * 4;
   case SV_FACE:           return 0x3fc;
   case SV_TESS_FACTOR:    return 0x000 + idx * 4;
   case SV_TESS_COORD:     return 0x2f0 + idx * 4;
   default:
      return 0xffffffff;
   }
}

bool
TargetNVC0::insnCanLoad(const Instruction *i, int s,
                        const Instruction *ld) const
{
   DataFile sf = ld->src(0).getFile();

   // immediate 0 can be represented by GPR $r63
   if (sf == FILE_IMMEDIATE && ld->getSrc(0)->reg.data.u64 == 0)
      return (!i->asTex() && i->op != OP_EXPORT && i->op != OP_STORE);

   if (s >= opInfo[i->op].srcNr)
      return false;
   if (!(opInfo[i->op].srcFiles[s] & (1 << (int)sf)))
      return false;

   // indirect loads can only be done by OP_LOAD/VFETCH/INTERP on nvc0
   if (ld->src(0).isIndirect(0))
      return false;

   for (int k = 0; i->srcExists(k); ++k) {
      if (i->src(k).getFile() == FILE_IMMEDIATE) {
         if (i->getSrc(k)->reg.data.u64 != 0)
            return false;
      } else
      if (i->src(k).getFile() != FILE_GPR &&
          i->src(k).getFile() != FILE_PREDICATE) {
         return false;
      }
   }

   // not all instructions support full 32 bit immediates
   if (sf == FILE_IMMEDIATE) {
      Storage &reg = ld->getSrc(0)->asImm()->reg;

      if (opInfo[i->op].immdBits != 0xffffffff) {
         if (i->sType == TYPE_F32) {
            if (reg.data.u32 & 0xfff)
               return false;
         } else
         if (i->sType == TYPE_S32 || i->sType == TYPE_U32) {
            // with u32, 0xfffff counts as 0xffffffff as well
            if (reg.data.s32 > 0x7ffff || reg.data.s32 < -0x80000)
               return false;
         }
      } else
      if (i->op == OP_MAD || i->op == OP_FMA) {
         // requires src == dst, cannot decide before RA
         // (except if we implement more constraints)
         if (ld->getSrc(0)->asImm()->reg.data.u32 & 0xfff)
            return false;
      }
   }

   return true;
}

bool
TargetNVC0::isAccessSupported(DataFile file, DataType ty) const
{
   if (ty == TYPE_NONE)
      return false;
   if (file == FILE_MEMORY_CONST && getChipset() >= 0xe0) // wrong encoding ?
      return typeSizeof(ty) <= 8;
   if (ty == TYPE_B96)
      return (file == FILE_SHADER_INPUT) || (file == FILE_SHADER_OUTPUT);
   return true;
}

bool
TargetNVC0::isOpSupported(operation op, DataType ty) const
{
   if ((op == OP_MAD || op == OP_FMA) && (ty != TYPE_F32))
      return false;
   if (op == OP_SAD && ty != TYPE_S32 && ty != TYPE_U32)
      return false;
   if (op == OP_POW || op == OP_SQRT || op == OP_DIV || op == OP_MOD)
      return false;
   return true;
}

bool
TargetNVC0::isModSupported(const Instruction *insn, int s, Modifier mod) const
{
   if (!isFloatType(insn->dType)) {
      switch (insn->op) {
      case OP_ABS:
      case OP_NEG:
      case OP_CVT:
      case OP_CEIL:
      case OP_FLOOR:
      case OP_TRUNC:
      case OP_AND:
      case OP_OR:
      case OP_XOR:
         break;
      case OP_ADD:
         if (mod.abs())
            return false;
         if (insn->src(s ? 0 : 1).mod.neg())
            return false;
         break;
      case OP_SUB:
         if (s == 0)
            return insn->src(1).mod.neg() ? false : true;
         break;
      default:
         return false;
      }
   }
   if (s > 3)
      return false;
   return (mod & Modifier(opInfo[insn->op].srcMods[s])) == mod;
}

bool
TargetNVC0::mayPredicate(const Instruction *insn, const Value *pred) const
{
   if (insn->getPredicate())
      return false;
   return opInfo[insn->op].predicate;
}

bool
TargetNVC0::isSatSupported(const Instruction *insn) const
{
   if (insn->op == OP_CVT)
      return true;
   if (!(opInfo[insn->op].dstMods & NV50_IR_MOD_SAT))
      return false;

   if (insn->dType == TYPE_U32)
      return (insn->op == OP_ADD) || (insn->op == OP_MAD);

   return insn->dType == TYPE_F32;
}

bool
TargetNVC0::isPostMultiplySupported(operation op, float f, int& e) const
{
   if (op != OP_MUL)
      return false;
   f = fabsf(f);
   e = static_cast<int>(log2f(f));
   if (e < -3 || e > 3)
      return false;
   return f == exp2f(static_cast<float>(e));
}

// TODO: better values
// this could be more precise, e.g. depending on the issue-to-read/write delay
// of the depending instruction, but it's good enough
int TargetNVC0::getLatency(const Instruction *i) const
{
   if (chipset >= 0xe4) {
      if (i->dType == TYPE_F64 || i->sType == TYPE_F64)
         return 20;
      switch (i->op) {
      case OP_LINTERP:
      case OP_PINTERP:
         return 15;
      case OP_LOAD:
         if (i->src(0).getFile() == FILE_MEMORY_CONST)
            return 9;
         // fall through
      case OP_VFETCH:
         return 24;
      default:
         if (Target::getOpClass(i->op) == OPCLASS_TEXTURE)
            return 17;
         if (i->op == OP_MUL && i->dType != TYPE_F32)
            return 15;
         return 9;
      }
   } else {
      if (i->op == OP_LOAD) {
         if (i->cache == CACHE_CV)
            return 700;
         return 48;
      }
      return 24;
   }
   return 32;
}

// These are "inverse" throughput values, i.e. the number of cycles required
// to issue a specific instruction for a full warp (32 threads).
//
// Assuming we have more than 1 warp in flight, a higher issue latency results
// in a lower result latency since the MP will have spent more time with other
// warps.
// This also helps to determine the number of cycles between instructions in
// a single warp.
//
int TargetNVC0::getThroughput(const Instruction *i) const
{
   // TODO: better values
   if (i->dType == TYPE_F32) {
      switch (i->op) {
      case OP_ADD:
      case OP_MUL:
      case OP_MAD:
      case OP_FMA:
         return 1;
      case OP_CVT:
      case OP_CEIL:
      case OP_FLOOR:
      case OP_TRUNC:
      case OP_SET:
      case OP_SLCT:
      case OP_MIN:
      case OP_MAX:
         return 2;
      case OP_RCP:
      case OP_RSQ:
      case OP_LG2:
      case OP_SIN:
      case OP_COS:
      case OP_PRESIN:
      case OP_PREEX2:
      default:
         return 8;
      }
   } else
   if (i->dType == TYPE_U32 || i->dType == TYPE_S32) {
      switch (i->op) {
      case OP_ADD:
      case OP_AND:
      case OP_OR:
      case OP_XOR:
      case OP_NOT:
         return 1;
      case OP_MUL:
      case OP_MAD:
      case OP_CVT:
      case OP_SET:
      case OP_SLCT:
      case OP_SHL:
      case OP_SHR:
      case OP_NEG:
      case OP_ABS:
      case OP_MIN:
      case OP_MAX:
      default:
         return 2;
      }
   } else
   if (i->dType == TYPE_F64) {
      return 2;
   } else {
      return 1;
   }
}

bool TargetNVC0::canDualIssue(const Instruction *a, const Instruction *b) const
{
   const OpClass clA = operationClass[a->op];
   const OpClass clB = operationClass[b->op];

   if (getChipset() >= 0xe4) {
      // not texturing
      // not if the 2nd instruction isn't necessarily executed
      if (clA == OPCLASS_TEXTURE || clA == OPCLASS_FLOW)
         return false;
      // anything with MOV
      if (a->op == OP_MOV || b->op == OP_MOV)
         return true;
      if (clA == clB) {
         // only F32 arith or integer additions
         if (clA != OPCLASS_ARITH)
            return false;
         return (a->dType == TYPE_F32 || a->op == OP_ADD ||
                 b->dType == TYPE_F32 || b->op == OP_ADD);
      }
      // nothing with TEXBAR
      if (a->op == OP_TEXBAR || b->op == OP_TEXBAR)
         return false;
      // no loads and stores accessing the the same space
      if ((clA == OPCLASS_LOAD && clB == OPCLASS_STORE) ||
          (clB == OPCLASS_LOAD && clA == OPCLASS_STORE))
         if (a->src(0).getFile() == b->src(0).getFile())
            return false;
      // no > 32-bit ops
      if (typeSizeof(a->dType) > 4 || typeSizeof(b->dType) > 4 ||
          typeSizeof(a->sType) > 4 || typeSizeof(b->sType) > 4)
         return false;
      return true;
   } else {
      return false; // info not needed (yet)
   }
}

} // namespace nv50_ir
