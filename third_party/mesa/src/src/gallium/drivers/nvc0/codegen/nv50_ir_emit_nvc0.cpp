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

// Argh, all these assertions ...

class CodeEmitterNVC0 : public CodeEmitter
{
public:
   CodeEmitterNVC0(const TargetNVC0 *);

   virtual bool emitInstruction(Instruction *);
   virtual uint32_t getMinEncodingSize(const Instruction *) const;
   virtual void prepareEmission(Function *);

   inline void setProgramType(Program::Type pType) { progType = pType; }

private:
   const TargetNVC0 *targ;

   Program::Type progType;

   const bool writeIssueDelays;

private:
   void emitForm_A(const Instruction *, uint64_t);
   void emitForm_B(const Instruction *, uint64_t);
   void emitForm_S(const Instruction *, uint32_t, bool pred);

   void emitPredicate(const Instruction *);

   void setAddress16(const ValueRef&);
   void setImmediate(const Instruction *, const int s); // needs op already set
   void setImmediateS8(const ValueRef&);

   void emitCondCode(CondCode cc, int pos);
   void emitInterpMode(const Instruction *);
   void emitLoadStoreType(DataType ty);
   void emitCachingMode(CacheMode c);

   void emitShortSrc2(const ValueRef&);

   inline uint8_t getSRegEncoding(const ValueRef&);

   void roundMode_A(const Instruction *);
   void roundMode_C(const Instruction *);
   void roundMode_CS(const Instruction *);

   void emitNegAbs12(const Instruction *);

   void emitNOP(const Instruction *);

   void emitLOAD(const Instruction *);
   void emitSTORE(const Instruction *);
   void emitMOV(const Instruction *);

   void emitINTERP(const Instruction *);
   void emitPFETCH(const Instruction *);
   void emitVFETCH(const Instruction *);
   void emitEXPORT(const Instruction *);
   void emitOUT(const Instruction *);

   void emitUADD(const Instruction *);
   void emitFADD(const Instruction *);
   void emitUMUL(const Instruction *);
   void emitFMUL(const Instruction *);
   void emitIMAD(const Instruction *);
   void emitISAD(const Instruction *);
   void emitFMAD(const Instruction *);

   void emitNOT(Instruction *);
   void emitLogicOp(const Instruction *, uint8_t subOp);
   void emitPOPC(const Instruction *);
   void emitINSBF(const Instruction *);
   void emitShift(const Instruction *);

   void emitSFnOp(const Instruction *, uint8_t subOp);

   void emitCVT(Instruction *);
   void emitMINMAX(const Instruction *);
   void emitPreOp(const Instruction *);

   void emitSET(const CmpInstruction *);
   void emitSLCT(const CmpInstruction *);
   void emitSELP(const Instruction *);

   void emitTEXBAR(const Instruction *);
   void emitTEX(const TexInstruction *);
   void emitTEXCSAA(const TexInstruction *);
   void emitTXQ(const TexInstruction *);
   void emitPIXLD(const TexInstruction *);

   void emitQUADOP(const Instruction *, uint8_t qOp, uint8_t laneMask);

   void emitFlow(const Instruction *);

   inline void defId(const ValueDef&, const int pos);
   inline void srcId(const ValueRef&, const int pos);
   inline void srcId(const ValueRef *, const int pos);
   inline void srcId(const Instruction *, int s, const int pos);

   inline void srcAddr32(const ValueRef&, const int pos); // address / 4

   inline bool isLIMM(const ValueRef&, DataType ty);
};

// for better visibility
#define HEX64(h, l) 0x##h##l##ULL

#define SDATA(a) ((a).rep()->reg.data)
#define DDATA(a) ((a).rep()->reg.data)

void CodeEmitterNVC0::srcId(const ValueRef& src, const int pos)
{
   code[pos / 32] |= (src.get() ? SDATA(src).id : 63) << (pos % 32);
}

void CodeEmitterNVC0::srcId(const ValueRef *src, const int pos)
{
   code[pos / 32] |= (src ? SDATA(*src).id : 63) << (pos % 32);
}

void CodeEmitterNVC0::srcId(const Instruction *insn, int s, int pos)
{
   int r = insn->srcExists(s) ? SDATA(insn->src(s)).id : 63;
   code[pos / 32] |= r << (pos % 32);
}

void CodeEmitterNVC0::srcAddr32(const ValueRef& src, const int pos)
{
   code[pos / 32] |= (SDATA(src).offset >> 2) << (pos % 32);
}

void CodeEmitterNVC0::defId(const ValueDef& def, const int pos)
{
   code[pos / 32] |= (def.get() ? DDATA(def).id : 63) << (pos % 32);
}

bool CodeEmitterNVC0::isLIMM(const ValueRef& ref, DataType ty)
{
   const ImmediateValue *imm = ref.get()->asImm();

   return imm && (imm->reg.data.u32 & ((ty == TYPE_F32) ? 0xfff : 0xfff00000));
}

void
CodeEmitterNVC0::roundMode_A(const Instruction *insn)
{
   switch (insn->rnd) {
   case ROUND_M: code[1] |= 1 << 23; break;
   case ROUND_P: code[1] |= 2 << 23; break;
   case ROUND_Z: code[1] |= 3 << 23; break;
   default:
      assert(insn->rnd == ROUND_N);
      break;
   }
}

void
CodeEmitterNVC0::emitNegAbs12(const Instruction *i)
{
   if (i->src(1).mod.abs()) code[0] |= 1 << 6;
   if (i->src(0).mod.abs()) code[0] |= 1 << 7;
   if (i->src(1).mod.neg()) code[0] |= 1 << 8;
   if (i->src(0).mod.neg()) code[0] |= 1 << 9;
}

void CodeEmitterNVC0::emitCondCode(CondCode cc, int pos)
{
   uint8_t val;

   switch (cc) {
   case CC_LT:  val = 0x1; break;
   case CC_LTU: val = 0x9; break;
   case CC_EQ:  val = 0x2; break;
   case CC_EQU: val = 0xa; break;
   case CC_LE:  val = 0x3; break;
   case CC_LEU: val = 0xb; break;
   case CC_GT:  val = 0x4; break;
   case CC_GTU: val = 0xc; break;
   case CC_NE:  val = 0x5; break;
   case CC_NEU: val = 0xd; break;
   case CC_GE:  val = 0x6; break;
   case CC_GEU: val = 0xe; break;
   case CC_TR:  val = 0xf; break;
   case CC_FL:  val = 0x0; break;

   case CC_A:  val = 0x14; break;
   case CC_NA: val = 0x13; break;
   case CC_S:  val = 0x15; break;
   case CC_NS: val = 0x12; break;
   case CC_C:  val = 0x16; break;
   case CC_NC: val = 0x11; break;
   case CC_O:  val = 0x17; break;
   case CC_NO: val = 0x10; break;

   default:
      val = 0;
      assert(!"invalid condition code");
      break;
   }
   code[pos / 32] |= val << (pos % 32);
}

void
CodeEmitterNVC0::emitPredicate(const Instruction *i)
{
   if (i->predSrc >= 0) {
      assert(i->getPredicate()->reg.file == FILE_PREDICATE);
      srcId(i->src(i->predSrc), 10);
      if (i->cc == CC_NOT_P)
         code[0] |= 0x2000; // negate
   } else {
      code[0] |= 0x1c00;
   }
}

void
CodeEmitterNVC0::setAddress16(const ValueRef& src)
{
   Symbol *sym = src.get()->asSym();

   assert(sym);

   code[0] |= (sym->reg.data.offset & 0x003f) << 26;
   code[1] |= (sym->reg.data.offset & 0xffc0) >> 6;
}

void
CodeEmitterNVC0::setImmediate(const Instruction *i, const int s)
{
   const ImmediateValue *imm = i->src(s).get()->asImm();
   uint32_t u32;

   assert(imm);
   u32 = imm->reg.data.u32;

   if ((code[0] & 0xf) == 0x2) {
      // LIMM
      code[0] |= (u32 & 0x3f) << 26;
      code[1] |= u32 >> 6;
   } else
   if ((code[0] & 0xf) == 0x3 || (code[0] & 0xf) == 4) {
      // integer immediate
      assert((u32 & 0xfff00000) == 0 || (u32 & 0xfff00000) == 0xfff00000);
      assert(!(code[1] & 0xc000));
      u32 &= 0xfffff;
      code[0] |= (u32 & 0x3f) << 26;
      code[1] |= 0xc000 | (u32 >> 6);
   } else {
      // float immediate
      assert(!(u32 & 0x00000fff));
      assert(!(code[1] & 0xc000));
      code[0] |= ((u32 >> 12) & 0x3f) << 26;
      code[1] |= 0xc000 | (u32 >> 18);
   }
}

void CodeEmitterNVC0::setImmediateS8(const ValueRef &ref)
{
   const ImmediateValue *imm = ref.get()->asImm();

   int8_t s8 = static_cast<int8_t>(imm->reg.data.s32);

   assert(s8 == imm->reg.data.s32);

   code[0] |= (s8 & 0x3f) << 26;
   code[0] |= (s8 >> 6) << 8;
}

