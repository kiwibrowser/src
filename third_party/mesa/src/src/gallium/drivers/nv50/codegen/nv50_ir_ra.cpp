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

#include <stack>
#include <limits>

namespace nv50_ir {

#define MAX_REGISTER_FILE_SIZE 256

class RegisterSet
{
public:
   RegisterSet(const Target *);

   void init(const Target *);
   void reset(DataFile, bool resetMax = false);

   void periodicMask(DataFile f, uint32_t lock, uint32_t unlock);
   void intersect(DataFile f, const RegisterSet *);

   bool assign(int32_t& reg, DataFile f, unsigned int size);
   void release(DataFile f, int32_t reg, unsigned int size);
   bool occupy(DataFile f, int32_t reg, unsigned int size);
   bool occupy(const Value *);
   void occupyMask(DataFile f, int32_t reg, uint8_t mask);

   inline int getMaxAssigned(DataFile f) const { return fill[f]; }

   inline unsigned int getFileSize(DataFile f, uint8_t regSize) const
   {
      if (restrictedGPR16Range && f == FILE_GPR && regSize == 2)
         return (last[f] + 1) / 2;
      return last[f] + 1;
   }

   inline unsigned int units(DataFile f, unsigned int size) const
   {
      return size >> unit[f];
   }
   // for regs of size >= 4, id is counted in 4-byte words (like nv50/c0 binary)
   inline unsigned int idToBytes(Value *v) const
   {
      return v->reg.data.id * MIN2(v->reg.size, 4);
   }
   inline unsigned int idToUnits(Value *v) const
   {
      return units(v->reg.file, idToBytes(v));
   }
   inline int bytesToId(Value *v, unsigned int bytes) const
   {
      if (v->reg.size < 4)
         return units(v->reg.file, bytes);
      return bytes / 4;
   }
   inline int unitsToId(DataFile f, int u, uint8_t size) const
   {
      if (u < 0)
         return -1;
      return (size < 4) ? u : ((u << unit[f]) / 4);
   }

   void print() const;

private:
   BitSet bits[LAST_REGISTER_FILE + 1];

   int unit[LAST_REGISTER_FILE + 1]; // log2 of allocation granularity

   int last[LAST_REGISTER_FILE + 1];
   int fill[LAST_REGISTER_FILE + 1];

   const bool restrictedGPR16Range;
};

void
RegisterSet::reset(DataFile f, bool resetMax)
{
   bits[f].fill(0);
   if (resetMax)
      fill[f] = -1;
}

void
RegisterSet::init(const Target *targ)
{
   for (unsigned int rf = 0; rf <= FILE_ADDRESS; ++rf) {
      DataFile f = static_cast<DataFile>(rf);
      last[rf] = targ->getFileSize(f) - 1;
      unit[rf] = targ->getFileUnit(f);
      fill[rf] = -1;
      assert(last[rf] < MAX_REGISTER_FILE_SIZE);
      bits[rf].allocate(last[rf] + 1, true);
   }
}

RegisterSet::RegisterSet(const Target *targ)
  : restrictedGPR16Range(targ->getChipset() < 0xc0)
{
   init(targ);
   for (unsigned int i = 0; i <= LAST_REGISTER_FILE; ++i)
      reset(static_cast<DataFile>(i));
}

void
RegisterSet::periodicMask(DataFile f, uint32_t lock, uint32_t unlock)
{
   bits[f].periodicMask32(lock, unlock);
}

void
RegisterSet::intersect(DataFile f, const RegisterSet *set)
{
   bits[f] |= set->bits[f];
}

void
RegisterSet::print() const
{
   INFO("GPR:");
   bits[FILE_GPR].print();
   INFO("\n");
}

bool
RegisterSet::assign(int32_t& reg, DataFile f, unsigned int size)
{
   reg = bits[f].findFreeRange(size);
   if (reg < 0)
      return false;
   fill[f] = MAX2(fill[f], (int32_t)(reg + size - 1));
   return true;
}

bool
RegisterSet::occupy(const Value *v)
{
   return occupy(v->reg.file, v->reg.data.id, v->reg.size >> unit[v->reg.file]);
}

void
RegisterSet::occupyMask(DataFile f, int32_t reg, uint8_t mask)
{
   bits[f].setMask(reg & ~31, static_cast<uint32_t>(mask) << (reg % 32));
}

bool
RegisterSet::occupy(DataFile f, int32_t reg, unsigned int size)
{
   if (bits[f].testRange(reg, size))
      return false;

   bits[f].setRange(reg, size);

   INFO_DBG(0, REG_ALLOC, "reg occupy: %u[%i] %u\n", f, reg, size);

   fill[f] = MAX2(fill[f], (int32_t)(reg + size - 1));

   return true;
}

void
RegisterSet::release(DataFile f, int32_t reg, unsigned int size)
{
   bits[f].clrRange(reg, size);

   INFO_DBG(0, REG_ALLOC, "reg release: %u[%i] %u\n", f, reg, size);
}

class RegAlloc
{
public:
   RegAlloc(Program *program) : prog(program), sequence(0) { }

   bool exec();
   bool execFunc();

private:
   class PhiMovesPass : public Pass {
   private:
      virtual bool visit(BasicBlock *);
      inline bool needNewElseBlock(BasicBlock *b, BasicBlock *p);
   };

   class ArgumentMovesPass : public Pass {
   private:
      virtual bool visit(BasicBlock *);
   };

   class BuildIntervalsPass : public Pass {
   private:
      virtual bool visit(BasicBlock *);
      void collectLiveValues(BasicBlock *);
      void addLiveRange(Value *, const BasicBlock *, int end);
   };

   class InsertConstraintsPass : public Pass {
   public:
      bool exec(Function *func);
   private:
      virtual bool visit(BasicBlock *);

      bool insertConstraintMoves();

      void condenseDefs(Instruction *);
      void condenseSrcs(Instruction *, const int first, const int last);

      void addHazard(Instruction *i, const ValueRef *src);
      void textureMask(TexInstruction *);
      void addConstraint(Instruction *, int s, int n);
      bool detectConflict(Instruction *, int s);

      // target specific functions, TODO: put in subclass or Target
      void texConstraintNV50(TexInstruction *);
      void texConstraintNVC0(TexInstruction *);
      void texConstraintNVE0(TexInstruction *);

      std::list<Instruction *> constrList;

      const Target *targ;
   };

   bool buildLiveSets(BasicBlock *);

private:
   Program *prog;
   Function *func;

   // instructions in control flow / chronological order
   ArrayList insns;

   int sequence; // for manual passes through CFG
};

typedef std::pair<Value *, Value *> ValuePair;

class SpillCodeInserter
{
public:
   SpillCodeInserter(Function *fn) : func(fn), stackSize(0), stackBase(0) { }

   bool run(const std::list<ValuePair>&);

   Symbol *assignSlot(const Interval&, unsigned int size);
   inline int32_t getStackSize() const { return stackSize; }

private:
   Function *func;

   struct SpillSlot
   {
      Interval occup;
      std::list<Value *> residents; // needed to recalculate occup
      Symbol *sym;
      int32_t offset;
      inline uint8_t size() const { return sym->reg.size; }
   };
   std::list<SpillSlot> slots;
   int32_t stackSize;
   int32_t stackBase;

