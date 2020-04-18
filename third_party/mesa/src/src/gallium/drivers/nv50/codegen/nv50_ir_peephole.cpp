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

#include "nv50_ir.h"
#include "nv50_ir_target.h"
#include "nv50_ir_build_util.h"

extern "C" {
#include "util/u_math.h"
}

namespace nv50_ir {

bool
Instruction::isNop() const
{
   if (op == OP_PHI || op == OP_SPLIT || op == OP_MERGE || op == OP_CONSTRAINT)
      return true;
   if (terminator || join) // XXX: should terminator imply flow ?
      return false;
   if (!fixed && op == OP_NOP)
      return true;

   if (defExists(0) && def(0).rep()->reg.data.id < 0) {
      for (int d = 1; defExists(d); ++d)
         if (def(d).rep()->reg.data.id >= 0)
            WARN("part of vector result is unused !\n");
      return true;
   }

   if (op == OP_MOV || op == OP_UNION) {
      if (!getDef(0)->equals(getSrc(0)))
         return false;
      if (op == OP_UNION)
         if (!def(0).rep()->equals(getSrc(1)))
            return false;
      return true;
   }

   return false;
}

bool Instruction::isDead() const
{
   if (op == OP_STORE ||
       op == OP_EXPORT ||
       op == OP_WRSV)
      return false;

   for (int d = 0; defExists(d); ++d)
      if (getDef(d)->refCount() || getDef(d)->reg.data.id >= 0)
         return false;

   if (terminator || asFlow())
      return false;
   if (fixed)
      return false;

   return true;
};

// =============================================================================

class CopyPropagation : public Pass
{
private:
   virtual bool visit(BasicBlock *);
};

// Propagate all MOVs forward to make subsequent optimization easier, except if
// the sources stem from a phi, in which case we don't want to mess up potential
// swaps $rX <-> $rY, i.e. do not create live range overlaps of phi src and def.
bool
CopyPropagation::visit(BasicBlock *bb)
{
   Instruction *mov, *si, *next;

   for (mov = bb->getEntry(); mov; mov = next) {
      next = mov->next;
      if (mov->op != OP_MOV || mov->fixed || !mov->getSrc(0)->asLValue())
         continue;
      if (mov->getPredicate())
         continue;
      if (mov->def(0).getFile() != mov->src(0).getFile())
         continue;
      si = mov->getSrc(0)->getInsn();
      if (mov->getDef(0)->reg.data.id < 0 && si && si->op != OP_PHI) {
         // propagate
         mov->def(0).replace(mov->getSrc(0), false);
         delete_Instruction(prog, mov);
      }
   }
   return true;
}

// =============================================================================

class LoadPropagation : public Pass
{
private:
   virtual bool visit(BasicBlock *);

   void checkSwapSrc01(Instruction *);

   bool isCSpaceLoad(Instruction *);
   bool isImmd32Load(Instruction *);
   bool isAttribOrSharedLoad(Instruction *);
};

bool
LoadPropagation::isCSpaceLoad(Instruction *ld)
{
   return ld && ld->op == OP_LOAD && ld->src(0).getFile() == FILE_MEMORY_CONST;
}

bool
LoadPropagation::isImmd32Load(Instruction *ld)
{
   if (!ld || (ld->op != OP_MOV) || (typeSizeof(ld->dType) != 4))
      return false;
   return ld->src(0).getFile() == FILE_IMMEDIATE;
}

bool
LoadPropagation::isAttribOrSharedLoad(Instruction *ld)
{
   return ld &&
      (ld->op == OP_VFETCH ||
       (ld->op == OP_LOAD &&
        (ld->src(0).getFile() == FILE_SHADER_INPUT ||
         ld->src(0).getFile() == FILE_MEMORY_SHARED)));
}

void
LoadPropagation::checkSwapSrc01(Instruction *insn)
{
   if (!prog->getTarget()->getOpInfo(insn).commutative)
      if (insn->op != OP_SET && insn->op != OP_SLCT)
         return;
   if (insn->src(1).getFile() != FILE_GPR)
      return;

   Instruction *i0 = insn->getSrc(0)->getInsn();
   Instruction *i1 = insn->getSrc(1)->getInsn();

   if (isCSpaceLoad(i0)) {
      if (!isCSpaceLoad(i1))
         insn->swapSources(0, 1);
      else
         return;
   } else
   if (isImmd32Load(i0)) {
      if (!isCSpaceLoad(i1) && !isImmd32Load(i1))
         insn->swapSources(0, 1);
      else
         return;
   } else
   if (isAttribOrSharedLoad(i1)) {
      if (!isAttribOrSharedLoad(i0))
         insn->swapSources(0, 1);
      else
         return;
   } else {
      return;
   }

   if (insn->op == OP_SET)
      insn->asCmp()->setCond = reverseCondCode(insn->asCmp()->setCond);
   else
   if (insn->op == OP_SLCT)
      insn->asCmp()->setCond = inverseCondCode(insn->asCmp()->setCond);
}

bool
LoadPropagation::visit(BasicBlock *bb)
{
   const Target *targ = prog->getTarget();
   Instruction *next;

   for (Instruction *i = bb->getEntry(); i; i = next) {
      next = i->next;

      if (i->srcExists(1))
         checkSwapSrc01(i);

      for (int s = 0; i->srcExists(s); ++s) {
         Instruction *ld = i->getSrc(s)->getInsn();

         if (!ld || ld->fixed || (ld->op != OP_LOAD && ld->op != OP_MOV))
            continue;
         if (!targ->insnCanLoad(i, s, ld))
            continue;

         // propagate !
         i->setSrc(s, ld->getSrc(0));
         if (ld->src(0).isIndirect(0))
            i->setIndirect(s, 0, ld->getIndirect(0, 0));

         if (ld->getDef(0)->refCount() == 0)
            delete_Instruction(prog, ld);
      }
   }
   return true;
}

// =============================================================================

// Evaluate constant expressions.
class ConstantFolding : public Pass
{
public:
   bool foldAll(Program *);

private:
   virtual bool visit(BasicBlock *);

   void expr(Instruction *, ImmediateValue&, ImmediateValue&);
   void opnd(Instruction *, ImmediateValue&, int s);

   void unary(Instruction *, const ImmediateValue&);

   void tryCollapseChainedMULs(Instruction *, const int s, ImmediateValue&);

   // TGSI 'true' is converted to -1 by F2I(NEG(SET)), track back to SET
   CmpInstruction *findOriginForTestWithZero(Value *);

   unsigned int foldCount;