void
CodeEmitterNVC0::emitForm_A(const Instruction *i, uint64_t opc)
{
   code[0] = opc;
   code[1] = opc >> 32;

   emitPredicate(i);

   defId(i->def(0), 14);

   int s1 = 26;
   if (i->srcExists(2) && i->getSrc(2)->reg.file == FILE_MEMORY_CONST)
      s1 = 49;

   for (int s = 0; s < 3 && i->srcExists(s); ++s) {
      switch (i->getSrc(s)->reg.file) {
      case FILE_MEMORY_CONST:
         assert(!(code[1] & 0xc000));
         code[1] |= (s == 2) ? 0x8000 : 0x4000;
         code[1] |= i->getSrc(s)->reg.fileIndex << 10;
         setAddress16(i->src(s));
         break;
      case FILE_IMMEDIATE:
         assert(s == 1 ||
                i->op == OP_MOV || i->op == OP_PRESIN || i->op == OP_PREEX2);
         assert(!(code[1] & 0xc000));
         setImmediate(i, s);
         break;
      case FILE_GPR:
         if ((s == 2) && ((code[0] & 0x7) == 2)) // LIMM: 3rd src == dst
            break;
         srcId(i->src(s), s ? ((s == 2) ? 49 : s1) : 20);
         break;
      default:
         // ignore here, can be predicate or flags, but must not be address
         break;
      }
   }
}

void
CodeEmitterNVC0::emitForm_B(const Instruction *i, uint64_t opc)
{
   code[0] = opc;
   code[1] = opc >> 32;

   emitPredicate(i);

   defId(i->def(0), 14);

   switch (i->src(0).getFile()) {
   case FILE_MEMORY_CONST:
      assert(!(code[1] & 0xc000));
      code[1] |= 0x4000 | (i->src(0).get()->reg.fileIndex << 10);
      setAddress16(i->src(0));
      break;
   case FILE_IMMEDIATE:
      assert(!(code[1] & 0xc000));
      setImmediate(i, 0);
      break;
   case FILE_GPR:
      srcId(i->src(0), 26);
      break;
   default:
      // ignore here, can be predicate or flags, but must not be address
      break;
   }
}

void
CodeEmitterNVC0::emitForm_S(const Instruction *i, uint32_t opc, bool pred)
{
   code[0] = opc;

   int ss2a = 0;
   if (opc == 0x0d || opc == 0x0e)
      ss2a = 2;

   defId(i->def(0), 14);
   srcId(i->src(0), 20);

   assert(pred || (i->predSrc < 0));
   if (pred)
      emitPredicate(i);

   for (int s = 1; s < 3 && i->srcExists(s); ++s) {
      if (i->src(s).get()->reg.file == FILE_MEMORY_CONST) {
         assert(!(code[0] & (0x300 >> ss2a)));
         switch (i->src(s).get()->reg.fileIndex) {
         case 0:  code[0] |= 0x100 >> ss2a; break;
         case 1:  code[0] |= 0x200 >> ss2a; break;
         case 16: code[0] |= 0x300 >> ss2a; break;
         default:
            ERROR("invalid c[] space for short form\n");
            break;
         }
         if (s == 1)
            code[0] |= i->getSrc(s)->reg.data.offset << 24;
         else
            code[0] |= i->getSrc(s)->reg.data.offset << 6;
      } else
      if (i->src(s).getFile() == FILE_IMMEDIATE) {
         assert(s == 1);
         setImmediateS8(i->src(s));
      } else
      if (i->src(s).getFile() == FILE_GPR) {
         srcId(i->src(s), (s == 1) ? 26 : 8);
      }
   }
}

void
CodeEmitterNVC0::emitShortSrc2(const ValueRef &src)
{
   if (src.getFile() == FILE_MEMORY_CONST) {
      switch (src.get()->reg.fileIndex) {
      case 0:  code[0] |= 0x100; break;
      case 1:  code[0] |= 0x200; break;
      case 16: code[0] |= 0x300; break;
      default:
         assert(!"unsupported file index for short op");
         break;
      }
      srcAddr32(src, 20);
   } else {
      srcId(src, 20);
      assert(src.getFile() == FILE_GPR);
   }
}

void
CodeEmitterNVC0::emitNOP(const Instruction *i)
{
   code[0] = 0x000001e4;
   code[1] = 0x40000000;
   emitPredicate(i);
}

void
CodeEmitterNVC0::emitFMAD(const Instruction *i)
{
   bool neg1 = (i->src(0).mod ^ i->src(1).mod).neg();

   if (i->encSize == 8) {
      if (isLIMM(i->src(1), TYPE_F32)) {
         emitForm_A(i, HEX64(20000000, 00000002));
      } else {
         emitForm_A(i, HEX64(30000000, 00000000));

         if (i->src(2).mod.neg())
            code[0] |= 1 << 8;
      }
      roundMode_A(i);

      if (neg1)
         code[0] |= 1 << 9;

      if (i->saturate)
         code[0] |= 1 << 5;
      if (i->ftz)
         code[0] |= 1 << 6;
   } else {
      assert(!i->saturate && !i->src(2).mod.neg());
      emitForm_S(i, (i->src(2).getFile() == FILE_MEMORY_CONST) ? 0x2e : 0x0e,
                 false);
      if (neg1)
         code[0] |= 1 << 4;
   }
}

void
CodeEmitterNVC0::emitFMUL(const Instruction *i)
{
   bool neg = (i->src(0).mod ^ i->src(1).mod).neg();

   assert(i->postFactor >= -3 && i->postFactor <= 3);

   if (i->encSize == 8) {
      if (isLIMM(i->src(1), TYPE_F32)) {
         assert(i->postFactor == 0); // constant folded, hopefully
         emitForm_A(i, HEX64(30000000, 00000002));
      } else {
         emitForm_A(i, HEX64(58000000, 00000000));
         roundMode_A(i);
         code[1] |= ((i->postFactor > 0) ?
                     (7 - i->postFactor) : (0 - i->postFactor)) << 17;
      }
      if (neg)
         code[1] ^= 1 << 25; // aliases with LIMM sign bit

      if (i->saturate)
         code[0] |= 1 << 5;

      if (i->dnz)
         code[0] |= 1 << 7;
      else
      if (i->ftz)
         code[0] |= 1 << 6;
   } else {
      assert(!neg && !i->saturate && !i->ftz && !i->postFactor);
      emitForm_S(i, 0xa8, true);
   }
}

void
CodeEmitterNVC0::emitUMUL(const Instruction *i)
{
   if (i->encSize == 8) {
      if (i->src(1).getFile() == FILE_IMMEDIATE) {
         emitForm_A(i, HEX64(10000000, 00000002));
      } else {
         emitForm_A(i, HEX64(50000000, 00000003));
      }
      if (i->subOp == NV50_IR_SUBOP_MUL_HIGH)
         code[0] |= 1 << 6;
      if (i->sType == TYPE_S32)
         code[0] |= 1 << 5;
      if (i->dType == TYPE_S32)
         code[0] |= 1 << 7;
   } else {
      emitForm_S(i, i->src(1).getFile() == FILE_IMMEDIATE ? 0xaa : 0x2a, true);

      if (i->sType == TYPE_S32)
         code[0] |= 1 << 6;
   }
}

void
CodeEmitterNVC0::emitFADD(const Instruction *i)
{
   if (i->encSize == 8) {
      if (isLIMM(i->src(1), TYPE_F32)) {
         assert(!i->saturate);
         emitForm_A(i, HEX64(28000000, 00000002));

         code[0] |= i->src(0).mod.abs() << 7;
         code[0] |= i->src(0).mod.neg() << 9;

         if (i->src(1).mod.abs())
            code[1] &= 0xfdffffff;
         if ((i->op == OP_SUB) != static_cast<bool>(i->src(1).mod.neg()))
            code[1] ^= 0x02000000;
      } else {
         emitForm_A(i, HEX64(50000000, 00000000));

         roundMode_A(i);
         if (i->saturate)
            code[1] |= 1 << 17;

         emitNegAbs12(i);
         if (i->op == OP_SUB) code[0] ^= 1 << 8;
      }
      if (i->ftz)
         code[0] |= 1 << 5;
   } else {
      assert(!i->saturate && i->op != OP_SUB &&
             !i->src(0).mod.abs() &&
             !i->src(1).mod.neg() && !i->src(1).mod.abs());

      emitForm_S(i, 0x49, true);

      if (i->src(0).mod.neg())
         code[0] |= 1 << 7;
   }
}