   LValue *unspill(Instruction *usei, LValue *, Value *slot);
   void spill(Instruction *defi, Value *slot, LValue *);
};

void
RegAlloc::BuildIntervalsPass::addLiveRange(Value *val,
                                           const BasicBlock *bb,
                                           int end)
{
   Instruction *insn = val->getUniqueInsn();

   if (!insn)
      insn = bb->getFirst();

   assert(bb->getFirst()->serial <= bb->getExit()->serial);
   assert(bb->getExit()->serial + 1 >= end);

   int begin = insn->serial;
   if (begin < bb->getEntry()->serial || begin > bb->getExit()->serial)
      begin = bb->getEntry()->serial;

   INFO_DBG(prog->dbgFlags, REG_ALLOC, "%%%i <- live range [%i(%i), %i)\n",
            val->id, begin, insn->serial, end);

   if (begin != end) // empty ranges are only added as hazards for fixed regs
      val->livei.extend(begin, end);
}

bool
RegAlloc::PhiMovesPass::needNewElseBlock(BasicBlock *b, BasicBlock *p)
{
   if (b->cfg.incidentCount() <= 1)
      return false;

   int n = 0;
   for (Graph::EdgeIterator ei = p->cfg.outgoing(); !ei.end(); ei.next())
      if (ei.getType() == Graph::Edge::TREE ||
          ei.getType() == Graph::Edge::FORWARD)
         ++n;
   return (n == 2);
}

// For each operand of each PHI in b, generate a new value by inserting a MOV
// at the end of the block it is coming from and replace the operand with its
// result. This eliminates liveness conflicts and enables us to let values be
// copied to the right register if such a conflict exists nonetheless.
//
// These MOVs are also crucial in making sure the live intervals of phi srces
// are extended until the end of the loop, since they are not included in the
// live-in sets.
bool
RegAlloc::PhiMovesPass::visit(BasicBlock *bb)
{
   Instruction *phi, *mov;
   BasicBlock *pb, *pn;

   std::stack<BasicBlock *> stack;

   for (Graph::EdgeIterator ei = bb->cfg.incident(); !ei.end(); ei.next()) {
      pb = BasicBlock::get(ei.getNode());
      assert(pb);
      if (needNewElseBlock(bb, pb))
         stack.push(pb);
   }
   while (!stack.empty()) {
      pb = stack.top();
      pn = new BasicBlock(func);
      stack.pop();

      pb->cfg.detach(&bb->cfg);
      pb->cfg.attach(&pn->cfg, Graph::Edge::TREE);
      pn->cfg.attach(&bb->cfg, Graph::Edge::FORWARD);

      assert(pb->getExit()->op != OP_CALL);
      if (pb->getExit()->asFlow()->target.bb == bb)
         pb->getExit()->asFlow()->target.bb = pn;
   }

   // insert MOVs (phi->src(j) should stem from j-th in-BB)
   int j = 0;
   for (Graph::EdgeIterator ei = bb->cfg.incident(); !ei.end(); ei.next()) {
      pb = BasicBlock::get(ei.getNode());
      if (!pb->isTerminated())
         pb->insertTail(new_FlowInstruction(func, OP_BRA, bb));

      for (phi = bb->getPhi(); phi && phi->op == OP_PHI; phi = phi->next) {
         mov = new_Instruction(func, OP_MOV, TYPE_U32);

         mov->setSrc(0, phi->getSrc(j));
         mov->setDef(0, new_LValue(func, phi->getDef(0)->asLValue()));
         phi->setSrc(j, mov->getDef(0));

         pb->insertBefore(pb->getExit(), mov);
      }
      ++j;
   }

   return true;
}

bool
RegAlloc::ArgumentMovesPass::visit(BasicBlock *bb)
{
   // Bind function call inputs/outputs to the same physical register
   // the callee uses, inserting moves as appropriate for the case a
   // conflict arises.
   for (Instruction *i = bb->getEntry(); i; i = i->next) {
      FlowInstruction *cal = i->asFlow();
      if (!cal || cal->op != OP_CALL || cal->builtin)
         continue;
      RegisterSet clobberSet(prog->getTarget());

      // Bind input values.
      for (int s = 0; cal->srcExists(s); ++s) {
         LValue *tmp = new_LValue(func, cal->getSrc(s)->asLValue());
         tmp->reg.data.id = cal->target.fn->ins[s].rep()->reg.data.id;

         Instruction *mov =
            new_Instruction(func, OP_MOV, typeOfSize(tmp->reg.size));
         mov->setDef(0, tmp);
         mov->setSrc(0, cal->getSrc(s));
         cal->setSrc(s, tmp);

         bb->insertBefore(cal, mov);
      }

      // Bind output values.
      for (int d = 0; cal->defExists(d); ++d) {
         LValue *tmp = new_LValue(func, cal->getDef(d)->asLValue());
         tmp->reg.data.id = cal->target.fn->outs[d].rep()->reg.data.id;

         Instruction *mov =
            new_Instruction(func, OP_MOV, typeOfSize(tmp->reg.size));
         mov->setSrc(0, tmp);
         mov->setDef(0, cal->getDef(d));
         cal->setDef(d, tmp);

         bb->insertAfter(cal, mov);
         clobberSet.occupy(tmp);
      }

      // Bind clobbered values.
      for (std::deque<Value *>::iterator it = cal->target.fn->clobbers.begin();
           it != cal->target.fn->clobbers.end();
           ++it) {
         if (clobberSet.occupy(*it)) {
            Value *tmp = new_LValue(func, (*it)->asLValue());
            tmp->reg.data.id = (*it)->reg.data.id;
            cal->setDef(cal->defCount(), tmp);
         }
      }
   }

   // Update the clobber set of the function.
   if (BasicBlock::get(func->cfgExit) == bb) {
      func->buildDefSets();
      for (unsigned int i = 0; i < bb->defSet.getSize(); ++i)
         if (bb->defSet.test(i))
            func->clobbers.push_back(func->getLValue(i));
   }

   return true;
}

// Build the set of live-in variables of bb.
bool
RegAlloc::buildLiveSets(BasicBlock *bb)
{
   Function *f = bb->getFunction();
   BasicBlock *bn;
   Instruction *i;
   unsigned int s, d;

   INFO_DBG(prog->dbgFlags, REG_ALLOC, "buildLiveSets(BB:%i)\n", bb->getId());

   bb->liveSet.allocate(func->allLValues.getSize(), false);

   int n = 0;
   for (Graph::EdgeIterator ei = bb->cfg.outgoing(); !ei.end(); ei.next()) {
      bn = BasicBlock::get(ei.getNode());
      if (bn == bb)
         continue;
      if (bn->cfg.visit(sequence))
         if (!buildLiveSets(bn))
            return false;
      if (n++ || bb->liveSet.marker)
         bb->liveSet |= bn->liveSet;
      else
         bb->liveSet = bn->liveSet;
   }
   if (!n && !bb->liveSet.marker)
      bb->liveSet.fill(0);
   bb->liveSet.marker = true;

   if (prog->dbgFlags & NV50_IR_DEBUG_REG_ALLOC) {
      INFO("BB:%i live set of out blocks:\n", bb->getId());
      bb->liveSet.print();
   }

   // if (!bb->getEntry())
   //   return true;

   if (bb == BasicBlock::get(f->cfgExit)) {
      for (std::deque<ValueRef>::iterator it = f->outs.begin();
           it != f->outs.end(); ++it) {
         assert(it->get()->asLValue());
         bb->liveSet.set(it->get()->id);
      }
   }

   for (i = bb->getExit(); i && i != bb->getEntry()->prev; i = i->prev) {
      for (d = 0; i->defExists(d); ++d)
         bb->liveSet.clr(i->getDef(d)->id);
      for (s = 0; i->srcExists(s); ++s)
         if (i->getSrc(s)->asLValue())
            bb->liveSet.set(i->getSrc(s)->id);
   }
   for (i = bb->getPhi(); i && i->op == OP_PHI; i = i->next)
      bb->liveSet.clr(i->getDef(0)->id);

   if (prog->dbgFlags & NV50_IR_DEBUG_REG_ALLOC) {
      INFO("BB:%i live set after propagation:\n", bb->getId());
      bb->liveSet.print();
   }

   return true;
}

void
RegAlloc::BuildIntervalsPass::collectLiveValues(BasicBlock *bb)
{
   BasicBlock *bbA = NULL, *bbB = NULL;

   if (bb->cfg.outgoingCount()) {
      // trickery to save a loop of OR'ing liveSets
      // aliasing works fine with BitSet::setOr
      for (Graph::EdgeIterator ei = bb->cfg.outgoing(); !ei.end(); ei.next()) {
         if (ei.getType() == Graph::Edge::DUMMY)
            continue;
         if (bbA) {
            bb->liveSet.setOr(&bbA->liveSet, &bbB->liveSet);
            bbA = bb;
         } else {
            bbA = bbB;
         }
         bbB = BasicBlock::get(ei.getNode());
      }
      bb->liveSet.setOr(&bbB->liveSet, bbA ? &bbA->liveSet : NULL);
   } else
   if (bb->cfg.incidentCount()) {
      bb->liveSet.fill(0);
   }
}

bool
RegAlloc::BuildIntervalsPass::visit(BasicBlock *bb)
{
   collectLiveValues(bb);

   INFO_DBG(prog->dbgFlags, REG_ALLOC, "BuildIntervals(BB:%i)\n", bb->getId());

   // go through out blocks and delete phi sources that do not originate from
   // the current block from the live set
   for (Graph::EdgeIterator ei = bb->cfg.outgoing(); !ei.end(); ei.next()) {
      BasicBlock *out = BasicBlock::get(ei.getNode());

      for (Instruction *i = out->getPhi(); i && i->op == OP_PHI; i = i->next) {
         bb->liveSet.clr(i->getDef(0)->id);

         for (int s = 0; i->srcExists(s); ++s) {
            assert(i->src(s).getInsn());
            if (i->getSrc(s)->getUniqueInsn()->bb == bb) // XXX: reachableBy ?
               bb->liveSet.set(i->getSrc(s)->id);
            else
               bb->liveSet.clr(i->getSrc(s)->id);
         }
      }
   }

   // remaining live-outs are live until end
   if (bb->getExit()) {
      for (unsigned int j = 0; j < bb->liveSet.getSize(); ++j)
         if (bb->liveSet.test(j))
            addLiveRange(func->getLValue(j), bb, bb->getExit()->serial + 1);
   }

   for (Instruction *i = bb->getExit(); i && i->op != OP_PHI; i = i->prev) {
      for (int d = 0; i->defExists(d); ++d) {
         bb->liveSet.clr(i->getDef(d)->id);
         if (i->getDef(d)->reg.data.id >= 0) // add hazard for fixed regs
            i->getDef(d)->livei.extend(i->serial, i->serial);
      }

      for (int s = 0; i->srcExists(s); ++s) {
         if (!i->getSrc(s)->asLValue())
            continue;
         if (!bb->liveSet.test(i->getSrc(s)->id)) {
            bb->liveSet.set(i->getSrc(s)->id);
            addLiveRange(i->getSrc(s), bb, i->serial);
         }
      }
   }

   if (bb == BasicBlock::get(func->cfg.getRoot())) {
      for (std::deque<ValueDef>::iterator it = func->ins.begin();
           it != func->ins.end(); ++it) {
         if (it->get()->reg.data.id >= 0) // add hazard for fixed regs
            it->get()->livei.extend(0, 1);
      }
   }

   return true;
}


#define JOIN_MASK_PHI        (1 << 0)
#define JOIN_MASK_UNION      (1 << 1)
#define JOIN_MASK_MOV        (1 << 2)
#define JOIN_MASK_TEX        (1 << 3)

class GCRA
{
public:
   GCRA(Function *, SpillCodeInserter&);
   ~GCRA();