   BuildUtil bld;
};

// TODO: remember generated immediates and only revisit these
bool
ConstantFolding::foldAll(Program *prog)
{
   unsigned int iterCount = 0;
   do {
      foldCount = 0;
      if (!run(prog))
         return false;
   } while (foldCount && ++iterCount < 2);
   return true;
}

bool
ConstantFolding::visit(BasicBlock *bb)
{
   Instruction *i, *next;

   for (i = bb->getEntry(); i; i = next) {
      next = i->next;
      if (i->op == OP_MOV || i->op == OP_CALL)
         continue;

      ImmediateValue src0, src1;

      if (i->srcExists(1) &&
          i->src(0).getImmediate(src0) && i->src(1).getImmediate(src1))
         expr(i, src0, src1);
      else
      if (i->srcExists(0) && i->src(0).getImmediate(src0))
         opnd(i, src0, 0);
      else
      if (i->srcExists(1) && i->src(1).getImmediate(src1))
         opnd(i, src1, 1);
   }
   return true;
}

CmpInstruction *
ConstantFolding::findOriginForTestWithZero(Value *value)
{
   if (!value)
      return NULL;
   Instruction *insn = value->getInsn();

   while (insn && insn->op != OP_SET) {
      Instruction *next = NULL;
      switch (insn->op) {
      case OP_NEG:
      case OP_ABS:
      case OP_CVT:
         next = insn->getSrc(0)->getInsn();
         if (insn->sType != next->dType)
            return NULL;
         break;
      case OP_MOV:
         next = insn->getSrc(0)->getInsn();
         break;
      default:
         return NULL;
      }
      insn = next;
   }
   return insn ? insn->asCmp() : NULL;
}

void
Modifier::applyTo(ImmediateValue& imm) const
{
   switch (imm.reg.type) {
   case TYPE_F32:
      if (bits & NV50_IR_MOD_ABS)
         imm.reg.data.f32 = fabsf(imm.reg.data.f32);
      if (bits & NV50_IR_MOD_NEG)
         imm.reg.data.f32 = -imm.reg.data.f32;
      if (bits & NV50_IR_MOD_SAT) {
         if (imm.reg.data.f32 < 0.0f)
            imm.reg.data.f32 = 0.0f;
         else
         if (imm.reg.data.f32 > 1.0f)
            imm.reg.data.f32 = 1.0f;
      }
      assert(!(bits & NV50_IR_MOD_NOT));
      break;

   case TYPE_S8: // NOTE: will be extended
   case TYPE_S16:
   case TYPE_S32:
   case TYPE_U8: // NOTE: treated as signed
   case TYPE_U16:
   case TYPE_U32:
      if (bits & NV50_IR_MOD_ABS)
         imm.reg.data.s32 = (imm.reg.data.s32 >= 0) ?
            imm.reg.data.s32 : -imm.reg.data.s32;
      if (bits & NV50_IR_MOD_NEG)
         imm.reg.data.s32 = -imm.reg.data.s32;
      if (bits & NV50_IR_MOD_NOT)
         imm.reg.data.s32 = ~imm.reg.data.s32;
      break;

   case TYPE_F64:
      if (bits & NV50_IR_MOD_ABS)
         imm.reg.data.f64 = fabs(imm.reg.data.f64);
      if (bits & NV50_IR_MOD_NEG)
         imm.reg.data.f64 = -imm.reg.data.f64;
      if (bits & NV50_IR_MOD_SAT) {
         if (imm.reg.data.f64 < 0.0)
            imm.reg.data.f64 = 0.0;
         else
         if (imm.reg.data.f64 > 1.0)
            imm.reg.data.f64 = 1.0;
      }
      assert(!(bits & NV50_IR_MOD_NOT));
      break;

   default:
      assert(!"invalid/unhandled type");
      imm.reg.data.u64 = 0;
      break;
   }
}

operation
Modifier::getOp() const
{
   switch (bits) {
   case NV50_IR_MOD_ABS: return OP_ABS;
   case NV50_IR_MOD_NEG: return OP_NEG;
   case NV50_IR_MOD_SAT: return OP_SAT;
   case NV50_IR_MOD_NOT: return OP_NOT;
   case 0:
      return OP_MOV;
   default:
      return OP_CVT;
   }
}

void
ConstantFolding::expr(Instruction *i,
                      ImmediateValue &imm0, ImmediateValue &imm1)
{
   struct Storage *const a = &imm0.reg, *const b = &imm1.reg;
   struct Storage res;

   memset(&res.data, 0, sizeof(res.data));

   switch (i->op) {
   case OP_MAD:
   case OP_FMA:
   case OP_MUL:
      if (i->dnz && i->dType == TYPE_F32) {
         if (!isfinite(a->data.f32))
            a->data.f32 = 0.0f;
         if (!isfinite(b->data.f32))
            b->data.f32 = 0.0f;
      }
      switch (i->dType) {
      case TYPE_F32: res.data.f32 = a->data.f32 * b->data.f32; break;
      case TYPE_F64: res.data.f64 = a->data.f64 * b->data.f64; break;
      case TYPE_S32:
      case TYPE_U32: res.data.u32 = a->data.u32 * b->data.u32; break;
      default:
         return;
      }
      break;
   case OP_DIV:
      if (b->data.u32 == 0)
         break;
      switch (i->dType) {
      case TYPE_F32: res.data.f32 = a->data.f32 / b->data.f32; break;
      case TYPE_F64: res.data.f64 = a->data.f64 / b->data.f64; break;
      case TYPE_S32: res.data.s32 = a->data.s32 / b->data.s32; break;
      case TYPE_U32: res.data.u32 = a->data.u32 / b->data.u32; break;
      default:
         return;
      }
      break;
   case OP_ADD:
      switch (i->dType) {
      case TYPE_F32: res.data.f32 = a->data.f32 + b->data.f32; break;
      case TYPE_F64: res.data.f64 = a->data.f64 + b->data.f64; break;
      case TYPE_S32:
      case TYPE_U32: res.data.u32 = a->data.u32 + b->data.u32; break;
      default:
         return;
      }
      break;
   case OP_POW:
      switch (i->dType) {
      case TYPE_F32: res.data.f32 = pow(a->data.f32, b->data.f32); break;
      case TYPE_F64: res.data.f64 = pow(a->data.f64, b->data.f64); break;
      default:
         return;
      }
      break;
   case OP_MAX:
      switch (i->dType) {
      case TYPE_F32: res.data.f32 = MAX2(a->data.f32, b->data.f32); break;
      case TYPE_F64: res.data.f64 = MAX2(a->data.f64, b->data.f64); break;
      case TYPE_S32: res.data.s32 = MAX2(a->data.s32, b->data.s32); break;
      case TYPE_U32: res.data.u32 = MAX2(a->data.u32, b->data.u32); break;
      default:
         return;
      }
      break;
   case OP_MIN:
      switch (i->dType) {
      case TYPE_F32: res.data.f32 = MIN2(a->data.f32, b->data.f32); break;
      case TYPE_F64: res.data.f64 = MIN2(a->data.f64, b->data.f64); break;
      case TYPE_S32: res.data.s32 = MIN2(a->data.s32, b->data.s32); break;
      case TYPE_U32: res.data.u32 = MIN2(a->data.u32, b->data.u32); break;
      default:
         return;
      }
      break;
   case OP_AND:
      res.data.u64 = a->data.u64 & b->data.u64;
      break;
   case OP_OR:
      res.data.u64 = a->data.u64 | b->data.u64;
      break;
   case OP_XOR:
      res.data.u64 = a->data.u64 ^ b->data.u64;
      break;
   case OP_SHL:
      res.data.u32 = a->data.u32 << b->data.u32;
      break;
   case OP_SHR:
      switch (i->dType) {
      case TYPE_S32: res.data.s32 = a->data.s32 >> b->data.u32; break;
      case TYPE_U32: res.data.u32 = a->data.u32 >> b->data.u32; break;
      default:
         return;
      }
      break;
   case OP_SLCT:
      if (a->data.u32 != b->data.u32)
         return;
      res.data.u32 = a->data.u32;
      break;
   default:
      return;
   }
   ++foldCount;

   i->src(0).mod = Modifier(0);
   i->src(1).mod = Modifier(0);

   i->setSrc(0, new_ImmediateValue(i->bb->getProgram(), res.data.u32));
   i->setSrc(1, NULL);

   i->getSrc(0)->reg.data = res.data;

   if (i->op == OP_MAD || i->op == OP_FMA) {
      i->op = OP_ADD;

      i->setSrc(1, i->getSrc(0));
      i->src(1).mod = i->src(2).mod;
      i->setSrc(0, i->getSrc(2));
      i->setSrc(2, NULL);

      ImmediateValue src0;
      if (i->src(0).getImmediate(src0))
         expr(i, src0, *i->getSrc(1)->asImm());
   } else {
      i->op = OP_MOV;
   }
}

void
ConstantFolding::unary(Instruction *i, const ImmediateValue &imm)
{
   Storage res;

   if (i->dType != TYPE_F32)
      return;
   switch (i->op) {
   case OP_NEG: res.data.f32 = -imm.reg.data.f32; break;
   case OP_ABS: res.data.f32 = fabsf(imm.reg.data.f32); break;
   case OP_RCP: res.data.f32 = 1.0f / imm.reg.data.f32; break;
   case OP_RSQ: res.data.f32 = 1.0f / sqrtf(imm.reg.data.f32); break;
   case OP_LG2: res.data.f32 = log2f(imm.reg.data.f32); break;
   case OP_EX2: res.data.f32 = exp2f(imm.reg.data.f32); break;
   case OP_SIN: res.data.f32 = sinf(imm.reg.data.f32); break;
   case OP_COS: res.data.f32 = cosf(imm.reg.data.f32); break;
   case OP_SQRT: res.data.f32 = sqrtf(imm.reg.data.f32); break;
   case OP_PRESIN:
   case OP_PREEX2:
      // these should be handled in subsequent OP_SIN/COS/EX2
      res.data.f32 = imm.reg.data.f32;
      break;
   default:
      return;
   }
   i->op = OP_MOV;
   i->setSrc(0, new_ImmediateValue(i->bb->getProgram(), res.data.f32));
   i->src(0).mod = Modifier(0);
}

void
ConstantFolding::tryCollapseChainedMULs(Instruction *mul2,
                                        const int s, ImmediateValue& imm2)
{
   const int t = s ? 0 : 1;
   Instruction *insn;
   Instruction *mul1 = NULL; // mul1 before mul2
   int e = 0;
   float f = imm2.reg.data.f32;
   ImmediateValue imm1;

   assert(mul2->op == OP_MUL && mul2->dType == TYPE_F32);

   if (mul2->getSrc(t)->refCount() == 1) {
      insn = mul2->getSrc(t)->getInsn();
      if (!mul2->src(t).mod && insn->op == OP_MUL && insn->dType == TYPE_F32)
         mul1 = insn;
      if (mul1 && !mul1->saturate) {
         int s1;

         if (mul1->src(s1 = 0).getImmediate(imm1) ||
             mul1->src(s1 = 1).getImmediate(imm1)) {
            bld.setPosition(mul1, false);
            // a = mul r, imm1
            // d = mul a, imm2 -> d = mul r, (imm1 * imm2)
            mul1->setSrc(s1, bld.loadImm(NULL, f * imm1.reg.data.f32));
            mul1->src(s1).mod = Modifier(0);
            mul2->def(0).replace(mul1->getDef(0), false);
         } else
         if (prog->getTarget()->isPostMultiplySupported(OP_MUL, f, e)) {
            // c = mul a, b
            // d = mul c, imm   -> d = mul_x_imm a, b
            mul1->postFactor = e;
            mul2->def(0).replace(mul1->getDef(0), false);
            if (f < 0)
               mul1->src(0).mod *= Modifier(NV50_IR_MOD_NEG);
         }
         mul1->saturate = mul2->saturate;
         return;
      }
   }
   if (mul2->getDef(0)->refCount() == 1 && !mul2->saturate) {
      // b = mul a, imm
      // d = mul b, c   -> d = mul_x_imm a, c
      int s2, t2;
      insn = mul2->getDef(0)->uses.front()->getInsn();
      if (!insn)
         return;
      mul1 = mul2;
      mul2 = NULL;
      s2 = insn->getSrc(0) == mul1->getDef(0) ? 0 : 1;
      t2 = s2 ? 0 : 1;
      if (insn->op == OP_MUL && insn->dType == TYPE_F32)
         if (!insn->src(s2).mod && !insn->src(t2).getImmediate(imm1))
            mul2 = insn;
      if (mul2 && prog->getTarget()->isPostMultiplySupported(OP_MUL, f, e)) {
         mul2->postFactor = e;
         mul2->setSrc(s2, mul1->src(t));
         if (f < 0)
            mul2->src(s2).mod *= Modifier(NV50_IR_MOD_NEG);
      }
   }
}

void
ConstantFolding::opnd(Instruction *i, ImmediateValue &imm0, int s)
{
   const int t = !s;
   const operation op = i->op;

   switch (i->op) {
   case OP_MUL:
      if (i->dType == TYPE_F32)
         tryCollapseChainedMULs(i, s, imm0);

      if (imm0.isInteger(0)) {
         i->op = OP_MOV;
         i->setSrc(0, new_ImmediateValue(prog, 0u));
         i->src(0).mod = Modifier(0);
         i->setSrc(1, NULL);
      } else
      if (imm0.isInteger(1) || imm0.isInteger(-1)) {
         if (imm0.isNegative())
            i->src(t).mod = i->src(t).mod ^ Modifier(NV50_IR_MOD_NEG);
         i->op = i->src(t).mod.getOp();
         if (s == 0) {
            i->setSrc(0, i->getSrc(1));
            i->src(0).mod = i->src(1).mod;
            i->src(1).mod = 0;
         }
         if (i->op != OP_CVT)
            i->src(0).mod = 0;
         i->setSrc(1, NULL);
      } else
      if (imm0.isInteger(2) || imm0.isInteger(-2)) {
         if (imm0.isNegative())
            i->src(t).mod = i->src(t).mod ^ Modifier(NV50_IR_MOD_NEG);
         i->op = OP_ADD;
         i->setSrc(s, i->getSrc(t));
         i->src(s).mod = i->src(t).mod;
      } else
      if (!isFloatType(i->sType) && !imm0.isNegative() && imm0.isPow2()) {
         i->op = OP_SHL;
         imm0.applyLog2();
         i->setSrc(0, i->getSrc(t));
         i->src(0).mod = i->src(t).mod;
         i->setSrc(1, new_ImmediateValue(prog, imm0.reg.data.u32));
         i->src(1).mod = 0;
      }
      break;
   case OP_ADD:
      if (imm0.isInteger(0)) {
         if (s == 0) {
            i->setSrc(0, i->getSrc(1));
            i->src(0).mod = i->src(1).mod;
         }
         i->setSrc(1, NULL);
         i->op = i->src(0).mod.getOp();
         if (i->op != OP_CVT)
            i->src(0).mod = Modifier(0);
      }
      break;

   case OP_DIV:
      if (s != 1 || (i->dType != TYPE_S32 && i->dType != TYPE_U32))
         break;
      bld.setPosition(i, false);
      if (imm0.reg.data.u32 == 0) {
         break;
      } else
      if (imm0.reg.data.u32 == 1) {
         i->op = OP_MOV;
         i->setSrc(1, NULL);
      } else
      if (i->dType == TYPE_U32 && imm0.isPow2()) {
         i->op = OP_SHR;
         i->setSrc(1, bld.mkImm(util_logbase2(imm0.reg.data.u32)));
      } else
      if (i->dType == TYPE_U32) {
         Instruction *mul;
         Value *tA, *tB;
         const uint32_t d = imm0.reg.data.u32;
         uint32_t m;
         int r, s;
         uint32_t l = util_logbase2(d);
         if (((uint32_t)1 << l) < d)
            ++l;
         m = (((uint64_t)1 << 32) * (((uint64_t)1 << l) - d)) / d + 1;
         r = l ? 1 : 0;
         s = l ? (l - 1) : 0;

         tA = bld.getSSA();
         tB = bld.getSSA();
         mul = bld.mkOp2(OP_MUL, TYPE_U32, tA, i->getSrc(0),
                         bld.loadImm(NULL, m));
         mul->subOp = NV50_IR_SUBOP_MUL_HIGH;
         bld.mkOp2(OP_SUB, TYPE_U32, tB, i->getSrc(0), tA);
         tA = bld.getSSA();
         if (r)
            bld.mkOp2(OP_SHR, TYPE_U32, tA, tB, bld.mkImm(r));
         else
            tA = tB;
         tB = s ? bld.getSSA() : i->getDef(0);
         bld.mkOp2(OP_ADD, TYPE_U32, tB, mul->getDef(0), tA);
         if (s)
            bld.mkOp2(OP_SHR, TYPE_U32, i->getDef(0), tB, bld.mkImm(s));

         delete_Instruction(prog, i);
      } else
      if (imm0.reg.data.s32 == -1) {
         i->op = OP_NEG;
         i->setSrc(1, NULL);
      } else {
         LValue *tA, *tB;
         LValue *tD;
         const int32_t d = imm0.reg.data.s32;
         int32_t m;
         int32_t l = util_logbase2(static_cast<unsigned>(abs(d)));
         if ((1 << l) < abs(d))
            ++l;
         if (!l)
            l = 1;
         m = ((uint64_t)1 << (32 + l - 1)) / abs(d) + 1 - ((uint64_t)1 << 32);

         tA = bld.getSSA();
         tB = bld.getSSA();
         bld.mkOp3(OP_MAD, TYPE_S32, tA, i->getSrc(0), bld.loadImm(NULL, m),
                   i->getSrc(0))->subOp = NV50_IR_SUBOP_MUL_HIGH;
         if (l > 1)
            bld.mkOp2(OP_SHR, TYPE_S32, tB, tA, bld.mkImm(l - 1));
         else
            tB = tA;
         tA = bld.getSSA();
         bld.mkCmp(OP_SET, CC_LT, TYPE_S32, tA, i->getSrc(0), bld.mkImm(0));
         tD = (d < 0) ? bld.getSSA() : i->getDef(0)->asLValue();
         bld.mkOp2(OP_SUB, TYPE_U32, tD, tB, tA);
         if (d < 0)
            bld.mkOp1(OP_NEG, TYPE_S32, i->getDef(0), tB);

         delete_Instruction(prog, i);
      }
      break;

   case OP_MOD:
      if (i->sType == TYPE_U32 && imm0.isPow2()) {
         bld.setPosition(i, false);
         i->op = OP_AND;
         i->setSrc(1, bld.loadImm(NULL, imm0.reg.data.u32 - 1));
      }
      break;

   case OP_SET: // TODO: SET_AND,OR,XOR
   {
      CmpInstruction *si = findOriginForTestWithZero(i->getSrc(t));
      CondCode cc, ccZ;
      if (i->src(t).mod != Modifier(0))
         return;
      if (imm0.reg.data.u32 != 0 || !si || si->op != OP_SET)
         return;
      cc = si->setCond;
      ccZ = (CondCode)((unsigned int)i->asCmp()->setCond & ~CC_U);
      if (s == 0)
         ccZ = reverseCondCode(ccZ);
      switch (ccZ) {
      case CC_LT: cc = CC_FL; break;
      case CC_GE: cc = CC_TR; break;
      case CC_EQ: cc = inverseCondCode(cc); break;
      case CC_LE: cc = inverseCondCode(cc); break;
      case CC_GT: break;
      case CC_NE: break;
      default:
         return;
      }
      i->asCmp()->setCond = cc;
      i->setSrc(0, si->src(0));
      i->setSrc(1, si->src(1));
      i->sType = si->sType;
   }
      break;

   case OP_SHL:
   {
      if (s != 1 || i->src(0).mod != Modifier(0))
         break;
      // try to concatenate shifts
      Instruction *si = i->getSrc(0)->getInsn();
      if (!si || si->op != OP_SHL)
         break;
      ImmediateValue imm1;
      if (si->src(1).getImmediate(imm1)) {
         bld.setPosition(i, false);
         i->setSrc(0, si->getSrc(0));
         i->setSrc(1, bld.loadImm(NULL, imm0.reg.data.u32 + imm1.reg.data.u32));
      }
   }
      break;

   case OP_ABS:
   case OP_NEG:
   case OP_LG2:
   case OP_RCP:
   case OP_SQRT:
   case OP_RSQ:
   case OP_PRESIN:
   case OP_SIN:
   case OP_COS:
   case OP_PREEX2:
   case OP_EX2:
      unary(i, imm0);
      break;
   default:
      return;
   }
   if (i->op != op)
      foldCount++;
}

// =============================================================================

// Merge modifier operations (ABS, NEG, NOT) into ValueRefs where allowed.
class ModifierFolding : public Pass
{
private:
   virtual bool visit(BasicBlock *);
};

bool
ModifierFolding::visit(BasicBlock *bb)
{
   const Target *target = prog->getTarget();

   Instruction *i, *next, *mi;
   Modifier mod;

   for (i = bb->getEntry(); i; i = next) {
      next = i->next;

      if (0 && i->op == OP_SUB) {
         // turn "sub" into "add neg" (do we really want this ?)
         i->op = OP_ADD;
         i->src(0).mod = i->src(0).mod ^ Modifier(NV50_IR_MOD_NEG);
      }

      for (int s = 0; s < 3 && i->srcExists(s); ++s) {
         mi = i->getSrc(s)->getInsn();
         if (!mi ||
             mi->predSrc >= 0 || mi->getDef(0)->refCount() > 8)
            continue;
         if (i->sType == TYPE_U32 && mi->dType == TYPE_S32) {
            if ((i->op != OP_ADD &&
                 i->op != OP_MUL) ||
                (mi->op != OP_ABS &&
                 mi->op != OP_NEG))
               continue;
         } else
         if (i->sType != mi->dType) {
            continue;
         }
         if ((mod = Modifier(mi->op)) == Modifier(0))
            continue;
         mod *= mi->src(0).mod;

         if ((i->op == OP_ABS) || i->src(s).mod.abs()) {
            // abs neg [abs] = abs
            mod = mod & Modifier(~(NV50_IR_MOD_NEG | NV50_IR_MOD_ABS));
         } else
         if ((i->op == OP_NEG) && mod.neg()) {
            assert(s == 0);
            // neg as both opcode and modifier on same insn is prohibited
            // neg neg abs = abs, neg neg = identity
            mod = mod & Modifier(~NV50_IR_MOD_NEG);
            i->op = mod.getOp();
            mod = mod & Modifier(~NV50_IR_MOD_ABS);
            if (mod == Modifier(0))
               i->op = OP_MOV;
         }

         if (target->isModSupported(i, s, mod)) {
            i->setSrc(s, mi->getSrc(0));
            i->src(s).mod *= mod;
         }
      }

      if (i->op == OP_SAT) {
         mi = i->getSrc(0)->getInsn();
         if (mi &&
             mi->getDef(0)->refCount() <= 1 && target->isSatSupported(mi)) {
            mi->saturate = 1;
            mi->setDef(0, i->getDef(0));
            delete_Instruction(prog, i);
         }
      }
   }

   return true;
}

// =============================================================================

// MUL + ADD -> MAD/FMA
// MIN/MAX(a, a) -> a, etc.
// SLCT(a, b, const) -> cc(const) ? a : b
// RCP(RCP(a)) -> a
// MUL(MUL(a, b), const) -> MUL_Xconst(a, b)
class AlgebraicOpt : public Pass
{
private:
   virtual bool visit(BasicBlock *);

