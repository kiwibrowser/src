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

namespace nv50_ir {

enum TextStyle
{
   TXT_DEFAULT,
   TXT_GPR,
   TXT_REGISTER,
   TXT_FLAGS,
   TXT_MEM,
   TXT_IMMD,
   TXT_BRA,
   TXT_INSN
};

static const char *_colour[8] =
{
   "\x1b[00m",
   "\x1b[34m",
   "\x1b[35m",
   "\x1b[35m",
   "\x1b[36m",
   "\x1b[33m",
   "\x1b[37m",
   "\x1b[32m"
};

static const char *_nocolour[8] =
{
      "", "", "", "", "", "", "", ""
};

static const char **colour;

static void init_colours()
{
   if (getenv("NV50_PROG_DEBUG_NO_COLORS") != NULL)
      colour = _nocolour;
   else
      colour = _colour;
}

static const char *OpClassStr[OPCLASS_OTHER + 1] =
{
   "MOVE",
   "LOAD",
   "STORE",
   "ARITH",
   "SHIFT",
   "SFU",
   "LOGIC",
   "COMPARE",
   "CONVERT",
   "ATOMIC",
   "TEXTURE",
   "SURFACE",
   "FLOW",
   "(INVALID)",
   "PSEUDO",
   "OTHER"
};

const char *operationStr[OP_LAST + 1] =
{
   "nop",
   "phi",
   "union",
   "split",
   "merge",
   "consec",
   "mov",
   "ld",
   "st",
   "add",
   "sub",
   "mul",
   "div",
   "mod",
   "mad",
   "fma",
   "sad",
   "abs",
   "neg",
   "not",
   "and",
   "or",
   "xor",
   "shl",
   "shr",
   "max",
   "min",
   "sat",
   "ceil",
   "floor",
   "trunc",
   "cvt",
   "set and",
   "set or",
   "set xor",
   "set",
   "selp",
   "slct",
   "rcp",
   "rsq",
   "lg2",
   "sin",
   "cos",
   "ex2",
   "exp",
   "log",
   "presin",
   "preex2",
   "sqrt",
   "pow",
   "bra",
   "call",
   "ret",
   "cont",
   "break",
   "preret",
   "precont",
   "prebreak",
   "brkpt",
   "joinat",
   "join",
   "discard",
   "exit",
   "barrier",
   "vfetch",
   "pfetch",
   "export",
   "linterp",
   "pinterp",
   "emit",
   "restart",
   "tex",
   "texbias",
   "texlod",
   "texfetch",
   "texquery",
   "texgrad",
   "texgather",
   "texcsaa",
   "suld",
   "sust",
   "dfdx",
   "dfdy",
   "rdsv",
   "wrsv",
   "pixld",
   "quadop",
   "quadon",
   "quadpop",
   "popcnt",
   "insbf",
   "extbf",
   "texbar",
   "(invalid)"
};

static const char *DataTypeStr[] =
{
   "-",
   "u8", "s8",
   "u16", "s16",
   "u32", "s32",
   "u64", "s64",
   "f16", "f32", "f64",
   "b96", "b128"
};

static const char *RoundModeStr[] =
{
   "", "rm", "rz", "rp", "rni", "rmi", "rzi", "rpi"
};

static const char *CondCodeStr[] =
{
   "never",
   "lt",
   "eq",
   "le",
   "gt",
   "ne",
   "ge",
   "",
   "(invalid)",
   "ltu",
   "equ",
   "leu",
   "gtu",
   "neu",
   "geu",
   "",
   "no",
   "nc",
   "ns",
   "na",
   "a",
   "s",
   "c",
   "o"
};

static const char *SemanticStr[SV_LAST + 1] =
{
   "POSITION",
   "VERTEX_ID",
   "INSTANCE_ID",
   "INVOCATION_ID",
   "PRIMITIVE_ID",
   "VERTEX_COUNT",
   "LAYER",
   "VIEWPORT_INDEX",
   "Y_DIR",
   "FACE",
   "POINT_SIZE",
   "POINT_COORD",
   "CLIP_DISTANCE",
   "SAMPLE_INDEX",
   "TESS_FACTOR",
   "TESS_COORD",
   "TID",
   "CTAID",
   "NTID",
   "GRIDID",
   "NCTAID",
   "LANEID",
   "PHYSID",
   "NPHYSID",
   "CLOCK",
   "LBASE",
   "SBASE",
   "?",
   "(INVALID)"
};

static const char *interpStr[16] =
{
   "pass",
   "mul",
   "flat",
   "sc",
   "cent pass",
   "cent mul",
   "cent flat",
   "cent sc",
   "off pass",
   "off mul",
   "off flat",
   "off sc",
   "samp pass",
   "samp mul",
   "samp flat",
   "samp sc"
};

#define PRINT(args...)                                \
   do {                                               \
      pos += snprintf(&buf[pos], size - pos, args);   \
   } while(0)

#define SPACE_PRINT(cond, args...)                      \
   do {                                                 \
      if (cond)                                         \
         buf[pos++] = ' ';                              \
      pos += snprintf(&buf[pos], size - pos, args);     \
   } while(0)

#define SPACE()                                    \
   do {                                            \
      if (pos < size)                              \
         buf[pos++] = ' ';                         \
   } while(0)

int Modifier::print(char *buf, size_t size) const
{
   size_t pos = 0;

   if (bits)
      PRINT("%s", colour[TXT_INSN]);

   size_t base = pos;

   if (bits & NV50_IR_MOD_NOT)
      PRINT("not");
   if (bits & NV50_IR_MOD_SAT)
      SPACE_PRINT(pos > base && pos < size, "sat");
   if (bits & NV50_IR_MOD_NEG)
      SPACE_PRINT(pos > base && pos < size, "neg");
   if (bits & NV50_IR_MOD_ABS)
      SPACE_PRINT(pos > base && pos < size, "abs");

   return pos;
}

int LValue::print(char *buf, size_t size, DataType ty) const
{
   const char *postFix = "";
   size_t pos = 0;
   int idx = join->reg.data.id >= 0 ? join->reg.data.id : id;
   char p = join->reg.data.id >= 0 ? '$' : '%';
   char r;
   int col = TXT_DEFAULT;

   switch (reg.file) {
   case FILE_GPR:
      r = 'r'; col = TXT_GPR;
      if (reg.size == 2) {
         if (p == '$') {
            postFix = (idx & 1) ? "h" : "l";
            idx /= 2;
         } else {
            postFix = "s";
         }
      } else
      if (reg.size == 8) {
         postFix = "d";
      } else
      if (reg.size == 16) {
         postFix = "q";
      } else
      if (reg.size == 12) {
         postFix = "t";
      }
      break;
   case FILE_PREDICATE:
      r = 'p'; col = TXT_REGISTER;
      if (reg.size == 2)
         postFix = "d";
      else
      if (reg.size == 4)
         postFix = "q";
      break;
   case FILE_FLAGS:
      r = 'c'; col = TXT_FLAGS;
      break;
   case FILE_ADDRESS:
      r = 'a'; col = TXT_REGISTER;
      break;
   default:
      assert(!"invalid file for lvalue");
      r = '?';
      break;
   }

   PRINT("%s%c%c%i%s", colour[col], p, r, idx, postFix);

   return pos;
}

int ImmediateValue::print(char *buf, size_t size, DataType ty) const
{
   size_t pos = 0;

   PRINT("%s", colour[TXT_IMMD]);

   switch (ty) {
   case TYPE_F32: PRINT("%f", reg.data.f32); break;
   case TYPE_F64: PRINT("%f", reg.data.f64); break;
   case TYPE_U8:  PRINT("0x%02x", reg.data.u8); break;
   case TYPE_S8:  PRINT("%i", reg.data.s8); break;
   case TYPE_U16: PRINT("0x%04x", reg.data.u16); break;
   case TYPE_S16: PRINT("%i", reg.data.s16); break;
   case TYPE_U32: PRINT("0x%08x", reg.data.u32); break;
   case TYPE_S32: PRINT("%i", reg.data.s32); break;
   case TYPE_U64:
   case TYPE_S64:
   default:
      PRINT("0x%016lx", reg.data.u64);
      break;
   }
   return pos;
}

int Symbol::print(char *buf, size_t size, DataType ty) const
{
   return print(buf, size, NULL, NULL, ty);
}

int Symbol::print(char *buf, size_t size,
                  Value *rel, Value *dimRel, DataType ty) const
{
   size_t pos = 0;
   char c;

   if (ty == TYPE_NONE)
      ty = typeOfSize(reg.size);

   if (reg.file == FILE_SYSTEM_VALUE) {
      PRINT("%ssv[%s%s:%i%s", colour[TXT_MEM],
            colour[TXT_REGISTER],
            SemanticStr[reg.data.sv.sv], reg.data.sv.index, colour[TXT_MEM]);
      if (rel) {
         PRINT("%s+", colour[TXT_DEFAULT]);
         pos += rel->print(&buf[pos], size - pos);
      }
      PRINT("%s]", colour[TXT_MEM]);
      return pos;
   }

   switch (reg.file) {
   case FILE_MEMORY_CONST:  c = 'c'; break;
   case FILE_SHADER_INPUT:  c = 'a'; break;
   case FILE_SHADER_OUTPUT: c = 'o'; break;
   case FILE_MEMORY_GLOBAL: c = 'g'; break;
   case FILE_MEMORY_SHARED: c = 's'; break;
   case FILE_MEMORY_LOCAL:  c = 'l'; break;
   default:
      assert(!"invalid file");
      c = '?';
      break;
   }

   if (c == 'c')
      PRINT("%s%c%i[", colour[TXT_MEM], c, reg.fileIndex);
   else
      PRINT("%s%c[", colour[TXT_MEM], c);

   if (dimRel) {
      pos += dimRel->print(&buf[pos], size - pos, TYPE_S32);
      PRINT("%s][", colour[TXT_MEM]);
   }

   if (rel) {
      pos += rel->print(&buf[pos], size - pos);
      PRINT("%s%c", colour[TXT_DEFAULT], (reg.data.offset < 0) ? '-' : '+');
   } else {
      assert(reg.data.offset >= 0);
   }
   PRINT("%s0x%x%s]", colour[TXT_IMMD], abs(reg.data.offset), colour[TXT_MEM]);

   return pos;
}

void Instruction::print() const
{
   #define BUFSZ 512

   const size_t size = BUFSZ;

   char buf[BUFSZ];
   int s, d;
   size_t pos = 0;

   PRINT("%s", colour[TXT_INSN]);

   if (join)
      PRINT("join ");

   if (predSrc >= 0) {
      const size_t pre = pos;
      if (getSrc(predSrc)->reg.file == FILE_PREDICATE) {
         if (cc == CC_NOT_P)
            PRINT("not");
      } else {
         PRINT("%s", CondCodeStr[cc]);
      }
      if (pos > pre)
         SPACE();
      pos += getSrc(predSrc)->print(&buf[pos], BUFSZ - pos);
      PRINT(" %s", colour[TXT_INSN]);
   }

   if (saturate)
      PRINT("sat ");

   if (asFlow()) {
      PRINT("%s", operationStr[op]);
      if (op == OP_CALL && asFlow()->builtin) {
         PRINT(" %sBUILTIN:%i", colour[TXT_BRA], asFlow()->target.builtin);
      } else
      if (op == OP_CALL && asFlow()->target.fn) {
         PRINT(" %s%s:%i", colour[TXT_BRA],
               asFlow()->target.fn->getName(),
               asFlow()->target.fn->getLabel());
      } else
      if (asFlow()->target.bb)
         PRINT(" %sBB:%i", colour[TXT_BRA], asFlow()->target.bb->getId());
   } else {
      PRINT("%s ", operationStr[op]);
      if (op == OP_LINTERP || op == OP_PINTERP)
         PRINT("%s ", interpStr[ipa]);
      if (subOp)
         PRINT("(SUBOP:%u) ", subOp);
      if (perPatch)
         PRINT("patch ");
      if (asTex())
         PRINT("%s ", asTex()->tex.target.getName());
      if (postFactor)
         PRINT("x2^%i ", postFactor);
      PRINT("%s%s", dnz ? "dnz " : (ftz ? "ftz " : ""),  DataTypeStr[dType]);
   }

   if (rnd != ROUND_N)
      PRINT(" %s", RoundModeStr[rnd]);

   if (defExists(1))
      PRINT(" {");
   for (d = 0; defExists(d); ++d) {
      SPACE();
      pos += getDef(d)->print(&buf[pos], size - pos);
   }
   if (d > 1)
      PRINT(" %s}", colour[TXT_INSN]);
   else
   if (!d && !asFlow())
      PRINT(" %s#", colour[TXT_INSN]);

   if (asCmp())
      PRINT(" %s%s", colour[TXT_INSN], CondCodeStr[asCmp()->setCond]);

   if (sType != dType)
      PRINT(" %s%s", colour[TXT_INSN], DataTypeStr[sType]);

   for (s = 0; srcExists(s); ++s) {
      if (s == predSrc || src(s).usedAsPtr)
         continue;
      const size_t pre = pos;
      SPACE();
      pos += src(s).mod.print(&buf[pos], BUFSZ - pos);
      if (pos > pre + 1)
         SPACE();
      if (src(s).isIndirect(0) || src(s).isIndirect(1))
         pos += getSrc(s)->asSym()->print(&buf[pos], BUFSZ - pos,
                                          getIndirect(s, 0),
                                          getIndirect(s, 1));
      else
         pos += getSrc(s)->print(&buf[pos], BUFSZ - pos, sType);
   }
   if (exit)
      PRINT("%s exit", colour[TXT_INSN]);

   PRINT("%s", colour[TXT_DEFAULT]);

   buf[MIN2(pos, BUFSZ - 1)] = 0;

   INFO("%s (%u)\n", buf, encSize);
}

class PrintPass : public Pass
{
public:
   PrintPass() : serial(0) { }