void
CodeEmitterNVC0::emitUADD(const Instruction *i)
{
   uint32_t addOp = 0;

   assert(!i->src(0).mod.abs() && !i->src(1).mod.abs());
   assert(!i->src(0).mod.neg() || !i->src(1).mod.neg());

   if (i->src(0).mod.neg())
      addOp |= 0x200;
   if (i->src(1).mod.neg())
      addOp |= 0x100;
   if (i->op == OP_SUB) {
      addOp ^= 0x100;
      assert(addOp != 0x300); // would be add-plus-one
   }

   if (i->encSize == 8) {
      if (isLIMM(i->src(1), TYPE_U32)) {
         emitForm_A(i, HEX64(08000000, 00000002));
         if (i->defExists(1))
            code[1] |= 1 << 26; // write carry
      } else {
         emitForm_A(i, HEX64(48000000, 00000003));
         if (i->defExists(1))
            code[1] |= 1 << 16; // write carry
      }
      code[0] |= addOp;

      if (i->saturate)
         code[0] |= 1 << 5;
      if (i->flagsSrc >= 0) // add carry
         code[0] |= 1 << 6;
   } else {
      assert(!(addOp & 0x100));
      emitForm_S(i, (addOp >> 3) |
                 ((i->src(1).getFile() == FILE_IMMEDIATE) ? 0xac : 0x2c), true);
   }
}

// TODO: shl-add
void
CodeEmitterNVC0::emitIMAD(const Instruction *i)
{
   assert(i->encSize == 8);
   emitForm_A(i, HEX64(20000000, 00000003));

   if (isSignedType(i->dType))
      code[0] |= 1 << 7;
   if (isSignedType(i->sType))
      code[0] |= 1 << 5;

   code[1] |= i->saturate << 24;

   if (i->flagsDef >= 0) code[1] |= 1 << 16;
   if (i->flagsSrc >= 0) code[1] |= 1 << 23;

   if (i->src(2).mod.neg()) code[0] |= 0x10;
   if (i->src(1).mod.neg() ^
       i->src(0).mod.neg()) code[0] |= 0x20;

   if (i->subOp == NV50_IR_SUBOP_MUL_HIGH)
      code[0] |= 1 << 6;
}

void
CodeEmitterNVC0::emitISAD(const Instruction *i)
{
   assert(i->dType == TYPE_S32 || i->dType == TYPE_U32);
   assert(i->encSize == 8);

   emitForm_A(i, HEX64(38000000, 00000003));

   if (i->dType == TYPE_S32)
      code[0] |= 1 << 5;
}

void
CodeEmitterNVC0::emitNOT(Instruction *i)
{
   assert(i->encSize == 8);
   i->setSrc(1, i->src(0));
   emitForm_A(i, HEX64(68000000, 000001c3));
}

void
CodeEmitterNVC0::emitLogicOp(const Instruction *i, uint8_t subOp)
{
   if (i->encSize == 8) {
      if (isLIMM(i->src(1), TYPE_U32)) {
         emitForm_A(i, HEX64(38000000, 00000002));

         if (i->srcExists(2))
            code[1] |= 1 << 26;
      } else {
         emitForm_A(i, HEX64(68000000, 00000003));

         if (i->srcExists(2))
            code[1] |= 1 << 16;
      }
      code[0] |= subOp << 6;

      if (i->srcExists(2)) // carry
         code[0] |= 1 << 5;

      if (i->src(0).mod & Modifier(NV50_IR_MOD_NOT)) code[0] |= 1 << 9;
      if (i->src(1).mod & Modifier(NV50_IR_MOD_NOT)) code[0] |= 1 << 8;
   } else {
      emitForm_S(i, (subOp << 5) |
                 ((i->src(1).getFile() == FILE_IMMEDIATE) ? 0x1d : 0x8d), true);
   }
}

void
CodeEmitterNVC0::emitPOPC(const Instruction *i)
{
   emitForm_A(i, HEX64(54000000, 00000004));

   if (i->src(0).mod & Modifier(NV50_IR_MOD_NOT)) code[0] |= 1 << 9;
   if (i->src(1).mod & Modifier(NV50_IR_MOD_NOT)) code[0] |= 1 << 8;
}

void
CodeEmitterNVC0::emitINSBF(const Instruction *i)
{
   emitForm_A(i, HEX64(28000000, 30000000));
}

void
CodeEmitterNVC0::emitShift(const Instruction *i)
{
   if (i->op == OP_SHR) {
      emitForm_A(i, HEX64(58000000, 00000003)
                 | (isSignedType(i->dType) ? 0x20 : 0x00));
   } else {
      emitForm_A(i, HEX64(60000000, 00000003));
   }

   if (i->subOp == NV50_IR_SUBOP_SHIFT_WRAP)
      code[0] |= 1 << 9;
}

void
CodeEmitterNVC0::emitPreOp(const Instruction *i)
{
   if (i->encSize == 8) {
      emitForm_B(i, HEX64(60000000, 00000000));

      if (i->op == OP_PREEX2)
         code[0] |= 0x20;

      if (i->src(0).mod.abs()) code[0] |= 1 << 6;
      if (i->src(0).mod.neg()) code[0] |= 1 << 8;
   } else {
      emitForm_S(i, i->op == OP_PREEX2 ? 0x74000008 : 0x70000008, true);
   }
}

void
CodeEmitterNVC0::emitSFnOp(const Instruction *i, uint8_t subOp)
{
   if (i->encSize == 8) {
      code[0] = 0x00000000 | (subOp << 26);
      code[1] = 0xc8000000;

      emitPredicate(i);

      defId(i->def(0), 14);
      srcId(i->src(0), 20);

      assert(i->src(0).getFile() == FILE_GPR);

      if (i->saturate) code[0] |= 1 << 5;

      if (i->src(0).mod.abs()) code[0] |= 1 << 7;
      if (i->src(0).mod.neg()) code[0] |= 1 << 9;
   } else {
      emitForm_S(i, 0x80000008 | (subOp << 26), true);

      assert(!i->src(0).mod.neg());
      if (i->src(0).mod.abs()) code[0] |= 1 << 30;
   }
}

void
CodeEmitterNVC0::emitMINMAX(const Instruction *i)
{
   uint64_t op;

   assert(i->encSize == 8);

   op = (i->op == OP_MIN) ? 0x080e000000000000ULL : 0x081e000000000000ULL;

   if (i->ftz)
      op |= 1 << 5;
   else
   if (!isFloatType(i->dType))
      op |= isSignedType(i->dType) ? 0x23 : 0x03;

   emitForm_A(i, op);
   emitNegAbs12(i);
}

void
CodeEmitterNVC0::roundMode_C(const Instruction *i)
{
   switch (i->rnd) {
   case ROUND_M:  code[1] |= 1 << 17; break;
   case ROUND_P:  code[1] |= 2 << 17; break;
   case ROUND_Z:  code[1] |= 3 << 17; break;
   case ROUND_NI: code[0] |= 1 << 7; break;
   case ROUND_MI: code[0] |= 1 << 7; code[1] |= 1 << 17; break;
   case ROUND_PI: code[0] |= 1 << 7; code[1] |= 2 << 17; break;
   case ROUND_ZI: code[0] |= 1 << 7; code[1] |= 3 << 17; break;
   case ROUND_N: break;
   default:
      assert(!"invalid round mode");
      break;
   }
}

void
CodeEmitterNVC0::roundMode_CS(const Instruction *i)
{
   switch (i->rnd) {
   case ROUND_M:
   case ROUND_MI: code[0] |= 1 << 16; break;
   case ROUND_P:
   case ROUND_PI: code[0] |= 2 << 16; break;
   case ROUND_Z:
   case ROUND_ZI: code[0] |= 3 << 16; break;
   default:
      break;
   }
}

void
CodeEmitterNVC0::emitCVT(Instruction *i)
{
   const bool f2f = isFloatType(i->dType) && isFloatType(i->sType);

   switch (i->op) {
   case OP_CEIL:  i->rnd = f2f ? ROUND_PI : ROUND_P; break;
   case OP_FLOOR: i->rnd = f2f ? ROUND_MI : ROUND_M; break;
   case OP_TRUNC: i->rnd = f2f ? ROUND_ZI : ROUND_Z; break;
   default:
      break;
   }

   const bool sat = (i->op == OP_SAT) || i->saturate;
   const bool abs = (i->op == OP_ABS) || i->src(0).mod.abs();
   const bool neg = (i->op == OP_NEG) || i->src(0).mod.neg();

   if (i->encSize == 8) {
      emitForm_B(i, HEX64(10000000, 00000004));

      roundMode_C(i);

      // cvt u16 f32 sets high bits to 0, so we don't have to use Value::Size()
      code[0] |= util_logbase2(typeSizeof(i->dType)) << 20;
      code[0] |= util_logbase2(typeSizeof(i->sType)) << 23;

      if (sat)
         code[0] |= 0x20;
      if (abs)
         code[0] |= 1 << 6;
      if (neg && i->op != OP_ABS)
         code[0] |= 1 << 8;

      if (i->ftz)
         code[1] |= 1 << 23;

      if (isSignedIntType(i->dType))
         code[0] |= 0x080;
      if (isSignedIntType(i->sType))
         code[0] |= 0x200;

      if (isFloatType(i->dType)) {
         if (!isFloatType(i->sType))
            code[1] |= 0x08000000;
      } else {
         if (isFloatType(i->sType))
            code[1] |= 0x04000000;
         else
            code[1] |= 0x0c000000;
      }
   } else {
      if (i->op == OP_CEIL || i->op == OP_FLOOR || i->op == OP_TRUNC) {
         code[0] = 0x298;
      } else
      if (isFloatType(i->dType)) {
         if (isFloatType(i->sType))
            code[0] = 0x098;
         else
            code[0] = 0x088 | (isSignedType(i->sType) ? (1 << 8) : 0);
      } else {
         assert(isFloatType(i->sType));

         code[0] = 0x288 | (isSignedType(i->sType) ? (1 << 8) : 0);
      }

      if (neg) code[0] |= 1 << 16;
      if (sat) code[0] |= 1 << 18;
      if (abs) code[0] |= 1 << 19;

      roundMode_CS(i);
   }
}

