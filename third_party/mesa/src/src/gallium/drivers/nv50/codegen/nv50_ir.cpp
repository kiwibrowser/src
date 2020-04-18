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
#include "nv50_ir_driver.h"

extern "C" {
#include "nv50/nv50_program.h"
#include "nv50/nv50_debug.h"
}

namespace nv50_ir {

Modifier::Modifier(operation op)
{
   switch (op) {
   case OP_NEG: bits = NV50_IR_MOD_NEG; break;
   case OP_ABS: bits = NV50_IR_MOD_ABS; break;
   case OP_SAT: bits = NV50_IR_MOD_SAT; break;
   case OP_NOT: bits = NV50_IR_MOD_NOT; break;
   default:
      bits = 0;
      break;
   }
}

Modifier Modifier::operator*(const Modifier m) const
{
   unsigned int a, b, c;

   b = m.bits;
   if (this->bits & NV50_IR_MOD_ABS)
      b &= ~NV50_IR_MOD_NEG;

   a = (this->bits ^ b)      & (NV50_IR_MOD_NOT | NV50_IR_MOD_NEG);
   c = (this->bits | m.bits) & (NV50_IR_MOD_ABS | NV50_IR_MOD_SAT);

   return Modifier(a | c);
}

ValueRef::ValueRef(Value *v) : value(NULL), insn(NULL)
{
   indirect[0] = -1;
   indirect[1] = -1;
   usedAsPtr = false;
   set(v);
}

ValueRef::ValueRef(const ValueRef& ref) : value(NULL), insn(ref.insn)
{
   set(ref);
   usedAsPtr = ref.usedAsPtr;
}

ValueRef::~ValueRef()
{
   this->set(NULL);
}

bool ValueRef::getImmediate(ImmediateValue &imm) const
{
   const ValueRef *src = this;
   Modifier m;
   DataType type = src->insn->sType;

   while (src) {
      if (src->mod) {
         if (src->insn->sType != type)
            break;
         m *= src->mod;
      }
      if (src->getFile() == FILE_IMMEDIATE) {
         imm = *(src->value->asImm());
         // The immediate's type isn't required to match its use, it's
         // more of a hint; applying a modifier makes use of that hint.
         imm.reg.type = type;
         m.applyTo(imm);
         return true;
      }

      Instruction *insn = src->value->getUniqueInsn();

      if (insn && insn->op == OP_MOV) {
         src = &insn->src(0);
         if (src->mod)
            WARN("OP_MOV with modifier encountered !\n");
      } else {
         src = NULL;
      }
   }
   return false;
}

ValueDef::ValueDef(Value *v) : value(NULL), insn(NULL)
{
   set(v);
}

ValueDef::ValueDef(const ValueDef& def) : value(NULL), insn(NULL)
{
   set(def.get());
}

ValueDef::~ValueDef()
{
   this->set(NULL);
}

void
ValueRef::set(const ValueRef &ref)
{
   this->set(ref.get());
   mod = ref.mod;
   indirect[0] = ref.indirect[0];
   indirect[1] = ref.indirect[1];
}

void
ValueRef::set(Value *refVal)
{
   if (value == refVal)
      return;
   if (value)
      value->uses.remove(this);
   if (refVal)
      refVal->uses.push_back(this);

   value = refVal;
}

void
ValueDef::set(Value *defVal)
{
   if (value == defVal)
      return;
   if (value)
      value->defs.remove(this);
   if (defVal)
      defVal->defs.push_back(this);

   value = defVal;
}

// Check if we can replace this definition's value by the value in @rep,
// including the source modifiers, i.e. make sure that all uses support
// @rep.mod.
bool
ValueDef::mayReplace(const ValueRef &rep)
{
   if (!rep.mod)
      return true;

   if (!insn || !insn->bb) // Unbound instruction ?
      return false;

   const Target *target = insn->bb->getProgram()->getTarget();

   for (Value::UseIterator it = value->uses.begin(); it != value->uses.end();
        ++it) {
      Instruction *insn = (*it)->getInsn();
      int s = -1;

      for (int i = 0; insn->srcExists(i); ++i) {
         if (insn->src(i).get() == value) {
            // If there are multiple references to us we'd have to check if the
            // combination of mods is still supported, but just bail for now.
            if (&insn->src(i) != (*it))
               return false;
            s = i;
         }
      }
      assert(s >= 0); // integrity of uses list

      if (!target->isModSupported(insn, s, rep.mod))
         return false;
   }
   return true;
}

void
ValueDef::replace(const ValueRef &repVal, bool doSet)
{
   assert(mayReplace(repVal));

   if (value == repVal.get())
      return;

   while (!value->uses.empty()) {
      ValueRef *ref = value->uses.front();
      ref->set(repVal.get());
      ref->mod *= repVal.mod;
   }

   if (doSet)
      set(repVal.get());
}

Value::Value()
{
  join = this;
  memset(&reg, 0, sizeof(reg));
  reg.size = 4;
}

LValue::LValue(Function *fn, DataFile file)
{
   reg.file = file;
   reg.size = (file != FILE_PREDICATE) ? 4 : 1;
   reg.data.id = -1;

   compMask = 0;
   compound = 0;
   ssa = 0;
   fixedReg = 0;
   noSpill = 0;

   fn->add(this, this->id);
}

LValue::LValue(Function *fn, LValue *lval)
{
   assert(lval);

   reg.file = lval->reg.file;
   reg.size = lval->reg.size;
   reg.data.id = -1;

   compMask = 0;
   compound = 0;
   ssa = 0;
   fixedReg = 0;
   noSpill = 0;

   fn->add(this, this->id);
}

LValue *
LValue::clone(ClonePolicy<Function>& pol) const
{
   LValue *that = new_LValue(pol.context(), reg.file);

   pol.set<Value>(this, that);

   that->reg.size = this->reg.size;
   that->reg.type = this->reg.type;
   that->reg.data = this->reg.data;

   return that;
}

bool
LValue::isUniform() const
{
   if (defs.size() > 1)
      return false;
   Instruction *insn = getInsn();
   // let's not try too hard here for now ...
   return !insn->srcExists(1) && insn->getSrc(0)->isUniform();
}

Symbol::Symbol(Program *prog, DataFile f, ubyte fidx)
{
   baseSym = NULL;

   reg.file = f;
   reg.fileIndex = fidx;
   reg.data.offset = 0;

   prog->add(this, this->id);
}

Symbol *
Symbol::clone(ClonePolicy<Function>& pol) const
{
   Program *prog = pol.context()->getProgram();

   Symbol *that = new_Symbol(prog, reg.file, reg.fileIndex);

   pol.set<Value>(this, that);

   that->reg.size = this->reg.size;
   that->reg.type = this->reg.type;
   that->reg.data = this->reg.data;

   that->baseSym = this->baseSym;

   return that;
}

bool
Symbol::isUniform() const
{
   return
      reg.file != FILE_SYSTEM_VALUE &&
      reg.file != FILE_MEMORY_LOCAL &&
      reg.file != FILE_SHADER_INPUT;
}

ImmediateValue::ImmediateValue(Program *prog, uint32_t uval)
{
   memset(&reg, 0, sizeof(reg));

   reg.file = FILE_IMMEDIATE;
   reg.size = 4;
   reg.type = TYPE_U32;

   reg.data.u32 = uval;

   prog->add(this, this->id);
}

ImmediateValue::ImmediateValue(Program *prog, float fval)
{
   memset(&reg, 0, sizeof(reg));

   reg.file = FILE_IMMEDIATE;
   reg.size = 4;
   reg.type = TYPE_F32;

   reg.data.f32 = fval;

   prog->add(this, this->id);
}

ImmediateValue::ImmediateValue(Program *prog, double dval)
{
   memset(&reg, 0, sizeof(reg));

   reg.file = FILE_IMMEDIATE;
   reg.size = 8;
   reg.type = TYPE_F64;

   reg.data.f64 = dval;

   prog->add(this, this->id);
}

ImmediateValue::ImmediateValue(const ImmediateValue *proto, DataType ty)
{
   reg = proto->reg;

   reg.type = ty;
   reg.size = typeSizeof(ty);
}

ImmediateValue *
ImmediateValue::clone(ClonePolicy<Function>& pol) const
{
   Program *prog = pol.context()->getProgram();
   ImmediateValue *that = new_ImmediateValue(prog, 0u);

   pol.set<Value>(this, that);

   that->reg.size = this->reg.size;
   that->reg.type = this->reg.type;
   that->reg.data = this->reg.data;

   return that;
}

bool
ImmediateValue::isInteger(const int i) const
{
   switch (reg.type) {
   case TYPE_S8:
      return reg.data.s8 == i;
   case TYPE_U8:
      return reg.data.u8 == i;
   case TYPE_S16:
      return reg.data.s16 == i;
   case TYPE_U16:
      return reg.data.u16 == i;
   case TYPE_S32:
   case TYPE_U32:
      return reg.data.s32 == i; // as if ...
   case TYPE_F32:
      return reg.data.f32 == static_cast<float>(i);
   case TYPE_F64:
      return reg.data.f64 == static_cast<double>(i);
   default:
      return false;
   }
}

bool
ImmediateValue::isNegative() const
{
   switch (reg.type) {
   case TYPE_S8:  return reg.data.s8 < 0;
   case TYPE_S16: return reg.data.s16 < 0;
   case TYPE_S32:
   case TYPE_U32: return reg.data.s32 < 0;
   case TYPE_F32: return reg.data.u32 & (1 << 31);
   case TYPE_F64: return reg.data.u64 & (1ULL << 63);
   default:
      return false;
   }
}

bool
ImmediateValue::isPow2() const
{
   switch (reg.type) {
   case TYPE_U8:
   case TYPE_U16:
   case TYPE_U32: return util_is_power_of_two(reg.data.u32);
   default:
      return false;
   }
}

void
ImmediateValue::applyLog2()
{
   switch (reg.type) {
   case TYPE_S8:
   case TYPE_S16:
   case TYPE_S32:
      assert(!this->isNegative());
      // fall through
   case TYPE_U8:
   case TYPE_U16:
   case TYPE_U32:
      reg.data.u32 = util_logbase2(reg.data.u32);
      break;
   case TYPE_F32:
      reg.data.f32 = log2f(reg.data.f32);
      break;
   case TYPE_F64:
      reg.data.f64 = log2(reg.data.f64);
      break;
   default:
      assert(0);
      break;
   }
}

bool
ImmediateValue::compare(CondCode cc, float fval) const
{
   if (reg.type != TYPE_F32)
      ERROR("immediate value is not of type f32");

   switch (static_cast<CondCode>(cc & 7)) {
   case CC_TR: return true;
   case CC_FL: return false;
   case CC_LT: return reg.data.f32 <  fval;
   case CC_LE: return reg.data.f32 <= fval;
   case CC_GT: return reg.data.f32 >  fval;
   case CC_GE: return reg.data.f32 >= fval;
   case CC_EQ: return reg.data.f32 == fval;
   case CC_NE: return reg.data.f32 != fval;
   default:
      assert(0);
      return false;
   }
}

ImmediateValue&
ImmediateValue::operator=(const ImmediateValue &that)
{
   this->reg = that.reg;
   return (*this);
}

bool
Value::interfers(const Value *that) const
{
   uint32_t idA, idB;

   if (that->reg.file != reg.file || that->reg.fileIndex != reg.fileIndex)
      return false;
   if (this->asImm())
      return false;

   if (this->asSym()) {
      idA = this->join->reg.data.offset;
      idB = that->join->reg.data.offset;
   } else {
      idA = this->join->reg.data.id * MIN2(this->reg.size, 4);
      idB = that->join->reg.data.id * MIN2(that->reg.size, 4);
   }

   if (idA < idB)
      return (idA + this->reg.size > idB);
   else
   if (idA > idB)
      return (idB + that->reg.size > idA);
   else
      return (idA == idB);
}

bool
Value::equals(const Value *that, bool strict) const
{
   if (strict)
      return this == that;

   if (that->reg.file != reg.file || that->reg.fileIndex != reg.fileIndex)
      return false;
   if (that->reg.size != this->reg.size)
      return false;

   if (that->reg.data.id != this->reg.data.id)
      return false;

   return true;
}

bool
ImmediateValue::equals(const Value *that, bool strict) const
{
   const ImmediateValue *imm = that->asImm();
   if (!imm)
      return false;
   return reg.data.u64 == imm->reg.data.u64;
}

bool
Symbol::equals(const Value *that, bool strict) const
{
   if (reg.file != that->reg.file || reg.fileIndex != that->reg.fileIndex)
      return false;
   assert(that->asSym());

   if (this->baseSym != that->asSym()->baseSym)
      return false;

   return this->reg.data.offset == that->reg.data.offset;
}

void Instruction::init()
{
   next = prev = 0;

   cc = CC_ALWAYS;
   rnd = ROUND_N;
   cache = CACHE_CA;
   subOp = 0;

   saturate = 0;
   join = 0;
   exit = 0;
   terminator = 0;
   ftz = 0;
   dnz = 0;
   atomic = 0;
   perPatch = 0;
   fixed = 0;
   encSize = 0;
   ipa = 0;

   lanes = 0xf;

   postFactor = 0;

   predSrc = -1;
   flagsDef = -1;
   flagsSrc = -1;
}

Instruction::Instruction()
{
   init();

   op = OP_NOP;
   dType = sType = TYPE_F32;

   id = -1;
   bb = 0;
}

Instruction::Instruction(Function *fn, operation opr, DataType ty)
{
   init();

   op = opr;
   dType = sType = ty;

   fn->add(this, id);
}

Instruction::~Instruction()
{
   if (bb) {
      Function *fn = bb->getFunction();
      bb->remove(this);
      fn->allInsns.remove(id);
   }

   for (int s = 0; srcExists(s); ++s)
      setSrc(s, NULL);
   // must unlink defs too since the list pointers will get deallocated
   for (int d = 0; defExists(d); ++d)
      setDef(d, NULL);
}

void
Instruction::setDef(int i, Value *val)
{
   int size = defs.size();
   if (i >= size) {
      defs.resize(i + 1);
      while (size <= i)
         defs[size++].setInsn(this);
   }
   defs[i].set(val);
}

void
Instruction::setSrc(int s, Value *val)
{
   int size = srcs.size();
   if (s >= size) {
      srcs.resize(s + 1);
      while (size <= s)
         srcs[size++].setInsn(this);
   }
   srcs[s].set(val);
}

void
Instruction::setSrc(int s, const ValueRef& ref)
{
   setSrc(s, ref.get());
   srcs[s].mod = ref.mod;
}

void
Instruction::swapSources(int a, int b)
{
   Value *value = srcs[a].get();
   Modifier m = srcs[a].mod;

   setSrc(a, srcs[b]);

   srcs[b].set(value);
   srcs[b].mod = m;
}

// TODO: extend for delta < 0
void
Instruction::moveSources(int s, int delta)
{
   if (delta == 0)
      return;
   assert(delta > 0);

   int k;
   for (k = 0; srcExists(k); ++k) {
      for (int i = 0; i < 2; ++i) {
         if (src(k).indirect[i] >= s)
            src(k).indirect[i] += delta;
      }
   }
   if (predSrc >= s)
      predSrc += delta;
   if (flagsSrc >= s)
      flagsSrc += delta;

   --k;
   for (int p = k + delta; k >= s; --k, --p)
      setSrc(p, src(k));
}

void
Instruction::takeExtraSources(int s, Value *values[3])
{
   values[0] = getIndirect(s, 0);
   if (values[0])
      setIndirect(s, 0, NULL);

   values[1] = getIndirect(s, 1);
   if (values[1])
      setIndirect(s, 1, NULL);

   values[2] = getPredicate();
   if (values[2])
      setPredicate(cc, NULL);
}

void
Instruction::putExtraSources(int s, Value *values[3])
{
   if (values[0])
      setIndirect(s, 0, values[0]);
   if (values[1])
      setIndirect(s, 1, values[1]);
   if (values[2])
      setPredicate(cc, values[2]);
}

Instruction *
Instruction::clone(ClonePolicy<Function>& pol, Instruction *i) const
{
   if (!i)
      i = new_Instruction(pol.context(), op, dType);
#ifndef NDEBUG // non-conformant assert, so this is required
   assert(typeid(*i) == typeid(*this));
#endif

   pol.set<Instruction>(this, i);

   i->sType = sType;

   i->rnd = rnd;
   i->cache = cache;
   i->subOp = subOp;

   i->saturate = saturate;
   i->join = join;
   i->exit = exit;
   i->atomic = atomic;
   i->ftz = ftz;
   i->dnz = dnz;
   i->ipa = ipa;
   i->lanes = lanes;
   i->perPatch = perPatch;

   i->postFactor = postFactor;

   for (int d = 0; defExists(d); ++d)
      i->setDef(d, pol.get(getDef(d)));

   for (int s = 0; srcExists(s); ++s) {
      i->setSrc(s, pol.get(getSrc(s)));
      i->src(s).mod = src(s).mod;
   }

   i->cc = cc;
   i->predSrc = predSrc;
   i->flagsDef = flagsDef;
   i->flagsSrc = flagsSrc;

   return i;
}

unsigned int
Instruction::defCount(unsigned int mask, bool singleFile) const
{
   unsigned int i, n;

   if (singleFile) {
      unsigned int d = ffs(mask);
      if (!d)
         return 0;
      for (i = d--; defExists(i); ++i)
         if (getDef(i)->reg.file != getDef(d)->reg.file)
            mask &= ~(1 << i);
   }

   for (n = 0, i = 0; this->defExists(i); ++i, mask >>= 1)
      n += mask & 1;
   return n;
}

unsigned int
Instruction::srcCount(unsigned int mask, bool singleFile) const
{
   unsigned int i, n;

   if (singleFile) {
      unsigned int s = ffs(mask);
      if (!s)
         return 0;
      for (i = s--; srcExists(i); ++i)
         if (getSrc(i)->reg.file != getSrc(s)->reg.file)
            mask &= ~(1 << i);
   }

   for (n = 0, i = 0; this->srcExists(i); ++i, mask >>= 1)
      n += mask & 1;
   return n;
}

bool
Instruction::setIndirect(int s, int dim, Value *value)
{
   assert(this->srcExists(s));

   int p = srcs[s].indirect[dim];
   if (p < 0) {
      if (!value)
         return true;
      p = srcs.size();
      while (p > 0 && !srcExists(p - 1))
         --p;
   }
   setSrc(p, value);
   srcs[p].usedAsPtr = (value != 0);
   srcs[s].indirect[dim] = value ? p : -1;
   return true;
}

bool
Instruction::setPredicate(CondCode ccode, Value *value)
{
   cc = ccode;

   if (!value) {
      if (predSrc >= 0) {
         srcs[predSrc].set(NULL);
         predSrc = -1;
      }
      return true;
   }

   if (predSrc < 0) {
      predSrc = srcs.size();
      while (predSrc > 0 && !srcExists(predSrc - 1))
         --predSrc;
   }

   setSrc(predSrc, value);
   return true;
}

bool
Instruction::writesPredicate() const
{
   for (int d = 0; defExists(d); ++d)
      if (getDef(d)->inFile(FILE_PREDICATE) || getDef(d)->inFile(FILE_FLAGS))
         return true;
   return false;
}

static bool
insnCheckCommutationDefSrc(const Instruction *a, const Instruction *b)
{
   for (int d = 0; a->defExists(d); ++d)
      for (int s = 0; b->srcExists(s); ++s)
         if (a->getDef(d)->interfers(b->getSrc(s)))
            return false;
   return true;
}

static bool
insnCheckCommutationDefDef(const Instruction *a, const Instruction *b)
{
   for (int d = 0; a->defExists(d); ++d)
      for (int c = 0; b->defExists(c); ++c)
         if (a->getDef(d)->interfers(b->getDef(c)))
            return false;
   return true;
}

bool
Instruction::isCommutationLegal(const Instruction *i) const
{
   bool ret = insnCheckCommutationDefDef(this, i);
   ret = ret && insnCheckCommutationDefSrc(this, i);
   ret = ret && insnCheckCommutationDefSrc(i, this);
   return ret;
}

TexInstruction::TexInstruction(Function *fn, operation op)
   : Instruction(fn, op, TYPE_F32)
{
   memset(&tex, 0, sizeof(tex));

   tex.rIndirectSrc = -1;
   tex.sIndirectSrc = -1;
}

TexInstruction::~TexInstruction()
{
   for (int c = 0; c < 3; ++c) {
      dPdx[c].set(NULL);
      dPdy[c].set(NULL);
   }
}

TexInstruction *
TexInstruction::clone(ClonePolicy<Function>& pol, Instruction *i) const
{
   TexInstruction *tex = (i ? static_cast<TexInstruction *>(i) :
                          new_TexInstruction(pol.context(), op));

   Instruction::clone(pol, tex);

   tex->tex = this->tex;

   if (op == OP_TXD) {
      for (unsigned int c = 0; c < tex->tex.target.getDim(); ++c) {
         tex->dPdx[c].set(dPdx[c]);
         tex->dPdy[c].set(dPdy[c]);
      }
   }

   return tex;
}

const struct TexInstruction::Target::Desc TexInstruction::Target::descTable[] =
{
   { "1D",                1, 1, false, false, false },
   { "2D",                2, 2, false, false, false },
   { "2D_MS",             2, 2, false, false, false },
   { "3D",                3, 3, false, false, false },
   { "CUBE",              2, 3, false, true,  false },
   { "1D_SHADOW",         1, 1, false, false, true  },
   { "2D_SHADOW",         2, 2, false, false, true  },
   { "CUBE_SHADOW",       2, 3, false, true,  true  },
   { "1D_ARRAY",          1, 2, true,  false, false },
   { "2D_ARRAY",          2, 3, true,  false, false },
   { "2D_MS_ARRAY",       2, 3, true,  false, false },
   { "CUBE_ARRAY",        2, 4, true,  true,  false },
   { "1D_ARRAY_SHADOW",   1, 2, true,  false, true  },
   { "2D_ARRAY_SHADOW",   2, 3, true,  false, true  },
   { "RECT",              2, 2, false, false, false },
   { "RECT_SHADOW",       2, 2, false, false, true  },
   { "CUBE_ARRAY_SHADOW", 2, 4, true,  true,  true  },
   { "BUFFER",            1, 1, false, false, false },
};

CmpInstruction::CmpInstruction(Function *fn, operation op)
   : Instruction(fn, op, TYPE_F32)
{
   setCond = CC_ALWAYS;
}

CmpInstruction *
CmpInstruction::clone(ClonePolicy<Function>& pol, Instruction *i) const
{
   CmpInstruction *cmp = (i ? static_cast<CmpInstruction *>(i) :
                          new_CmpInstruction(pol.context(), op));
   cmp->dType = dType;
   Instruction::clone(pol, cmp);
   cmp->setCond = setCond;
   return cmp;
}

FlowInstruction::FlowInstruction(Function *fn, operation op, void *targ)
   : Instruction(fn, op, TYPE_NONE)
{
   if (op == OP_CALL)
      target.fn = reinterpret_cast<Function *>(targ);
   else
      target.bb = reinterpret_cast<BasicBlock *>(targ);

   if (op == OP_BRA ||
       op == OP_CONT || op == OP_BREAK ||
       op == OP_RET || op == OP_EXIT)
      terminator = 1;
   else
   if (op == OP_JOIN)
      terminator = targ ? 1 : 0;

   allWarp = absolute = limit = builtin = 0;
}

FlowInstruction *
FlowInstruction::clone(ClonePolicy<Function>& pol, Instruction *i) const
{
   FlowInstruction *flow = (i ? static_cast<FlowInstruction *>(i) :
                            new_FlowInstruction(pol.context(), op, NULL));

   Instruction::clone(pol, flow);
   flow->allWarp = allWarp;
   flow->absolute = absolute;
   flow->limit = limit;
   flow->builtin = builtin;

   if (builtin)
      flow->target.builtin = target.builtin;
   else
   if (op == OP_CALL)
      flow->target.fn = target.fn;
   else
   if (target.bb)
      flow->target.bb = pol.get<BasicBlock>(target.bb);

   return flow;
}

Program::Program(Type type, Target *arch)
   : progType(type),
     target(arch),
     mem_Instruction(sizeof(Instruction), 6),
     mem_CmpInstruction(sizeof(CmpInstruction), 4),
     mem_TexInstruction(sizeof(TexInstruction), 4),
     mem_FlowInstruction(sizeof(FlowInstruction), 4),
     mem_LValue(sizeof(LValue), 8),
     mem_Symbol(sizeof(Symbol), 7),
     mem_ImmediateValue(sizeof(ImmediateValue), 7)
{
   code = NULL;
   binSize = 0;

   maxGPR = -1;

   main = new Function(this, "MAIN", ~0);
   calls.insert(&main->call);

   dbgFlags = 0;
   optLevel = 0;

   targetPriv = NULL;
}

Program::~Program()
{
   for (ArrayList::Iterator it = allFuncs.iterator(); !it.end(); it.next())
      delete reinterpret_cast<Function *>(it.get());

   for (ArrayList::Iterator it = allRValues.iterator(); !it.end(); it.next())
      releaseValue(reinterpret_cast<Value *>(it.get()));
}

void Program::releaseInstruction(Instruction *insn)
{
   // TODO: make this not suck so much

   insn->~Instruction();

   if (insn->asCmp())
      mem_CmpInstruction.release(insn);
   else
   if (insn->asTex())
      mem_TexInstruction.release(insn);
   else
   if (insn->asFlow())
      mem_FlowInstruction.release(insn);
   else
      mem_Instruction.release(insn);
}

void Program::releaseValue(Value *value)
{
   value->~Value();

   if (value->asLValue())
      mem_LValue.release(value);
   else
   if (value->asImm())
      mem_ImmediateValue.release(value);
   else
   if (value->asSym())
      mem_Symbol.release(value);
}


} // namespace nv50_ir