   virtual bool visit(Function *);
   virtual bool visit(BasicBlock *);
   virtual bool visit(Instruction *);

private:
   int serial;
};

bool
PrintPass::visit(Function *fn)
{
   INFO("\n%s:%i\n", fn->getName(), fn->getLabel());

   return true;
}

bool
PrintPass::visit(BasicBlock *bb)
{
#if 0
   INFO("---\n");
   for (Graph::EdgeIterator ei = bb->cfg.incident(); !ei.end(); ei.next())
      INFO(" <- BB:%i (%s)\n",
           BasicBlock::get(ei.getNode())->getId(),
           ei.getEdge()->typeStr());
#endif
   INFO("BB:%i (%u instructions) - ", bb->getId(), bb->getInsnCount());

   if (bb->idom())
      INFO("idom = BB:%i, ", bb->idom()->getId());

   INFO("df = { ");
   for (DLList::Iterator df = bb->getDF().iterator(); !df.end(); df.next())
      INFO("BB:%i ", BasicBlock::get(df)->getId());

   INFO("}\n");

   for (Graph::EdgeIterator ei = bb->cfg.outgoing(); !ei.end(); ei.next())
      INFO(" -> BB:%i (%s)\n",
           BasicBlock::get(ei.getNode())->getId(),
           ei.getEdge()->typeStr());

   return true;
}

bool
PrintPass::visit(Instruction *insn)
{
   INFO("%3i: ", serial++);
   insn->print();
   return true;
}

void
Function::print()
{
   PrintPass pass;
   pass.run(this, true, false);
}

void
Program::print()
{
   PrintPass pass;
   init_colours();
   pass.run(this, true, false);
}

void
Function::printLiveIntervals() const
{
   INFO("printing live intervals ...\n");

   for (ArrayList::Iterator it = allLValues.iterator(); !it.end(); it.next()) {
      const Value *lval = Value::get(it)->asLValue();
      if (lval && !lval->livei.isEmpty()) {
         INFO("livei(%%%i): ", lval->id);
         lval->livei.print();
      }
   }
}

} // namespace nv50_ir