void
CodeEmitterNVC0::emitSET(const CmpInstruction *i)
{
   uint32_t hi;
   uint32_t lo = 0;

   if (i->sType == TYPE_F64)
      lo = 0x1;
   else
   if (!isFloatType(i->sType))
      lo = 0x3;

   if (isFloatType(i->dType) || isSignedIntType(i->sType))
      lo |= 0x20;

   switch (i->op) {
   case OP_SET_AND: hi = 0x10000000; break;
   case OP_SET_OR:  hi = 0x10200000; break;
   case OP_SET_XOR: hi = 0x10400000; break;
   default:
      hi = 0x100e0000;
      break;
   }
   emitForm_A(i, (static_cast<uint64_t>(hi) << 32) | lo);

   if (i->op != OP_SET)
      srcId(i->src(2), 32 + 17);

   if (i->def(0).getFile() == FILE_PREDICATE) {
      if (i->sType == TYPE_F32)
         code[1] += 0x10000000;
      else
         code[1] += 0x08000000;

      code[0] &= ~0xfc000;
      defId(i->def(0), 17);
      if (i->defExists(1))
         defId(i->def(1), 14);
      else
         code[0] |= 0x1c000;
   }

   if (i->ftz)
      code[1] |= 1 << 27;

   emitCondCode(i->setCond, 32 + 23);
   emitNegAbs12(i);
}

void
CodeEmitterNVC0::emitSLCT(const CmpInstruction *i)
{
   uint64_t op;

   switch (i->dType) {
   case TYPE_S32:
      op = HEX64(30000000, 00000023);
      break;
   case TYPE_U32:
      op = HEX64(30000000, 00000003);
      break;
   case TYPE_F32:
      op = HEX64(38000000, 00000000);
      break;
   default:
      assert(!"invalid type for SLCT");
      op = 0;
      break;
   }
   emitForm_A(i, op);

   CondCode cc = i->setCond;

   if (i->src(2).mod.neg())
      cc = reverseCondCode(cc);

   emitCondCode(cc, 32 + 23);

   if (i->ftz)
      code[0] |= 1 << 5;
}

void CodeEmitterNVC0::emitSELP(const Instruction *i)
{
   emitForm_A(i, HEX64(20000000, 00000004));

   if (i->cc == CC_NOT_P || i->src(2).mod & Modifier(NV50_IR_MOD_NOT))
      code[1] |= 1 << 20;
}

void CodeEmitterNVC0::emitTEXBAR(const Instruction *i)
{
   code[0] = 0x00000006 | (i->subOp << 26);
   code[1] = 0xf0000000;
   emitPredicate(i);
   emitCondCode(i->flagsSrc >= 0 ? i->cc : CC_ALWAYS, 5);
}

void CodeEmitterNVC0::emitTEXCSAA(const TexInstruction *i)
{
   code[0] = 0x00000086;
   code[1] = 0xd0000000;

   code[1] |= i->tex.r;
   code[1] |= i->tex.s << 8;

   if (i->tex.liveOnly)
      code[0] |= 1 << 9;

   defId(i->def(0), 14);
   srcId(i->src(0), 20);
}

static inline bool
isNextIndependentTex(const TexInstruction *i)
{
   if (!i->next || !isTextureOp(i->next->op))
      return false;
   if (i->getDef(0)->interfers(i->next->getSrc(0)))
      return false;
   return !i->next->srcExists(1) || !i->getDef(0)->interfers(i->next->getSrc(1));
}

void
CodeEmitterNVC0::emitTEX(const TexInstruction *i)
{
   code[0] = 0x00000006;

   if (isNextIndependentTex(i))
      code[0] |= 0x080; // t mode
   else
      code[0] |= 0x100; // p mode

   if (i->tex.liveOnly)
      code[0] |= 1 << 9;

   switch (i->op) {
   case OP_TEX: code[1] = 0x80000000; break;
   case OP_TXB: code[1] = 0x84000000; break;
   case OP_TXL: code[1] = 0x86000000; break;
   case OP_TXF: code[1] = 0x90000000; break;
   case OP_TXG: code[1] = 0xa0000000; break;
   case OP_TXD: code[1] = 0xe0000000; break;
   default:
      assert(!"invalid texture op");
      break;
   }
   if (i->op == OP_TXF) {
      if (!i->tex.levelZero)
         code[1] |= 0x02000000;
   } else
   if (i->tex.levelZero) {
      code[1] |= 0x02000000;
   }

   if (i->op != OP_TXD && i->tex.derivAll)
      code[1] |= 1 << 13;

   defId(i->def(0), 14);
   srcId(i->src(0), 20);

   emitPredicate(i);

   if (i->op == OP_TXG) code[0] |= i->tex.gatherComp << 5;

   code[1] |= i->tex.mask << 14;

   code[1] |= i->tex.r;
   code[1] |= i->tex.s << 8;
   if (i->tex.rIndirectSrc >= 0 || i->tex.sIndirectSrc >= 0)
      code[1] |= 1 << 18; // in 1st source (with array index)

   // texture target:
   code[1] |= (i->tex.target.getDim() - 1) << 20;
   if (i->tex.target.isCube())
      code[1] += 2 << 20;
   if (i->tex.target.isArray())
      code[1] |= 1 << 19;
   if (i->tex.target.isShadow())
      code[1] |= 1 << 24;

   const int src1 = (i->predSrc == 1) ? 2 : 1; // if predSrc == 1, !srcExists(2)

   if (i->srcExists(src1) && i->src(src1).getFile() == FILE_IMMEDIATE) {
      // lzero
      if (i->op == OP_TXL)
         code[1] &= ~(1 << 26);
      else
      if (i->op == OP_TXF)
         code[1] &= ~(1 << 25);
   }
   if (i->tex.target == TEX_TARGET_2D_MS ||
       i->tex.target == TEX_TARGET_2D_MS_ARRAY)
      code[1] |= 1 << 23;

   if (i->tex.useOffsets) // in vecSrc0.w
      code[1] |= 1 << 22;

   srcId(i, src1, 26);
}

void
CodeEmitterNVC0::emitTXQ(const TexInstruction *i)
{
   code[0] = 0x00000086;
   code[1] = 0xc0000000;

   switch (i->tex.query) {
   case TXQ_DIMS:            code[1] |= 0 << 22; break;
   case TXQ_TYPE:            code[1] |= 1 << 22; break;
   case TXQ_SAMPLE_POSITION: code[1] |= 2 << 22; break;
   case TXQ_FILTER:          code[1] |= 3 << 22; break;
   case TXQ_LOD:             code[1] |= 4 << 22; break;
   case TXQ_BORDER_COLOUR:   code[1] |= 5 << 22; break;
   default:
      assert(!"invalid texture query");
      break;
   }

   code[1] |= i->tex.mask << 14;

   code[1] |= i->tex.r;
   code[1] |= i->tex.s << 8;
   if (i->tex.sIndirectSrc >= 0 || i->tex.rIndirectSrc >= 0)
      code[1] |= 1 << 18;

   const int src1 = (i->predSrc == 1) ? 2 : 1; // if predSrc == 1, !srcExists(2)

   defId(i->def(0), 14);
   srcId(i->src(0), 20);
   srcId(i, src1, 26);

   emitPredicate(i);
}

void
CodeEmitterNVC0::emitQUADOP(const Instruction *i, uint8_t qOp, uint8_t laneMask)
{
   code[0] = 0x00000000 | (laneMask << 6);
   code[1] = 0x48000000 | qOp;

   defId(i->def(0), 14);
   srcId(i->src(0), 20);
   srcId(i->srcExists(1) ? i->src(1) : i->src(0), 26);

   if (i->op == OP_QUADOP && progType != Program::TYPE_FRAGMENT)
      code[0] |= 1 << 9; // dall

   emitPredicate(i);
}