   bool allocateRegisters(ArrayList& insns);

   void printNodeInfo() const;

private:
   class RIG_Node : public Graph::Node
   {
   public:
      RIG_Node();

      void init(const RegisterSet&, LValue *);

      void addInterference(RIG_Node *);
      void addRegPreference(RIG_Node *);

      inline LValue *getValue() const
      {
         return reinterpret_cast<LValue *>(data);
      }
      inline void setValue(LValue *lval) { data = lval; }

      inline uint8_t getCompMask() const
      {
         return ((1 << colors) - 1) << (reg & 7);
      }

      static inline RIG_Node *get(const Graph::EdgeIterator& ei)
      {
         return static_cast<RIG_Node *>(ei.getNode());
      }

   public:
      uint32_t degree;
      uint16_t degreeLimit; // if deg < degLimit, node is trivially colourable
      uint16_t colors;

      DataFile f;
      int32_t reg;

      float weight;

      // list pointers for simplify() phase
      RIG_Node *next;
      RIG_Node *prev;

      // union of the live intervals of all coalesced values (we want to retain
      //  the separate intervals for testing interference of compound values)
      Interval livei;

      std::list<RIG_Node *> prefRegs;
   };

private:
   inline RIG_Node *getNode(const LValue *v) const { return &nodes[v->id]; }

   void buildRIG(ArrayList&);
   bool coalesce(ArrayList&);
   bool doCoalesce(ArrayList&, unsigned int mask);
   void calculateSpillWeights();
   void simplify();
   bool selectRegisters();
   void cleanup(const bool success);

   void simplifyEdge(RIG_Node *, RIG_Node *);
   void simplifyNode(RIG_Node *);

   bool coalesceValues(Value *, Value *, bool force);
   void resolveSplitsAndMerges();
   void makeCompound(Instruction *, bool isSplit);

   inline void checkInterference(const RIG_Node *, Graph::EdgeIterator&);

   inline void insertOrderedTail(std::list<RIG_Node *>&, RIG_Node *);
   void checkList(std::list<RIG_Node *>&);

private:
   std::stack<uint32_t> stack;

   // list headers for simplify() phase
   RIG_Node lo[2];
   RIG_Node hi;

   Graph RIG;
   RIG_Node *nodes;
   unsigned int nodeCount;

   Function *func;
   Program *prog;

   static uint8_t relDegree[17][17];

   RegisterSet regs;

   // need to fixup register id for participants of OP_MERGE/SPLIT
   std::list<Instruction *> merges;
   std::list<Instruction *> splits;