extern "C" {

static void
nv50_ir_init_prog_info(struct nv50_ir_prog_info *info)
{
#if defined(PIPE_SHADER_HULL) && defined(PIPE_SHADER_DOMAIN)
   if (info->type == PIPE_SHADER_HULL || info->type == PIPE_SHADER_DOMAIN) {
      info->prop.tp.domain = PIPE_PRIM_MAX;
      info->prop.tp.outputPrim = PIPE_PRIM_MAX;
   }
#endif
   if (info->type == PIPE_SHADER_GEOMETRY) {
      info->prop.gp.instanceCount = 1;
      info->prop.gp.maxVertices = 1;
   }
   info->io.clipDistance = 0xff;
   info->io.pointSize = 0xff;
   info->io.instanceId = 0xff;
   info->io.vertexId = 0xff;
   info->io.edgeFlagIn = 0xff;
   info->io.edgeFlagOut = 0xff;
   info->io.fragDepth = 0xff;
   info->io.sampleMask = 0xff;
   info->io.backFaceColor[0] = info->io.backFaceColor[1] = 0xff;
}
   
int
nv50_ir_generate_code(struct nv50_ir_prog_info *info)
{
   int ret = 0;

   nv50_ir::Program::Type type;

   nv50_ir_init_prog_info(info);

#define PROG_TYPE_CASE(a, b)                                      \
   case PIPE_SHADER_##a: type = nv50_ir::Program::TYPE_##b; break

   switch (info->type) {
   PROG_TYPE_CASE(VERTEX, VERTEX);
// PROG_TYPE_CASE(HULL, TESSELLATION_CONTROL);
// PROG_TYPE_CASE(DOMAIN, TESSELLATION_EVAL);
   PROG_TYPE_CASE(GEOMETRY, GEOMETRY);
   PROG_TYPE_CASE(FRAGMENT, FRAGMENT);
   default:
      type = nv50_ir::Program::TYPE_COMPUTE;
      break;
   }
   INFO_DBG(info->dbgFlags, VERBOSE, "translating program of type %u\n", type);

   nv50_ir::Target *targ = nv50_ir::Target::create(info->target);
   if (!targ)
      return -1;

   nv50_ir::Program *prog = new nv50_ir::Program(type, targ);
   if (!prog)
      return -1;
   prog->dbgFlags = info->dbgFlags;
   prog->optLevel = info->optLevel;

   switch (info->bin.sourceRep) {
#if 0
   case PIPE_IR_LLVM:
   case PIPE_IR_GLSL:
      return -1;
   case PIPE_IR_SM4:
      ret = prog->makeFromSM4(info) ? 0 : -2;
      break;
   case PIPE_IR_TGSI:
#endif
   default:
      ret = prog->makeFromTGSI(info) ? 0 : -2;
      break;
   }
   if (ret < 0)
      goto out;
   if (prog->dbgFlags & NV50_IR_DEBUG_VERBOSE)
      prog->print();

   targ->parseDriverInfo(info);
   prog->getTarget()->runLegalizePass(prog, nv50_ir::CG_STAGE_PRE_SSA);

   prog->convertToSSA();

   if (prog->dbgFlags & NV50_IR_DEBUG_VERBOSE)
      prog->print();

   prog->optimizeSSA(info->optLevel);
   prog->getTarget()->runLegalizePass(prog, nv50_ir::CG_STAGE_SSA);

   if (prog->dbgFlags & NV50_IR_DEBUG_BASIC)
      prog->print();

   if (!prog->registerAllocation()) {
      ret = -4;
      goto out;
   }
   prog->getTarget()->runLegalizePass(prog, nv50_ir::CG_STAGE_POST_RA);

   prog->optimizePostRA(info->optLevel);

   if (!prog->emitBinary(info)) {
      ret = -5;
      goto out;
   }

out:
   INFO_DBG(prog->dbgFlags, VERBOSE, "nv50_ir_generate_code: ret = %i\n", ret);

   info->bin.maxGPR = prog->maxGPR;
   info->bin.code = prog->code;
   info->bin.codeSize = prog->binSize;
   info->bin.tlsSpace = prog->tlsSize;

   delete prog;
   nv50_ir::Target::destroy(targ);

   return ret;
}

} // extern "C"