void
CodeEmitterNVC0::emitFlow(const Instruction *i)
{
   const FlowInstruction *f = i->asFlow();

   unsigned mask; // bit 0: predicate, bit 1: target

   code[0] = 0x00000007;

   switch (i->op) {
   case OP_BRA:
      code[1] = f->absolute ? 0x00000000 : 0x40000000;
      if (i->srcExists(0) && i->src(0).getFile() == FILE_MEMORY_CONST)
         code[0] |= 0x4000;
      mask = 3;
      break;
   case OP_CALL:
      code[1] = f->absolute ? 0x10000000 : 0x50000000;
      if (i->srcExists(0) && i->src(0).getFile() == FILE_MEMORY_CONST)
         code[0] |= 0x4000;
      mask = 2;
      break;

   case OP_EXIT:    code[1] = 0x80000000; mask = 1; break;
   case OP_RET:     code[1] = 0x90000000; mask = 1; break;
   case OP_DISCARD: code[1] = 0x98000000; mask = 1; break;
   case OP_BREAK:   code[1] = 0xa8000000; mask = 1; break;
   case OP_CONT:    code[1] = 0xb0000000; mask = 1; break;

   case OP_JOINAT:   code[1] = 0x60000000; mask = 2; break;
   case OP_PREBREAK: code[1] = 0x68000000; mask = 2; break;
   case OP_PRECONT:  code[1] = 0x70000000; mask = 2; break;
   case OP_PRERET:   code[1] = 0x78000000; mask = 2; break;

   case OP_QUADON:  code[1] = 0xc0000000; mask = 0; break;
   case OP_QUADPOP: code[1] = 0xc8000000; mask = 0; break;
   case OP_BRKPT:   code[1] = 0xd0000000; mask = 0; break;
   default:
      assert(!"invalid flow operation");
      return;
   }

   if (mask & 1) {
      emitPredicate(i);
      if (i->flagsSrc < 0)
         code[0] |= 0x1e0;
   }

   if (!f)
      return;

   if (f->allWarp)
      code[0] |= 1 << 15;
   if (f->limit)
      code[0] |= 1 << 16;

   if (f->op == OP_CALL) {
      if (f->builtin) {
         assert(f->absolute);
         uint32_t pcAbs = targ->getBuiltinOffset(f->target.builtin);
         addReloc(RelocEntry::TYPE_BUILTIN, 0, pcAbs, 0xfc000000, 26);
         addReloc(RelocEntry::TYPE_BUILTIN, 1, pcAbs, 0x03ffffff, -6);
      } else {
         assert(!f->absolute);
         int32_t pcRel = f->target.fn->binPos - (codeSize + 8);
         code[0] |= (pcRel & 0x3f) << 26;
         code[1] |= (pcRel >> 6) & 0x3ffff;
      }
   } else
   if (mask & 2) {
      int32_t pcRel = f->target.bb->binPos - (codeSize + 8);
      // currently we don't want absolute branches
      assert(!f->absolute);
      code[0] |= (pcRel & 0x3f) << 26;
      code[1] |= (pcRel >> 6) & 0x3ffff;
   }
}

void
CodeEmitterNVC0::emitPFETCH(const Instruction *i)
{
   uint32_t prim = i->src(0).get()->reg.data.u32;

   code[0] = 0x00000006 | ((prim & 0x3f) << 26);
   code[1] = 0x00000000 | (prim >> 6);

   emitPredicate(i);

   defId(i->def(0), 14);
   srcId(i->src(1), 20);
}

void
CodeEmitterNVC0::emitVFETCH(const Instruction *i)
{
   code[0] = 0x00000006;
   code[1] = 0x06000000 | i->src(0).get()->reg.data.offset;

   if (i->perPatch)
      code[0] |= 0x100;
   if (i->getSrc(0)->reg.file == FILE_SHADER_OUTPUT)
      code[0] |= 0x200; // yes, TCPs can read from *outputs* of other threads

   emitPredicate(i);

   code[0] |= ((i->getDef(0)->reg.size / 4) - 1) << 5;

   defId(i->def(0), 14);
   srcId(i->src(0).getIndirect(0), 20);
   srcId(i->src(0).getIndirect(1), 26); // vertex address
}

void
CodeEmitterNVC0::emitEXPORT(const Instruction *i)
{
   unsigned int size = typeSizeof(i->dType);

   code[0] = 0x00000006 | ((size / 4 - 1) << 5);
   code[1] = 0x0a000000 | i->src(0).get()->reg.data.offset;

   assert(!(code[1] & ((size == 12) ? 15 : (size - 1))));

   if (i->perPatch)
      code[0] |= 0x100;

   emitPredicate(i);

   assert(i->src(1).getFile() == FILE_GPR);

   srcId(i->src(0).getIndirect(0), 20);
   srcId(i->src(0).getIndirect(1), 32 + 17); // vertex base address
   srcId(i->src(1), 26);
}

void
CodeEmitterNVC0::emitOUT(const Instruction *i)
{
   code[0] = 0x00000006;
   code[1] = 0x1c000000;

   emitPredicate(i);

   defId(i->def(0), 14); // new secret address
   srcId(i->src(0), 20); // old secret address, should be 0 initially

   assert(i->src(0).getFile() == FILE_GPR);

   if (i->op == OP_EMIT)
      code[0] |= 1 << 5;
   if (i->op == OP_RESTART || i->subOp == NV50_IR_SUBOP_EMIT_RESTART)
      code[0] |= 1 << 6;

   // vertex stream
   if (i->src(1).getFile() == FILE_IMMEDIATE) {
      code[1] |= 0xc000;
      code[0] |= SDATA(i->src(1)).u32 << 26;
   } else {
      srcId(i->src(1), 26);
   }
}

void
CodeEmitterNVC0::emitInterpMode(const Instruction *i)
{
   if (i->encSize == 8) {
      code[0] |= i->ipa << 6; // TODO: INTERP_SAMPLEID
   } else {
      if (i->getInterpMode() == NV50_IR_INTERP_SC)
         code[0] |= 0x80;
      assert(i->op == OP_PINTERP && i->getSampleMode() == 0);
   }
}

void
CodeEmitterNVC0::emitINTERP(const Instruction *i)
{
   const uint32_t base = i->getSrc(0)->reg.data.offset;

   if (i->encSize == 8) {
      code[0] = 0x00000000;
      code[1] = 0xc0000000 | (base & 0xffff);

      if (i->saturate)
         code[0] |= 1 << 5;

      if (i->op == OP_PINTERP)
         srcId(i->src(1), 26);
      else
         code[0] |= 0x3f << 26;

      srcId(i->src(0).getIndirect(0), 20);
   } else {
      assert(i->op == OP_PINTERP);
      code[0] = 0x00000009 | ((base & 0xc) << 6) | ((base >> 4) << 26);
      srcId(i->src(1), 20);
   }
   emitInterpMode(i);

   emitPredicate(i);
   defId(i->def(0), 14);

   if (i->getSampleMode() == NV50_IR_INTERP_OFFSET)
      srcId(i->src(i->op == OP_PINTERP ? 2 : 1), 17);
   else
      code[1] |= 0x3f << 17;
}

void
CodeEmitterNVC0::emitLoadStoreType(DataType ty)
{
   uint8_t val;

   switch (ty) {
   case TYPE_U8:
      val = 0x00;
      break;
   case TYPE_S8:
      val = 0x20;
      break;
   case TYPE_F16:
   case TYPE_U16:
      val = 0x40;
      break;
   case TYPE_S16:
      val = 0x60;
      break;
   case TYPE_F32:
   case TYPE_U32:
   case TYPE_S32:
      val = 0x80;
      break;
   case TYPE_F64:
   case TYPE_U64:
   case TYPE_S64:
      val = 0xa0;
      break;
   case TYPE_B128:
      val = 0xc0;
      break;
   default:
      val = 0x80;
      assert(!"invalid type");
      break;
   }
   code[0] |= val;
}

void
CodeEmitterNVC0::emitCachingMode(CacheMode c)
{
   uint32_t val;

   switch (c) {
   case CACHE_CA:
// case CACHE_WB:
      val = 0x000;
      break;
   case CACHE_CG:
      val = 0x100;
      break;
   case CACHE_CS:
      val = 0x200;
      break;
   case CACHE_CV:
// case CACHE_WT:
      val = 0x300;
      break;
   default:
      val = 0;
      assert(!"invalid caching mode");
      break;
   }
   code[0] |= val;
}

void
CodeEmitterNVC0::emitSTORE(const Instruction *i)
{
   uint32_t opc;

   switch (i->src(0).getFile()) {
   case FILE_MEMORY_GLOBAL: opc = 0x90000000; break;
   case FILE_MEMORY_LOCAL:  opc = 0xc8000000; break;
   case FILE_MEMORY_SHARED: opc = 0xc9000000; break;
   default:
      assert(!"invalid memory file");
      opc = 0;
      break;
   }
   code[0] = 0x00000005;
   code[1] = opc;

   setAddress16(i->src(0));
   srcId(i->src(1), 14);
   srcId(i->src(0).getIndirect(0), 20);

   emitPredicate(i);

   emitLoadStoreType(i->dType);
   emitCachingMode(i->cache);
}