   SpillCodeInserter& spill;
   std::list<ValuePair> mustSpill;
};

uint8_t GCRA::relDegree[17][17];

GCRA::RIG_Node::RIG_Node() : Node(NULL), next(this), prev(this)
{
   colors = 0;
}

void
GCRA::printNodeInfo() const
{
   for (unsigned int i = 0; i < nodeCount; ++i) {
      if (!nodes[i].colors)
         continue;
      INFO("RIG_Node[%%%i]($[%u]%i): %u colors, weight %f, deg %u/%u\n X",
           i,
           nodes[i].f,nodes[i].reg,nodes[i].colors,
           nodes[i].weight,
           nodes[i].degree, nodes[i].degreeLimit);

      for (Graph::EdgeIterator ei = nodes[i].outgoing(); !ei.end(); ei.next())
         INFO(" %%%i", RIG_Node::get(ei)->getValue()->id);
      for (Graph::EdgeIterator ei = nodes[i].incident(); !ei.end(); ei.next())
         INFO(" %%%i", RIG_Node::get(ei)->getValue()->id);
      INFO("\n");
   }
}

void
GCRA::RIG_Node::init(const RegisterSet& regs, LValue *lval)
{
   setValue(lval);
   if (lval->reg.data.id >= 0)
      lval->noSpill = lval->fixedReg = 1;

   colors = regs.units(lval->reg.file, lval->reg.size);
   f = lval->reg.file;
   reg = -1;
   if (lval->reg.data.id >= 0)
      reg = regs.idToUnits(lval);

   weight = std::numeric_limits<float>::infinity();
   degree = 0;
   degreeLimit = regs.getFileSize(f, lval->reg.size);

   livei.insert(lval->livei);
}

bool
GCRA::coalesceValues(Value *dst, Value *src, bool force)
{
   LValue *rep = dst->join->asLValue();
   LValue *val = src->join->asLValue();

   if (!force && val->reg.data.id >= 0) {
      rep = src->join->asLValue();
      val = dst->join->asLValue();
   }
   RIG_Node *nRep = &nodes[rep->id];
   RIG_Node *nVal = &nodes[val->id];

   if (src->reg.file != dst->reg.file) {
      if (!force)
         return false;
      WARN("forced coalescing of values in different files !\n");
   }
   if (!force && dst->reg.size != src->reg.size)
      return false;

   if ((rep->reg.data.id >= 0) && (rep->reg.data.id != val->reg.data.id)) {
      if (force) {
         if (val->reg.data.id >= 0)
            WARN("forced coalescing of values in different fixed regs !\n");
      } else {
         if (val->reg.data.id >= 0)
            return false;
         // make sure that there is no overlap with the fixed register of rep
         for (ArrayList::Iterator it = func->allLValues.iterator();
              !it.end(); it.next()) {
            Value *reg = reinterpret_cast<Value *>(it.get())->asLValue();
            assert(reg);
            if (reg->interfers(rep) && reg->livei.overlaps(nVal->livei))
               return false;
         }
      }
   }

   if (!force && nRep->livei.overlaps(nVal->livei))
      return false;

   INFO_DBG(prog->dbgFlags, REG_ALLOC, "joining %%%i($%i) <- %%%i\n",
            rep->id, rep->reg.data.id, val->id);

   // set join pointer of all values joined with val
   for (Value::DefIterator def = val->defs.begin(); def != val->defs.end();
        ++def)
      (*def)->get()->join = rep;
   assert(rep->join == rep && val->join == rep);

   // add val's definitions to rep and extend the live interval of its RIG node
   rep->defs.insert(rep->defs.end(), val->defs.begin(), val->defs.end());
   nRep->livei.unify(nVal->livei);
   return true;
}

bool
GCRA::coalesce(ArrayList& insns)
{
   bool ret = doCoalesce(insns, JOIN_MASK_PHI);
   if (!ret)
      return false;
   switch (func->getProgram()->getTarget()->getChipset() & ~0xf) {
   case 0x50:
   case 0x80:
   case 0x90:
   case 0xa0:
      ret = doCoalesce(insns, JOIN_MASK_UNION | JOIN_MASK_TEX);
      break;
   case 0xc0:
   case 0xd0:
   case 0xe0:
      ret = doCoalesce(insns, JOIN_MASK_UNION);
      break;
   default:
      break;
   }
   if (!ret)
      return false;
   return doCoalesce(insns, JOIN_MASK_MOV);
}

static inline uint8_t makeCompMask(int compSize, int base, int size)
{
   uint8_t m = ((1 << size) - 1) << base;

   switch (compSize) {
   case 1:
      return 0xff;
   case 2:
      m |= (m << 2);
      return (m << 4) | m;
   case 3:
   case 4:
      return (m << 4) | m;
   default:
      assert(compSize <= 8);
      return m;
   }
}

static inline void copyCompound(Value *dst, Value *src)
{
   LValue *ldst = dst->asLValue();
   LValue *lsrc = src->asLValue();

   ldst->compound = lsrc->compound;
   ldst->compMask = lsrc->compMask;
}

void
GCRA::makeCompound(Instruction *insn, bool split)
{
   LValue *rep = (split ? insn->getSrc(0) : insn->getDef(0))->asLValue();

   if (prog->dbgFlags & NV50_IR_DEBUG_REG_ALLOC) {
      INFO("makeCompound(split = %i): ", split);
      insn->print();
   }

   const unsigned int size = getNode(rep)->colors;
   unsigned int base = 0;

   if (!rep->compound)
      rep->compMask = 0xff;
   rep->compound = 1;

   for (int c = 0; split ? insn->defExists(c) : insn->srcExists(c); ++c) {
      LValue *val = (split ? insn->getDef(c) : insn->getSrc(c))->asLValue();

      val->compound = 1;
      if (!val->compMask)
         val->compMask = 0xff;
      val->compMask &= makeCompMask(size, base, getNode(val)->colors);
      assert(val->compMask);

      INFO_DBG(prog->dbgFlags, REG_ALLOC, "compound: %%%i:%02x <- %%%i:%02x\n",
           rep->id, rep->compMask, val->id, val->compMask);

      base += getNode(val)->colors;
   }
   assert(base == size);
}

bool
GCRA::doCoalesce(ArrayList& insns, unsigned int mask)
{
   int c, n;

   for (n = 0; n < insns.getSize(); ++n) {
      Instruction *i;
      Instruction *insn = reinterpret_cast<Instruction *>(insns.get(n));

      switch (insn->op) {
      case OP_PHI:
         if (!(mask & JOIN_MASK_PHI))
            break;
         for (c = 0; insn->srcExists(c); ++c)
            if (!coalesceValues(insn->getDef(0), insn->getSrc(c), false)) {
               // this is bad
               ERROR("failed to coalesce phi operands\n");
               return false;
            }
         break;
      case OP_UNION:
      case OP_MERGE:
         if (!(mask & JOIN_MASK_UNION))
            break;
         for (c = 0; insn->srcExists(c); ++c)
            coalesceValues(insn->getDef(0), insn->getSrc(c), true);
         if (insn->op == OP_MERGE) {
            merges.push_back(insn);
            if (insn->srcExists(1))
               makeCompound(insn, false);
         }
         break;
      case OP_SPLIT:
         if (!(mask & JOIN_MASK_UNION))
            break;
         splits.push_back(insn);
         for (c = 0; insn->defExists(c); ++c)
            coalesceValues(insn->getSrc(0), insn->getDef(c), true);
         makeCompound(insn, true);
         break;
      case OP_MOV:
         if (!(mask & JOIN_MASK_MOV))
            break;
         i = NULL;
         if (!insn->getDef(0)->uses.empty())
            i = insn->getDef(0)->uses.front()->getInsn();
         // if this is a contraint-move there will only be a single use
         if (i && i->op == OP_MERGE) // do we really still need this ?
            break;
         i = insn->getSrc(0)->getUniqueInsn();
         if (i && !i->constrainedDefs()) {
            if (coalesceValues(insn->getDef(0), insn->getSrc(0), false))
               copyCompound(insn->getSrc(0), insn->getDef(0));
         }
         break;
      case OP_TEX:
      case OP_TXB:
      case OP_TXL:
      case OP_TXF:
      case OP_TXQ:
      case OP_TXD:
      case OP_TXG:
      case OP_TEXCSAA:
         if (!(mask & JOIN_MASK_TEX))
            break;
         for (c = 0; insn->srcExists(c) && c != insn->predSrc; ++c)
            coalesceValues(insn->getDef(c), insn->getSrc(c), true);
         break;
      default:
         break;
      }
   }
   return true;
}

void
GCRA::RIG_Node::addInterference(RIG_Node *node)
{
   this->degree += relDegree[node->colors][colors];
   node->degree += relDegree[colors][node->colors];

   this->attach(node, Graph::Edge::CROSS);
}

void
GCRA::RIG_Node::addRegPreference(RIG_Node *node)
{
   prefRegs.push_back(node);
}

GCRA::GCRA(Function *fn, SpillCodeInserter& spill) :
   func(fn),
   regs(fn->getProgram()->getTarget()),
   spill(spill)
{
   prog = func->getProgram();

   // initialize relative degrees array - i takes away from j
   for (int i = 1; i <= 16; ++i)
      for (int j = 1; j <= 16; ++j)
         relDegree[i][j] = j * ((i + j - 1) / j);
}

GCRA::~GCRA()
{
   if (nodes)
      delete[] nodes;
}

void
GCRA::checkList(std::list<RIG_Node *>& lst)
{
   GCRA::RIG_Node *prev = NULL;

   for (std::list<RIG_Node *>::iterator it = lst.begin();
        it != lst.end();
        ++it) {
      assert((*it)->getValue()->join == (*it)->getValue());
      if (prev)
         assert(prev->livei.begin() <= (*it)->livei.begin());
      prev = *it;
   }
}

void
GCRA::insertOrderedTail(std::list<RIG_Node *>& list, RIG_Node *node)
{
   if (node->livei.isEmpty())
      return;
   // only the intervals of joined values don't necessarily arrive in order
   std::list<RIG_Node *>::iterator prev, it;
   for (it = list.end(); it != list.begin(); it = prev) {
      prev = it;
      --prev;
      if ((*prev)->livei.begin() <= node->livei.begin())
         break;
   }
   list.insert(it, node);
}

void
GCRA::buildRIG(ArrayList& insns)
{
   std::list<RIG_Node *> values, active;

   for (std::deque<ValueDef>::iterator it = func->ins.begin();
        it != func->ins.end(); ++it)
      insertOrderedTail(values, getNode(it->get()->asLValue()));

   for (int i = 0; i < insns.getSize(); ++i) {
      Instruction *insn = reinterpret_cast<Instruction *>(insns.get(i));
      for (int d = 0; insn->defExists(d); ++d)
         if (insn->getDef(d)->rep() == insn->getDef(d))
            insertOrderedTail(values, getNode(insn->getDef(d)->asLValue()));
   }
   checkList(values);

   while (!values.empty()) {
      RIG_Node *cur = values.front();

      for (std::list<RIG_Node *>::iterator it = active.begin();
           it != active.end();
           ++it) {
         RIG_Node *node = *it;

         if (node->livei.end() <= cur->livei.begin()) {
            it = active.erase(it);
            --it;
         } else
         if (node->f == cur->f && node->livei.overlaps(cur->livei)) {
            cur->addInterference(node);
         }
      }
      values.pop_front();
      active.push_back(cur);
   }
}

void
GCRA::calculateSpillWeights()
{
   for (unsigned int i = 0; i < nodeCount; ++i) {
      RIG_Node *const n = &nodes[i];
      if (!nodes[i].colors || nodes[i].livei.isEmpty())
         continue;
      if (nodes[i].reg >= 0) {
         // update max reg
         regs.occupy(n->f, n->reg, n->colors);
         continue;
      }
      LValue *val = nodes[i].getValue();

      if (!val->noSpill) {
         int rc = 0;
         for (Value::DefIterator it = val->defs.begin();
              it != val->defs.end();
              ++it)
            rc += (*it)->get()->refCount();

         nodes[i].weight =
            (float)rc * (float)rc / (float)nodes[i].livei.extent();
      }

      if (nodes[i].degree < nodes[i].degreeLimit) {
         int l = 0;
         if (val->reg.size > 4)
            l = 1;
         DLLIST_ADDHEAD(&lo[l], &nodes[i]);
      } else {
         DLLIST_ADDHEAD(&hi, &nodes[i]);
      }
   }
   if (prog->dbgFlags & NV50_IR_DEBUG_REG_ALLOC)
      printNodeInfo();
}

void
GCRA::simplifyEdge(RIG_Node *a, RIG_Node *b)
{
   bool move = b->degree >= b->degreeLimit;

   INFO_DBG(prog->dbgFlags, REG_ALLOC,
            "edge: (%%%i, deg %u/%u) >-< (%%%i, deg %u/%u)\n",
            a->getValue()->id, a->degree, a->degreeLimit,
            b->getValue()->id, b->degree, b->degreeLimit);

   b->degree -= relDegree[a->colors][b->colors];

   move = move && b->degree < b->degreeLimit;
   if (move && !DLLIST_EMPTY(b)) {
      int l = (b->getValue()->reg.size > 4) ? 1 : 0;
      DLLIST_DEL(b);
      DLLIST_ADDTAIL(&lo[l], b);
   }
}

void
GCRA::simplifyNode(RIG_Node *node)
{
   for (Graph::EdgeIterator ei = node->outgoing(); !ei.end(); ei.next())
      simplifyEdge(node, RIG_Node::get(ei));

   for (Graph::EdgeIterator ei = node->incident(); !ei.end(); ei.next())
      simplifyEdge(node, RIG_Node::get(ei));

   DLLIST_DEL(node);
   stack.push(node->getValue()->id);

   INFO_DBG(prog->dbgFlags, REG_ALLOC, "SIMPLIFY: pushed %%%i%s\n",
            node->getValue()->id,
            (node->degree < node->degreeLimit) ? "" : "(spill)");
}

void
GCRA::simplify()
{
   for (;;) {
      if (!DLLIST_EMPTY(&lo[0])) {
         do {
            simplifyNode(lo[0].next);
         } while (!DLLIST_EMPTY(&lo[0]));
      } else
      if (!DLLIST_EMPTY(&lo[1])) {
         simplifyNode(lo[1].next);
      } else
      if (!DLLIST_EMPTY(&hi)) {
         RIG_Node *best = hi.next;
         float bestScore = best->weight / (float)best->degree;
         // spill candidate
         for (RIG_Node *it = best->next; it != &hi; it = it->next) {
            float score = it->weight / (float)it->degree;
            if (score < bestScore) {
               best = it;
               bestScore = score;
            }
         }
         if (isinf(bestScore)) {
            ERROR("no viable spill candidates left\n");
            break;
         }
         simplifyNode(best);
      } else {
         break;
      }
   }
}

void
GCRA::checkInterference(const RIG_Node *node, Graph::EdgeIterator& ei)
{
   const RIG_Node *intf = RIG_Node::get(ei);

   if (intf->reg < 0)
      return;
   const LValue *vA = node->getValue();
   const LValue *vB = intf->getValue();

   const uint8_t intfMask = ((1 << intf->colors) - 1) << (intf->reg & 7);

   if (vA->compound | vB->compound) {
      // NOTE: this only works for >aligned< register tuples !
      for (Value::DefCIterator D = vA->defs.begin(); D != vA->defs.end(); ++D) {
      for (Value::DefCIterator d = vB->defs.begin(); d != vB->defs.end(); ++d) {
         const LValue *vD = (*D)->get()->asLValue();
         const LValue *vd = (*d)->get()->asLValue();

         if (!vD->livei.overlaps(vd->livei)) {
            INFO_DBG(prog->dbgFlags, REG_ALLOC, "(%%%i) X (%%%i): no overlap\n",
                     vD->id, vd->id);
            continue;
         }

         uint8_t mask = vD->compound ? vD->compMask : ~0;
         if (vd->compound) {
            assert(vB->compound);
            mask &= vd->compMask & vB->compMask;
         } else {
            mask &= intfMask;
         }

         INFO_DBG(prog->dbgFlags, REG_ALLOC,
                  "(%%%i)%02x X (%%%i)%02x & %02x: $r%i.%02x\n",
                  vD->id,
                  vD->compound ? vD->compMask : 0xff,
                  vd->id,
                  vd->compound ? vd->compMask : intfMask,
                  vB->compMask, intf->reg & ~7, mask);
         if (mask)
            regs.occupyMask(node->f, intf->reg & ~7, mask);
      }
      }
   } else {
      INFO_DBG(prog->dbgFlags, REG_ALLOC,
               "(%%%i) X (%%%i): $r%i + %u\n",
               vA->id, vB->id, intf->reg, intf->colors);
      regs.occupy(node->f, intf->reg, intf->colors);
   }
}

bool
GCRA::selectRegisters()
{
   INFO_DBG(prog->dbgFlags, REG_ALLOC, "\nSELECT phase\n");

   while (!stack.empty()) {
      RIG_Node *node = &nodes[stack.top()];
      stack.pop();

      regs.reset(node->f);

      INFO_DBG(prog->dbgFlags, REG_ALLOC, "\nNODE[%%%i, %u colors]\n",
               node->getValue()->id, node->colors);

      for (Graph::EdgeIterator ei = node->outgoing(); !ei.end(); ei.next())
         checkInterference(node, ei);
      for (Graph::EdgeIterator ei = node->incident(); !ei.end(); ei.next())
         checkInterference(node, ei);

      if (!node->prefRegs.empty()) {
         for (std::list<RIG_Node *>::const_iterator it = node->prefRegs.begin();
              it != node->prefRegs.end();
              ++it) {
            if ((*it)->reg >= 0 &&
                regs.occupy(node->f, (*it)->reg, node->colors)) {
               node->reg = (*it)->reg;
               break;
            }
         }
      }
      if (node->reg >= 0)
         continue;
      LValue *lval = node->getValue();
      if (prog->dbgFlags & NV50_IR_DEBUG_REG_ALLOC)
         regs.print();
      bool ret = regs.assign(node->reg, node->f, node->colors);
      if (ret) {
         INFO_DBG(prog->dbgFlags, REG_ALLOC, "assigned reg %i\n", node->reg);
         lval->compMask = node->getCompMask();
      } else {
         INFO_DBG(prog->dbgFlags, REG_ALLOC, "must spill: %%%i (size %u)\n",
                  lval->id, lval->reg.size);
         Symbol *slot = NULL;
         if (lval->reg.file == FILE_GPR)
            slot = spill.assignSlot(node->livei, lval->reg.size);
         mustSpill.push_back(ValuePair(lval, slot));
      }
   }
   if (!mustSpill.empty())
      return false;
   for (unsigned int i = 0; i < nodeCount; ++i) {
      LValue *lval = nodes[i].getValue();
      if (nodes[i].reg >= 0 && nodes[i].colors > 0)
         lval->reg.data.id =
            regs.unitsToId(nodes[i].f, nodes[i].reg, lval->reg.size);
   }
   return true;
}

bool
GCRA::allocateRegisters(ArrayList& insns)
{
   bool ret;

   INFO_DBG(prog->dbgFlags, REG_ALLOC,
            "allocateRegisters to %u instructions\n", insns.getSize());

   nodeCount = func->allLValues.getSize();
   nodes = new RIG_Node[nodeCount];
   if (!nodes)
      return false;
   for (unsigned int i = 0; i < nodeCount; ++i) {
      LValue *lval = reinterpret_cast<LValue *>(func->allLValues.get(i));
      if (lval) {
         nodes[i].init(regs, lval);
         RIG.insert(&nodes[i]);
      }
   }

   // coalesce first, we use only 1 RIG node for a group of joined values
   ret = coalesce(insns);
   if (!ret)
      goto out;

   if (func->getProgram()->dbgFlags & NV50_IR_DEBUG_REG_ALLOC)
      func->printLiveIntervals();

   buildRIG(insns);
   calculateSpillWeights();
   simplify();

   ret = selectRegisters();
   if (!ret) {
      INFO_DBG(prog->dbgFlags, REG_ALLOC,
               "selectRegisters failed, inserting spill code ...\n");
      regs.reset(FILE_GPR, true);
      spill.run(mustSpill);
      if (prog->dbgFlags & NV50_IR_DEBUG_REG_ALLOC)
         func->print();
   } else {
      prog->maxGPR = regs.getMaxAssigned(FILE_GPR);
   }

out:
   cleanup(ret);
   return ret;
}

void
GCRA::cleanup(const bool success)
{
   mustSpill.clear();

   for (ArrayList::Iterator it = func->allLValues.iterator();
        !it.end(); it.next()) {
      LValue *lval =  reinterpret_cast<LValue *>(it.get());

      lval->livei.clear();

      lval->compound = 0;
      lval->compMask = 0;

      if (lval->join == lval)
         continue;

      if (success) {
         lval->reg.data.id = lval->join->reg.data.id;
      } else {
         for (Value::DefIterator d = lval->defs.begin(); d != lval->defs.end();
              ++d)
            lval->join->defs.remove(*d);
         lval->join = lval;
      }
   }

   if (success)
      resolveSplitsAndMerges();
   splits.clear(); // avoid duplicate entries on next coalesce pass
   merges.clear();

   delete[] nodes;
   nodes = NULL;
}

Symbol *
SpillCodeInserter::assignSlot(const Interval &livei, unsigned int size)
{
   SpillSlot slot;
   int32_t offsetBase = stackSize;
   int32_t offset;
   std::list<SpillSlot>::iterator pos = slots.end(), it = slots.begin();

   if (offsetBase % size)
      offsetBase += size - (offsetBase % size);

   slot.sym = NULL;

   for (offset = offsetBase; offset < stackSize; offset += size) {
      while (it != slots.end() && it->offset < offset)
         ++it;
      if (it == slots.end()) // no slots left
         break;
      std::list<SpillSlot>::iterator bgn = it;

      while (it != slots.end() && it->offset < (offset + size)) {
         it->occup.print();
         if (it->occup.overlaps(livei))
            break;
         ++it;
      }
      if (it == slots.end() || it->offset >= (offset + size)) {
         // fits
         for (; bgn != slots.end() && bgn->offset < (offset + size); ++bgn) {
            bgn->occup.insert(livei);
            if (bgn->size() == size)
               slot.sym = bgn->sym;
         }
         break;
      }
   }
   if (!slot.sym) {
      stackSize = offset + size;
      slot.offset = offset;
      slot.sym = new_Symbol(func->getProgram(), FILE_MEMORY_LOCAL);
      if (!func->stackPtr)
         offset += func->tlsBase;
      slot.sym->setAddress(NULL, offset);
      slot.sym->reg.size = size;
      slots.insert(pos, slot)->occup.insert(livei);
   }
   return slot.sym;
}

void
SpillCodeInserter::spill(Instruction *defi, Value *slot, LValue *lval)
{
   const DataType ty = typeOfSize(slot->reg.size);

   Instruction *st;
   if (slot->reg.file == FILE_MEMORY_LOCAL) {
      st = new_Instruction(func, OP_STORE, ty);
      st->setSrc(0, slot);
      st->setSrc(1, lval);
      lval->noSpill = 1;
   } else {
      st = new_Instruction(func, OP_CVT, ty);
      st->setDef(0, slot);
      st->setSrc(0, lval);
   }
   defi->bb->insertAfter(defi, st);
}

LValue *
SpillCodeInserter::unspill(Instruction *usei, LValue *lval, Value *slot)
{
   const DataType ty = typeOfSize(slot->reg.size);

   lval = cloneShallow(func, lval);

   Instruction *ld;
   if (slot->reg.file == FILE_MEMORY_LOCAL) {
      lval->noSpill = 1;
      ld = new_Instruction(func, OP_LOAD, ty);
   } else {
      ld = new_Instruction(func, OP_CVT, ty);
   }
   ld->setDef(0, lval);
   ld->setSrc(0, slot);

   usei->bb->insertBefore(usei, ld);
   return lval;
}

bool
SpillCodeInserter::run(const std::list<ValuePair>& lst)
{
   for (std::list<ValuePair>::const_iterator it = lst.begin(); it != lst.end();
        ++it) {
      LValue *lval = it->first->asLValue();
      Symbol *mem = it->second ? it->second->asSym() : NULL;

      for (Value::DefIterator d = lval->defs.begin(); d != lval->defs.end();
           ++d) {
         Value *slot = mem ?
            static_cast<Value *>(mem) : new_LValue(func, FILE_GPR);
         Value *tmp = NULL;
         Instruction *last = NULL;

         LValue *dval = (*d)->get()->asLValue();
         Instruction *defi = (*d)->getInsn();

         // handle uses first or they'll contain the spill stores
         while (!dval->uses.empty()) {
            ValueRef *u = dval->uses.front();
            Instruction *usei = u->getInsn();
            assert(usei);
            if (usei->op == OP_PHI) {
               tmp = (slot->reg.file == FILE_MEMORY_LOCAL) ? NULL : slot;
               last = NULL;
            } else
            if (!last || usei != last->next) { // TODO: sort uses
               tmp = unspill(usei, dval, slot);
               last = usei;
            }
            u->set(tmp);
         }

         assert(defi);
         if (defi->op == OP_PHI) {
            d = lval->defs.erase(d);
            --d;
            if (slot->reg.file == FILE_MEMORY_LOCAL)
               delete_Instruction(func->getProgram(), defi);
            else
               defi->setDef(0, slot);
         } else {
            spill(defi, slot, dval);
         }
      }

   }

   // TODO: We're not trying to reuse old slots in a potential next iteration.
   //  We have to update the slots' livei intervals to be able to do that.
   stackBase = stackSize;
   slots.clear();
   return true;
}

bool
RegAlloc::exec()
{
   for (IteratorRef it = prog->calls.iteratorDFS(false);
        !it->end(); it->next()) {
      func = Function::get(reinterpret_cast<Graph::Node *>(it->get()));

      func->tlsBase = prog->tlsSize;
      if (!execFunc())
         return false;
      prog->tlsSize += func->tlsSize;
   }
   return true;
}

bool
RegAlloc::execFunc()
{
   InsertConstraintsPass insertConstr;
   PhiMovesPass insertPhiMoves;
   ArgumentMovesPass insertArgMoves;
   BuildIntervalsPass buildIntervals;
   SpillCodeInserter insertSpills(func);

   GCRA gcra(func, insertSpills);

   unsigned int i, retries;
   bool ret;

   ret = insertConstr.exec(func);
   if (!ret)
      goto out;

   ret = insertPhiMoves.run(func);
   if (!ret)
      goto out;

   ret = insertArgMoves.run(func);
   if (!ret)
      goto out;

   // TODO: need to fix up spill slot usage ranges to support > 1 retry
   for (retries = 0; retries < 3; ++retries) {
      if (retries && (prog->dbgFlags & NV50_IR_DEBUG_REG_ALLOC))
         INFO("Retry: %i\n", retries);
      if (prog->dbgFlags & NV50_IR_DEBUG_REG_ALLOC)
         func->print();

      // spilling to registers may add live ranges, need to rebuild everything
      ret = true;
      for (sequence = func->cfg.nextSequence(), i = 0;
           ret && i <= func->loopNestingBound;
           sequence = func->cfg.nextSequence(), ++i)
         ret = buildLiveSets(BasicBlock::get(func->cfg.getRoot()));
      if (!ret)
         break;
      func->orderInstructions(this->insns);

      ret = buildIntervals.run(func);
      if (!ret)
         break;
      ret = gcra.allocateRegisters(insns);
      if (ret)
         break; // success
   }
   INFO_DBG(prog->dbgFlags, REG_ALLOC, "RegAlloc done: %i\n", ret);

   func->tlsSize = insertSpills.getStackSize();
out:
   return ret;
}

// TODO: check if modifying Instruction::join here breaks anything
void
GCRA::resolveSplitsAndMerges()
{
   for (std::list<Instruction *>::iterator it = splits.begin();
        it != splits.end();
        ++it) {
      Instruction *split = *it;
      unsigned int reg = regs.idToBytes(split->getSrc(0));
      for (int d = 0; split->defExists(d); ++d) {
         Value *v = split->getDef(d);
         v->reg.data.id = regs.bytesToId(v, reg);
         v->join = v;
         reg += v->reg.size;
      }
   }
   splits.clear();

   for (std::list<Instruction *>::iterator it = merges.begin();
        it != merges.end();
        ++it) {
      Instruction *merge = *it;
      unsigned int reg = regs.idToBytes(merge->getDef(0));
      for (int s = 0; merge->srcExists(s); ++s) {
         Value *v = merge->getSrc(s);
         v->reg.data.id = regs.bytesToId(v, reg);
         v->join = v;
         reg += v->reg.size;
      }
   }
   merges.clear();
}

bool Program::registerAllocation()
{
   RegAlloc ra(this);
   return ra.exec();
}

bool
RegAlloc::InsertConstraintsPass::exec(Function *ir)
{
   constrList.clear();

   bool ret = run(ir, true, true);
   if (ret)
      ret = insertConstraintMoves();
   return ret;
}

// TODO: make part of texture insn
void
RegAlloc::InsertConstraintsPass::textureMask(TexInstruction *tex)
{
   Value *def[4];
   int c, k, d;
   uint8_t mask = 0;

   for (d = 0, k = 0, c = 0; c < 4; ++c) {
      if (!(tex->tex.mask & (1 << c)))
         continue;
      if (tex->getDef(k)->refCount()) {
         mask |= 1 << c;
         def[d++] = tex->getDef(k);
      }
      ++k;
   }
   tex->tex.mask = mask;

   for (c = 0; c < d; ++c)
      tex->setDef(c, def[c]);
   for (; c < 4; ++c)
      tex->setDef(c, NULL);
}

bool
RegAlloc::InsertConstraintsPass::detectConflict(Instruction *cst, int s)
{
   Value *v = cst->getSrc(s);

   // current register allocation can't handle it if a value participates in
   // multiple constraints
   for (Value::UseIterator it = v->uses.begin(); it != v->uses.end(); ++it) {
      if (cst != (*it)->getInsn())
         return true;
   }

   // can start at s + 1 because detectConflict is called on all sources
   for (int c = s + 1; cst->srcExists(c); ++c)
      if (v == cst->getSrc(c))
         return true;

   Instruction *defi = v->getInsn();

   return (!defi || defi->constrainedDefs());
}

void
RegAlloc::InsertConstraintsPass::addConstraint(Instruction *i, int s, int n)
{
   Instruction *cst;
   int d;

   // first, look for an existing identical constraint op
   for (std::list<Instruction *>::iterator it = constrList.begin();
        it != constrList.end();
        ++it) {
      cst = (*it);
      if (!i->bb->dominatedBy(cst->bb))
         break;
      for (d = 0; d < n; ++d)
         if (cst->getSrc(d) != i->getSrc(d + s))
            break;
      if (d >= n) {
         for (d = 0; d < n; ++d, ++s)
            i->setSrc(s, cst->getDef(d));
         return;
      }
   }
   cst = new_Instruction(func, OP_CONSTRAINT, i->dType);

   for (d = 0; d < n; ++s, ++d) {
      cst->setDef(d, new_LValue(func, FILE_GPR));
      cst->setSrc(d, i->getSrc(s));
      i->setSrc(s, cst->getDef(d));
   }
   i->bb->insertBefore(i, cst);

   constrList.push_back(cst);
}

// Add a dummy use of the pointer source of >= 8 byte loads after the load
// to prevent it from being assigned a register which overlapping the load's
// destination, which would produce random corruptions.
void
RegAlloc::InsertConstraintsPass::addHazard(Instruction *i, const ValueRef *src)
{
   Instruction *hzd = new_Instruction(func, OP_NOP, TYPE_NONE);
   hzd->setSrc(0, src->get());
   i->bb->insertAfter(i, hzd);

}

// b32 { %r0 %r1 %r2 %r3 } -> b128 %r0q
void
RegAlloc::InsertConstraintsPass::condenseDefs(Instruction *insn)
{
   uint8_t size = 0;
   int n;
   for (n = 0; insn->defExists(n) && insn->def(n).getFile() == FILE_GPR; ++n)
      size += insn->getDef(n)->reg.size;
   if (n < 2)
      return;
   LValue *lval = new_LValue(func, FILE_GPR);
   lval->reg.size = size;

   Instruction *split = new_Instruction(func, OP_SPLIT, typeOfSize(size));
   split->setSrc(0, lval);
   for (int d = 0; d < n; ++d) {
      split->setDef(d, insn->getDef(d));
      insn->setDef(d, NULL);
   }
   insn->setDef(0, lval);

   for (int k = 1, d = n; insn->defExists(d); ++d, ++k) {
      insn->setDef(k, insn->getDef(d));
      insn->setDef(d, NULL);
   }
   // carry over predicate if any (mainly for OP_UNION uses)
   split->setPredicate(insn->cc, insn->getPredicate());

   insn->bb->insertAfter(insn, split);
   constrList.push_back(split);
}
void
RegAlloc::InsertConstraintsPass::condenseSrcs(Instruction *insn,
                                              const int a, const int b)
{
   uint8_t size = 0;
   if (a >= b)
      return;
   for (int s = a; s <= b; ++s)
      size += insn->getSrc(s)->reg.size;
   if (!size)
      return;
   LValue *lval = new_LValue(func, FILE_GPR);
   lval->reg.size = size;

   Value *save[3];
   insn->takeExtraSources(0, save);

   Instruction *merge = new_Instruction(func, OP_MERGE, typeOfSize(size));
   merge->setDef(0, lval);
   for (int s = a, i = 0; s <= b; ++s, ++i) {
      merge->setSrc(i, insn->getSrc(s));
      insn->setSrc(s, NULL);
   }
   insn->setSrc(a, lval);

   for (int k = a + 1, s = b + 1; insn->srcExists(s); ++s, ++k) {
      insn->setSrc(k, insn->getSrc(s));
      insn->setSrc(s, NULL);
   }
   insn->bb->insertBefore(insn, merge);

   insn->putExtraSources(0, save);

   constrList.push_back(merge);
}

void
RegAlloc::InsertConstraintsPass::texConstraintNVE0(TexInstruction *tex)
{
   textureMask(tex);
   condenseDefs(tex);

   int n = tex->srcCount(0xff, true);
   if (n > 4) {
      condenseSrcs(tex, 0, 3);
      if (n > 5) // NOTE: first call modified positions already
         condenseSrcs(tex, 4 - (4 - 1), n - 1 - (4 - 1));
   } else
   if (n > 1) {
      condenseSrcs(tex, 0, n - 1);
   }
}

void
RegAlloc::InsertConstraintsPass::texConstraintNVC0(TexInstruction *tex)
{
   int n, s;

   textureMask(tex);

   if (tex->op == OP_TXQ) {
      s = tex->srcCount(0xff);
      n = 0;
   } else {
      s = tex->tex.target.getArgCount();
      if (!tex->tex.target.isArray() &&
          (tex->tex.rIndirectSrc >= 0 || tex->tex.sIndirectSrc >= 0))
         ++s;
      if (tex->op == OP_TXD && tex->tex.useOffsets)
         ++s;
      n = tex->srcCount(0xff) - s;
      assert(n <= 4);
   }

   if (s > 1)
      condenseSrcs(tex, 0, s - 1);
   if (n > 1) // NOTE: first call modified positions already
      condenseSrcs(tex, 1, n);

   condenseDefs(tex);
}

void
RegAlloc::InsertConstraintsPass::texConstraintNV50(TexInstruction *tex)
{
   Value *pred = tex->getPredicate();
   if (pred)
      tex->setPredicate(tex->cc, NULL);

   textureMask(tex);

   assert(tex->defExists(0) && tex->srcExists(0));
   // make src and def count match
   int c;
   for (c = 0; tex->srcExists(c) || tex->defExists(c); ++c) {
      if (!tex->srcExists(c))
         tex->setSrc(c, new_LValue(func, tex->getSrc(0)->asLValue()));
      if (!tex->defExists(c))
         tex->setDef(c, new_LValue(func, tex->getDef(0)->asLValue()));
   }
   if (pred)
      tex->setPredicate(tex->cc, pred);
   condenseDefs(tex);
   condenseSrcs(tex, 0, c - 1);
}

// Insert constraint markers for instructions whose multiple sources must be
// located in consecutive registers.
bool
RegAlloc::InsertConstraintsPass::visit(BasicBlock *bb)
{
   TexInstruction *tex;
   Instruction *next;
   int s, size;

   targ = bb->getProgram()->getTarget();

   for (Instruction *i = bb->getEntry(); i; i = next) {
      next = i->next;

      if ((tex = i->asTex())) {
         switch (targ->getChipset() & ~0xf) {
         case 0x50:
         case 0x80:
         case 0x90:
         case 0xa0:
            texConstraintNV50(tex);
            break;
         case 0xc0:
         case 0xd0:
            texConstraintNVC0(tex);
            break;
         case 0xe0:
            texConstraintNVE0(tex);
            break;
         default:
            break;
         }
      } else
      if (i->op == OP_EXPORT || i->op == OP_STORE) {
         for (size = typeSizeof(i->dType), s = 1; size > 0; ++s) {
            assert(i->srcExists(s));
            size -= i->getSrc(s)->reg.size;
         }
         condenseSrcs(i, 1, s - 1);
      } else
      if (i->op == OP_LOAD || i->op == OP_VFETCH) {
         condenseDefs(i);
         if (i->src(0).isIndirect(0) && typeSizeof(i->dType) >= 8)
            addHazard(i, i->src(0).getIndirect(0));
      } else
      if (i->op == OP_UNION) {
         constrList.push_back(i);
      }
   }
   return true;
}

// Insert extra moves so that, if multiple register constraints on a value are
// in conflict, these conflicts can be resolved.
bool
RegAlloc::InsertConstraintsPass::insertConstraintMoves()
{
   for (std::list<Instruction *>::iterator it = constrList.begin();
        it != constrList.end();
        ++it) {
      Instruction *cst = *it;
      Instruction *mov;

      if (cst->op == OP_SPLIT && 0) {
         // spilling splits is annoying, just make sure they're separate
         for (int d = 0; cst->defExists(d); ++d) {
            if (!cst->getDef(d)->refCount())
               continue;
            LValue *lval = new_LValue(func, cst->def(d).getFile());
            const uint8_t size = cst->def(d).getSize();
            lval->reg.size = size;

            mov = new_Instruction(func, OP_MOV, typeOfSize(size));
            mov->setSrc(0, lval);
            mov->setDef(0, cst->getDef(d));
            cst->setDef(d, mov->getSrc(0));
            cst->bb->insertAfter(cst, mov);

            cst->getSrc(0)->asLValue()->noSpill = 1;
            mov->getSrc(0)->asLValue()->noSpill = 1;
         }
      } else
      if (cst->op == OP_MERGE || cst->op == OP_UNION) {
         for (int s = 0; cst->srcExists(s); ++s) {
            const uint8_t size = cst->src(s).getSize();

            if (!cst->getSrc(s)->defs.size()) {
               mov = new_Instruction(func, OP_NOP, typeOfSize(size));
               mov->setDef(0, cst->getSrc(s));
               cst->bb->insertBefore(cst, mov);
               continue;
            }
            assert(cst->getSrc(s)->defs.size() == 1); // still SSA

            Instruction *defi = cst->getSrc(s)->defs.front()->getInsn();
            // catch some cases where don't really need MOVs
            if (cst->getSrc(s)->refCount() == 1 && !defi->constrainedDefs())
               continue;

            LValue *lval = new_LValue(func, cst->src(s).getFile());
            lval->reg.size = size;

            mov = new_Instruction(func, OP_MOV, typeOfSize(size));
            mov->setDef(0, lval);
            mov->setSrc(0, cst->getSrc(s));
            cst->setSrc(s, mov->getDef(0));
            cst->bb->insertBefore(cst, mov);

            cst->getDef(0)->asLValue()->noSpill = 1; // doesn't help

            if (cst->op == OP_UNION)
               mov->setPredicate(defi->cc, defi->getPredicate());
         }
      }
   }

   return true;
}

} // namespace nv50_ir