   void handleABS(Instruction *);
   bool handleADD(Instruction *);
   bool tryADDToMADOrSAD(Instruction *, operation toOp);
   void handleMINMAX(Instruction *);
   void handleRCP(Instruction *);
   void handleSLCT(Instruction *);
   void handleLOGOP(Instruction *);
   void handleCVT(Instruction *);

   BuildUtil bld;
};

void
AlgebraicOpt::handleABS(Instruction *abs)
{
   Instruction *sub = abs->getSrc(0)->getInsn();
   DataType ty;
   if (!sub ||
       !prog->getTarget()->isOpSupported(OP_SAD, abs->dType))
      return;
   // expect not to have mods yet, if we do, bail
   if (sub->src(0).mod || sub->src(1).mod)
      return;
   // hidden conversion ?
   ty = intTypeToSigned(sub->dType);
   if (abs->dType != abs->sType || ty != abs->sType)
      return;

   if ((sub->op != OP_ADD && sub->op != OP_SUB) ||
       sub->src(0).getFile() != FILE_GPR || sub->src(0).mod ||
       sub->src(1).getFile() != FILE_GPR || sub->src(1).mod)
         return;

   Value *src0 = sub->getSrc(0);
   Value *src1 = sub->getSrc(1);

   if (sub->op == OP_ADD) {
      Instruction *neg = sub->getSrc(1)->getInsn();
      if (neg && neg->op != OP_NEG) {
         neg = sub->getSrc(0)->getInsn();
         src0 = sub->getSrc(1);
      }
      if (!neg || neg->op != OP_NEG ||
          neg->dType != neg->sType || neg->sType != ty)
         return;
      src1 = neg->getSrc(0);
   }

   // found ABS(SUB))
   abs->moveSources(1, 2); // move sources >=1 up by 2
   abs->op = OP_SAD;
   abs->setType(sub->dType);
   abs->setSrc(0, src0);
   abs->setSrc(1, src1);
   bld.setPosition(abs, false);
   abs->setSrc(2, bld.loadImm(bld.getSSA(typeSizeof(ty)), 0));
}

bool
AlgebraicOpt::handleADD(Instruction *add)
{
   Value *src0 = add->getSrc(0);
   Value *src1 = add->getSrc(1);

   if (src0->reg.file != FILE_GPR || src1->reg.file != FILE_GPR)
      return false;

   bool changed = false;
   if (!changed && prog->getTarget()->isOpSupported(OP_MAD, add->dType))
      changed = tryADDToMADOrSAD(add, OP_MAD);
   if (!changed && prog->getTarget()->isOpSupported(OP_SAD, add->dType))
      changed = tryADDToMADOrSAD(add, OP_SAD);
   return changed;
}

// ADD(SAD(a,b,0), c) -> SAD(a,b,c)
// ADD(MUL(a,b), c) -> MAD(a,b,c)
bool
AlgebraicOpt::tryADDToMADOrSAD(Instruction *add, operation toOp)
{
   Value *src0 = add->getSrc(0);
   Value *src1 = add->getSrc(1);
   Value *src;
   int s;
   const operation srcOp = toOp == OP_SAD ? OP_SAD : OP_MUL;
   const Modifier modBad = Modifier(~((toOp == OP_MAD) ? NV50_IR_MOD_NEG : 0));
   Modifier mod[4];

   if (src0->refCount() == 1 &&
       src0->getUniqueInsn() && src0->getUniqueInsn()->op == srcOp)
      s = 0;
   else
   if (src1->refCount() == 1 &&
       src1->getUniqueInsn() && src1->getUniqueInsn()->op == srcOp)
      s = 1;
   else
      return false;

   if ((src0->getUniqueInsn() && src0->getUniqueInsn()->bb != add->bb) ||
       (src1->getUniqueInsn() && src1->getUniqueInsn()->bb != add->bb))
      return false;

   src = add->getSrc(s);

   if (src->getInsn()->postFactor)
      return false;
   if (toOp == OP_SAD) {
      ImmediateValue imm;
      if (!src->getInsn()->src(2).getImmediate(imm))
         return false;
      if (!imm.isInteger(0))
         return false;
   }

   mod[0] = add->src(0).mod;
   mod[1] = add->src(1).mod;
   mod[2] = src->getUniqueInsn()->src(0).mod;
   mod[3] = src->getUniqueInsn()->src(1).mod;

   if (((mod[0] | mod[1]) | (mod[2] | mod[3])) & modBad)
      return false;

   add->op = toOp;
   add->subOp = src->getInsn()->subOp; // potentially mul-high

   add->setSrc(2, add->src(s ? 0 : 1));

   add->setSrc(0, src->getInsn()->getSrc(0));
   add->src(0).mod = mod[2] ^ mod[s];
   add->setSrc(1, src->getInsn()->getSrc(1));
   add->src(1).mod = mod[3];

   return true;
}

void
AlgebraicOpt::handleMINMAX(Instruction *minmax)
{
   Value *src0 = minmax->getSrc(0);
   Value *src1 = minmax->getSrc(1);

   if (src0 != src1 || src0->reg.file != FILE_GPR)
      return;
   if (minmax->src(0).mod == minmax->src(1).mod) {
      if (minmax->def(0).mayReplace(minmax->src(0))) {
         minmax->def(0).replace(minmax->src(0), false);
         minmax->bb->remove(minmax);
      } else {
         minmax->op = OP_CVT;
         minmax->setSrc(1, NULL);
      }
   } else {
      // TODO:
      // min(x, -x) = -abs(x)
      // min(x, -abs(x)) = -abs(x)
      // min(x, abs(x)) = x
      // max(x, -abs(x)) = x
      // max(x, abs(x)) = abs(x)
      // max(x, -x) = abs(x)
   }
}

void
AlgebraicOpt::handleRCP(Instruction *rcp)
{
   Instruction *si = rcp->getSrc(0)->getUniqueInsn();

   if (si && si->op == OP_RCP) {
      Modifier mod = rcp->src(0).mod * si->src(0).mod;
      rcp->op = mod.getOp();
      rcp->setSrc(0, si->getSrc(0));
   }
}

void
AlgebraicOpt::handleSLCT(Instruction *slct)
{
   if (slct->getSrc(2)->reg.file == FILE_IMMEDIATE) {
      if (slct->getSrc(2)->asImm()->compare(slct->asCmp()->setCond, 0.0f))
         slct->setSrc(0, slct->getSrc(1));
   } else
   if (slct->getSrc(0) != slct->getSrc(1)) {
      return;
   }
   slct->op = OP_MOV;
   slct->setSrc(1, NULL);
   slct->setSrc(2, NULL);
}

void
AlgebraicOpt::handleLOGOP(Instruction *logop)
{
   Value *src0 = logop->getSrc(0);
   Value *src1 = logop->getSrc(1);

   if (src0->reg.file != FILE_GPR || src1->reg.file != FILE_GPR)
      return;

   if (src0 == src1) {
      if ((logop->op == OP_AND || logop->op == OP_OR) &&
          logop->def(0).mayReplace(logop->src(0))) {
         logop->def(0).replace(logop->src(0), false);
         delete_Instruction(prog, logop);
      }
   } else {
      // try AND(SET, SET) -> SET_AND(SET)
      Instruction *set0 = src0->getInsn();
      Instruction *set1 = src1->getInsn();

      if (!set0 || set0->fixed || !set1 || set1->fixed)
         return;
      if (set1->op != OP_SET) {
         Instruction *xchg = set0;
         set0 = set1;
         set1 = xchg;
         if (set1->op != OP_SET)
            return;
      }
      operation redOp = (logop->op == OP_AND ? OP_SET_AND :
                         logop->op == OP_XOR ? OP_SET_XOR : OP_SET_OR);
      if (!prog->getTarget()->isOpSupported(redOp, set1->sType))
         return;
      if (set0->op != OP_SET &&
          set0->op != OP_SET_AND &&
          set0->op != OP_SET_OR &&
          set0->op != OP_SET_XOR)
         return;
      if (set0->getDef(0)->refCount() > 1 &&
          set1->getDef(0)->refCount() > 1)
         return;
      if (set0->getPredicate() || set1->getPredicate())
         return;
      // check that they don't source each other
      for (int s = 0; s < 2; ++s)
         if (set0->getSrc(s) == set1->getDef(0) ||
             set1->getSrc(s) == set0->getDef(0))
            return;

      set0 = cloneForward(func, set0);
      set1 = cloneShallow(func, set1);
      logop->bb->insertAfter(logop, set1);
      logop->bb->insertAfter(logop, set0);

      set0->dType = TYPE_U8;
      set0->getDef(0)->reg.file = FILE_PREDICATE;
      set0->getDef(0)->reg.size = 1;
      set1->setSrc(2, set0->getDef(0));
      set1->op = redOp;
      set1->setDef(0, logop->getDef(0));
      delete_Instruction(prog, logop);
   }
}

// F2I(NEG(SET with result 1.0f/0.0f)) -> SET with result -1/0
// nv50:
//  F2I(NEG(I2F(ABS(SET))))
void
AlgebraicOpt::handleCVT(Instruction *cvt)
{
   if (cvt->sType != TYPE_F32 ||
       cvt->dType != TYPE_S32 || cvt->src(0).mod != Modifier(0))
      return;
   Instruction *insn = cvt->getSrc(0)->getInsn();
   if (!insn || insn->op != OP_NEG || insn->dType != TYPE_F32)
      return;
   if (insn->src(0).mod != Modifier(0))
      return;
   insn = insn->getSrc(0)->getInsn();

   // check for nv50 SET(-1,0) -> SET(1.0f/0.0f) chain and nvc0's f32 SET
   if (insn && insn->op == OP_CVT &&
       insn->dType == TYPE_F32 &&
       insn->sType == TYPE_S32) {
      insn = insn->getSrc(0)->getInsn();
      if (!insn || insn->op != OP_ABS || insn->sType != TYPE_S32 ||
          insn->src(0).mod)
         return;
      insn = insn->getSrc(0)->getInsn();
      if (!insn || insn->op != OP_SET || insn->dType != TYPE_U32)
         return;
   } else
   if (!insn || insn->op != OP_SET || insn->dType != TYPE_F32) {
      return;
   }

   Instruction *bset = cloneShallow(func, insn);
   bset->dType = TYPE_U32;
   bset->setDef(0, cvt->getDef(0));
   cvt->bb->insertAfter(cvt, bset);
   delete_Instruction(prog, cvt);
}

bool
AlgebraicOpt::visit(BasicBlock *bb)
{
   Instruction *next;
   for (Instruction *i = bb->getEntry(); i; i = next) {
      next = i->next;
      switch (i->op) {
      case OP_ABS:
         handleABS(i);
         break;
      case OP_ADD:
         handleADD(i);
         break;
      case OP_RCP:
         handleRCP(i);
         break;
      case OP_MIN:
      case OP_MAX:
         handleMINMAX(i);
         break;
      case OP_SLCT:
         handleSLCT(i);
         break;
      case OP_AND:
      case OP_OR:
      case OP_XOR:
         handleLOGOP(i);
         break;
      case OP_CVT:
         handleCVT(i);
         break;
      default:
         break;
      }
   }

   return true;
}

// =============================================================================

static inline void
updateLdStOffset(Instruction *ldst, int32_t offset, Function *fn)
{
   if (offset != ldst->getSrc(0)->reg.data.offset) {
      if (ldst->getSrc(0)->refCount() > 1)
         ldst->setSrc(0, cloneShallow(fn, ldst->getSrc(0)));
      ldst->getSrc(0)->reg.data.offset = offset;
   }
}

// Combine loads and stores, forward stores to loads where possible.
class MemoryOpt : public Pass
{
private:
   class Record
   {
   public:
      Record *next;
      Instruction *insn;
      const Value *rel[2];
      const Value *base;
      int32_t offset;
      int8_t fileIndex;
      uint8_t size;
      bool locked;
      Record *prev;