void
CodeEmitterNVC0::emitLOAD(const Instruction *i)
{
   uint32_t opc;

   code[0] = 0x00000005;

   switch (i->src(0).getFile()) {
   case FILE_MEMORY_GLOBAL: opc = 0x80000000; break;
   case FILE_MEMORY_LOCAL:  opc = 0xc0000000; break;
   case FILE_MEMORY_SHARED: opc = 0xc1000000; break;
   case FILE_MEMORY_CONST:
      if (!i->src(0).isIndirect(0) && typeSizeof(i->dType) == 4) {
         emitMOV(i); // not sure if this is any better
         return;
      }
      opc = 0x14000000 | (i->src(0).get()->reg.fileIndex << 10);
      code[0] = 0x00000006 | (i->subOp << 8);
      break;
   default:
      assert(!"invalid memory file");
      opc = 0;
      break;
   }
   code[1] = opc;

   defId(i->def(0), 14);

   setAddress16(i->src(0));
   srcId(i->src(0).getIndirect(0), 20);

   emitPredicate(i);

   emitLoadStoreType(i->dType);
   emitCachingMode(i->cache);
}

uint8_t
CodeEmitterNVC0::getSRegEncoding(const ValueRef& ref)
{
   switch (SDATA(ref).sv.sv) {
   case SV_LANEID:        return 0x00;
   case SV_PHYSID:        return 0x03;
   case SV_VERTEX_COUNT:  return 0x10;
   case SV_INVOCATION_ID: return 0x11;
   case SV_YDIR:          return 0x12;
   case SV_TID:           return 0x21 + SDATA(ref).sv.index;
   case SV_CTAID:         return 0x25 + SDATA(ref).sv.index;
   case SV_NTID:          return 0x29 + SDATA(ref).sv.index;
   case SV_GRIDID:        return 0x2c;
   case SV_NCTAID:        return 0x2d + SDATA(ref).sv.index;
   case SV_LBASE:         return 0x34;
   case SV_SBASE:         return 0x30;
   case SV_CLOCK:         return 0x50 + SDATA(ref).sv.index;
   default:
      assert(!"no sreg for system value");
      return 0;
   }
}

void
CodeEmitterNVC0::emitMOV(const Instruction *i)
{
   if (i->src(0).getFile() == FILE_SYSTEM_VALUE) {
      uint8_t sr = getSRegEncoding(i->src(0));

      if (i->encSize == 8) {
         code[0] = 0x00000004 | (sr << 26);
         code[1] = 0x2c000000;
      } else {
         code[0] = 0x40000008 | (sr << 20);
      }
      defId(i->def(0), 14);

      emitPredicate(i);
   } else
   if (i->encSize == 8) {
      uint64_t opc;

      if (i->src(0).getFile() == FILE_IMMEDIATE)
         opc = HEX64(18000000, 000001e2);
      else
      if (i->src(0).getFile() == FILE_PREDICATE)
         opc = HEX64(080e0000, 1c000004);
      else
         opc = HEX64(28000000, 00000004);

      opc |= i->lanes << 5;

      emitForm_B(i, opc);
   } else {
      uint32_t imm;

      if (i->src(0).getFile() == FILE_IMMEDIATE) {
         imm = SDATA(i->src(0)).u32;
         if (imm & 0xfff00000) {
            assert(!(imm & 0x000fffff));
            code[0] = 0x00000318 | imm;
         } else {
            assert(imm < 0x800 || ((int32_t)imm >= -0x800));
            code[0] = 0x00000118 | (imm << 20);
         }
      } else {
         code[0] = 0x0028;
         emitShortSrc2(i->src(0));
      }
      defId(i->def(0), 14);

      emitPredicate(i);
   }
}

bool
CodeEmitterNVC0::emitInstruction(Instruction *insn)
{
   unsigned int size = insn->encSize;

   if (writeIssueDelays && !(codeSize & 0x3f))
      size += 8;

   if (!insn->encSize) {
      ERROR("skipping unencodable instruction: "); insn->print();
      return false;
   } else
   if (codeSize + size > codeSizeLimit) {
      ERROR("code emitter output buffer too small\n");
      return false;
   }

   if (writeIssueDelays) {
      if (!(codeSize & 0x3f)) {
         code[0] = 0x00000007; // cf issue delay "instruction"
         code[1] = 0x20000000;
         code += 2;
         codeSize += 8;
      }
      const unsigned int id = (codeSize & 0x3f) / 8 - 1;
      uint32_t *data = code - (id * 2 + 2);
      if (id <= 2) {
         data[0] |= insn->sched << (id * 8 + 4);
      } else
      if (id == 3) {
         data[0] |= insn->sched << 28;
         data[1] |= insn->sched >> 4;
      } else {
         data[1] |= insn->sched << ((id - 4) * 8 + 4);
      }
   }

   // assert that instructions with multiple defs don't corrupt registers
   for (int d = 0; insn->defExists(d); ++d)
      assert(insn->asTex() || insn->def(d).rep()->reg.data.id >= 0);

   switch (insn->op) {
   case OP_MOV:
   case OP_RDSV:
      emitMOV(insn);
      break;
   case OP_NOP:
      break;
   case OP_LOAD:
      emitLOAD(insn);
      break;
   case OP_STORE:
      emitSTORE(insn);
      break;
   case OP_LINTERP:
   case OP_PINTERP:
      emitINTERP(insn);
      break;
   case OP_VFETCH:
      emitVFETCH(insn);
      break;
   case OP_EXPORT:
      emitEXPORT(insn);
      break;
   case OP_PFETCH:
      emitPFETCH(insn);
      break;
   case OP_EMIT:
   case OP_RESTART:
      emitOUT(insn);
      break;
   case OP_ADD:
   case OP_SUB:
      if (isFloatType(insn->dType))
         emitFADD(insn);
      else
         emitUADD(insn);
      break;
   case OP_MUL:
      if (isFloatType(insn->dType))
         emitFMUL(insn);
      else
         emitUMUL(insn);
      break;
   case OP_MAD:
   case OP_FMA:
      if (isFloatType(insn->dType))
         emitFMAD(insn);
      else
         emitIMAD(insn);
      break;
   case OP_SAD:
      emitISAD(insn);
      break;
   case OP_NOT:
      emitNOT(insn);
      break;
   case OP_AND:
      emitLogicOp(insn, 0);
      break;
   case OP_OR:
      emitLogicOp(insn, 1);
      break;
   case OP_XOR:
      emitLogicOp(insn, 2);
      break;
   case OP_SHL:
   case OP_SHR:
      emitShift(insn);
      break;
   case OP_SET:
   case OP_SET_AND:
   case OP_SET_OR:
   case OP_SET_XOR:
      emitSET(insn->asCmp());
      break;
   case OP_SELP:
      emitSELP(insn);
      break;
   case OP_SLCT:
      emitSLCT(insn->asCmp());
      break;
   case OP_MIN:
   case OP_MAX:
      emitMINMAX(insn);
      break;
   case OP_ABS:
   case OP_NEG:
   case OP_CEIL:
   case OP_FLOOR:
   case OP_TRUNC:
   case OP_CVT:
   case OP_SAT:
      emitCVT(insn);
      break;
   case OP_RSQ:
      emitSFnOp(insn, 5);
      break;
   case OP_RCP:
      emitSFnOp(insn, 4);
      break;
   case OP_LG2:
      emitSFnOp(insn, 3);
      break;
   case OP_EX2:
      emitSFnOp(insn, 2);
      break;
   case OP_SIN:
      emitSFnOp(insn, 1);
      break;
   case OP_COS:
      emitSFnOp(insn, 0);
      break;
   case OP_PRESIN:
   case OP_PREEX2:
      emitPreOp(insn);
      break;
   case OP_TEX:
   case OP_TXB:
   case OP_TXL:
   case OP_TXD:
   case OP_TXF:
      emitTEX(insn->asTex());
      break;
   case OP_TXQ:
      emitTXQ(insn->asTex());
      break;
   case OP_TEXBAR:
      emitTEXBAR(insn);
      break;
   case OP_BRA:
   case OP_CALL:
   case OP_PRERET:
   case OP_RET:
   case OP_DISCARD:
   case OP_EXIT:
   case OP_PRECONT:
   case OP_CONT:
   case OP_PREBREAK:
   case OP_BREAK:
   case OP_JOINAT:
   case OP_BRKPT:
   case OP_QUADON:
   case OP_QUADPOP:
      emitFlow(insn);
      break;
   case OP_QUADOP:
      emitQUADOP(insn, insn->subOp, insn->lanes);
      break;
   case OP_DFDX:
      emitQUADOP(insn, insn->src(0).mod.neg() ? 0x66 : 0x99, 0x4);
      break;
   case OP_DFDY:
      emitQUADOP(insn, insn->src(0).mod.neg() ? 0x5a : 0xa5, 0x5);
      break;
   case OP_POPCNT:
      emitPOPC(insn);
      break;
   case OP_JOIN:
      emitNOP(insn);
      insn->join = 1;
      break;
   case OP_PHI:
   case OP_UNION:
   case OP_CONSTRAINT:
      ERROR("operation should have been eliminated");
      return false;
   case OP_EXP:
   case OP_LOG:
   case OP_SQRT:
   case OP_POW:
      ERROR("operation should have been lowered\n");
      return false;
   default:
      ERROR("unknow op\n");
      return false;
   }

   if (insn->join) {
      code[0] |= 0x10;
      assert(insn->encSize == 8);
   }

   code += insn->encSize / 4;
   codeSize += insn->encSize;
   return true;
}

