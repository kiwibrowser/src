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

#include "nv50/codegen/nv50_ir.h"
#include "nv50/codegen/nv50_ir_build_util.h"

#include "nv50_ir_target_nv50.h"

namespace nv50_ir {

// nv50 doesn't support 32 bit integer multiplication
//
//       ah al * bh bl = LO32: (al * bh + ah * bl) << 16 + (al * bl)
// -------------------
//    al*bh 00           HI32: (al * bh + ah * bl) >> 16 + (ah * bh) +
// ah*bh 00 00                 (           carry1) << 16 + ( carry2)
//       al*bl
//    ah*bl 00
//
// fffe0001 + fffe0001
static bool
expandIntegerMUL(BuildUtil *bld, Instruction *mul)
{
   const bool highResult = mul->subOp == NV50_IR_SUBOP_MUL_HIGH;

   DataType fTy = mul->sType; // full type
   DataType hTy;
   switch (fTy) {
   case TYPE_S32: hTy = TYPE_S16; break;
   case TYPE_U32: hTy = TYPE_U16; break;
   case TYPE_U64: hTy = TYPE_U32; break;
   case TYPE_S64: hTy = TYPE_S32; break;
   default:
      return false;
   }
   unsigned int fullSize = typeSizeof(fTy);
   unsigned int halfSize = typeSizeof(hTy);

   Instruction *i[9];

   bld->setPosition(mul, true);

   Value *a[2], *b[2];
   Value *c[2];
   Value *t[4];
   for (int j = 0; j < 4; ++j)
      t[j] = bld->getSSA(fullSize);

   // split sources into halves
   i[0] = bld->mkSplit(a, halfSize, mul->getSrc(0));
   i[1] = bld->mkSplit(b, halfSize, mul->getSrc(1));

   i[2] = bld->mkOp2(OP_MUL, fTy, t[0], a[0], b[1]);
   i[3] = bld->mkOp3(OP_MAD, fTy, t[1], a[1], b[0], t[0]);
   i[7] = bld->mkOp2(OP_SHL, fTy, t[2], t[1], bld->mkImm(halfSize * 8));
   i[4] = bld->mkOp3(OP_MAD, fTy, t[3], a[0], b[0], t[2]);

   if (highResult) {
      Value *r[3];
      Value *imm = bld->loadImm(NULL, 1 << (halfSize * 8));
      c[0] = bld->getSSA(1, FILE_FLAGS);
      c[1] = bld->getSSA(1, FILE_FLAGS);
      for (int j = 0; j < 3; ++j)
         r[j] = bld->getSSA(fullSize);

      i[8] = bld->mkOp2(OP_SHR, fTy, r[0], t[1], bld->mkImm(halfSize * 8));
      i[6] = bld->mkOp2(OP_ADD, fTy, r[1], r[0], imm);
      bld->mkOp2(OP_UNION, TYPE_U32, r[2], r[1], r[0]);
      i[5] = bld->mkOp3(OP_MAD, fTy, mul->getDef(0), a[1], b[1], r[2]);

      // set carry defs / sources
      i[3]->setFlagsDef(1, c[0]);
      i[4]->setFlagsDef(0, c[1]); // actual result not required, just the carry
      i[6]->setPredicate(CC_C, c[0]);
      i[5]->setFlagsSrc(3, c[1]);
   } else {
      bld->mkMov(mul->getDef(0), t[3]);
   }
   delete_Instruction(bld->getProgram(), mul);

   for (int j = 2; j <= (highResult ? 5 : 4); ++j)
      if (i[j])
         i[j]->sType = hTy;

   return true;
}

#define QOP_ADD  0
#define QOP_SUBR 1
#define QOP_SUB  2
#define QOP_MOV2 3

//             UL UR LL LR
#define QUADOP(q, r, s, t)            \
   ((QOP_##q << 6) | (QOP_##r << 4) | \
    (QOP_##s << 2) | (QOP_##t << 0))

class NV50LegalizePostRA : public Pass
{
private:
   virtual bool visit(Function *);
   virtual bool visit(BasicBlock *);

   void handlePRERET(FlowInstruction *);
   void replaceZero(Instruction *);
   void split64BitOp(Instruction *);

   LValue *r63;
};

bool
NV50LegalizePostRA::visit(Function *fn)
{
   Program *prog = fn->getProgram();

   r63 = new_LValue(fn, FILE_GPR);
   r63->reg.data.id = 63;

   // this is actually per-program, but we can do it all on visiting main()
   std::list<Instruction *> *outWrites =
      reinterpret_cast<std::list<Instruction *> *>(prog->targetPriv);

   if (outWrites) {
      for (std::list<Instruction *>::iterator it = outWrites->begin();
           it != outWrites->end(); ++it)
         (*it)->getSrc(1)->defs.front()->getInsn()->setDef(0, (*it)->getSrc(0));
      // instructions will be deleted on exit
      outWrites->clear();
   }

   return true;
}

void
NV50LegalizePostRA::replaceZero(Instruction *i)
{
   for (int s = 0; i->srcExists(s); ++s) {
      ImmediateValue *imm = i->getSrc(s)->asImm();
      if (imm && imm->reg.data.u64 == 0)
         i->setSrc(s, r63);
   }
}

void
NV50LegalizePostRA::split64BitOp(Instruction *i)
{
   if (i->dType == TYPE_F64) {
      if (i->op == OP_MAD)
         i->op = OP_FMA;
      if (i->op == OP_ADD || i->op == OP_MUL || i->op == OP_FMA ||
          i->op == OP_CVT || i->op == OP_MIN || i->op == OP_MAX ||
          i->op == OP_SET)
         return;
      i->dType = i->sType = TYPE_U32;

      i->bb->insertAfter(i, cloneForward(func, i));
   }
}

// Emulate PRERET: jump to the target and call to the origin from there
//
// WARNING: atm only works if BBs are affected by at most a single PRERET
//
// BB:0
// preret BB:3
// (...)
// BB:3
// (...)
//             --->
// BB:0
// bra BB:3 + n0 (directly to the call; move to beginning of BB and fixate)
// (...)
// BB:3
// bra BB:3 + n1 (skip the call)
// call BB:0 + n2 (skip bra at beginning of BB:0)
// (...)
void
NV50LegalizePostRA::handlePRERET(FlowInstruction *pre)
{
   BasicBlock *bbE = pre->bb;
   BasicBlock *bbT = pre->target.bb;

   pre->subOp = NV50_IR_SUBOP_EMU_PRERET + 0;
   bbE->remove(pre);
   bbE->insertHead(pre);

   Instruction *skip = new_FlowInstruction(func, OP_PRERET, bbT);
   Instruction *call = new_FlowInstruction(func, OP_PRERET, bbE);

   bbT->insertHead(call);
   bbT->insertHead(skip);

   // NOTE: maybe split blocks to prevent the instructions from moving ?

   skip->subOp = NV50_IR_SUBOP_EMU_PRERET + 1;
   call->subOp = NV50_IR_SUBOP_EMU_PRERET + 2;
}

bool
NV50LegalizePostRA::visit(BasicBlock *bb)
{
   Instruction *i, *next;

   // remove pseudo operations and non-fixed no-ops, split 64 bit operations
   for (i = bb->getFirst(); i; i = next) {
      next = i->next;
      if (i->isNop()) {
         bb->remove(i);
      } else
      if (i->op == OP_PRERET && prog->getTarget()->getChipset() < 0xa0) {
         handlePRERET(i->asFlow());
      } else {
         if (i->op != OP_MOV && i->op != OP_PFETCH &&
             (!i->defExists(0) || i->def(0).getFile() != FILE_ADDRESS))
            replaceZero(i);
         if (typeSizeof(i->dType) == 8)
            split64BitOp(i);
      }
   }
   if (!bb->getEntry())
      return true;

   return true;
}

class NV50LegalizeSSA : public Pass
{
public:
   NV50LegalizeSSA(Program *);

   virtual bool visit(BasicBlock *bb);

private:
   void propagateWriteToOutput(Instruction *);
   void handleDIV(Instruction *);
   void handleMOD(Instruction *);
   void handleMUL(Instruction *);
   void handleAddrDef(Instruction *);

   inline bool isARL(const Instruction *) const;

   BuildUtil bld;

   std::list<Instruction *> *outWrites;
};

NV50LegalizeSSA::NV50LegalizeSSA(Program *prog)
{
   bld.setProgram(prog);

   if (prog->optLevel >= 2 &&
       (prog->getType() == Program::TYPE_GEOMETRY ||
        prog->getType() == Program::TYPE_VERTEX))
      outWrites =
         reinterpret_cast<std::list<Instruction *> *>(prog->targetPriv);
   else
      outWrites = NULL;
}

void
NV50LegalizeSSA::propagateWriteToOutput(Instruction *st)
{
   if (st->src(0).isIndirect(0) || st->getSrc(1)->refCount() != 1)
      return;

   // check def instruction can store
   Instruction *di = st->getSrc(1)->defs.front()->getInsn();

   // TODO: move exports (if beneficial) in common opt pass
   if (di->isPseudo() || isTextureOp(di->op) || di->defCount(0xff, true) > 1)
      return;
   for (int s = 0; di->srcExists(s); ++s)
      if (di->src(s).getFile() == FILE_IMMEDIATE)
         return;

   // We cannot set defs to non-lvalues before register allocation, so
   // save & remove (to save registers) the exports and replace later.
   outWrites->push_back(st);
   st->bb->remove(st);
}

bool
NV50LegalizeSSA::isARL(const Instruction *i) const
{
   ImmediateValue imm;

   if (i->op != OP_SHL || i->src(0).getFile() != FILE_GPR)
      return false;
   if (!i->src(1).getImmediate(imm))
      return false;
   return imm.isInteger(0);
}

void
NV50LegalizeSSA::handleAddrDef(Instruction *i)
{
   Instruction *arl;

   i->getDef(0)->reg.size = 2; // $aX are only 16 bit

   // only ADDR <- SHL(GPR, IMM) and ADDR <- ADD(ADDR, IMM) are valid
   if (i->srcExists(1) && i->src(1).getFile() == FILE_IMMEDIATE) {
      if (i->op == OP_SHL && i->src(0).getFile() == FILE_GPR)
         return;
      if (i->op == OP_ADD && i->src(0).getFile() == FILE_ADDRESS)
         return;
   }

   // turn $a sources into $r sources (can't operate on $a)
   for (int s = 0; i->srcExists(s); ++s) {
      Value *a = i->getSrc(s);
      Value *r;
      if (a->reg.file == FILE_ADDRESS) {
         if (a->getInsn() && isARL(a->getInsn())) {
            i->setSrc(s, a->getInsn()->getSrc(0));
         } else {
            bld.setPosition(i, false);
            r = bld.getSSA();
            bld.mkMov(r, a);
            i->setSrc(s, r);
         }
      }
   }
   if (i->op == OP_SHL && i->src(1).getFile() == FILE_IMMEDIATE)
      return;

   // turn result back into $a
   bld.setPosition(i, true);
   arl = bld.mkOp2(OP_SHL, TYPE_U32, i->getDef(0), bld.getSSA(), bld.mkImm(0));
   i->setDef(0, arl->getSrc(0));
}

void
NV50LegalizeSSA::handleMUL(Instruction *mul)
{
   if (isFloatType(mul->sType) || typeSizeof(mul->sType) <= 2)
      return;
   Value *def = mul->getDef(0);
   Value *pred = mul->getPredicate();
   CondCode cc = mul->cc;
   if (pred)
      mul->setPredicate(CC_ALWAYS, NULL);

   if (mul->op == OP_MAD) {
      Instruction *add = mul;
      bld.setPosition(add, false);
      Value *res = cloneShallow(func, mul->getDef(0));
      mul = bld.mkOp2(OP_MUL, add->sType, res, add->getSrc(0), add->getSrc(1));
      add->op = OP_ADD;
      add->setSrc(0, mul->getDef(0));
      add->setSrc(1, add->getSrc(2));
      for (int s = 2; add->srcExists(s); ++s)
         add->setSrc(s, NULL);
      mul->subOp = add->subOp;
      add->subOp = 0;
   }
   expandIntegerMUL(&bld, mul);
   if (pred)
      def->getInsn()->setPredicate(cc, pred);
}

// Use f32 division: first compute an approximate result, use it to reduce
// the dividend, which should then be representable as f32, divide the reduced
// dividend, and add the quotients.
void
NV50LegalizeSSA::handleDIV(Instruction *div)
{
   const DataType ty = div->sType;

   if (ty != TYPE_U32 && ty != TYPE_S32)
      return;

   Value *q, *q0, *qf, *aR, *aRf, *qRf, *qR, *t, *s, *m, *cond;

   bld.setPosition(div, false);

   Value *a, *af = bld.getSSA();
   Value *b, *bf = bld.getSSA();

   bld.mkCvt(OP_CVT, TYPE_F32, af, ty, div->getSrc(0));
   bld.mkCvt(OP_CVT, TYPE_F32, bf, ty, div->getSrc(1));

   if (isSignedType(ty)) {
      af->getInsn()->src(0).mod = Modifier(NV50_IR_MOD_ABS);
      bf->getInsn()->src(0).mod = Modifier(NV50_IR_MOD_ABS);
      a = bld.getSSA();
      b = bld.getSSA();
      bld.mkOp1(OP_ABS, ty, a, div->getSrc(0));
      bld.mkOp1(OP_ABS, ty, b, div->getSrc(1));
   } else {
      a = div->getSrc(0);
      b = div->getSrc(1);
   }

   bf = bld.mkOp1v(OP_RCP, TYPE_F32, bld.getSSA(), bf);
   bf = bld.mkOp2v(OP_ADD, TYPE_U32, bld.getSSA(), bf, bld.mkImm(-2));

   bld.mkOp2(OP_MUL, TYPE_F32, (qf = bld.getSSA()), af, bf)->rnd = ROUND_Z;
   bld.mkCvt(OP_CVT, ty, (q0 = bld.getSSA()), TYPE_F32, qf)->rnd = ROUND_Z;

   // get error of 1st result
   expandIntegerMUL(&bld,
      bld.mkOp2(OP_MUL, TYPE_U32, (t = bld.getSSA()), q0, b));
   bld.mkOp2(OP_SUB, TYPE_U32, (aRf = bld.getSSA()), a, t);

   bld.mkCvt(OP_CVT, TYPE_F32, (aR = bld.getSSA()), TYPE_U32, aRf);

   bld.mkOp2(OP_MUL, TYPE_F32, (qRf = bld.getSSA()), aR, bf)->rnd = ROUND_Z;
   bld.mkCvt(OP_CVT, TYPE_U32, (qR = bld.getSSA()), TYPE_F32, qRf)
      ->rnd = ROUND_Z;
   bld.mkOp2(OP_ADD, ty, (q = bld.getSSA()), q0, qR); // add quotients

   // correction: if modulus >= divisor, add 1
   expandIntegerMUL(&bld,
      bld.mkOp2(OP_MUL, TYPE_U32, (t = bld.getSSA()), q, b));
   bld.mkOp2(OP_SUB, TYPE_U32, (m = bld.getSSA()), a, t);
   bld.mkCmp(OP_SET, CC_GE, TYPE_U32, (s = bld.getSSA()), m, b);
   if (!isSignedType(ty)) {
      div->op = OP_SUB;
      div->setSrc(0, q);
      div->setSrc(1, s);
   } else {
      t = q;
      bld.mkOp2(OP_SUB, TYPE_U32, (q = bld.getSSA()), t, s);
      s = bld.getSSA();
      t = bld.getSSA();
      // fix the sign
      bld.mkOp2(OP_XOR, TYPE_U32, NULL, div->getSrc(0), div->getSrc(1))
         ->setFlagsDef(0, (cond = bld.getSSA(1, FILE_FLAGS)));
      bld.mkOp1(OP_NEG, ty, s, q)->setPredicate(CC_S, cond);
      bld.mkOp1(OP_MOV, ty, t, q)->setPredicate(CC_NS, cond);

      div->op = OP_UNION;
      div->setSrc(0, s);
      div->setSrc(1, t);
   }
}

void
NV50LegalizeSSA::handleMOD(Instruction *mod)
{
   if (mod->dType != TYPE_U32 && mod->dType != TYPE_S32)
      return;
   bld.setPosition(mod, false);

   Value *q = bld.getSSA();
   Value *m = bld.getSSA();

   bld.mkOp2(OP_DIV, mod->dType, q, mod->getSrc(0), mod->getSrc(1));
   handleDIV(q->getInsn());

   bld.setPosition(mod, false);
   expandIntegerMUL(&bld, bld.mkOp2(OP_MUL, TYPE_U32, m, q, mod->getSrc(1)));

   mod->op = OP_SUB;
   mod->setSrc(1, m);
}

bool
NV50LegalizeSSA::visit(BasicBlock *bb)
{
   Instruction *insn, *next;
   // skipping PHIs (don't pass them to handleAddrDef) !
   for (insn = bb->getEntry(); insn; insn = next) {
      next = insn->next;

      switch (insn->op) {
      case OP_EXPORT:
         if (outWrites)
            propagateWriteToOutput(insn);
         break;
      case OP_DIV:
         handleDIV(insn);
         break;
      case OP_MOD:
         handleMOD(insn);
         break;
      case OP_MAD:
      case OP_MUL:
         handleMUL(insn);
         break;
      default:
         break;
      }

      if (insn->defExists(0) && insn->getDef(0)->reg.file == FILE_ADDRESS)
         handleAddrDef(insn);
   }
   return true;
}

class NV50LoweringPreSSA : public Pass
{
public:
   NV50LoweringPreSSA(Program *);

private:
   virtual bool visit(Instruction *);
   virtual bool visit(Function *);

   bool handleRDSV(Instruction *);
   bool handleWRSV(Instruction *);

   bool handleEXPORT(Instruction *);

   bool handleDIV(Instruction *);
   bool handleSQRT(Instruction *);
   bool handlePOW(Instruction *);

   bool handleSET(Instruction *);
   bool handleSLCT(CmpInstruction *);
   bool handleSELP(Instruction *);

   bool handleTEX(TexInstruction *);
   bool handleTXB(TexInstruction *); // I really
   bool handleTXL(TexInstruction *); // hate
   bool handleTXD(TexInstruction *); // these 3

   bool handleCALL(Instruction *);
   bool handlePRECONT(Instruction *);
   bool handleCONT(Instruction *);

   void checkPredicate(Instruction *);

private:
   const Target *const targ;

   BuildUtil bld;

   Value *tid;
};

NV50LoweringPreSSA::NV50LoweringPreSSA(Program *prog) :
   targ(prog->getTarget()), tid(NULL)
{
   bld.setProgram(prog);
}

bool
NV50LoweringPreSSA::visit(Function *f)
{
   BasicBlock *root = BasicBlock::get(func->cfg.getRoot());

   if (prog->getType() == Program::TYPE_COMPUTE) {
      // Add implicit "thread id" argument in $r0 to the function
      Value *arg = new_LValue(func, FILE_GPR);
      arg->reg.data.id = 0;
      f->ins.push_back(arg);

      bld.setPosition(root, false);
      tid = bld.mkMov(bld.getScratch(), arg, TYPE_U32)->getDef(0);
   }

   return true;
}

// move array source to first slot, convert to u16, add indirections
bool
NV50LoweringPreSSA::handleTEX(TexInstruction *i)
{
   const int arg = i->tex.target.getArgCount();
   const int dref = arg;
   const int lod = i->tex.target.isShadow() ? (arg + 1) : arg;

   // dref comes before bias/lod
   if (i->tex.target.isShadow())
      if (i->op == OP_TXB || i->op == OP_TXL)
         i->swapSources(dref, lod);

   // array index must be converted to u32
   if (i->tex.target.isArray()) {
      Value *layer = i->getSrc(arg - 1);
      LValue *src = new_LValue(func, FILE_GPR);
      bld.mkCvt(OP_CVT, TYPE_U32, src, TYPE_F32, layer);
      bld.mkOp2(OP_MIN, TYPE_U32, src, src, bld.loadImm(NULL, 511));
      i->setSrc(arg - 1, src);

      if (i->tex.target.isCube()) {
         // Value *face = layer;
         Value *x, *y;
         x = new_LValue(func, FILE_GPR);
         y = new_LValue(func, FILE_GPR);
         layer = new_LValue(func, FILE_GPR);

         i->tex.target = TEX_TARGET_2D_ARRAY;

         // TODO: use TEXPREP to convert x,y,z,face -> x,y,layer
         bld.mkMov(x, i->getSrc(0));
         bld.mkMov(y, i->getSrc(1));
         bld.mkMov(layer, i->getSrc(3));

         i->setSrc(0, x);
         i->setSrc(1, y);
         i->setSrc(2, layer);
         i->setSrc(3, i->getSrc(4));
         i->setSrc(4, NULL);
      }
   }

   // texel offsets are 3 immediate fields in the instruction,
   // nv50 cannot do textureGatherOffsets
   assert(i->tex.useOffsets <= 1);

   return true;
}

// Bias must be equal for all threads of a quad or lod calculation will fail.
//
// The lanes of a quad are grouped by the bit in the condition register they
// have set, which is selected by differing bias values.
// Move the input values for TEX into a new register set for each group and
// execute TEX only for a specific group.
// We always need to use 4 new registers for the inputs/outputs because the
// implicitly calculated derivatives must be correct.
//
// TODO: move to SSA phase so we can easily determine whether bias is constant
bool
NV50LoweringPreSSA::handleTXB(TexInstruction *i)
{
   const CondCode cc[4] = { CC_EQU, CC_S, CC_C, CC_O };
   int l, d;

   handleTEX(i);
   Value *bias = i->getSrc(i->tex.target.getArgCount());
   if (bias->isUniform())
      return true;

   Instruction *cond = bld.mkOp1(OP_UNION, TYPE_U32, bld.getScratch(),
                                 bld.loadImm(NULL, 1));
   bld.setPosition(cond, false);

   for (l = 1; l < 4; ++l) {
      const uint8_t qop = QUADOP(SUBR, SUBR, SUBR, SUBR);
      Value *bit = bld.getSSA();
      Value *pred = bld.getScratch(1, FILE_FLAGS);
      Value *imm = bld.loadImm(NULL, (1 << l));
      bld.mkQuadop(qop, pred, l, bias, bias)->flagsDef = 0;
      bld.mkMov(bit, imm)->setPredicate(CC_EQ, pred);
      cond->setSrc(l, bit);
   }
   Value *flags = bld.getScratch(1, FILE_FLAGS);
   bld.setPosition(cond, true);
   bld.mkCvt(OP_CVT, TYPE_U8, flags, TYPE_U32, cond->getDef(0));

   Instruction *tex[4];
   for (l = 0; l < 4; ++l) {
      (tex[l] = cloneForward(func, i))->setPredicate(cc[l], flags);
      bld.insert(tex[l]);
   }

   Value *res[4][4];
   for (d = 0; i->defExists(d); ++d)
      res[0][d] = tex[0]->getDef(d);
   for (l = 1; l < 4; ++l) {
      for (d = 0; tex[l]->defExists(d); ++d) {
         res[l][d] = cloneShallow(func, res[0][d]);
         bld.mkMov(res[l][d], tex[l]->getDef(d))->setPredicate(cc[l], flags);
      }
   }

   for (d = 0; i->defExists(d); ++d) {
      Instruction *dst = bld.mkOp(OP_UNION, TYPE_U32, i->getDef(d));
      for (l = 0; l < 4; ++l)
         dst->setSrc(l, res[l][d]);
   }
   delete_Instruction(prog, i);
   return true;
}

// LOD must be equal for all threads of a quad.
// Unlike with TXB, here we can just diverge since there's no LOD calculation
// that would require all 4 threads' sources to be set up properly.
bool
NV50LoweringPreSSA::handleTXL(TexInstruction *i)
{
   handleTEX(i);
   Value *lod = i->getSrc(i->tex.target.getArgCount());
   if (lod->isUniform())
      return true;

   BasicBlock *currBB = i->bb;
   BasicBlock *texiBB = i->bb->splitBefore(i, false);
   BasicBlock *joinBB = i->bb->splitAfter(i);

   bld.setPosition(currBB, true);
   currBB->joinAt = bld.mkFlow(OP_JOINAT, joinBB, CC_ALWAYS, NULL);

   for (int l = 0; l <= 3; ++l) {
      const uint8_t qop = QUADOP(SUBR, SUBR, SUBR, SUBR);
      Value *pred = bld.getScratch(1, FILE_FLAGS);
      bld.setPosition(currBB, true);
      bld.mkQuadop(qop, pred, l, lod, lod)->flagsDef = 0;
      bld.mkFlow(OP_BRA, texiBB, CC_EQ, pred)->fixed = 1;
      currBB->cfg.attach(&texiBB->cfg, Graph::Edge::FORWARD);
      if (l <= 2) {
         BasicBlock *laneBB = new BasicBlock(func);
         currBB->cfg.attach(&laneBB->cfg, Graph::Edge::TREE);
         currBB = laneBB;
      }
   }
   bld.setPosition(joinBB, false);
   bld.mkOp(OP_JOIN, TYPE_NONE, NULL);
   return true;
}

bool
NV50LoweringPreSSA::handleTXD(TexInstruction *i)
{
   static const uint8_t qOps[4][2] =
   {
      { QUADOP(MOV2, ADD,  MOV2, ADD),  QUADOP(MOV2, MOV2, ADD,  ADD) }, // l0
      { QUADOP(SUBR, MOV2, SUBR, MOV2), QUADOP(MOV2, MOV2, ADD,  ADD) }, // l1
      { QUADOP(MOV2, ADD,  MOV2, ADD),  QUADOP(SUBR, SUBR, MOV2, MOV2) }, // l2
      { QUADOP(SUBR, MOV2, SUBR, MOV2), QUADOP(SUBR, SUBR, MOV2, MOV2) }, // l3
   };
   Value *def[4][4];
   Value *crd[3];
   Instruction *tex;
   Value *zero = bld.loadImm(bld.getSSA(), 0);
   int l, c;
   const int dim = i->tex.target.getDim();

   handleTEX(i);
   i->op = OP_TEX; // no need to clone dPdx/dPdy later

   for (c = 0; c < dim; ++c)
      crd[c] = bld.getScratch();

   bld.mkOp(OP_QUADON, TYPE_NONE, NULL);
   for (l = 0; l < 4; ++l) {
      // mov coordinates from lane l to all lanes
      for (c = 0; c < dim; ++c)
         bld.mkQuadop(0x00, crd[c], l, i->getSrc(c), zero);
      // add dPdx from lane l to lanes dx
      for (c = 0; c < dim; ++c)
         bld.mkQuadop(qOps[l][0], crd[c], l, i->dPdx[c].get(), crd[c]);
      // add dPdy from lane l to lanes dy
      for (c = 0; c < dim; ++c)
         bld.mkQuadop(qOps[l][1], crd[c], l, i->dPdy[c].get(), crd[c]);
      // texture
      bld.insert(tex = cloneForward(func, i));
      for (c = 0; c < dim; ++c)
         tex->setSrc(c, crd[c]);
      // save results
      for (c = 0; i->defExists(c); ++c) {
         Instruction *mov;
         def[c][l] = bld.getSSA();
         mov = bld.mkMov(def[c][l], tex->getDef(c));
         mov->fixed = 1;
         mov->lanes = 1 << l;
      }
   }
   bld.mkOp(OP_QUADPOP, TYPE_NONE, NULL);

   for (c = 0; i->defExists(c); ++c) {
      Instruction *u = bld.mkOp(OP_UNION, TYPE_U32, i->getDef(c));
      for (l = 0; l < 4; ++l)
         u->setSrc(l, def[c][l]);
   }

   i->bb->remove(i);
   return true;
}

bool
NV50LoweringPreSSA::handleSET(Instruction *i)
{
   if (i->dType == TYPE_F32) {
      bld.setPosition(i, true);
      i->dType = TYPE_U32;
      bld.mkOp1(OP_ABS, TYPE_S32, i->getDef(0), i->getDef(0));
      bld.mkCvt(OP_CVT, TYPE_F32, i->getDef(0), TYPE_S32, i->getDef(0));
   }
   return true;
}

bool
NV50LoweringPreSSA::handleSLCT(CmpInstruction *i)
{
   Value *src0 = bld.getSSA();
   Value *src1 = bld.getSSA();
   Value *pred = bld.getScratch(1, FILE_FLAGS);

   Value *v0 = i->getSrc(0);
   Value *v1 = i->getSrc(1);
   // XXX: these probably shouldn't be immediates in the first place ...
   if (v0->asImm())
      v0 = bld.mkMov(bld.getSSA(), v0)->getDef(0);
   if (v1->asImm())
      v1 = bld.mkMov(bld.getSSA(), v1)->getDef(0);

   bld.setPosition(i, true);
   bld.mkMov(src0, v0)->setPredicate(CC_NE, pred);
   bld.mkMov(src1, v1)->setPredicate(CC_EQ, pred);
   bld.mkOp2(OP_UNION, i->dType, i->getDef(0), src0, src1);

   bld.setPosition(i, false);
   i->op = OP_SET;
   i->setFlagsDef(0, pred);
   i->dType = TYPE_U8;
   i->setSrc(0, i->getSrc(2));
   i->setSrc(2, NULL);
   i->setSrc(1, bld.loadImm(NULL, 0));

   return true;
}

bool
NV50LoweringPreSSA::handleSELP(Instruction *i)
{
   Value *src0 = bld.getSSA();
   Value *src1 = bld.getSSA();

   Value *v0 = i->getSrc(0);
   Value *v1 = i->getSrc(1);
   if (v0->asImm())
      v0 = bld.mkMov(bld.getSSA(), v0)->getDef(0);
   if (v1->asImm())
      v1 = bld.mkMov(bld.getSSA(), v1)->getDef(0);

   bld.mkMov(src0, v0)->setPredicate(CC_NE, i->getSrc(2));
   bld.mkMov(src1, v1)->setPredicate(CC_EQ, i->getSrc(2));
   bld.mkOp2(OP_UNION, i->dType, i->getDef(0), src0, src1);
   delete_Instruction(prog, i);
   return true;
}

bool
NV50LoweringPreSSA::handleWRSV(Instruction *i)
{
   Symbol *sym = i->getSrc(0)->asSym();

   // these are all shader outputs, $sreg are not writeable
   uint32_t addr = targ->getSVAddress(FILE_SHADER_OUTPUT, sym);
   if (addr >= 0x400)
      return false;
   sym = bld.mkSymbol(FILE_SHADER_OUTPUT, 0, i->sType, addr);

   bld.mkStore(OP_EXPORT, i->dType, sym, i->getIndirect(0, 0), i->getSrc(1));

   bld.getBB()->remove(i);
   return true;
}

bool
NV50LoweringPreSSA::handleCALL(Instruction *i)
{
   if (prog->getType() == Program::TYPE_COMPUTE) {
      // Add implicit "thread id" argument in $r0 to the function
      i->setSrc(i->srcCount(), tid);
   }
   return true;
}

bool
NV50LoweringPreSSA::handlePRECONT(Instruction *i)
{
   delete_Instruction(prog, i);
   return true;
}

bool
NV50LoweringPreSSA::handleCONT(Instruction *i)
{
   i->op = OP_BRA;
   return true;
}

bool
NV50LoweringPreSSA::handleRDSV(Instruction *i)
{
   Symbol *sym = i->getSrc(0)->asSym();
   uint32_t addr = targ->getSVAddress(FILE_SHADER_INPUT, sym);
   Value *def = i->getDef(0);
   SVSemantic sv = sym->reg.data.sv.sv;
   int idx = sym->reg.data.sv.index;

   if (addr >= 0x400) // mov $sreg
      return true;

   switch (sv) {
   case SV_POSITION:
      assert(prog->getType() == Program::TYPE_FRAGMENT);
      bld.mkInterp(NV50_IR_INTERP_LINEAR, i->getDef(0), addr, NULL);
      break;
   case SV_FACE:
      bld.mkInterp(NV50_IR_INTERP_FLAT, def, addr, NULL);
      if (i->dType == TYPE_F32) {
         bld.mkOp2(OP_AND, TYPE_U32, def, def, bld.mkImm(0x80000000));
         bld.mkOp2(OP_XOR, TYPE_U32, def, def, bld.mkImm(0xbf800000));
      }
      break;
   case SV_NCTAID:
   case SV_CTAID:
   case SV_NTID:
      if ((sv == SV_NCTAID && idx >= 2) ||
          (sv == SV_NTID && idx >= 3)) {
         bld.mkMov(def, bld.mkImm(1));
      } else if (sv == SV_CTAID && idx >= 2) {
         bld.mkMov(def, bld.mkImm(0));
      } else {
         Value *x = bld.getSSA(2);
         bld.mkOp1(OP_LOAD, TYPE_U16, x,
                   bld.mkSymbol(FILE_MEMORY_SHARED, 0, TYPE_U16, addr));
         bld.mkCvt(OP_CVT, TYPE_U32, def, TYPE_U16, x);
      }
      break;
   case SV_TID:
      if (idx == 0) {
         bld.mkOp2(OP_AND, TYPE_U32, def, tid, bld.mkImm(0x0000ffff));
      } else if (idx == 1) {
         bld.mkOp2(OP_AND, TYPE_U32, def, tid, bld.mkImm(0x03ff0000));
         bld.mkOp2(OP_SHR, TYPE_U32, def, def, bld.mkImm(16));
      } else if (idx == 2) {
         bld.mkOp2(OP_SHR, TYPE_U32, def, tid, bld.mkImm(26));
      } else {
         bld.mkMov(def, bld.mkImm(0));
      }
      break;
   default:
      bld.mkFetch(i->getDef(0), i->dType,
                  FILE_SHADER_INPUT, addr, i->getIndirect(0, 0), NULL);
      break;
   }
   bld.getBB()->remove(i);
   return true;
}

bool
NV50LoweringPreSSA::handleDIV(Instruction *i)
{
   if (!isFloatType(i->dType))
      return true;
   bld.setPosition(i, false);
   Instruction *rcp = bld.mkOp1(OP_RCP, i->dType, bld.getSSA(), i->getSrc(1));
   i->op = OP_MUL;
   i->setSrc(1, rcp->getDef(0));
   return true;
}

bool
NV50LoweringPreSSA::handleSQRT(Instruction *i)
{
   Instruction *rsq = bld.mkOp1(OP_RSQ, TYPE_F32,
                                bld.getSSA(), i->getSrc(0));
   i->op = OP_MUL;
   i->setSrc(1, rsq->getDef(0));

   return true;
}

bool
NV50LoweringPreSSA::handlePOW(Instruction *i)
{
   LValue *val = bld.getScratch();

   bld.mkOp1(OP_LG2, TYPE_F32, val, i->getSrc(0));
   bld.mkOp2(OP_MUL, TYPE_F32, val, i->getSrc(1), val)->dnz = 1;
   bld.mkOp1(OP_PREEX2, TYPE_F32, val, val);

   i->op = OP_EX2;
   i->setSrc(0, val);
   i->setSrc(1, NULL);

   return true;
}

bool
NV50LoweringPreSSA::handleEXPORT(Instruction *i)
{
   if (prog->getType() == Program::TYPE_FRAGMENT) {
      if (i->getIndirect(0, 0)) {
         // TODO: redirect to l[] here, load to GPRs at exit
         return false;
      } else {
         int id = i->getSrc(0)->reg.data.offset / 4; // in 32 bit reg units

         i->op = OP_MOV;
         i->subOp = NV50_IR_SUBOP_MOV_FINAL;
         i->src(0).set(i->src(1));
         i->setSrc(1, NULL);
         i->setDef(0, new_LValue(func, FILE_GPR));
         i->getDef(0)->reg.data.id = id;

         prog->maxGPR = MAX2(prog->maxGPR, id);
      }
   }
   return true;
}

// Set flags according to predicate and make the instruction read $cX.
void
NV50LoweringPreSSA::checkPredicate(Instruction *insn)
{
   Value *pred = insn->getPredicate();
   Value *cdst;

   if (!pred || pred->reg.file == FILE_FLAGS)
      return;
   cdst = bld.getSSA(1, FILE_FLAGS);

   bld.mkCmp(OP_SET, CC_NEU, TYPE_U32, cdst, bld.loadImm(NULL, 0), pred);

   insn->setPredicate(insn->cc, cdst);
}

//
// - add quadop dance for texturing
// - put FP outputs in GPRs
// - convert instruction sequences
//
bool
NV50LoweringPreSSA::visit(Instruction *i)
{
   bld.setPosition(i, false);

   if (i->cc != CC_ALWAYS)
      checkPredicate(i);

   switch (i->op) {
   case OP_TEX:
   case OP_TXF:
   case OP_TXG:
      return handleTEX(i->asTex());
   case OP_TXB:
      return handleTXB(i->asTex());
   case OP_TXL:
      return handleTXL(i->asTex());
   case OP_TXD:
      return handleTXD(i->asTex());
   case OP_EX2:
      bld.mkOp1(OP_PREEX2, TYPE_F32, i->getDef(0), i->getSrc(0));
      i->setSrc(0, i->getDef(0));
      break;
   case OP_SET:
      return handleSET(i);
   case OP_SLCT:
      return handleSLCT(i->asCmp());
   case OP_SELP:
      return handleSELP(i);
   case OP_POW:
      return handlePOW(i);
   case OP_DIV:
      return handleDIV(i);
   case OP_SQRT:
      return handleSQRT(i);
   case OP_EXPORT:
      return handleEXPORT(i);
   case OP_RDSV:
      return handleRDSV(i);
   case OP_WRSV:
      return handleWRSV(i);
   case OP_CALL:
      return handleCALL(i);
   case OP_PRECONT:
      return handlePRECONT(i);
   case OP_CONT:
      return handleCONT(i);
   default:
      break;
   }
   return true;
}

bool
TargetNV50::runLegalizePass(Program *prog, CGStage stage) const
{
   bool ret = false;

   if (stage == CG_STAGE_PRE_SSA) {
      NV50LoweringPreSSA pass(prog);
      ret = pass.run(prog, false, true);
   } else
   if (stage == CG_STAGE_SSA) {
      if (!prog->targetPriv)
         prog->targetPriv = new std::list<Instruction *>();
      NV50LegalizeSSA pass(prog);
      ret = pass.run(prog, false, true);
   } else
   if (stage == CG_STAGE_POST_RA) {
      NV50LegalizePostRA pass;
      ret = pass.run(prog, false, true);
      if (prog->targetPriv)
         delete reinterpret_cast<std::list<Instruction *> *>(prog->targetPriv);
   }
   return ret;
}

} // namespace nv50_ir