      bool overlaps(const Instruction *ldst) const;

      inline void link(Record **);
      inline void unlink(Record **);
      inline void set(const Instruction *ldst);
   };

public:
   MemoryOpt();

   Record *loads[DATA_FILE_COUNT];
   Record *stores[DATA_FILE_COUNT];

   MemoryPool recordPool;

private:
   virtual bool visit(BasicBlock *);
   bool runOpt(BasicBlock *);

   Record **getList(const Instruction *);

   Record *findRecord(const Instruction *, bool load, bool& isAdjacent) const;

   // merge @insn into load/store instruction from @rec
   bool combineLd(Record *rec, Instruction *ld);
   bool combineSt(Record *rec, Instruction *st);

   bool replaceLdFromLd(Instruction *ld, Record *ldRec);
   bool replaceLdFromSt(Instruction *ld, Record *stRec);
   bool replaceStFromSt(Instruction *restrict st, Record *stRec);

   void addRecord(Instruction *ldst);
   void purgeRecords(Instruction *const st, DataFile);
   void lockStores(Instruction *const ld);
   void reset();

private:
   Record *prevRecord;
};

MemoryOpt::MemoryOpt() : recordPool(sizeof(MemoryOpt::Record), 6)
{
   for (int i = 0; i < DATA_FILE_COUNT; ++i) {
      loads[i] = NULL;
      stores[i] = NULL;
   }
   prevRecord = NULL;
}

void
MemoryOpt::reset()
{
   for (unsigned int i = 0; i < DATA_FILE_COUNT; ++i) {
      Record *it, *next;
      for (it = loads[i]; it; it = next) {
         next = it->next;
         recordPool.release(it);
      }
      loads[i] = NULL;
      for (it = stores[i]; it; it = next) {
         next = it->next;
         recordPool.release(it);
      }
      stores[i] = NULL;
   }
}

bool
MemoryOpt::combineLd(Record *rec, Instruction *ld)
{
   int32_t offRc = rec->offset;
   int32_t offLd = ld->getSrc(0)->reg.data.offset;
   int sizeRc = rec->size;
   int sizeLd = typeSizeof(ld->dType);
   int size = sizeRc + sizeLd;
   int d, j;

   if (!prog->getTarget()->
       isAccessSupported(ld->getSrc(0)->reg.file, typeOfSize(size)))
      return false;
   // no unaligned loads
   if (((size == 0x8) && (MIN2(offLd, offRc) & 0x7)) ||
       ((size == 0xc) && (MIN2(offLd, offRc) & 0xf)))
      return false;

   assert(sizeRc + sizeLd <= 16 && offRc != offLd);

   for (j = 0; sizeRc; sizeRc -= rec->insn->getDef(j)->reg.size, ++j);

   if (offLd < offRc) {
      int sz;
      for (sz = 0, d = 0; sz < sizeLd; sz += ld->getDef(d)->reg.size, ++d);
      // d: nr of definitions in ld
      // j: nr of definitions in rec->insn, move:
      for (d = d + j - 1; j > 0; --j, --d)
         rec->insn->setDef(d, rec->insn->getDef(j - 1));

      if (rec->insn->getSrc(0)->refCount() > 1)
         rec->insn->setSrc(0, cloneShallow(func, rec->insn->getSrc(0)));
      rec->offset = rec->insn->getSrc(0)->reg.data.offset = offLd;

      d = 0;
   } else {
      d = j;
   }
   // move definitions of @ld to @rec->insn
   for (j = 0; sizeLd; ++j, ++d) {
      sizeLd -= ld->getDef(j)->reg.size;
      rec->insn->setDef(d, ld->getDef(j));
   }

   rec->size = size;
   rec->insn->getSrc(0)->reg.size = size;
   rec->insn->setType(typeOfSize(size));

   delete_Instruction(prog, ld);

   return true;
}

bool
MemoryOpt::combineSt(Record *rec, Instruction *st)
{
   int32_t offRc = rec->offset;
   int32_t offSt = st->getSrc(0)->reg.data.offset;
   int sizeRc = rec->size;
   int sizeSt = typeSizeof(st->dType);
   int s = sizeSt / 4;
   int size = sizeRc + sizeSt;
   int j, k;
   Value *src[4]; // no modifiers in ValueRef allowed for st
   Value *extra[3];

   if (!prog->getTarget()->
       isAccessSupported(st->getSrc(0)->reg.file, typeOfSize(size)))
      return false;
   if (size == 8 && MIN2(offRc, offSt) & 0x7)
      return false;

   st->takeExtraSources(0, extra); // save predicate and indirect address

   if (offRc < offSt) {
      // save values from @st
      for (s = 0; sizeSt; ++s) {
         sizeSt -= st->getSrc(s + 1)->reg.size;
         src[s] = st->getSrc(s + 1);
      }
      // set record's values as low sources of @st
      for (j = 1; sizeRc; ++j) {
         sizeRc -= rec->insn->getSrc(j)->reg.size;
         st->setSrc(j, rec->insn->getSrc(j));
      }
      // set saved values as high sources of @st
      for (k = j, j = 0; j < s; ++j)
         st->setSrc(k++, src[j]);

      updateLdStOffset(st, offRc, func);
   } else {
      for (j = 1; sizeSt; ++j)
         sizeSt -= st->getSrc(j)->reg.size;
      for (s = 1; sizeRc; ++j, ++s) {
         sizeRc -= rec->insn->getSrc(s)->reg.size;
         st->setSrc(j, rec->insn->getSrc(s));
      }
      rec->offset = offSt;
   }
   st->putExtraSources(0, extra); // restore pointer and predicate

   delete_Instruction(prog, rec->insn);
   rec->insn = st;
   rec->size = size;
   rec->insn->getSrc(0)->reg.size = size;
   rec->insn->setType(typeOfSize(size));
   return true;
}

void
MemoryOpt::Record::set(const Instruction *ldst)
{
   const Symbol *mem = ldst->getSrc(0)->asSym();
   fileIndex = mem->reg.fileIndex;
   rel[0] = ldst->getIndirect(0, 0);
   rel[1] = ldst->getIndirect(0, 1);
   offset = mem->reg.data.offset;
   base = mem->getBase();
   size = typeSizeof(ldst->sType);
}

void
MemoryOpt::Record::link(Record **list)
{
   next = *list;
   if (next)
      next->prev = this;
   prev = NULL;
   *list = this;
}

void
MemoryOpt::Record::unlink(Record **list)
{
   if (next)
      next->prev = prev;
   if (prev)
      prev->next = next;
   else
      *list = next;
}

MemoryOpt::Record **
MemoryOpt::getList(const Instruction *insn)
{
   if (insn->op == OP_LOAD || insn->op == OP_VFETCH)
      return &loads[insn->src(0).getFile()];
   return &stores[insn->src(0).getFile()];
}

void
MemoryOpt::addRecord(Instruction *i)
{
   Record **list = getList(i);
   Record *it = reinterpret_cast<Record *>(recordPool.allocate());

   it->link(list);
   it->set(i);
   it->insn = i;
   it->locked = false;
}

MemoryOpt::Record *
MemoryOpt::findRecord(const Instruction *insn, bool load, bool& isAdj) const
{
   const Symbol *sym = insn->getSrc(0)->asSym();
   const int size = typeSizeof(insn->sType);
   Record *rec = NULL;
   Record *it = load ? loads[sym->reg.file] : stores[sym->reg.file];

   for (; it; it = it->next) {
      if (it->locked && insn->op != OP_LOAD)
         continue;
      if ((it->offset >> 4) != (sym->reg.data.offset >> 4) ||
          it->rel[0] != insn->getIndirect(0, 0) ||
          it->fileIndex != sym->reg.fileIndex ||
          it->rel[1] != insn->getIndirect(0, 1))
         continue;

      if (it->offset < sym->reg.data.offset) {
         if (it->offset + it->size >= sym->reg.data.offset) {
            isAdj = (it->offset + it->size == sym->reg.data.offset);
            if (!isAdj)
               return it;
            if (!(it->offset & 0x7))
               rec = it;
         }
      } else {
         isAdj = it->offset != sym->reg.data.offset;
         if (size <= it->size && !isAdj)
            return it;
         else
         if (!(sym->reg.data.offset & 0x7))
            if (it->offset - size <= sym->reg.data.offset)
               rec = it;
      }
   }
   return rec;
}

bool
MemoryOpt::replaceLdFromSt(Instruction *ld, Record *rec)
{
   Instruction *st = rec->insn;
   int32_t offSt = rec->offset;
   int32_t offLd = ld->getSrc(0)->reg.data.offset;
   int d, s;

   for (s = 1; offSt != offLd && st->srcExists(s); ++s)
      offSt += st->getSrc(s)->reg.size;
   if (offSt != offLd)
      return false;

   for (d = 0; ld->defExists(d) && st->srcExists(s); ++d, ++s) {
      if (ld->getDef(d)->reg.size != st->getSrc(s)->reg.size)
         return false;
      if (st->getSrc(s)->reg.file != FILE_GPR)
         return false;
      ld->def(d).replace(st->src(s), false);
   }
   ld->bb->remove(ld);
   return true;
}

bool
MemoryOpt::replaceLdFromLd(Instruction *ldE, Record *rec)
{
   Instruction *ldR = rec->insn;
   int32_t offR = rec->offset;
   int32_t offE = ldE->getSrc(0)->reg.data.offset;
   int dR, dE;

   assert(offR <= offE);
   for (dR = 0; offR < offE && ldR->defExists(dR); ++dR)
      offR += ldR->getDef(dR)->reg.size;
   if (offR != offE)
      return false;

   for (dE = 0; ldE->defExists(dE) && ldR->defExists(dR); ++dE, ++dR) {
      if (ldE->getDef(dE)->reg.size != ldR->getDef(dR)->reg.size)
         return false;
      ldE->def(dE).replace(ldR->getDef(dR), false);
   }

   delete_Instruction(prog, ldE);
   return true;
}

bool
MemoryOpt::replaceStFromSt(Instruction *restrict st, Record *rec)
{
   const Instruction *const ri = rec->insn;
   Value *extra[3];

   int32_t offS = st->getSrc(0)->reg.data.offset;
   int32_t offR = rec->offset;
   int32_t endS = offS + typeSizeof(st->dType);
   int32_t endR = offR + typeSizeof(ri->dType);

   rec->size = MAX2(endS, endR) - MIN2(offS, offR);

   st->takeExtraSources(0, extra);

   if (offR < offS) {
      Value *vals[10];
      int s, n;
      int k = 0;
      // get non-replaced sources of ri
      for (s = 1; offR < offS; offR += ri->getSrc(s)->reg.size, ++s)
         vals[k++] = ri->getSrc(s);
      n = s;
      // get replaced sources of st
      for (s = 1; st->srcExists(s); offS += st->getSrc(s)->reg.size, ++s)
         vals[k++] = st->getSrc(s);
      // skip replaced sources of ri
      for (s = n; offR < endS; offR += ri->getSrc(s)->reg.size, ++s);
      // get non-replaced sources after values covered by st
      for (; offR < endR; offR += ri->getSrc(s)->reg.size, ++s)
         vals[k++] = ri->getSrc(s);
      assert((unsigned int)k <= Elements(vals));
      for (s = 0; s < k; ++s)
         st->setSrc(s + 1, vals[s]);
      st->setSrc(0, ri->getSrc(0));
   } else
   if (endR > endS) {
      int j, s;
      for (j = 1; offR < endS; offR += ri->getSrc(j++)->reg.size);
      for (s = 1; offS < endS; offS += st->getSrc(s++)->reg.size);
      for (; offR < endR; offR += ri->getSrc(j++)->reg.size)
         st->setSrc(s++, ri->getSrc(j));
   }
   st->putExtraSources(0, extra);

   delete_Instruction(prog, rec->insn);

   rec->insn = st;
   rec->offset = st->getSrc(0)->reg.data.offset;

   st->setType(typeOfSize(rec->size));

   return true;
}

bool
MemoryOpt::Record::overlaps(const Instruction *ldst) const
{
   Record that;
   that.set(ldst);

   if (this->fileIndex != that.fileIndex)
      return false;

   if (this->rel[0] || that.rel[0])
      return this->base == that.base;
   return
      (this->offset < that.offset + that.size) &&
      (this->offset + this->size > that.offset);
}

// We must not eliminate stores that affect the result of @ld if
// we find later stores to the same location, and we may no longer
// merge them with later stores.
// The stored value can, however, still be used to determine the value
// returned by future loads.
void
MemoryOpt::lockStores(Instruction *const ld)
{
   for (Record *r = stores[ld->src(0).getFile()]; r; r = r->next)
      if (!r->locked && r->overlaps(ld))
         r->locked = true;
}

// Prior loads from the location of @st are no longer valid.
// Stores to the location of @st may no longer be used to derive
// the value at it nor be coalesced into later stores.
void
MemoryOpt::purgeRecords(Instruction *const st, DataFile f)
{
   if (st)
      f = st->src(0).getFile();

   for (Record *r = loads[f]; r; r = r->next)
      if (!st || r->overlaps(st))
         r->unlink(&loads[f]);

   for (Record *r = stores[f]; r; r = r->next)
      if (!st || r->overlaps(st))
         r->unlink(&stores[f]);
}

bool
MemoryOpt::visit(BasicBlock *bb)
{
   bool ret = runOpt(bb);
   // Run again, one pass won't combine 4 32 bit ld/st to a single 128 bit ld/st
   // where 96 bit memory operations are forbidden.
   if (ret)
      ret = runOpt(bb);
   return ret;
}

bool
MemoryOpt::runOpt(BasicBlock *bb)
{
   Instruction *ldst, *next;
   Record *rec;
   bool isAdjacent = true;

   for (ldst = bb->getEntry(); ldst; ldst = next) {
      bool keep = true;
      bool isLoad = true;
      next = ldst->next;

      if (ldst->op == OP_LOAD || ldst->op == OP_VFETCH) {
         if (ldst->isDead()) {
            // might have been produced by earlier optimization
            delete_Instruction(prog, ldst);
            continue;
         }
      } else
      if (ldst->op == OP_STORE || ldst->op == OP_EXPORT) {
         isLoad = false;
      } else {
         // TODO: maybe have all fixed ops act as barrier ?
         if (ldst->op == OP_CALL) {
            purgeRecords(NULL, FILE_MEMORY_LOCAL);
            purgeRecords(NULL, FILE_MEMORY_GLOBAL);
            purgeRecords(NULL, FILE_MEMORY_SHARED);
            purgeRecords(NULL, FILE_SHADER_OUTPUT);
         } else
         if (ldst->op == OP_EMIT || ldst->op == OP_RESTART) {
            purgeRecords(NULL, FILE_SHADER_OUTPUT);
         }
         continue;
      }
      if (ldst->getPredicate()) // TODO: handle predicated ld/st
         continue;

      if (isLoad) {
         DataFile file = ldst->src(0).getFile();

         // if ld l[]/g[] look for previous store to eliminate the reload
         if (file == FILE_MEMORY_GLOBAL || file == FILE_MEMORY_LOCAL) {
            // TODO: shared memory ?
            rec = findRecord(ldst, false, isAdjacent);
            if (rec && !isAdjacent)
               keep = !replaceLdFromSt(ldst, rec);
         }

         // or look for ld from the same location and replace this one
         rec = keep ? findRecord(ldst, true, isAdjacent) : NULL;
         if (rec) {
            if (!isAdjacent)
               keep = !replaceLdFromLd(ldst, rec);
            else
               // or combine a previous load with this one
               keep = !combineLd(rec, ldst);
         }
         if (keep)
            lockStores(ldst);
      } else {
         rec = findRecord(ldst, false, isAdjacent);
         if (rec) {
            if (!isAdjacent)
               keep = !replaceStFromSt(ldst, rec);
            else
               keep = !combineSt(rec, ldst);
         }
         if (keep)
            purgeRecords(ldst, DATA_FILE_COUNT);
      }
      if (keep)
         addRecord(ldst);
   }
   reset();

   return true;
}

// =============================================================================

// Turn control flow into predicated instructions (after register allocation !).
// TODO:
// Could move this to before register allocation on NVC0 and also handle nested
// constructs.
class FlatteningPass : public Pass
{
private:
   virtual bool visit(BasicBlock *);