uint32_t
CodeEmitterNVC0::getMinEncodingSize(const Instruction *i) const
{
   const Target::OpInfo &info = targ->getOpInfo(i);

   if (writeIssueDelays || info.minEncSize == 8 || 1)
      return 8;

   if (i->ftz || i->saturate || i->join)
      return 8;
   if (i->rnd != ROUND_N)
      return 8;
   if (i->predSrc >= 0 && i->op == OP_MAD)
      return 8;

   if (i->op == OP_PINTERP) {
      if (i->getSampleMode() || 1) // XXX: grr, short op doesn't work
         return 8;
   } else
   if (i->op == OP_MOV && i->lanes != 0xf) {
      return 8;
   }

   for (int s = 0; i->srcExists(s); ++s) {
      if (i->src(s).isIndirect(0))
         return 8;

      if (i->src(s).getFile() == FILE_MEMORY_CONST) {
         if (SDATA(i->src(s)).offset >= 0x100)
            return 8;
         if (i->getSrc(s)->reg.fileIndex > 1 &&
             i->getSrc(s)->reg.fileIndex != 16)
             return 8;
      } else
      if (i->src(s).getFile() == FILE_IMMEDIATE) {
         if (i->dType == TYPE_F32) {
            if (SDATA(i->src(s)).u32 >= 0x100)
               return 8;
         } else {
            if (SDATA(i->src(s)).u32 > 0xff)
               return 8;
         }
      }

      if (i->op == OP_CVT)
         continue;
      if (i->src(s).mod != Modifier(0)) {
         if (i->src(s).mod == Modifier(NV50_IR_MOD_ABS))
            if (i->op != OP_RSQ)
               return 8;
         if (i->src(s).mod == Modifier(NV50_IR_MOD_NEG))
            if (i->op != OP_ADD || s != 0)
               return 8;
      }
   }

   return 4;
}

// Simplified, erring on safe side.
class SchedDataCalculator : public Pass
{
public:
   SchedDataCalculator(const Target *targ) : targ(targ) { }

private:
   struct RegScores
   {
      struct Resource {
         int st[DATA_FILE_COUNT]; // LD to LD delay 3
         int ld[DATA_FILE_COUNT]; // ST to ST delay 3
         int tex; // TEX to non-TEX delay 17 (0x11)
         int sfu; // SFU to SFU delay 3 (except PRE-ops)
         int imul; // integer MUL to MUL delay 3
      } res;
      struct ScoreData {
         int r[64];
         int p[8];
         int c;
      } rd, wr;
      int base;

      void rebase(const int base)
      {
         const int delta = this->base - base;
         if (!delta)
            return;
         this->base = 0;

         for (int i = 0; i < 64; ++i) {
            rd.r[i] += delta;
            wr.r[i] += delta;
         }
         for (int i = 0; i < 8; ++i) {
            rd.p[i] += delta;
            wr.p[i] += delta;
         }
         rd.c += delta;
         wr.c += delta;

         for (unsigned int f = 0; f < DATA_FILE_COUNT; ++f) {
            res.ld[f] += delta;
            res.st[f] += delta;
         }
         res.sfu += delta;
         res.imul += delta;
         res.tex += delta;
      }
      void wipe()
      {
         memset(&rd, 0, sizeof(rd));
         memset(&wr, 0, sizeof(wr));
         memset(&res, 0, sizeof(res));
      }
      int getLatest(const ScoreData& d) const
      {
         int max = 0;
         for (int i = 0; i < 64; ++i)
            if (d.r[i] > max)
               max = d.r[i];
         for (int i = 0; i < 8; ++i)
            if (d.p[i] > max)
               max = d.p[i];
         if (d.c > max)
            max = d.c;
         return max;
      }
      inline int getLatestRd() const
      {
         return getLatest(rd);
      }
      inline int getLatestWr() const
      {
         return getLatest(wr);
      }
      inline int getLatest() const
      {
         const int a = getLatestRd();
         const int b = getLatestWr();

         int max = MAX2(a, b);
         for (unsigned int f = 0; f < DATA_FILE_COUNT; ++f) {
            max = MAX2(res.ld[f], max);
            max = MAX2(res.st[f], max);
         }
         max = MAX2(res.sfu, max);
         max = MAX2(res.imul, max);
         max = MAX2(res.tex, max);
         return max;
      }
      void setMax(const RegScores *that)
      {
         for (int i = 0; i < 64; ++i) {
            rd.r[i] = MAX2(rd.r[i], that->rd.r[i]);
            wr.r[i] = MAX2(wr.r[i], that->wr.r[i]);
         }
         for (int i = 0; i < 8; ++i) {
            rd.p[i] = MAX2(rd.p[i], that->rd.p[i]);
            wr.p[i] = MAX2(wr.p[i], that->wr.p[i]);
         }
         rd.c = MAX2(rd.c, that->rd.c);
         wr.c = MAX2(wr.c, that->wr.c);

         for (unsigned int f = 0; f < DATA_FILE_COUNT; ++f) {
            res.ld[f] = MAX2(res.ld[f], that->res.ld[f]);
            res.st[f] = MAX2(res.st[f], that->res.st[f]);
         }
         res.sfu = MAX2(res.sfu, that->res.sfu);
         res.imul = MAX2(res.imul, that->res.imul);
         res.tex = MAX2(res.tex, that->res.tex);
      }
      void print(int cycle)
      {
         for (int i = 0; i < 64; ++i) {
            if (rd.r[i] > cycle)
               INFO("rd $r%i @ %i\n", i, rd.r[i]);
            if (wr.r[i] > cycle)
               INFO("wr $r%i @ %i\n", i, wr.r[i]);
         }
         for (int i = 0; i < 8; ++i) {
            if (rd.p[i] > cycle)
               INFO("rd $p%i @ %i\n", i, rd.p[i]);
            if (wr.p[i] > cycle)
               INFO("wr $p%i @ %i\n", i, wr.p[i]);
         }
         if (rd.c > cycle)
            INFO("rd $c @ %i\n", rd.c);
         if (wr.c > cycle)
            INFO("wr $c @ %i\n", wr.c);
         if (res.sfu > cycle)
            INFO("sfu @ %i\n", res.sfu);
         if (res.imul > cycle)
            INFO("imul @ %i\n", res.imul);
         if (res.tex > cycle)
            INFO("tex @ %i\n", res.tex);
      }
   };

   RegScores *score; // for current BB
   std::vector<RegScores> scoreBoards;
   int cycle;
   int prevData;
   operation prevOp;

   const Target *targ;

   bool visit(Function *);
   bool visit(BasicBlock *);

   void commitInsn(const Instruction *, int cycle);
   int calcDelay(const Instruction *, int cycle) const;
   void setDelay(Instruction *, int delay, Instruction *next);

   void recordRd(const Value *, const int ready);
   void recordWr(const Value *, const int ready);
   void checkRd(const Value *, int cycle, int& delay) const;
   void checkWr(const Value *, int cycle, int& delay) const;

   int getCycles(const Instruction *, int origDelay) const;
};

void
SchedDataCalculator::setDelay(Instruction *insn, int delay, Instruction *next)
{
   if (insn->op == OP_EXIT)
      delay = MAX2(delay, 14);

   if (insn->op == OP_TEXBAR) {
      // TODO: except if results not used before EXIT
      insn->sched = 0xc2;
   } else
   if (insn->op == OP_JOIN || insn->join) {
      insn->sched = 0x00;
   } else
   if (delay >= 0 || prevData == 0x04 ||
       !next || !targ->canDualIssue(insn, next)) {
      insn->sched = static_cast<uint8_t>(MAX2(delay, 0));
      if (prevOp == OP_EXPORT)
         insn->sched |= 0x40;
      else
         insn->sched |= 0x20;
   } else {
      insn->sched = 0x04; // dual-issue
   }

   if (prevData != 0x04 || prevOp != OP_EXPORT)
      if (insn->sched != 0x04 || insn->op == OP_EXPORT)
         prevOp = insn->op;

   prevData = insn->sched;
}

int
SchedDataCalculator::getCycles(const Instruction *insn, int origDelay) const
{
   if (insn->sched & 0x80) {
      int c = (insn->sched & 0x0f) * 2 + 1;
      if (insn->op == OP_TEXBAR && origDelay > 0)
         c += origDelay;
      return c;
   }
   if (insn->sched & 0x60)
      return (insn->sched & 0x1f) + 1;
   return (insn->sched == 0x04) ? 0 : 32;
}

bool
SchedDataCalculator::visit(Function *func)
{
   scoreBoards.resize(func->cfg.getSize());
   for (size_t i = 0; i < scoreBoards.size(); ++i)
      scoreBoards[i].wipe();
   return true;
}