   bool tryPredicateConditional(BasicBlock *);
   void predicateInstructions(BasicBlock *, Value *pred, CondCode cc);
   void tryPropagateBranch(BasicBlock *);
   inline bool isConstantCondition(Value *pred);
   inline bool mayPredicate(const Instruction *, const Value *pred) const;
   inline void removeFlow(Instruction *);
};

bool
FlatteningPass::isConstantCondition(Value *pred)
{
   Instruction *insn = pred->getUniqueInsn();
   assert(insn);
   if (insn->op != OP_SET || insn->srcExists(2))
      return false;

   for (int s = 0; s < 2 && insn->srcExists(s); ++s) {
      Instruction *ld = insn->getSrc(s)->getUniqueInsn();
      DataFile file;
      if (ld) {
         if (ld->op != OP_MOV && ld->op != OP_LOAD)
            return false;
         if (ld->src(0).isIndirect(0))
            return false;
         file = ld->src(0).getFile();
      } else {
         file = insn->src(s).getFile();
         // catch $r63 on NVC0
         if (file == FILE_GPR && insn->getSrc(s)->reg.data.id > prog->maxGPR)
            file = FILE_IMMEDIATE;
      }
      if (file != FILE_IMMEDIATE && file != FILE_MEMORY_CONST)
         return false;
   }
   return true;
}

void
FlatteningPass::removeFlow(Instruction *insn)
{
   FlowInstruction *term = insn ? insn->asFlow() : NULL;
   if (!term)
      return;
   Graph::Edge::Type ty = term->bb->cfg.outgoing().getType();

   if (term->op == OP_BRA) {
      // TODO: this might get more difficult when we get arbitrary BRAs
      if (ty == Graph::Edge::CROSS || ty == Graph::Edge::BACK)
         return;
   } else
   if (term->op != OP_JOIN)
      return;

   Value *pred = term->getPredicate();

   delete_Instruction(prog, term);

   if (pred && pred->refCount() == 0) {
      Instruction *pSet = pred->getUniqueInsn();
      pred->join->reg.data.id = -1; // deallocate
      if (pSet->isDead())
         delete_Instruction(prog, pSet);
   }
}

void
FlatteningPass::predicateInstructions(BasicBlock *bb, Value *pred, CondCode cc)
{
   for (Instruction *i = bb->getEntry(); i; i = i->next) {
      if (i->isNop())
         continue;
      assert(!i->getPredicate());
      i->setPredicate(cc, pred);
   }
   removeFlow(bb->getExit());
}

bool
FlatteningPass::mayPredicate(const Instruction *insn, const Value *pred) const
{
   if (insn->isPseudo())
      return true;
   // TODO: calls where we don't know which registers are modified

   if (!prog->getTarget()->mayPredicate(insn, pred))
      return false;
   for (int d = 0; insn->defExists(d); ++d)
      if (insn->getDef(d)->equals(pred))
         return false;
   return true;
}

// If we conditionally skip over or to a branch instruction, replace it.
// NOTE: We do not update the CFG anymore here !
void
FlatteningPass::tryPropagateBranch(BasicBlock *bb)
{
   BasicBlock *bf = NULL;
   unsigned int i;

   if (bb->cfg.outgoingCount() != 2)
      return;
   if (!bb->getExit() || bb->getExit()->op != OP_BRA)
      return;
   Graph::EdgeIterator ei = bb->cfg.outgoing();

   for (i = 0; !ei.end(); ++i, ei.next()) {
      bf = BasicBlock::get(ei.getNode());
      if (bf->getInsnCount() == 1)
         break;
   }
   if (ei.end() || !bf->getExit())
      return;
   FlowInstruction *bra = bb->getExit()->asFlow();
   FlowInstruction *rep = bf->getExit()->asFlow();

   if (rep->getPredicate())
      return;
   if (rep->op != OP_BRA &&
       rep->op != OP_JOIN &&
       rep->op != OP_EXIT)
      return;

   bra->op = rep->op;
   bra->target.bb = rep->target.bb;
   if (i) // 2nd out block means branch not taken
      bra->cc = inverseCondCode(bra->cc);
   bf->remove(rep);
}

bool
FlatteningPass::visit(BasicBlock *bb)
{
   if (tryPredicateConditional(bb))
      return true;

   // try to attach join to previous instruction
   Instruction *insn = bb->getExit();
   if (insn && insn->op == OP_JOIN && !insn->getPredicate()) {
      insn = insn->prev;
      if (insn && !insn->getPredicate() &&
          !insn->asFlow() &&
          insn->op != OP_TEXBAR &&
          !isTextureOp(insn->op) && // probably just nve4
          insn->op != OP_LINTERP && // probably just nve4
          insn->op != OP_PINTERP && // probably just nve4
          ((insn->op != OP_LOAD && insn->op != OP_STORE) ||
           typeSizeof(insn->dType) <= 4) &&
          !insn->isNop()) {
         insn->join = 1;
         bb->remove(bb->getExit());
         return true;
      }
   }

   tryPropagateBranch(bb);

   return true;
}

bool
FlatteningPass::tryPredicateConditional(BasicBlock *bb)
{
   BasicBlock *bL = NULL, *bR = NULL;
   unsigned int nL = 0, nR = 0, limit = 12;
   Instruction *insn;
   unsigned int mask;

   mask = bb->initiatesSimpleConditional();
   if (!mask)
      return false;

   assert(bb->getExit());
   Value *pred = bb->getExit()->getPredicate();
   assert(pred);

   if (isConstantCondition(pred))
      limit = 4;

   Graph::EdgeIterator ei = bb->cfg.outgoing();

   if (mask & 1) {
      bL = BasicBlock::get(ei.getNode());
      for (insn = bL->getEntry(); insn; insn = insn->next, ++nL)
         if (!mayPredicate(insn, pred))
            return false;
      if (nL > limit)
         return false; // too long, do a real branch
   }
   ei.next();

   if (mask & 2) {
      bR = BasicBlock::get(ei.getNode());
      for (insn = bR->getEntry(); insn; insn = insn->next, ++nR)
         if (!mayPredicate(insn, pred))
            return false;
      if (nR > limit)
         return false; // too long, do a real branch
   }

   if (bL)
      predicateInstructions(bL, pred, bb->getExit()->cc);
   if (bR)
      predicateInstructions(bR, pred, inverseCondCode(bb->getExit()->cc));

   if (bb->joinAt) {
      bb->remove(bb->joinAt);
      bb->joinAt = NULL;
   }
   removeFlow(bb->getExit()); // delete the branch/join at the fork point

   // remove potential join operations at the end of the conditional
   if (prog->getTarget()->joinAnterior) {
      bb = BasicBlock::get((bL ? bL : bR)->cfg.outgoing().getNode());
      if (bb->getEntry() && bb->getEntry()->op == OP_JOIN)
         removeFlow(bb->getEntry());
   }

   return true;
}

// =============================================================================

// Common subexpression elimination. Stupid O^2 implementation.
class LocalCSE : public Pass
{
private:
   virtual bool visit(BasicBlock *);

   inline bool tryReplace(Instruction **, Instruction *);

   DLList ops[OP_LAST + 1];
};

class GlobalCSE : public Pass
{
private:
   virtual bool visit(BasicBlock *);
};

bool
Instruction::isActionEqual(const Instruction *that) const
{
   if (this->op != that->op ||
       this->dType != that->dType ||
       this->sType != that->sType)
      return false;
   if (this->cc != that->cc)
      return false;

   if (this->asTex()) {
      if (memcmp(&this->asTex()->tex,
                 &that->asTex()->tex,
                 sizeof(this->asTex()->tex)))
         return false;
   } else
   if (this->asCmp()) {
      if (this->asCmp()->setCond != that->asCmp()->setCond)
         return false;
   } else
   if (this->asFlow()) {
      return false;
   } else {
      if (this->atomic != that->atomic ||
          this->ipa != that->ipa ||
          this->lanes != that->lanes ||
          this->perPatch != that->perPatch)
         return false;
      if (this->postFactor != that->postFactor)
         return false;
   }

   if (this->subOp != that->subOp ||
       this->saturate != that->saturate ||
       this->rnd != that->rnd ||
       this->ftz != that->ftz ||
       this->dnz != that->dnz ||
       this->cache != that->cache)
      return false;

   return true;
}

bool
Instruction::isResultEqual(const Instruction *that) const
{
   unsigned int d, s;

   // NOTE: location of discard only affects tex with liveOnly and quadops
   if (!this->defExists(0) && this->op != OP_DISCARD)
      return false;

   if (!isActionEqual(that))
      return false;

   if (this->predSrc != that->predSrc)
      return false;

   for (d = 0; this->defExists(d); ++d) {
      if (!that->defExists(d) ||
          !this->getDef(d)->equals(that->getDef(d), false))
         return false;
   }
   if (that->defExists(d))
      return false;

   for (s = 0; this->srcExists(s); ++s) {
      if (!that->srcExists(s))
         return false;
      if (this->src(s).mod != that->src(s).mod)
         return false;
      if (!this->getSrc(s)->equals(that->getSrc(s), true))
         return false;
   }
   if (that->srcExists(s))
      return false;

   if (op == OP_LOAD || op == OP_VFETCH) {
      switch (src(0).getFile()) {
      case FILE_MEMORY_CONST:
      case FILE_SHADER_INPUT:
         return true;
      default:
         return false;
      }
   }

   return true;
}

// pull through common expressions from different in-blocks
bool
GlobalCSE::visit(BasicBlock *bb)
{
   Instruction *phi, *next, *ik;
   int s;

   // TODO: maybe do this with OP_UNION, too

   for (phi = bb->getPhi(); phi && phi->op == OP_PHI; phi = next) {
      next = phi->next;
      if (phi->getSrc(0)->refCount() > 1)
         continue;
      ik = phi->getSrc(0)->getInsn();
      if (!ik)
         continue; // probably a function input
      for (s = 1; phi->srcExists(s); ++s) {
         if (phi->getSrc(s)->refCount() > 1)
            break;
         if (!phi->getSrc(s)->getInsn() ||
             !phi->getSrc(s)->getInsn()->isResultEqual(ik))
            break;
      }
      if (!phi->srcExists(s)) {
         Instruction *entry = bb->getEntry();
         ik->bb->remove(ik);
         if (!entry || entry->op != OP_JOIN)
            bb->insertHead(ik);
         else
            bb->insertAfter(entry, ik);
         ik->setDef(0, phi->getDef(0));
         delete_Instruction(prog, phi);
      }
   }

   return true;
}

bool
LocalCSE::tryReplace(Instruction **ptr, Instruction *i)
{
   Instruction *old = *ptr;

   // TODO: maybe relax this later (causes trouble with OP_UNION)
   if (i->isPredicated())
      return false;

   if (!old->isResultEqual(i))
      return false;

   for (int d = 0; old->defExists(d); ++d)
      old->def(d).replace(i->getDef(d), false);
   delete_Instruction(prog, old);
   *ptr = NULL;
   return true;
}

bool
LocalCSE::visit(BasicBlock *bb)
{
   unsigned int replaced;

   do {
      Instruction *ir, *next;

      replaced = 0;

      // will need to know the order of instructions
      int serial = 0;
      for (ir = bb->getFirst(); ir; ir = ir->next)
         ir->serial = serial++;

      for (ir = bb->getEntry(); ir; ir = next) {
         int s;
         Value *src = NULL;

         next = ir->next;

         if (ir->fixed) {
            ops[ir->op].insert(ir);
            continue;
         }

         for (s = 0; ir->srcExists(s); ++s)
            if (ir->getSrc(s)->asLValue())
               if (!src || ir->getSrc(s)->refCount() < src->refCount())
                  src = ir->getSrc(s);

         if (src) {
            for (Value::UseIterator it = src->uses.begin();
                 it != src->uses.end(); ++it) {
               Instruction *ik = (*it)->getInsn();
               if (ik && ik->bb == ir->bb && ik->serial < ir->serial)
                  if (tryReplace(&ir, ik))
                     break;
            }
         } else {
            DLLIST_FOR_EACH(&ops[ir->op], iter)
            {
               Instruction *ik = reinterpret_cast<Instruction *>(iter.get());
               if (tryReplace(&ir, ik))
                  break;
            }
         }

         if (ir)
            ops[ir->op].insert(ir);
         else
            ++replaced;
      }
      for (unsigned int i = 0; i <= OP_LAST; ++i)
         ops[i].clear();

   } while (replaced);

   return true;
}

// =============================================================================

// Remove computations of unused values.
class DeadCodeElim : public Pass
{
public:
   bool buryAll(Program *);

private:
   virtual bool visit(BasicBlock *);