bool
SchedDataCalculator::visit(BasicBlock *bb)
{
   Instruction *insn;
   Instruction *next = NULL;

   int cycle = 0;

   prevData = 0x00;
   prevOp = OP_NOP;
   score = &scoreBoards.at(bb->getId());

   for (Graph::EdgeIterator ei = bb->cfg.incident(); !ei.end(); ei.next()) {
      BasicBlock *in = BasicBlock::get(ei.getNode());
      if (in->getExit()) {
         if (prevData != 0x04)
            prevData = in->getExit()->sched;
         prevOp = in->getExit()->op;
      }
      if (ei.getType() != Graph::Edge::BACK)
         score->setMax(&scoreBoards.at(in->getId()));
      // back branches will wait until all target dependencies are satisfied
   }
   if (bb->cfg.incidentCount() > 1)
      prevOp = OP_NOP;

#ifdef NVC0_DEBUG_SCHED_DATA
   INFO("=== BB:%i initial scores\n", bb->getId());
   score->print(cycle);
#endif

   for (insn = bb->getEntry(); insn && insn->next; insn = insn->next) {
      next = insn->next;

      commitInsn(insn, cycle);
      int delay = calcDelay(next, cycle);
      setDelay(insn, delay, next);
      cycle += getCycles(insn, delay);

#ifdef NVC0_DEBUG_SCHED_DATA
      INFO("cycle %i, sched %02x\n", cycle, insn->sched);
      insn->print();
      next->print();
#endif
   }
   if (!insn)
      return true;
   commitInsn(insn, cycle);

   int bbDelay = -1;

   for (Graph::EdgeIterator ei = bb->cfg.outgoing(); !ei.end(); ei.next()) {
      BasicBlock *out = BasicBlock::get(ei.getNode());

      if (ei.getType() != Graph::Edge::BACK) {
         // only test the first instruction of the outgoing block
         next = out->getEntry();
         if (next)
            bbDelay = MAX2(bbDelay, calcDelay(next, cycle));
      } else {
         // wait until all dependencies are satisfied
         const int regsFree = score->getLatest();
         next = out->getFirst();
         for (int c = cycle; next && c < regsFree; next = next->next) {
            bbDelay = MAX2(bbDelay, calcDelay(next, c));
            c += getCycles(next, bbDelay);
         }
         next = NULL;
      }
   }
   if (bb->cfg.outgoingCount() != 1)
      next = NULL;
   setDelay(insn, bbDelay, next);
   cycle += getCycles(insn, bbDelay);

   score->rebase(cycle); // common base for initializing out blocks' scores
   return true;
}

#define NVE4_MAX_ISSUE_DELAY 0x1f
int
SchedDataCalculator::calcDelay(const Instruction *insn, int cycle) const
{
   int delay = 0, ready = cycle;

   for (int s = 0; insn->srcExists(s); ++s)
      checkRd(insn->getSrc(s), cycle, delay);
   // WAR & WAW don't seem to matter
   // for (int s = 0; insn->srcExists(s); ++s)
   //   recordRd(insn->getSrc(s), cycle);

   switch (Target::getOpClass(insn->op)) {
   case OPCLASS_SFU:
      ready = score->res.sfu;
      break;
   case OPCLASS_ARITH:
      if (insn->op == OP_MUL && !isFloatType(insn->dType))
         ready = score->res.imul;
      break;
   case OPCLASS_TEXTURE:
      ready = score->res.tex;
      break;
   case OPCLASS_LOAD:
      ready = score->res.ld[insn->src(0).getFile()];
      break;
   case OPCLASS_STORE:
      ready = score->res.st[insn->src(0).getFile()];
      break;
   default:
      break;
   }
   if (Target::getOpClass(insn->op) != OPCLASS_TEXTURE)
      ready = MAX2(ready, score->res.tex);

   delay = MAX2(delay, ready - cycle);

   // if can issue next cycle, delay is 0, not 1
   return MIN2(delay - 1, NVE4_MAX_ISSUE_DELAY);
}

void
SchedDataCalculator::commitInsn(const Instruction *insn, int cycle)
{
   const int ready = cycle + targ->getLatency(insn);

   for (int d = 0; insn->defExists(d); ++d)
      recordWr(insn->getDef(d), ready);
   // WAR & WAW don't seem to matter
   // for (int s = 0; insn->srcExists(s); ++s)
   //   recordRd(insn->getSrc(s), cycle);

   switch (Target::getOpClass(insn->op)) {
   case OPCLASS_SFU:
      score->res.sfu = cycle + 4;
      break;
   case OPCLASS_ARITH:
      if (insn->op == OP_MUL && !isFloatType(insn->dType))
         score->res.imul = cycle + 4;
      break;
   case OPCLASS_TEXTURE:
      score->res.tex = cycle + 18;
      break;
   case OPCLASS_LOAD:
      if (insn->src(0).getFile() == FILE_MEMORY_CONST)
         break;
      score->res.ld[insn->src(0).getFile()] = cycle + 4;
      score->res.st[insn->src(0).getFile()] = ready;
      break;
   case OPCLASS_STORE:
      score->res.st[insn->src(0).getFile()] = cycle + 4;
      score->res.ld[insn->src(0).getFile()] = ready;
      break;
   case OPCLASS_OTHER:
      if (insn->op == OP_TEXBAR)
         score->res.tex = cycle;
      break;
   default:
      break;
   }

#ifdef NVC0_DEBUG_SCHED_DATA
   score->print(cycle);
#endif
}

void
SchedDataCalculator::checkRd(const Value *v, int cycle, int& delay) const
{
   int ready = cycle;
   int a, b;

   switch (v->reg.file) {
   case FILE_GPR:
      a = v->reg.data.id;
      b = a + v->reg.size / 4;
      for (int r = a; r < b; ++r)
         ready = MAX2(ready, score->rd.r[r]);
      break;
   case FILE_PREDICATE:
      ready = MAX2(ready, score->rd.p[v->reg.data.id]);
      break;
   case FILE_FLAGS:
      ready = MAX2(ready, score->rd.c);
      break;
   case FILE_SHADER_INPUT:
   case FILE_SHADER_OUTPUT: // yes, TCPs can read outputs
   case FILE_MEMORY_LOCAL:
   case FILE_MEMORY_CONST:
   case FILE_MEMORY_SHARED:
   case FILE_MEMORY_GLOBAL:
   case FILE_SYSTEM_VALUE:
      // TODO: any restrictions here ?
      break;
   case FILE_IMMEDIATE:
      break;
   default:
      assert(0);
      break;
   }
   if (cycle < ready)
      delay = MAX2(delay, ready - cycle);
}

void
SchedDataCalculator::checkWr(const Value *v, int cycle, int& delay) const
{
   int ready = cycle;
   int a, b;

   switch (v->reg.file) {
   case FILE_GPR:
      a = v->reg.data.id;
      b = a + v->reg.size / 4;
      for (int r = a; r < b; ++r)
         ready = MAX2(ready, score->wr.r[r]);
      break;
   case FILE_PREDICATE:
      ready = MAX2(ready, score->wr.p[v->reg.data.id]);
      break;
   default:
      assert(v->reg.file == FILE_FLAGS);
      ready = MAX2(ready, score->wr.c);
      break;
   }
   if (cycle < ready)
      delay = MAX2(delay, ready - cycle);
}

void
SchedDataCalculator::recordWr(const Value *v, const int ready)
{
   int a = v->reg.data.id;

   if (v->reg.file == FILE_GPR) {
      int b = a + v->reg.size / 4;
      for (int r = a; r < b; ++r)
         score->rd.r[r] = ready;
   } else
   // $c, $pX: shorter issue-to-read delay (at least as exec pred and carry)
   if (v->reg.file == FILE_PREDICATE) {
      score->rd.p[a] = ready + 4;
   } else {
      assert(v->reg.file == FILE_FLAGS);
      score->rd.c = ready + 4;
   }
}

void
SchedDataCalculator::recordRd(const Value *v, const int ready)
{
   int a = v->reg.data.id;

   if (v->reg.file == FILE_GPR) {
      int b = a + v->reg.size / 4;
      for (int r = a; r < b; ++r)
         score->wr.r[r] = ready;
   } else
   if (v->reg.file == FILE_PREDICATE) {
      score->wr.p[a] = ready;
   } else
   if (v->reg.file == FILE_FLAGS) {
      score->wr.c = ready;
   }
}

void
CodeEmitterNVC0::prepareEmission(Function *func)
{
   const Target *targ = func->getProgram()->getTarget();

   CodeEmitter::prepareEmission(func);

   if (targ->hasSWSched) {
      SchedDataCalculator sched(targ);
      sched.run(func, true, true);
   }
}

CodeEmitterNVC0::CodeEmitterNVC0(const TargetNVC0 *target)
   : CodeEmitter(target),
     writeIssueDelays(target->hasSWSched)
{
   code = NULL;
   codeSize = codeSizeLimit = 0;
   relocInfo = NULL;
}

CodeEmitter *
TargetNVC0::getCodeEmitter(Program::Type type)
{
   CodeEmitterNVC0 *emit = new CodeEmitterNVC0(this);
   emit->setProgramType(type);
   return emit;
}

} // namespace nv50_ir