   void checkSplitLoad(Instruction *ld); // for partially dead loads

   unsigned int deadCount;
};

bool
DeadCodeElim::buryAll(Program *prog)
{
   do {
      deadCount = 0;
      if (!this->run(prog, false, false))
         return false;
   } while (deadCount);

   return true;
}

bool
DeadCodeElim::visit(BasicBlock *bb)
{
   Instruction *next;

   for (Instruction *i = bb->getFirst(); i; i = next) {
      next = i->next;
      if (i->isDead()) {
         ++deadCount;
         delete_Instruction(prog, i);
      } else
      if (i->defExists(1) && (i->op == OP_VFETCH || i->op == OP_LOAD)) {
         checkSplitLoad(i);
      }
   }
   return true;
}

void
DeadCodeElim::checkSplitLoad(Instruction *ld1)
{
   Instruction *ld2 = NULL; // can get at most 2 loads
   Value *def1[4];
   Value *def2[4];
   int32_t addr1, addr2;
   int32_t size1, size2;
   int d, n1, n2;
   uint32_t mask = 0xffffffff;

   for (d = 0; ld1->defExists(d); ++d)
      if (!ld1->getDef(d)->refCount() && ld1->getDef(d)->reg.data.id < 0)
         mask &= ~(1 << d);
   if (mask == 0xffffffff)
      return;

   addr1 = ld1->getSrc(0)->reg.data.offset;
   n1 = n2 = 0;
   size1 = size2 = 0;
   for (d = 0; ld1->defExists(d); ++d) {
      if (mask & (1 << d)) {
         if (size1 && (addr1 & 0x7))
            break;
         def1[n1] = ld1->getDef(d);
         size1 += def1[n1++]->reg.size;
      } else
      if (!n1) {
         addr1 += ld1->getDef(d)->reg.size;
      } else {
         break;
      }
   }
   for (addr2 = addr1 + size1; ld1->defExists(d); ++d) {
      if (mask & (1 << d)) {
         def2[n2] = ld1->getDef(d);
         size2 += def2[n2++]->reg.size;
      } else {
         assert(!n2);
         addr2 += ld1->getDef(d)->reg.size;
      }
   }

   updateLdStOffset(ld1, addr1, func);
   ld1->setType(typeOfSize(size1));
   for (d = 0; d < 4; ++d)
      ld1->setDef(d, (d < n1) ? def1[d] : NULL);

   if (!n2)
      return;

   ld2 = cloneShallow(func, ld1);
   updateLdStOffset(ld2, addr2, func);
   ld2->setType(typeOfSize(size2));
   for (d = 0; d < 4; ++d)
      ld2->setDef(d, (d < n2) ? def2[d] : NULL);

   ld1->bb->insertAfter(ld1, ld2);
}

// =============================================================================

#define RUN_PASS(l, n, f)                       \
   if (level >= (l)) {                          \
      if (dbgFlags & NV50_IR_DEBUG_VERBOSE)     \
         INFO("PEEPHOLE: %s\n", #n);            \
      n pass;                                   \
      if (!pass.f(this))                        \
         return false;                          \
   }

bool
Program::optimizeSSA(int level)
{
   RUN_PASS(1, DeadCodeElim, buryAll);
   RUN_PASS(1, CopyPropagation, run);
   RUN_PASS(2, GlobalCSE, run);
   RUN_PASS(1, LocalCSE, run);
   RUN_PASS(2, AlgebraicOpt, run);
   RUN_PASS(2, ModifierFolding, run); // before load propagation -> less checks
   RUN_PASS(1, ConstantFolding, foldAll);
   RUN_PASS(1, LoadPropagation, run);
   RUN_PASS(2, MemoryOpt, run);
   RUN_PASS(2, LocalCSE, run);
   RUN_PASS(0, DeadCodeElim, buryAll);

   return true;
}

bool
Program::optimizePostRA(int level)
{
   RUN_PASS(2, FlatteningPass, run);
   return true;
}

}
