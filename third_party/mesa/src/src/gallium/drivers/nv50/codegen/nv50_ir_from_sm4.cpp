
#include "nv50_ir.h"
#include "nv50_ir_target.h"
#include "nv50_ir_build_util.h"

#include "nv50_ir_from_sm4.h"

// WTF: pass-through is implicit ??? check ReadWriteMask

namespace tgsi {

static nv50_ir::SVSemantic irSemantic(unsigned sn)
{
   switch (sn) {
   case TGSI_SEMANTIC_POSITION:      return nv50_ir::SV_POSITION;
   case TGSI_SEMANTIC_FACE:          return nv50_ir::SV_FACE;
   case NV50_SEMANTIC_LAYER:         return nv50_ir::SV_LAYER;
   case NV50_SEMANTIC_VIEWPORTINDEX: return nv50_ir::SV_VIEWPORT_INDEX;
   case TGSI_SEMANTIC_PSIZE:         return nv50_ir::SV_POINT_SIZE;
   case NV50_SEMANTIC_CLIPDISTANCE:  return nv50_ir::SV_CLIP_DISTANCE;
   case TGSI_SEMANTIC_VERTEXID:      return nv50_ir::SV_VERTEX_ID;
   case TGSI_SEMANTIC_INSTANCEID:    return nv50_ir::SV_INSTANCE_ID;
   case TGSI_SEMANTIC_PRIMID:        return nv50_ir::SV_PRIMITIVE_ID;
   case NV50_SEMANTIC_TESSFACTOR:    return nv50_ir::SV_TESS_FACTOR;
   case NV50_SEMANTIC_TESSCOORD:     return nv50_ir::SV_TESS_COORD;
   default:
      return nv50_ir::SV_UNDEFINED;
   }
}

} // namespace tgsi

namespace {

using namespace nv50_ir;

#define NV50_IR_MAX_RESOURCES 64

class Converter : public BuildUtil
{
public:
   Converter(Program *, struct nv50_ir_prog_info *);
   ~Converter();

private:
   DataArray tData32;
   DataArray tData64;
   unsigned int nrRegVals;

   DataArray *lData;
   unsigned int nrArrays;
   unsigned int arrayVol;

   DataArray oData;

   uint8_t interpMode[PIPE_MAX_SHADER_INPUTS];

   // outputs for each phase
   struct nv50_ir_varying out[3][PIPE_MAX_SHADER_OUTPUTS];

   int phase;
   int subPhaseCnt[2];
   int subPhase;
   unsigned int phaseStart;
   unsigned int phaseInstance;
   unsigned int *phaseInstCnt[2];
   bool unrollPhase;
   bool phaseInstanceUsed;
   int phaseEnded; // (phase + 1) if $phase ended

   bool finalized;

   Value *srcPtr[3][3]; // for indirect addressing, save pointer values
   Value *dstPtr[3];
   Value *vtxBase[3]; // base address of vertex in a primitive (TP/GP)

   Value *domainPt[3]; // pre-fetched TessCoord

   unsigned int nDstOpnds;

   Stack condBBs;
   Stack joinBBs;
   Stack loopBBs;
   Stack breakBBs;
   Stack entryBBs;
   Stack leaveBBs;
   Stack retIPs;

   bool shadow[NV50_IR_MAX_RESOURCES];
   TexTarget resourceType[NV50_IR_MAX_RESOURCES][2];

   struct nv50_ir_prog_info& info;

   Value *fragCoord[4];

public:
   bool run();

private:
   bool handleInstruction(unsigned int pos);
   bool inspectInstruction(unsigned int pos);
   bool handleDeclaration(const sm4_dcl& dcl);
   bool inspectDeclaration(const sm4_dcl& dcl);
   bool parseSignature();

   bool haveNextPhase(unsigned int pos) const;

   void allocateValues();
   void exportOutputs();

   void emitTex(Value *dst0[4], TexInstruction *, const uint8_t swizzle[4]);
   void handleLOAD(Value *dst0[4]);
   void handleSAMPLE(operation, Value *dst0[4]);
   void handleQUERY(Value *dst0[4], enum TexQuery query);
   void handleDP(Value *dst0[4], int dim);

   Symbol *iSym(int i, int c);
   Symbol *oSym(int i, int c);

   Value *src(int i, int c);
   Value *src(const sm4_op&, int c, int i);
   Value *dst(int i, int c);
   Value *dst(const sm4_op&, int c, int i);
   void saveDst(int i, int c, Value *value);
   void saveDst(const sm4_op&, int c, Value *value, int i);
   void saveFragDepth(operation op, Value *value);

   Value *interpolate(const sm4_op&, int c, int i);

   Value *getSrcPtr(int s, int dim, int shl);
   Value *getDstPtr(int d, int dim, int shl);
   Value *getVtxPtr(int s);

   bool checkDstSrcAliasing() const;
   void insertConvergenceOps(BasicBlock *conv, BasicBlock *fork);
   void finalizeShader();

   operation cvtOpcode(enum sm4_opcode op) const;
   unsigned int getDstOpndCount(enum sm4_opcode opcode) const;

   DataType inferSrcType(enum sm4_opcode op) const;
   DataType inferDstType(enum sm4_opcode op) const;

   unsigned g3dPrim(const unsigned prim, unsigned *patchSize = NULL) const;
   CondCode cvtCondCode(enum sm4_opcode op) const;
   RoundMode cvtRoundingMode(enum sm4_opcode op) const;
   TexTarget cvtTexTarget(enum sm4_target,
                           enum sm4_opcode, operation *) const;
   SVSemantic cvtSemantic(enum sm4_sv, uint8_t &index) const;
   uint8_t cvtInterpMode(enum sm4_interpolation) const;

   unsigned tgsiSemantic(SVSemantic, int index);
   void recordSV(unsigned sn, unsigned si, unsigned mask, bool input);

private:
   sm4_insn *insn;
   DataType dTy, sTy;

   const struct sm4_program& sm4;
   Program *prog;
};

#define PRIM_CASE(a, b) \
   case D3D_PRIMITIVE_TOPOLOGY_##a: return PIPE_PRIM_##b;

unsigned
Converter::g3dPrim(const unsigned prim, unsigned *patchSize) const
{
   switch (prim) {
   PRIM_CASE(UNDEFINED, POINTS);
   PRIM_CASE(POINTLIST, POINTS);
   PRIM_CASE(LINELIST, LINES);
   PRIM_CASE(LINESTRIP, LINE_STRIP);
   PRIM_CASE(TRIANGLELIST, TRIANGLES);
   PRIM_CASE(TRIANGLESTRIP, TRIANGLE_STRIP);
   PRIM_CASE(LINELIST_ADJ, LINES_ADJACENCY);
   PRIM_CASE(LINESTRIP_ADJ, LINE_STRIP_ADJACENCY);
   PRIM_CASE(TRIANGLELIST_ADJ, TRIANGLES_ADJACENCY);
   PRIM_CASE(TRIANGLESTRIP_ADJ, TRIANGLES_ADJACENCY);
   default:
      if (prim < D3D_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST ||
          prim > D3D_PRIMITIVE_TOPOLOGY_32_CONTROL_POINT_PATCHLIST)
         return PIPE_PRIM_POINTS;
      if (patchSize)
         *patchSize =
            prim - D3D_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST + 1;
      return NV50_PRIM_PATCHES;
   }
}

#define IPM_CASE(n, a, b) \
   case SM4_INTERPOLATION_##n: return NV50_IR_INTERP_##a | NV50_IR_INTERP_##b

uint8_t
Converter::cvtInterpMode(enum sm4_interpolation mode) const
{
   switch (mode) {
   IPM_CASE(CONSTANT,                      FLAT, FLAT);
   IPM_CASE(LINEAR,                        PERSPECTIVE, PERSPECTIVE);
   IPM_CASE(LINEAR_CENTROID,               PERSPECTIVE, CENTROID);
   IPM_CASE(LINEAR_NOPERSPECTIVE,          LINEAR, LINEAR);
   IPM_CASE(LINEAR_NOPERSPECTIVE_CENTROID, LINEAR, CENTROID);
   IPM_CASE(LINEAR_SAMPLE,                 PERSPECTIVE, OFFSET);
   IPM_CASE(LINEAR_NOPERSPECTIVE_SAMPLE,   LINEAR, OFFSET);
   IPM_CASE(UNDEFINED,                     LINEAR, LINEAR);
   default:
      assert(!"invalid interpolation mode");
      return 0;
   }
}

static void
setVaryingInterpMode(struct nv50_ir_varying *var, uint8_t mode)
{
   switch (mode & NV50_IR_INTERP_MODE_MASK) {
   case NV50_IR_INTERP_LINEAR:
      var->linear = 1;
      break;
   case NV50_IR_INTERP_FLAT:
      var->flat = 1;
      break;
   default:
      break;
   }
   if (mode & NV50_IR_INTERP_CENTROID)
      var->centroid = 1;
}

RoundMode
Converter::cvtRoundingMode(enum sm4_opcode op) const
{
   switch (op) {
   case SM4_OPCODE_ROUND_NE: return ROUND_NI;
   case SM4_OPCODE_ROUND_NI: return ROUND_MI;
   case SM4_OPCODE_ROUND_PI: return ROUND_PI;
   case SM4_OPCODE_ROUND_Z:  return ROUND_ZI;
   default:
      return ROUND_N;
   }
}

CondCode
Converter::cvtCondCode(enum sm4_opcode op) const
{
   switch (op) {
   case SM4_OPCODE_EQ:
   case SM4_OPCODE_DEQ:
   case SM4_OPCODE_IEQ: return CC_EQ;
   case SM4_OPCODE_GE:
   case SM4_OPCODE_DGE:
   case SM4_OPCODE_IGE:
   case SM4_OPCODE_UGE: return CC_GE;
   case SM4_OPCODE_LT:
   case SM4_OPCODE_DLT:
   case SM4_OPCODE_ILT:
   case SM4_OPCODE_ULT: return CC_LT;
   case SM4_OPCODE_NE:
   case SM4_OPCODE_INE:
   case SM4_OPCODE_DNE: return CC_NEU;
   default:
      return CC_ALWAYS;
   }
}

DataType
Converter::inferSrcType(enum sm4_opcode op) const
{
   switch (op) {
   case SM4_OPCODE_IADD:
   case SM4_OPCODE_IEQ:
   case SM4_OPCODE_IGE:
   case SM4_OPCODE_ILT:
   case SM4_OPCODE_IMAD:
   case SM4_OPCODE_IMAX:
   case SM4_OPCODE_IMIN:
   case SM4_OPCODE_IMUL:
   case SM4_OPCODE_INE:
   case SM4_OPCODE_INEG:
   case SM4_OPCODE_ISHL:
   case SM4_OPCODE_ISHR:
   case SM4_OPCODE_ITOF:
   case SM4_OPCODE_ATOMIC_IADD:
   case SM4_OPCODE_ATOMIC_IMAX:
   case SM4_OPCODE_ATOMIC_IMIN:
      return TYPE_S32;
   case SM4_OPCODE_AND:
   case SM4_OPCODE_NOT:
   case SM4_OPCODE_OR:
   case SM4_OPCODE_UDIV:
   case SM4_OPCODE_ULT:
   case SM4_OPCODE_UGE:
   case SM4_OPCODE_UMUL:
   case SM4_OPCODE_UMAD:
   case SM4_OPCODE_UMAX:
   case SM4_OPCODE_UMIN:
   case SM4_OPCODE_USHR:
   case SM4_OPCODE_UTOF:
   case SM4_OPCODE_XOR:
   case SM4_OPCODE_UADDC:
   case SM4_OPCODE_USUBB:
   case SM4_OPCODE_ATOMIC_AND:
   case SM4_OPCODE_ATOMIC_OR:
   case SM4_OPCODE_ATOMIC_XOR:
   case SM4_OPCODE_ATOMIC_UMAX:
   case SM4_OPCODE_ATOMIC_UMIN:
      return TYPE_U32;
   case SM4_OPCODE_DADD:
   case SM4_OPCODE_DMAX:
   case SM4_OPCODE_DMIN:
   case SM4_OPCODE_DMUL:
   case SM4_OPCODE_DEQ:
   case SM4_OPCODE_DGE:
   case SM4_OPCODE_DLT:
   case SM4_OPCODE_DNE:
   case SM4_OPCODE_DMOV:
   case SM4_OPCODE_DMOVC:
   case SM4_OPCODE_DTOF:
      return TYPE_F64;
   case SM4_OPCODE_F16TOF32:
      return TYPE_F16;
   default:
      return TYPE_F32;
   }
}

DataType
Converter::inferDstType(enum sm4_opcode op) const
{
   switch (op) {
   case SM4_OPCODE_FTOI:
      return TYPE_S32;
   case SM4_OPCODE_FTOU:
   case SM4_OPCODE_EQ:
   case SM4_OPCODE_GE:
   case SM4_OPCODE_LT:
   case SM4_OPCODE_NE:
      return TYPE_U32;
   case SM4_OPCODE_FTOD:
      return TYPE_F64;
   case SM4_OPCODE_F32TOF16:
      return TYPE_F16;
   case SM4_OPCODE_ITOF:
   case SM4_OPCODE_UTOF:
   case SM4_OPCODE_DTOF:
      return TYPE_F32;
   default:
      return inferSrcType(op);
   }
}

operation
Converter::cvtOpcode(enum sm4_opcode op) const
{
   switch (op) {
   case SM4_OPCODE_ADD:         return OP_ADD;
   case SM4_OPCODE_AND:         return OP_AND;
   case SM4_OPCODE_BREAK:       return OP_BREAK;
   case SM4_OPCODE_BREAKC:      return OP_BREAK;
   case SM4_OPCODE_CALL:        return OP_CALL;
   case SM4_OPCODE_CALLC:       return OP_CALL;
   case SM4_OPCODE_CASE:        return OP_NOP;
   case SM4_OPCODE_CONTINUE:    return OP_CONT;
   case SM4_OPCODE_CONTINUEC:   return OP_CONT;
   case SM4_OPCODE_CUT:         return OP_RESTART;
   case SM4_OPCODE_DEFAULT:     return OP_NOP;
   case SM4_OPCODE_DERIV_RTX:   return OP_DFDX;
   case SM4_OPCODE_DERIV_RTY:   return OP_DFDY;
   case SM4_OPCODE_DISCARD:     return OP_DISCARD;
   case SM4_OPCODE_DIV:         return OP_DIV;
   case SM4_OPCODE_DP2:         return OP_MAD;
   case SM4_OPCODE_DP3:         return OP_MAD;
   case SM4_OPCODE_DP4:         return OP_MAD;
   case SM4_OPCODE_ELSE:        return OP_BRA;
   case SM4_OPCODE_EMIT:        return OP_EMIT;
   case SM4_OPCODE_EMITTHENCUT: return OP_EMIT;
   case SM4_OPCODE_ENDIF:       return OP_BRA;
   case SM4_OPCODE_ENDLOOP:     return OP_PREBREAK;
   case SM4_OPCODE_ENDSWITCH:   return OP_NOP;
   case SM4_OPCODE_EQ:          return OP_SET;
   case SM4_OPCODE_EXP:         return OP_EX2;
   case SM4_OPCODE_FRC:         return OP_CVT;
   case SM4_OPCODE_FTOI:        return OP_CVT;
   case SM4_OPCODE_FTOU:        return OP_CVT;
   case SM4_OPCODE_GE:          return OP_SET;
   case SM4_OPCODE_IADD:        return OP_ADD;
   case SM4_OPCODE_IF:          return OP_BRA;
   case SM4_OPCODE_IEQ:         return OP_SET;
   case SM4_OPCODE_IGE:         return OP_SET;
   case SM4_OPCODE_ILT:         return OP_SET;
   case SM4_OPCODE_IMAD:        return OP_MAD;
   case SM4_OPCODE_IMAX:        return OP_MAX;
   case SM4_OPCODE_IMIN:        return OP_MIN;
   case SM4_OPCODE_IMUL:        return OP_MUL;
   case SM4_OPCODE_INE:         return OP_SET;
   case SM4_OPCODE_INEG:        return OP_NEG;
   case SM4_OPCODE_ISHL:        return OP_SHL;
   case SM4_OPCODE_ISHR:        return OP_SHR;
   case SM4_OPCODE_ITOF:        return OP_CVT;
   case SM4_OPCODE_LD:          return OP_TXF;
   case SM4_OPCODE_LD_MS:       return OP_TXF;
   case SM4_OPCODE_LOG:         return OP_LG2;
   case SM4_OPCODE_LOOP:        return OP_PRECONT;
   case SM4_OPCODE_LT:          return OP_SET;
   case SM4_OPCODE_MAD:         return OP_MAD;
   case SM4_OPCODE_MIN:         return OP_MIN;
   case SM4_OPCODE_MAX:         return OP_MAX;
   case SM4_OPCODE_MOV:         return OP_MOV;
   case SM4_OPCODE_MOVC:        return OP_MOV;
   case SM4_OPCODE_MUL:         return OP_MUL;
   case SM4_OPCODE_NE:          return OP_SET;
   case SM4_OPCODE_NOP:         return OP_NOP;
   case SM4_OPCODE_NOT:         return OP_NOT;
   case SM4_OPCODE_OR:          return OP_OR;
   case SM4_OPCODE_RESINFO:     return OP_TXQ;
   case SM4_OPCODE_RET:         return OP_RET;
   case SM4_OPCODE_RETC:        return OP_RET;
   case SM4_OPCODE_ROUND_NE:    return OP_CVT;
   case SM4_OPCODE_ROUND_NI:    return OP_FLOOR;
   case SM4_OPCODE_ROUND_PI:    return OP_CEIL;
   case SM4_OPCODE_ROUND_Z:     return OP_TRUNC;
   case SM4_OPCODE_RSQ:         return OP_RSQ;
   case SM4_OPCODE_SAMPLE:      return OP_TEX;
   case SM4_OPCODE_SAMPLE_C:    return OP_TEX;
   case SM4_OPCODE_SAMPLE_C_LZ: return OP_TEX;
   case SM4_OPCODE_SAMPLE_L:    return OP_TXL;
   case SM4_OPCODE_SAMPLE_D:    return OP_TXD;
   case SM4_OPCODE_SAMPLE_B:    return OP_TXB;
   case SM4_OPCODE_SQRT:        return OP_SQRT;
   case SM4_OPCODE_SWITCH:      return OP_NOP;
   case SM4_OPCODE_SINCOS:      return OP_PRESIN;
   case SM4_OPCODE_UDIV:        return OP_DIV;
   case SM4_OPCODE_ULT:         return OP_SET;
   case SM4_OPCODE_UGE:         return OP_SET;
   case SM4_OPCODE_UMUL:        return OP_MUL;
   case SM4_OPCODE_UMAD:        return OP_MAD;
   case SM4_OPCODE_UMAX:        return OP_MAX;
   case SM4_OPCODE_UMIN:        return OP_MIN;
   case SM4_OPCODE_USHR:        return OP_SHR;
   case SM4_OPCODE_UTOF:        return OP_CVT;
   case SM4_OPCODE_XOR:         return OP_XOR;

   case SM4_OPCODE_GATHER4:            return OP_TXG;
   case SM4_OPCODE_SAMPLE_POS:         return OP_PIXLD;
   case SM4_OPCODE_SAMPLE_INFO:        return OP_PIXLD;
   case SM4_OPCODE_EMIT_STREAM:        return OP_EMIT;
   case SM4_OPCODE_CUT_STREAM:         return OP_RESTART;
   case SM4_OPCODE_EMITTHENCUT_STREAM: return OP_EMIT;
   case SM4_OPCODE_INTERFACE_CALL:     return OP_CALL;
   case SM4_OPCODE_BUFINFO:            return OP_TXQ;
   case SM4_OPCODE_DERIV_RTX_COARSE:   return OP_DFDX;
   case SM4_OPCODE_DERIV_RTX_FINE:     return OP_DFDX;
   case SM4_OPCODE_DERIV_RTY_COARSE:   return OP_DFDY;
   case SM4_OPCODE_DERIV_RTY_FINE:     return OP_DFDY;
   case SM4_OPCODE_GATHER4_C:          return OP_TXG;
   case SM4_OPCODE_GATHER4_PO:         return OP_TXG;
   case SM4_OPCODE_GATHER4_PO_C:       return OP_TXG;

   case SM4_OPCODE_RCP:       return OP_RCP;
   case SM4_OPCODE_F32TOF16:  return OP_CVT;
   case SM4_OPCODE_F16TOF32:  return OP_CVT;
   case SM4_OPCODE_UADDC:     return OP_ADD;
   case SM4_OPCODE_USUBB:     return OP_SUB;
   case SM4_OPCODE_COUNTBITS: return OP_POPCNT;

   case SM4_OPCODE_ATOMIC_AND:       return OP_AND;
   case SM4_OPCODE_ATOMIC_OR:        return OP_OR;
   case SM4_OPCODE_ATOMIC_XOR:       return OP_XOR;
   case SM4_OPCODE_ATOMIC_CMP_STORE: return OP_STORE;
   case SM4_OPCODE_ATOMIC_IADD:      return OP_ADD;
   case SM4_OPCODE_ATOMIC_IMAX:      return OP_MAX;
   case SM4_OPCODE_ATOMIC_IMIN:      return OP_MIN;
   case SM4_OPCODE_ATOMIC_UMAX:      return OP_MAX;
   case SM4_OPCODE_ATOMIC_UMIN:      return OP_MIN;

   case SM4_OPCODE_SYNC:  return OP_MEMBAR;
   case SM4_OPCODE_DADD:  return OP_ADD;
   case SM4_OPCODE_DMAX:  return OP_MAX;
   case SM4_OPCODE_DMIN:  return OP_MIN;
   case SM4_OPCODE_DMUL:  return OP_MUL;
   case SM4_OPCODE_DEQ:   return OP_SET;
   case SM4_OPCODE_DGE:   return OP_SET;
   case SM4_OPCODE_DLT:   return OP_SET;
   case SM4_OPCODE_DNE:   return OP_SET;
   case SM4_OPCODE_DMOV:  return OP_MOV;
   case SM4_OPCODE_DMOVC: return OP_MOV;
   case SM4_OPCODE_DTOF:  return OP_CVT;
   case SM4_OPCODE_FTOD:  return OP_CVT;

   default:
      return OP_NOP;
   }
}

unsigned int
Converter::getDstOpndCount(enum sm4_opcode opcode) const
{
   switch (opcode) {
   case SM4_OPCODE_SINCOS:
   case SM4_OPCODE_UDIV:
   case SM4_OPCODE_IMUL:
   case SM4_OPCODE_UMUL:
      return 2;
   case SM4_OPCODE_BREAK:
   case SM4_OPCODE_BREAKC:
   case SM4_OPCODE_CALL:
   case SM4_OPCODE_CALLC:
   case SM4_OPCODE_CONTINUE:
   case SM4_OPCODE_CONTINUEC:
   case SM4_OPCODE_DISCARD:
   case SM4_OPCODE_EMIT:
   case SM4_OPCODE_EMIT_STREAM:
   case SM4_OPCODE_CUT:
   case SM4_OPCODE_CUT_STREAM:
   case SM4_OPCODE_EMITTHENCUT:
   case SM4_OPCODE_EMITTHENCUT_STREAM:
   case SM4_OPCODE_IF:
   case SM4_OPCODE_ELSE:
   case SM4_OPCODE_ENDIF:
   case SM4_OPCODE_LOOP:
   case SM4_OPCODE_ENDLOOP:
   case SM4_OPCODE_RET:
   case SM4_OPCODE_RETC:
   case SM4_OPCODE_SYNC:
   case SM4_OPCODE_SWITCH:
   case SM4_OPCODE_CASE:
   case SM4_OPCODE_HS_DECLS:
   case SM4_OPCODE_HS_CONTROL_POINT_PHASE:
   case SM4_OPCODE_HS_FORK_PHASE:
   case SM4_OPCODE_HS_JOIN_PHASE:
      return 0;
   default:
      return 1;
   }
}

#define TARG_CASE_1(a, b) case SM4_TARGET_##a: return TEX_TARGET_##b;
#define TARG_CASE_2(a, b) case SM4_TARGET_##a: \
   return dc ? TEX_TARGET_##b##_SHADOW : TEX_TARGET_##b

TexTarget
Converter::cvtTexTarget(enum sm4_target targ,
                        enum sm4_opcode op, operation *opr) const
{
   bool dc = (op == SM4_OPCODE_SAMPLE_C ||
              op == SM4_OPCODE_SAMPLE_C_LZ ||
              op == SM4_OPCODE_GATHER4_C ||
              op == SM4_OPCODE_GATHER4_PO_C);

   if (opr) {
      switch (targ) {
      case SM4_TARGET_RAW_BUFFER:        *opr = OP_LOAD; break;
      case SM4_TARGET_STRUCTURED_BUFFER: *opr = OP_SULD; break;
      default:
         *opr = OP_TEX;
         break;
      }
   }

   switch (targ) {
   TARG_CASE_1(UNKNOWN, 2D);
   TARG_CASE_2(TEXTURE1D,         1D);
   TARG_CASE_2(TEXTURE2D,         2D);
   TARG_CASE_1(TEXTURE2DMS,       2D_MS);
   TARG_CASE_1(TEXTURE3D,         3D);
   TARG_CASE_2(TEXTURECUBE,       CUBE);
   TARG_CASE_2(TEXTURE1DARRAY,    1D_ARRAY);
   TARG_CASE_2(TEXTURE2DARRAY,    2D_ARRAY);
   TARG_CASE_1(TEXTURE2DMSARRAY,  2D_MS_ARRAY);
   TARG_CASE_2(TEXTURECUBEARRAY,  CUBE_ARRAY);
   TARG_CASE_1(BUFFER,            BUFFER);
   TARG_CASE_1(RAW_BUFFER,        BUFFER);
   TARG_CASE_1(STRUCTURED_BUFFER, BUFFER);
   default:
      assert(!"invalid SM4 texture target");
      return dc ? TEX_TARGET_2D_SHADOW : TEX_TARGET_2D;
   }
}

static inline uint32_t
getSVIndex(enum sm4_sv sv)
{
   switch (sv) {
   case SM4_SV_FINAL_QUAD_U_EQ_0_EDGE_TESSFACTOR: return 0;
   case SM4_SV_FINAL_QUAD_V_EQ_0_EDGE_TESSFACTOR: return 1;
   case SM4_SV_FINAL_QUAD_U_EQ_1_EDGE_TESSFACTOR: return 2;
   case SM4_SV_FINAL_QUAD_V_EQ_1_EDGE_TESSFACTOR: return 3;

   case SM4_SV_FINAL_QUAD_U_INSIDE_TESSFACTOR: return 4;
   case SM4_SV_FINAL_QUAD_V_INSIDE_TESSFACTOR: return 5;

   case SM4_SV_FINAL_TRI_U_EQ_0_EDGE_TESSFACTOR: return 0;
   case SM4_SV_FINAL_TRI_V_EQ_0_EDGE_TESSFACTOR: return 1;
   case SM4_SV_FINAL_TRI_W_EQ_0_EDGE_TESSFACTOR: return 2;

   case SM4_SV_FINAL_TRI_INSIDE_TESSFACTOR: return 4;

   case SM4_SV_FINAL_LINE_DETAIL_TESSFACTOR: return 0;

   case SM4_SV_FINAL_LINE_DENSITY_TESSFACTOR: return 4;

   default:
      return 0;
   }
}

SVSemantic
Converter::cvtSemantic(enum sm4_sv sv, uint8_t &idx) const
{
   idx = 0;

   switch (sv) {
   case SM4_SV_UNDEFINED:     return SV_UNDEFINED;
   case SM4_SV_POSITION:      return SV_POSITION;
   case SM4_SV_CLIP_DISTANCE: return SV_CLIP_DISTANCE;
   case SM4_SV_CULL_DISTANCE: return SV_CLIP_DISTANCE; // XXX: distinction
   case SM4_SV_RENDER_TARGET_ARRAY_INDEX: return SV_LAYER;
   case SM4_SV_VIEWPORT_ARRAY_INDEX:  return SV_VIEWPORT_INDEX;
   case SM4_SV_VERTEX_ID:     return SV_VERTEX_ID;
   case SM4_SV_PRIMITIVE_ID:  return SV_PRIMITIVE_ID;
   case SM4_SV_INSTANCE_ID:   return SV_INSTANCE_ID;
   case SM4_SV_IS_FRONT_FACE: return SV_FACE;
   case SM4_SV_SAMPLE_INDEX:  return SV_SAMPLE_INDEX;

   case SM4_SV_FINAL_QUAD_U_EQ_0_EDGE_TESSFACTOR:
   case SM4_SV_FINAL_QUAD_V_EQ_0_EDGE_TESSFACTOR:
   case SM4_SV_FINAL_QUAD_U_EQ_1_EDGE_TESSFACTOR:
   case SM4_SV_FINAL_QUAD_V_EQ_1_EDGE_TESSFACTOR:
   case SM4_SV_FINAL_QUAD_U_INSIDE_TESSFACTOR:
   case SM4_SV_FINAL_QUAD_V_INSIDE_TESSFACTOR:
   case SM4_SV_FINAL_TRI_U_EQ_0_EDGE_TESSFACTOR:
   case SM4_SV_FINAL_TRI_V_EQ_0_EDGE_TESSFACTOR:
   case SM4_SV_FINAL_TRI_W_EQ_0_EDGE_TESSFACTOR:
   case SM4_SV_FINAL_TRI_INSIDE_TESSFACTOR:
   case SM4_SV_FINAL_LINE_DETAIL_TESSFACTOR:
   case SM4_SV_FINAL_LINE_DENSITY_TESSFACTOR:
      idx = getSVIndex(sv);
      return SV_TESS_FACTOR;

   default:
      assert(!"invalid SM4 system value");
      return SV_UNDEFINED;
   }
}

unsigned
Converter::tgsiSemantic(SVSemantic sv, int index)
{
   switch (sv) {
   case SV_POSITION:       return TGSI_SEMANTIC_POSITION;
   case SV_FACE:           return TGSI_SEMANTIC_FACE;
   case SV_LAYER:          return NV50_SEMANTIC_LAYER;
   case SV_VIEWPORT_INDEX: return NV50_SEMANTIC_VIEWPORTINDEX;
   case SV_POINT_SIZE:     return TGSI_SEMANTIC_PSIZE;
   case SV_CLIP_DISTANCE:  return NV50_SEMANTIC_CLIPDISTANCE;
   case SV_VERTEX_ID:      return TGSI_SEMANTIC_VERTEXID;
   case SV_INSTANCE_ID:    return TGSI_SEMANTIC_INSTANCEID;
   case SV_PRIMITIVE_ID:   return TGSI_SEMANTIC_PRIMID;
   case SV_TESS_FACTOR:    return NV50_SEMANTIC_TESSFACTOR;
   case SV_TESS_COORD:     return NV50_SEMANTIC_TESSCOORD;
   case SV_INVOCATION_ID:  return NV50_SEMANTIC_INVOCATIONID;
   default:
      return TGSI_SEMANTIC_GENERIC;
   }
}

void
Converter::recordSV(unsigned sn, unsigned si, unsigned mask, bool input)
{
   unsigned int i;
   for (i = 0; i < info.numSysVals; ++i)
      if (info.sv[i].sn == sn &&
          info.sv[i].si == si)
         return;
   info.numSysVals = i + 1;
   info.sv[i].sn = sn;
   info.sv[i].si = si;
   info.sv[i].mask = mask;
   info.sv[i].input = input ? 1 : 0;
}

bool
Converter::parseSignature()
{
   struct nv50_ir_varying *patch;
   unsigned int i, r, n;

   info.numInputs = 0;
   info.numOutputs = 0;
   info.numPatchConstants = 0;

   for (n = 0, i = 0; i < sm4.num_params_in; ++i) {
      r = sm4.params_in[i].Register;

      info.in[r].mask |= sm4.params_in[i].ReadWriteMask;
      // mask might be uninitialized ...
      if (!sm4.params_in[i].ReadWriteMask)
	  info.in[r].mask = 0xf;
      info.in[r].id = r;
      if (info.in[r].regular) // already assigned semantic name/index
         continue;
      info.in[r].regular = 1;
      info.in[r].patch = 0;

      info.numInputs = MAX2(info.numInputs, r + 1);

      switch (sm4.params_in[i].SystemValueType) {
      case D3D_NAME_UNDEFINED:
         info.in[r].sn = TGSI_SEMANTIC_GENERIC;
         info.in[r].si = n++;
         break;
      case D3D_NAME_POSITION:
         info.in[r].sn = TGSI_SEMANTIC_POSITION;
         break;
      case D3D_NAME_VERTEX_ID:
         info.in[r].sn = TGSI_SEMANTIC_VERTEXID;
         break;
      case D3D_NAME_PRIMITIVE_ID:
         info.in[r].sn = TGSI_SEMANTIC_PRIMID;
         // no corresponding output
         recordSV(TGSI_SEMANTIC_PRIMID, 0, 1, true);
         break;
      case D3D_NAME_INSTANCE_ID:
         info.in[r].sn = TGSI_SEMANTIC_INSTANCEID;
         break;
      case D3D_NAME_IS_FRONT_FACE:
         info.in[r].sn = TGSI_SEMANTIC_FACE;
         // no corresponding output
         recordSV(TGSI_SEMANTIC_FACE, 0, 1, true);
         break;
      default:
         assert(!"invalid/unsupported input linkage semantic");
         break;
      }
   }

   for (n = 0, i = 0; i < sm4.num_params_out; ++i) {
      r = sm4.params_out[i].Register;

      info.out[r].mask |= ~sm4.params_out[i].ReadWriteMask;
      info.out[r].id = r;
      if (info.out[r].regular) // already assigned semantic name/index
         continue;
      info.out[r].regular = 1;
      info.out[r].patch = 0;

      info.numOutputs = MAX2(info.numOutputs, r + 1);

      switch (sm4.params_out[i].SystemValueType) {
      case D3D_NAME_UNDEFINED:
         if (prog->getType() == Program::TYPE_FRAGMENT) {
            info.out[r].sn = TGSI_SEMANTIC_COLOR;
            info.out[r].si = info.prop.fp.numColourResults++;
         } else {
            info.out[r].sn = TGSI_SEMANTIC_GENERIC;
            info.out[r].si = n++;
         }
         break;
      case D3D_NAME_POSITION:
      case D3D_NAME_DEPTH:
      case D3D_NAME_DEPTH_GREATER_EQUAL:
      case D3D_NAME_DEPTH_LESS_EQUAL:
         info.out[r].sn = TGSI_SEMANTIC_POSITION;
         info.io.fragDepth = r;
         break;
      case D3D_NAME_CULL_DISTANCE:
      case D3D_NAME_CLIP_DISTANCE:
         info.out[r].sn = NV50_SEMANTIC_CLIPDISTANCE;
         info.out[r].si = sm4.params_out[i].SemanticIndex;
         break;
      case D3D_NAME_RENDER_TARGET_ARRAY_INDEX:
         info.out[r].sn = NV50_SEMANTIC_LAYER;
         break;
      case D3D_NAME_VIEWPORT_ARRAY_INDEX:
         info.out[r].sn = NV50_SEMANTIC_VIEWPORTINDEX;
         break;
      case D3D_NAME_PRIMITIVE_ID:
         info.out[r].sn = TGSI_SEMANTIC_PRIMID;
         break;
      case D3D_NAME_TARGET:
         info.out[r].sn = TGSI_SEMANTIC_COLOR;
         info.out[r].si = sm4.params_out[i].SemanticIndex;
         break;
      case D3D_NAME_COVERAGE:
         info.out[r].sn = NV50_SEMANTIC_SAMPLEMASK;
         info.io.sampleMask = r;
         break;
      case D3D_NAME_SAMPLE_INDEX:
      default:
         assert(!"invalid/unsupported output linkage semantic");
         break;
      }
   }

   if (prog->getType() == Program::TYPE_TESSELLATION_EVAL)
      patch = &info.in[info.numInputs];
   else
      patch = &info.out[info.numOutputs];

   for (n = 0, i = 0; i < sm4.num_params_patch; ++i) {
      r = sm4.params_patch[i].Register;

      patch[r].mask |= sm4.params_patch[i].Mask;
      patch[r].id = r;
      if (patch[r].regular) // already visited
         continue;
      patch[r].regular = 1;
      patch[r].patch = 1;

      info.numPatchConstants = MAX2(info.numPatchConstants, r + 1);

      switch (sm4.params_patch[i].SystemValueType) {
      case D3D_NAME_UNDEFINED:
         patch[r].sn = TGSI_SEMANTIC_GENERIC;
         patch[r].si = n++;
         break;
      case D3D_NAME_FINAL_QUAD_EDGE_TESSFACTOR:
      case D3D_NAME_FINAL_TRI_EDGE_TESSFACTOR:
      case D3D_NAME_FINAL_LINE_DETAIL_TESSFACTOR:
         patch[r].sn = NV50_SEMANTIC_TESSFACTOR;
         patch[r].si = sm4.params_patch[i].SemanticIndex;
         break;
      case D3D_NAME_FINAL_QUAD_INSIDE_TESSFACTOR:
      case D3D_NAME_FINAL_TRI_INSIDE_TESSFACTOR:
      case D3D_NAME_FINAL_LINE_DENSITY_TESSFACTOR:
         patch[r].sn = NV50_SEMANTIC_TESSFACTOR;
         patch[r].si = sm4.params_patch[i].SemanticIndex + 4;
         break;
      default:
         assert(!"invalid patch-constant linkage semantic");
         break;
      }
   }
   if (prog->getType() == Program::TYPE_TESSELLATION_EVAL)
      info.numInputs += info.numPatchConstants;
   else
      info.numOutputs += info.numPatchConstants;

   return true;
}

bool
Converter::inspectDeclaration(const sm4_dcl& dcl)
{
   int idx = -1;
   enum sm4_interpolation ipa_mode;

   if (dcl.op.get() && dcl.op->is_index_simple(0))
      idx = dcl.op->indices[0].disp;

   switch (dcl.opcode) {
   case SM4_OPCODE_DCL_SAMPLER:
      assert(idx >= 0);
      shadow[idx] = dcl.dcl_sampler.shadow;
      break;
   case SM4_OPCODE_DCL_RESOURCE:
   {
      enum sm4_target targ = (enum sm4_target)dcl.dcl_resource.target;

      assert(idx >= 0 && idx < NV50_IR_MAX_RESOURCES);
      resourceType[idx][0] = cvtTexTarget(targ, SM4_OPCODE_SAMPLE, NULL);
      resourceType[idx][1] = cvtTexTarget(targ, SM4_OPCODE_SAMPLE_C, NULL);
   }
      break;
   case SM4_OPCODE_DCL_CONSTANT_BUFFER:
      // nothing to do
      break;
   case SM4_OPCODE_CUSTOMDATA:
      info.immd.bufSize = dcl.num * 4;
      info.immd.buf = (uint32_t *)MALLOC(info.immd.bufSize);
      memcpy(info.immd.buf, dcl.data, info.immd.bufSize);
      break;
   case SM4_OPCODE_DCL_INDEX_RANGE:
      // XXX: ?
      break;
   case SM4_OPCODE_DCL_INPUT_PS_SGV:
   case SM4_OPCODE_DCL_INPUT_PS_SIV:
   case SM4_OPCODE_DCL_INPUT_PS:
   {
      assert(idx >= 0 && idx < info.numInputs);
      ipa_mode = (enum sm4_interpolation)dcl.dcl_input_ps.interpolation;
      interpMode[idx] = cvtInterpMode(ipa_mode);
      setVaryingInterpMode(&info.in[idx], interpMode[idx]);
   }
      break;
   case SM4_OPCODE_DCL_INPUT_SGV:
   case SM4_OPCODE_DCL_INPUT_SIV:
   case SM4_OPCODE_DCL_INPUT:
      if (dcl.op->file == SM4_FILE_INPUT_DOMAIN_POINT) {
         idx = info.numInputs++;
         info.in[idx].sn = NV50_SEMANTIC_TESSCOORD;
         info.in[idx].mask = dcl.op->mask;
      }
      // rest handled in parseSignature
      break;
   case SM4_OPCODE_DCL_OUTPUT_SGV:
   case SM4_OPCODE_DCL_OUTPUT_SIV:
      switch (dcl.sv) {
      case SM4_SV_POSITION:
         assert(prog->getType() != Program::TYPE_FRAGMENT);
         break;
      case SM4_SV_CULL_DISTANCE: // XXX: order ?
         info.io.cullDistanceMask |= 1 << info.io.clipDistanceMask;
      // fall through
      case SM4_SV_CLIP_DISTANCE:
         info.io.clipDistanceMask++; // abuse as count
         break;
      default:
         break;
      }
      switch (dcl.op->file) {
      case SM4_FILE_OUTPUT_DEPTH_LESS_EQUAL:
      case SM4_FILE_OUTPUT_DEPTH_GREATER_EQUAL:
      case SM4_FILE_OUTPUT_DEPTH:
         if (info.io.fragDepth < 0xff)
            break;
         idx = info.io.fragDepth = info.numOutputs++;
         info.out[idx].sn = TGSI_SEMANTIC_POSITION;
         break;
      case SM4_FILE_OUTPUT_COVERAGE_MASK:
         if (info.io.sampleMask < 0xff)
            break;
         idx = info.io.sampleMask = info.numOutputs++;
         info.out[idx].sn = NV50_SEMANTIC_SAMPLEMASK;
         break;
      default:
         break;
      }
      break;
   case SM4_OPCODE_DCL_OUTPUT:
      // handled in parseSignature
      break;
   case SM4_OPCODE_DCL_TEMPS:
      nrRegVals += dcl.num;
      break;
   case SM4_OPCODE_DCL_INDEXABLE_TEMP:
      nrArrays++;
      break;
   case SM4_OPCODE_DCL_GLOBAL_FLAGS:
      if (prog->getType() == Program::TYPE_FRAGMENT)
         info.prop.fp.earlyFragTests = dcl.dcl_global_flags.early_depth_stencil;
      break;

   case SM4_OPCODE_DCL_FUNCTION_BODY:
      break;
   case SM4_OPCODE_DCL_FUNCTION_TABLE:
      break;
   case SM4_OPCODE_DCL_INTERFACE:
      break;

      // GP
   case SM4_OPCODE_DCL_GS_OUTPUT_PRIMITIVE_TOPOLOGY:
      info.prop.gp.outputPrim = g3dPrim(
         dcl.dcl_gs_output_primitive_topology.primitive_topology);
      break;
   case SM4_OPCODE_DCL_GS_INPUT_PRIMITIVE:
      info.prop.gp.inputPrim = g3dPrim(dcl.dcl_gs_input_primitive.primitive);
      break;
   case SM4_OPCODE_DCL_MAX_OUTPUT_VERTEX_COUNT:
      info.prop.gp.maxVertices = dcl.num;
      break;
   case SM4_OPCODE_DCL_GS_INSTANCE_COUNT:
      info.prop.gp.instanceCount = dcl.num;
      break;
   case SM4_OPCODE_DCL_STREAM:
      break;

      // TCP/TEP
   case SM4_OPCODE_DCL_INPUT_CONTROL_POINT_COUNT:
      info.prop.tp.inputPatchSize =
         dcl.dcl_input_control_point_count.control_points;
      break;
   case SM4_OPCODE_DCL_OUTPUT_CONTROL_POINT_COUNT:
      info.prop.tp.outputPatchSize =
         dcl.dcl_output_control_point_count.control_points;
      break;
   case SM4_OPCODE_DCL_TESS_DOMAIN:
      switch (dcl.dcl_tess_domain.domain) {
      case D3D_TESSELLATOR_DOMAIN_ISOLINE:
         info.prop.tp.domain = PIPE_PRIM_LINES;
         break;
      case D3D_TESSELLATOR_DOMAIN_TRI:
         info.prop.tp.domain = PIPE_PRIM_TRIANGLES;
         break;
      case D3D_TESSELLATOR_DOMAIN_QUAD:
         info.prop.tp.domain = PIPE_PRIM_QUADS;
         break;
      case D3D_TESSELLATOR_DOMAIN_UNDEFINED:
      default:
         info.prop.tp.domain = PIPE_PRIM_MAX;
         break;
      }
      break;
   case SM4_OPCODE_DCL_TESS_PARTITIONING:
      switch (dcl.dcl_tess_partitioning.partitioning) {
      case D3D_TESSELLATOR_PARTITIONING_FRACTIONAL_ODD:
         info.prop.tp.partitioning = NV50_TESS_PART_FRACT_ODD;
         break;
      case D3D_TESSELLATOR_PARTITIONING_FRACTIONAL_EVEN:
         info.prop.tp.partitioning = NV50_TESS_PART_FRACT_EVEN;
         break;
      case D3D_TESSELLATOR_PARTITIONING_POW2:
         info.prop.tp.partitioning = NV50_TESS_PART_POW2;
         break;
      case D3D_TESSELLATOR_PARTITIONING_INTEGER:
      case D3D_TESSELLATOR_PARTITIONING_UNDEFINED:
      default:
         info.prop.tp.partitioning = NV50_TESS_PART_INTEGER;
         break;
      }
      break;
   case SM4_OPCODE_DCL_TESS_OUTPUT_PRIMITIVE:
      switch (dcl.dcl_tess_output_primitive.primitive) {
      case D3D_TESSELLATOR_OUTPUT_LINE:
         info.prop.tp.outputPrim = PIPE_PRIM_LINES;
         break;
      case D3D_TESSELLATOR_OUTPUT_TRIANGLE_CW:
         info.prop.tp.outputPrim = PIPE_PRIM_TRIANGLES;
         info.prop.tp.winding = +1;
         break;
      case D3D_TESSELLATOR_OUTPUT_TRIANGLE_CCW:
         info.prop.tp.outputPrim = PIPE_PRIM_TRIANGLES;
         info.prop.tp.winding = -1;
         break;
      case D3D_TESSELLATOR_OUTPUT_POINT:
         info.prop.tp.outputPrim = PIPE_PRIM_POINTS;
         break;
      case D3D_TESSELLATOR_OUTPUT_UNDEFINED:
      default:
         info.prop.tp.outputPrim = PIPE_PRIM_MAX;
         break;
      }
      break;

   case SM4_OPCODE_HS_FORK_PHASE:
      ++subPhaseCnt[0];
      phase = 1;
      break;
   case SM4_OPCODE_HS_JOIN_PHASE:
      phase = 2;
      ++subPhaseCnt[1];
      break;
   case SM4_OPCODE_DCL_HS_FORK_PHASE_INSTANCE_COUNT:
   case SM4_OPCODE_DCL_HS_JOIN_PHASE_INSTANCE_COUNT:
   case SM4_OPCODE_DCL_HS_MAX_TESSFACTOR:
      break;

      // weird stuff
   case SM4_OPCODE_DCL_THREAD_GROUP:
   case SM4_OPCODE_DCL_UNORDERED_ACCESS_VIEW_TYPED:
   case SM4_OPCODE_DCL_UNORDERED_ACCESS_VIEW_RAW:
   case SM4_OPCODE_DCL_UNORDERED_ACCESS_VIEW_STRUCTURED:
   case SM4_OPCODE_DCL_THREAD_GROUP_SHARED_MEMORY_RAW:
   case SM4_OPCODE_DCL_THREAD_GROUP_SHARED_MEMORY_STRUCTURED:
   case SM4_OPCODE_DCL_RESOURCE_RAW:
   case SM4_OPCODE_DCL_RESOURCE_STRUCTURED:
      ERROR("unhandled declaration\n");
      abort();
      return false;

   default:
      assert(!"invalid SM4 declaration");
      return false;
   }
   return true;
}

void
Converter::allocateValues()
{
   lData = new DataArray[nrArrays];

   for (unsigned int i = 0; i < nrArrays; ++i)
      lData[i].setParent(this);

   tData32.setup(0, nrRegVals, 4, 4, FILE_GPR);
   tData64.setup(0, nrRegVals, 2, 8, FILE_GPR);

   if (prog->getType() == Program::TYPE_FRAGMENT)
      oData.setup(0, info.numOutputs, 4, 4, FILE_GPR);
}

bool Converter::handleDeclaration(const sm4_dcl& dcl)
{
   switch (dcl.opcode) {
   case SM4_OPCODE_DCL_INDEXABLE_TEMP:
      lData[nrArrays++].setup(arrayVol,
                              dcl.indexable_temp.num, dcl.indexable_temp.comps,
                              4, FILE_MEMORY_LOCAL);
      arrayVol += dcl.indexable_temp.num * dcl.indexable_temp.comps * 4;
      break;
   case SM4_OPCODE_HS_FORK_PHASE:
      if (subPhaseCnt[0])
         phaseInstCnt[0][subPhaseCnt[0]] = phaseInstCnt[0][subPhaseCnt[0] - 1];
      ++subPhaseCnt[0];
      break;
   case SM4_OPCODE_HS_JOIN_PHASE:
      if (subPhaseCnt[1])
         phaseInstCnt[1][subPhaseCnt[1]] = phaseInstCnt[1][subPhaseCnt[1] - 1];
      ++subPhaseCnt[1];
      break;
   case SM4_OPCODE_DCL_HS_FORK_PHASE_INSTANCE_COUNT:
      phaseInstCnt[0][subPhaseCnt[0] - 1] = dcl.num;
      break;
   case SM4_OPCODE_DCL_HS_JOIN_PHASE_INSTANCE_COUNT:
      phaseInstCnt[1][subPhaseCnt[1] - 1] = dcl.num;
      break;

   default:
      break; // already handled in inspection
   }

   return true;
}

Symbol *
Converter::iSym(int i, int c)
{
   if (info.in[i].regular) {
      return mkSymbol(FILE_SHADER_INPUT, 0, sTy, info.in[i].slot[c] * 4);
   } else {
      return mkSysVal(tgsi::irSemantic(info.in[i].sn), info.in[i].si);
   }
}

Symbol *
Converter::oSym(int i, int c)
{
   if (info.out[i].regular) {
      return mkSymbol(FILE_SHADER_OUTPUT, 0, dTy, info.out[i].slot[c] * 4);
   } else {
      return mkSysVal(tgsi::irSemantic(info.out[i].sn), info.out[i].si);
   }
}

Value *
Converter::getSrcPtr(int s, int dim, int shl)
{
   if (srcPtr[s][dim])
      return srcPtr[s][dim];

   sm4_op *op = insn->ops[s + nDstOpnds]->indices[dim].reg.get();

   if (!op)
      return NULL;

   Value *index = src(*op, 0, s);

   srcPtr[s][dim] = index;
   if (shl)
      srcPtr[s][dim] = mkOp2v(OP_SHL, TYPE_U32, getSSA(), index, mkImm(shl));
   return srcPtr[s][dim];
}

Value *
Converter::getDstPtr(int d, int dim, int shl)
{
   assert(d == 0);
   if (dstPtr[dim])
      return dstPtr[dim];

   sm4_op *op = insn->ops[d]->indices[dim].reg.get();
   if (!op)
      return NULL;

   Value *index = src(*op, 0, d);
   if (shl)
      index = mkOp2v(OP_SHL, TYPE_U32, getSSA(), index, mkImm(shl));

   return (dstPtr[dim] = index);
}

Value *
Converter::getVtxPtr(int s)
{
   assert(s < 3);
   if (vtxBase[s])
      return vtxBase[s];

   sm4_op *op = insn->ops[s + nDstOpnds].get();
   if (!op)
      return NULL;
   int idx = op->indices[0].disp;

   vtxBase[s] = getSrcPtr(s, 0, 0);
   vtxBase[s] = mkOp2v(OP_PFETCH, TYPE_U32, getSSA(), mkImm(idx), vtxBase[s]);
   return vtxBase[s];
}

Value *
Converter::src(int i, int c)
{
   return src(*insn->ops[i + nDstOpnds], c, i);
}

Value *
Converter::dst(int i, int c)
{
   return dst(*insn->ops[i], c, i);
}

void
Converter::saveDst(int i, int c, Value *value)
{
   if (insn->insn.sat)
      mkOp1(OP_SAT, dTy, value, value);
   return saveDst(*insn->ops[i], c, value, i);
}

Value *
Converter::interpolate(const sm4_op& op, int c, int i)
{
   int idx = op.indices[0].disp;
   int swz = op.swizzle[c];
   operation opr =
      (info.in[idx].linear || info.in[idx].flat) ? OP_LINTERP : OP_PINTERP;

   Value *ptr = getSrcPtr(i, 0, 4);

   Instruction *insn = new_Instruction(func, opr, TYPE_F32);

   insn->setDef(0, getScratch());
   insn->setSrc(0, iSym(idx, swz));
   if (opr == OP_PINTERP)
      insn->setSrc(1, fragCoord[3]);
   if (ptr)
      insn->setIndirect(0, 0, ptr);

   insn->setInterpolate(interpMode[idx]);

   bb->insertTail(insn);
   return insn->getDef(0);
}

Value *
Converter::src(const sm4_op& op, int c, int s)
{
   const int size = typeSizeof(sTy);

   Instruction *ld;
   Value *res, *ptr, *vtx;
   int idx, dim, off;
   const int swz = op.swizzle[c];

   switch (op.file) {
   case SM4_FILE_IMMEDIATE32:
      res = loadImm(NULL, (uint32_t)op.imm_values[swz].u32);
      break;
   case SM4_FILE_IMMEDIATE64:
      assert(c < 2);
      res = loadImm(NULL, op.imm_values[swz].u64);
      break;
   case SM4_FILE_TEMP:
      assert(op.is_index_simple(0));
      idx = op.indices[0].disp;
      if (size == 8)
         res = tData64.load(idx, swz, NULL);
      else
         res = tData32.load(idx, swz, NULL);
      break;
   case SM4_FILE_INPUT:
   case SM4_FILE_INPUT_CONTROL_POINT:
   case SM4_FILE_INPUT_PATCH_CONSTANT:
      if (prog->getType() == Program::TYPE_FRAGMENT)
         return interpolate(op, c, s);

      idx = 0;
      if (op.file == SM4_FILE_INPUT_PATCH_CONSTANT)
         idx = info.numInputs - info.numPatchConstants;

      if (op.num_indices == 2) {
         vtx = getVtxPtr(s);
         ptr = getSrcPtr(s, 1, 4);
         idx += op.indices[1].disp;
         res = getSSA();
         ld = mkOp1(OP_VFETCH, TYPE_U32, res, iSym(idx, swz));
         ld->setIndirect(0, 0, ptr);
         ld->setIndirect(0, 1, vtx);
      } else {
         idx += op.indices[0].disp;
         res = mkLoad(sTy, iSym(idx, swz), getSrcPtr(s, 0, 4));
      }
      if (op.file == SM4_FILE_INPUT_PATCH_CONSTANT)
         res->defs->getInsn()->perPatch = 1;
      break;
   case SM4_FILE_CONSTANT_BUFFER:
      assert(op.num_indices == 2);
      assert(op.is_index_simple(0));

      ptr = getSrcPtr(s, 1, 4);
      dim = op.indices[0].disp;
      off = (op.indices[1].disp * 4 + swz) * (sTy == TYPE_F64 ? 8 : 4);

      res = mkLoad(sTy, mkSymbol(FILE_MEMORY_CONST, dim, sTy, off), ptr);
      break;
   case SM4_FILE_IMMEDIATE_CONSTANT_BUFFER:
      ptr = getSrcPtr(s, 0, 4);
      off = (op.indices[0].disp * 4 + swz) * 4;
      res = mkLoad(sTy, mkSymbol(FILE_MEMORY_CONST, 14, sTy, off), ptr);
      break;
   case SM4_FILE_INDEXABLE_TEMP:
   {
      assert(op.is_index_simple(0));
      int a = op.indices[0].disp;
      idx = op.indices[1].disp;
      res = lData[a].load(idx, swz, getSrcPtr(s, 1, 4));
   }
      break;
   case SM4_FILE_INPUT_PRIMITIVEID:
      recordSV(TGSI_SEMANTIC_PRIMID, 0, 1, true);
      res = mkOp1v(OP_RDSV, TYPE_U32, getSSA(), mkSysVal(SV_PRIMITIVE_ID, 0));
      break;
   case SM4_FILE_INPUT_GS_INSTANCE_ID:
   case SM4_FILE_OUTPUT_CONTROL_POINT_ID:
      recordSV(NV50_SEMANTIC_INVOCATIONID, 0, 1, true);
      res = mkOp1v(OP_RDSV, TYPE_U32, getSSA(), mkSysVal(SV_INVOCATION_ID, 0));
      break;
   case SM4_FILE_CYCLE_COUNTER:
      res =
         mkOp1v(OP_RDSV, TYPE_U32, getSSA(), mkSysVal(SV_CLOCK, swz ? 1 : 0));
      break;
   case SM4_FILE_INPUT_FORK_INSTANCE_ID:
   case SM4_FILE_INPUT_JOIN_INSTANCE_ID:
   {
      phaseInstanceUsed = true;
      if (unrollPhase)
         return loadImm(NULL, phaseInstance);
      const unsigned int cnt = phaseInstCnt[phase - 1][subPhase];
      res = getScratch();
      res = mkOp1v(OP_RDSV, TYPE_U32, res, mkSysVal(SV_INVOCATION_ID, 0));
      res = mkOp2v(OP_MIN, TYPE_U32, res, res, loadImm(NULL, cnt - 1));
   }
      break;
   case SM4_FILE_INPUT_DOMAIN_POINT:
      assert(swz < 3);
      res = domainPt[swz];
      break;
   case SM4_FILE_THREAD_GROUP_SHARED_MEMORY:
      off = (op.indices[0].disp * 4 + swz) * (sTy == TYPE_F64 ? 8 : 4);
      ptr = getSrcPtr(s, 0, 4);
      res = mkLoad(sTy, mkSymbol(FILE_MEMORY_SHARED, 0, sTy, off), ptr);
      break;
   case SM4_FILE_RESOURCE:
   case SM4_FILE_SAMPLER:
   case SM4_FILE_UNORDERED_ACCESS_VIEW:
      return NULL;
   case SM4_FILE_INPUT_THREAD_ID:
      res = mkOp1v(OP_RDSV, TYPE_U32, getSSA(), mkSysVal(SV_TID, swz));
      break;
   case SM4_FILE_INPUT_THREAD_GROUP_ID:
      res = mkOp1v(OP_RDSV, TYPE_U32, getSSA(), mkSysVal(SV_CTAID, swz));
      break;
   case SM4_FILE_FUNCTION_INPUT:
   case SM4_FILE_INPUT_THREAD_ID_IN_GROUP:
      assert(!"unhandled source file");
      return NULL;
   default:
      assert(!"invalid source file");
      return NULL;
   }

   if (op.abs)
      res = mkOp1v(OP_ABS, sTy, getSSA(res->reg.size), res);
   if (op.neg)
      res = mkOp1v(OP_NEG, sTy, getSSA(res->reg.size), res);
   return res;
}

Value *
Converter::dst(const sm4_op &op, int c, int i)
{
   switch (op.file) {
   case SM4_FILE_TEMP:
      return tData32.acquire(op.indices[0].disp, c);
   case SM4_FILE_INDEXABLE_TEMP:
      return getScratch();
   case SM4_FILE_OUTPUT:
      if (prog->getType() == Program::TYPE_FRAGMENT)
         return oData.acquire(op.indices[0].disp, c);
      return getScratch();
   case SM4_FILE_NULL:
      return NULL;
   case SM4_FILE_OUTPUT_DEPTH:
   case SM4_FILE_OUTPUT_DEPTH_GREATER_EQUAL:
   case SM4_FILE_OUTPUT_DEPTH_LESS_EQUAL:
   case SM4_FILE_OUTPUT_COVERAGE_MASK:
      return getScratch();
   case SM4_FILE_IMMEDIATE32:
   case SM4_FILE_IMMEDIATE64:
   case SM4_FILE_CONSTANT_BUFFER:
   case SM4_FILE_RESOURCE:
   case SM4_FILE_SAMPLER:
   case SM4_FILE_UNORDERED_ACCESS_VIEW:
      assert(!"invalid destination file");
      return NULL;
   default:
      assert(!"invalid file");
      return NULL;
   }
}

void
Converter::saveFragDepth(operation op, Value *value)
{
   if (op == OP_MIN || op == OP_MAX) {
      Value *zIn;
      zIn = mkOp1v(OP_RDSV, TYPE_F32, getSSA(), mkSysVal(SV_POSITION, 2));
      value = mkOp2v(op, TYPE_F32, getSSA(), value, zIn);
   }
   oData.store(info.io.fragDepth, 2, NULL, value);
}

void
Converter::saveDst(const sm4_op &op, int c, Value *value, int s)
{
   Symbol *sym;
   Instruction *st;
   int a, idx;

   switch (op.file) {
   case SM4_FILE_TEMP:
      idx = op.indices[0].disp;
      tData32.store(idx, c, NULL, value);
      break;
   case SM4_FILE_INDEXABLE_TEMP:
      a = op.indices[0].disp;
      idx = op.indices[1].disp;
      // FIXME: shift is wrong, depends in lData
      lData[a].store(idx, c, getDstPtr(s, 1, 4), value);
      break;
   case SM4_FILE_OUTPUT:
      assert(op.num_indices == 1);
      idx = op.indices[0].disp;
      if (prog->getType() == Program::TYPE_FRAGMENT) {
         oData.store(idx, c, NULL, value);
      } else {
         if (phase)
            idx += info.numOutputs - info.numPatchConstants;
         const int shl = (info.out[idx].sn == NV50_SEMANTIC_TESSFACTOR) ? 2 : 4;
         sym = oSym(idx, c);
         if (sym->reg.file == FILE_SHADER_OUTPUT)
            st = mkStore(OP_EXPORT, dTy, sym, getDstPtr(s, 0, shl), value);
         else
            st = mkStore(OP_WRSV, dTy, sym, getDstPtr(s, 0, 2), value);
         st->perPatch = phase ? 1 : 0;
      }
      break;
   case SM4_FILE_OUTPUT_DEPTH_GREATER_EQUAL:
      saveFragDepth(OP_MAX, value);
      break;
   case SM4_FILE_OUTPUT_DEPTH_LESS_EQUAL:
      saveFragDepth(OP_MIN, value);
      break;
   case SM4_FILE_OUTPUT_DEPTH:
      saveFragDepth(OP_NOP, value);
      break;
   case SM4_FILE_OUTPUT_COVERAGE_MASK:
      oData.store(info.io.sampleMask, 0, NULL, value);
      break;
   case SM4_FILE_IMMEDIATE32:
   case SM4_FILE_IMMEDIATE64:
   case SM4_FILE_INPUT:
   case SM4_FILE_CONSTANT_BUFFER:
   case SM4_FILE_RESOURCE:
   case SM4_FILE_SAMPLER:
      assert(!"invalid destination file");
      return;
   default:
      assert(!"invalid file");
      return;
   }
}

void
Converter::emitTex(Value *dst0[4], TexInstruction *tex, const uint8_t swz[4])
{
   Value *res[4] = { NULL, NULL, NULL, NULL };
   unsigned int c, d;

   for (c = 0; c < 4; ++c)
      if (dst0[c])
         tex->tex.mask |= 1 << swz[c];
   for (d = 0, c = 0; c < 4; ++c)
      if (tex->tex.mask & (1 << c))
         tex->setDef(d++, (res[c] = getScratch()));

   bb->insertTail(tex);

   if (insn->opcode == SM4_OPCODE_RESINFO) {
      if (tex->tex.target.getDim() == 1) {
	 res[2] = loadImm(NULL, 0);
         if (!tex->tex.target.isArray())
            res[1] = res[2];
      } else
      if (tex->tex.target.getDim() == 2 && !tex->tex.target.isArray()) {
         res[2] = loadImm(NULL, 0);
      }
      for (c = 0; c < 4; ++c) {
         if (!dst0[c])
            continue;
         Value *src = res[swz[c]];
         assert(src);
         switch (insn->insn.resinfo_return_type) {
         case 0:
            mkCvt(OP_CVT, TYPE_F32, dst0[c], TYPE_U32, src);
            break;
         case 1:
            mkCvt(OP_CVT, TYPE_F32, dst0[c], TYPE_U32, src);
            if (swz[c] < tex->tex.target.getDim())
               mkOp1(OP_RCP, TYPE_F32, dst0[c], dst0[c]);
            break;
         default:
            mkMov(dst0[c], src);
            break;
         }
      }
   } else {
      for (c = 0; c < 4; ++c)
         if (dst0[c])
            mkMov(dst0[c], res[swz[c]]);
   }
}

void
Converter::handleQUERY(Value *dst0[4], enum TexQuery query)
{
   TexInstruction *texi = new_TexInstruction(func, OP_TXQ);
   texi->tex.query = query;

   assert(insn->ops[2]->file == SM4_FILE_RESOURCE); // TODO: UAVs

   const int rOp = (query == TXQ_DIMS) ? 2 : 1;
   const int sOp = (query == TXQ_DIMS) ? 0 : 1;

   const int tR = insn->ops[rOp]->indices[0].disp;

   texi->setTexture(resourceType[tR][0], tR, 0);

   texi->setSrc(0, src(sOp, 0)); // mip level or sample index

   emitTex(dst0, texi, insn->ops[rOp]->swizzle);
}

void
Converter::handleLOAD(Value *dst0[4])
{
   TexInstruction *texi = new_TexInstruction(func, OP_TXF);
   unsigned int c;

   const int tR = insn->ops[2]->indices[0].disp;

   texi->setTexture(resourceType[tR][0], tR, 0);

   for (c = 0; c < texi->tex.target.getArgCount(); ++c)
      texi->setSrc(c, src(0, c));

   if (texi->tex.target == TEX_TARGET_BUFFER) {
      texi->tex.levelZero = true;
   } else {
      texi->setSrc(c++, src(0, 3));
      for (c = 0; c < 3; ++c) {
         texi->tex.offset[0][c] = insn->sample_offset[c];
	 if (texi->tex.offset[0][c])
            texi->tex.useOffsets = 1;
      }
   }

   emitTex(dst0, texi, insn->ops[2]->swizzle);
}

// order of nv50 ir sources: x y z/layer lod/bias dc
void
Converter::handleSAMPLE(operation opr, Value *dst0[4])
{
   TexInstruction *texi = new_TexInstruction(func, opr);
   unsigned int c, s;
   Value *arg[4], *src0[4];
   Value *val;
   Value *lod = NULL, *dc = NULL;

   const int tR = insn->ops[2]->indices[0].disp;
   const int tS = insn->ops[3]->indices[0].disp;

   TexInstruction::Target tgt = resourceType[tR][shadow[tS] ? 1 : 0];

   for (c = 0; c < tgt.getArgCount(); ++c)
      arg[c] = src0[c] = src(0, c);

   if (insn->opcode == SM4_OPCODE_SAMPLE_L ||
       insn->opcode == SM4_OPCODE_SAMPLE_B) {
      lod = src(3, 0);
   } else
   if (insn->opcode == SM4_OPCODE_SAMPLE_C ||
       insn->opcode == SM4_OPCODE_SAMPLE_C_LZ) {
      dc = src(3, 0);
      if (insn->opcode == SM4_OPCODE_SAMPLE_C_LZ)
         texi->tex.levelZero = true;
   } else
   if (insn->opcode == SM4_OPCODE_SAMPLE_D) {
      for (c = 0; c < tgt.getDim(); ++c) {
         texi->dPdx[c] = src(3, c);
         texi->dPdy[c] = src(4, c);
      }
   }

   if (tgt.isCube()) {
      for (c = 0; c < 3; ++c)
         src0[c] = mkOp1v(OP_ABS, TYPE_F32, getSSA(), arg[c]);
      val = getScratch();
      mkOp2(OP_MAX, TYPE_F32, val, src0[0], src0[1]);
      mkOp2(OP_MAX, TYPE_F32, val, src0[2], val);
      mkOp1(OP_RCP, TYPE_F32, val, val);
      for (c = 0; c < 3; ++c)
         src0[c] = mkOp2v(OP_MUL, TYPE_F32, getSSA(), arg[c], val);
   }

   for (s = 0; s < tgt.getArgCount(); ++s)
      texi->setSrc(s, src0[s]);
   if (lod)
      texi->setSrc(s++, lod);
   if (dc)
      texi->setSrc(s++, dc);

   for (c = 0; c < 3; ++c) {
      texi->tex.offset[0][c] = insn->sample_offset[c];
      if (texi->tex.offset[0][c])
         texi->tex.useOffsets = 1;
   }

   texi->setTexture(tgt, tR, tS);

   emitTex(dst0, texi, insn->ops[2]->swizzle);
}

void
Converter::handleDP(Value *dst0[4], int dim)
{
   Value *src0 = src(0, 0), *src1 = src(1, 0);
   Value *dotp = getScratch();

   assert(dim > 0);

   mkOp2(OP_MUL, TYPE_F32, dotp, src0, src1);
   for (int c = 1; c < dim; ++c)
      mkOp3(OP_MAD, TYPE_F32, dotp, src(0, c), src(1, c), dotp);

   for (int c = 0; c < 4; ++c)
      dst0[c] = dotp;
}

void
Converter::insertConvergenceOps(BasicBlock *conv, BasicBlock *fork)
{
   FlowInstruction *join = new_FlowInstruction(func, OP_JOIN, NULL);
   join->fixed = 1;
   conv->insertHead(join);

   fork->joinAt = new_FlowInstruction(func, OP_JOINAT, conv);
   fork->insertBefore(fork->getExit(), fork->joinAt);
}

void
Converter::finalizeShader()
{
   if (finalized)
      return;
   BasicBlock *epilogue = reinterpret_cast<BasicBlock *>(leaveBBs.pop().u.p);
   entryBBs.pop();

   finalized = true;

   bb->cfg.attach(&epilogue->cfg, Graph::Edge::TREE);
   setPosition(epilogue, true);

   if (prog->getType() == Program::TYPE_FRAGMENT)
      exportOutputs();

   mkOp(OP_EXIT, TYPE_NONE, NULL)->terminator = 1;
}

#define FOR_EACH_DST0_ENABLED_CHANNEL32(chan)         \
   for ((chan) = 0; (chan) < 4; ++(chan))             \
      if (insn->ops[0].get()->mask & (1 << (chan)))

#define FOR_EACH_DST0_ENABLED_CHANNEL64(chan)         \
   for ((chan) = 0; (chan) < 2; ++(chan))             \
      if (insn->ops[0].get()->mask & (1 << (chan)))

bool
Converter::checkDstSrcAliasing() const
{
   for (unsigned int d = 0; d < nDstOpnds; ++d) {
      for (unsigned int s = nDstOpnds; s < insn->num_ops; ++s) {
         if (insn->ops[d]->file != insn->ops[s]->file)
            continue;
         int i = insn->ops[s]->num_indices - 1;
         if (i != insn->ops[d]->num_indices - 1)
            continue;
         if (insn->ops[d]->is_index_simple(i) &&
             insn->ops[s]->is_index_simple(i) &&
             insn->ops[d]->indices[i].disp == insn->ops[s]->indices[i].disp)
            return true;
      }
   }
   return false;
}

bool
Converter::handleInstruction(unsigned int pos)
{
   Value *dst0[4], *rDst0[4];
   Value *dst1[4], *rDst1[4];
   int c, nc;

   insn = sm4.insns[pos];
   enum sm4_opcode opcode = static_cast<sm4_opcode>(insn->opcode);

   operation op = cvtOpcode(opcode);

   sTy = inferSrcType(opcode);
   dTy = inferDstType(opcode);

   nc = dTy == TYPE_F64 ? 2 : 4;

   nDstOpnds = getDstOpndCount(opcode);

   bool useScratchDst = checkDstSrcAliasing();

   INFO("SM4_OPCODE_##%u, aliasing = %u\n", insn->opcode, useScratchDst);

   if (nDstOpnds >= 1) {
      for (c = 0; c < nc; ++c)
         rDst0[c] = dst0[c] =
            insn->ops[0].get()->mask & (1 << c) ? dst(0, c) : NULL;
      if (useScratchDst)
         for (c = 0; c < nc; ++c)
            dst0[c] = rDst0[c] ? getScratch() : NULL;
   }

   if (nDstOpnds >= 2) {
      for (c = 0; c < nc; ++c)
         rDst1[c] = dst1[c] =
            insn->ops[1].get()->mask & (1 << c) ? dst(1, c) : NULL;
      if (useScratchDst)
         for (c = 0; c < nc; ++c)
            dst1[c] = rDst1[c] ? getScratch() : NULL;
   }

   switch (insn->opcode) {
   case SM4_OPCODE_ADD:
   case SM4_OPCODE_AND:
   case SM4_OPCODE_DIV:
   case SM4_OPCODE_IADD:
   case SM4_OPCODE_IMAX:
   case SM4_OPCODE_IMIN:
   case SM4_OPCODE_MIN:
   case SM4_OPCODE_MAX:
   case SM4_OPCODE_MUL:
   case SM4_OPCODE_OR:
   case SM4_OPCODE_UMAX:
   case SM4_OPCODE_UMIN:
   case SM4_OPCODE_XOR:
      FOR_EACH_DST0_ENABLED_CHANNEL32(c) {
         Instruction *insn = mkOp2(op, dTy, dst0[c], src(0, c), src(1, c));
         if (dTy == TYPE_F32)
            insn->ftz = 1;
      }
      break;

   case SM4_OPCODE_ISHL:
   case SM4_OPCODE_ISHR:
   case SM4_OPCODE_USHR:
      FOR_EACH_DST0_ENABLED_CHANNEL32(c) {
         Instruction *insn = mkOp2(op, dTy, dst0[c], src(0, c), src(1, c));
         insn->subOp = NV50_IR_SUBOP_SHIFT_WRAP;
      }
      break;

   case SM4_OPCODE_IMAD:
   case SM4_OPCODE_MAD:
   case SM4_OPCODE_UMAD:
      FOR_EACH_DST0_ENABLED_CHANNEL32(c) {
         mkOp3(OP_MAD, dTy, dst0[c], src(0, c), src(1, c), src(2, c));
      }
      break;

   case SM4_OPCODE_DADD:
   case SM4_OPCODE_DMAX:
   case SM4_OPCODE_DMIN:
   case SM4_OPCODE_DMUL:
      FOR_EACH_DST0_ENABLED_CHANNEL64(c) {
         mkOp2(op, dTy, dst0[c], src(0, c), src(1, c));
      }
      break;

   case SM4_OPCODE_UDIV:
      for (c = 0; c < 4; ++c) {
         Value *dvn, *dvs;
         if (dst0[c] || dst1[c]) {
            dvn = src(0, c);
            dvs = src(1, c);
         }
         if (dst0[c])
            mkOp2(OP_DIV, TYPE_U32, dst0[c], dvn, dvs);
         if (dst1[c])
            mkOp2(OP_MOD, TYPE_U32, dst1[c], dvn, dvs);
      }
      break;

   case SM4_OPCODE_IMUL:
   case SM4_OPCODE_UMUL:
      for (c = 0; c < 4; ++c) {
         Value *a, *b;
         if (dst0[c] || dst1[c]) {
            a = src(0, c);
            b = src(1, c);
         }
         if (dst0[c])
            mkOp2(OP_MUL, dTy, dst0[c], a, b)->subOp =
               NV50_IR_SUBOP_MUL_HIGH;
         if (dst1[c])
            mkOp2(OP_MUL, dTy, dst1[c], a, b);
      }
      break;

   case SM4_OPCODE_DP2:
      handleDP(dst0, 2);
      break;
   case SM4_OPCODE_DP3:
      handleDP(dst0, 3);
      break;
   case SM4_OPCODE_DP4:
      handleDP(dst0, 4);
      break;

   case SM4_OPCODE_DERIV_RTX:
   case SM4_OPCODE_DERIV_RTX_COARSE:
   case SM4_OPCODE_DERIV_RTX_FINE:
   case SM4_OPCODE_DERIV_RTY:
   case SM4_OPCODE_DERIV_RTY_COARSE:
   case SM4_OPCODE_DERIV_RTY_FINE:
   case SM4_OPCODE_MOV:
   case SM4_OPCODE_INEG:
   case SM4_OPCODE_NOT:
   case SM4_OPCODE_SQRT:
   case SM4_OPCODE_COUNTBITS:
   case SM4_OPCODE_EXP:
   case SM4_OPCODE_LOG:
   case SM4_OPCODE_RCP:
      FOR_EACH_DST0_ENABLED_CHANNEL32(c) {
         mkOp1(op, dTy, dst0[c], src(0, c));
      }
      break;

   case SM4_OPCODE_FRC:
      FOR_EACH_DST0_ENABLED_CHANNEL32(c) {
         Value *val = getScratch();
         Value *src0 = src(0, c);
         mkOp1(OP_FLOOR, TYPE_F32, val, src0);
         mkOp2(OP_SUB, TYPE_F32, dst0[c], src0, val);
      }
      break;

   case SM4_OPCODE_MOVC:
      FOR_EACH_DST0_ENABLED_CHANNEL32(c)
         mkCmp(OP_SLCT, CC_NE, TYPE_U32, dst0[c], src(1, c), src(2, c),
               src(0, c));
      break;

   case SM4_OPCODE_ROUND_NE:
   case SM4_OPCODE_ROUND_NI:
   case SM4_OPCODE_ROUND_PI:
   case SM4_OPCODE_ROUND_Z:
      FOR_EACH_DST0_ENABLED_CHANNEL32(c) {
         Instruction *rnd = mkOp1(op, dTy, dst0[c], src(0, c));
         rnd->ftz = 1;
         rnd->rnd = cvtRoundingMode(opcode);
      }
      break;

   case SM4_OPCODE_RSQ:
      FOR_EACH_DST0_ENABLED_CHANNEL32(c)
         mkOp1(op, dTy, dst0[c], src(0, c));
      break;

   case SM4_OPCODE_SINCOS:
      for (c = 0; c < 4; ++c) {
         if (!dst0[c] && !dst1[c])
            continue;
         Value *val = mkOp1v(OP_PRESIN, TYPE_F32, getScratch(), src(0, c));
         if (dst0[c])
            mkOp1(OP_SIN, TYPE_F32, dst0[c], val);
         if (dst1[c])
            mkOp1(OP_COS, TYPE_F32, dst1[c], val);
      }
      break;

   case SM4_OPCODE_EQ:
   case SM4_OPCODE_GE:
   case SM4_OPCODE_IEQ:
   case SM4_OPCODE_IGE:
   case SM4_OPCODE_ILT:
   case SM4_OPCODE_LT:
   case SM4_OPCODE_NE:
   case SM4_OPCODE_INE:
   case SM4_OPCODE_ULT:
   case SM4_OPCODE_UGE:
   case SM4_OPCODE_DEQ:
   case SM4_OPCODE_DGE:
   case SM4_OPCODE_DLT:
   case SM4_OPCODE_DNE:
   {
      CondCode cc = cvtCondCode(opcode);
      FOR_EACH_DST0_ENABLED_CHANNEL32(c) {
         CmpInstruction *set;
         set = mkCmp(op, cc, sTy, dst0[c], src(0, c), src(1, c), NULL);
         set->setType(dTy, sTy);
         if (sTy == TYPE_F32)
            set->ftz = 1;
      }
   }
      break;

   case SM4_OPCODE_FTOI:
   case SM4_OPCODE_FTOU:
      FOR_EACH_DST0_ENABLED_CHANNEL32(c)
         mkCvt(op, dTy, dst0[c], sTy, src(0, c))->rnd = ROUND_Z;
      break;
   case SM4_OPCODE_ITOF:
   case SM4_OPCODE_UTOF:
   case SM4_OPCODE_F32TOF16:
   case SM4_OPCODE_F16TOF32:
   case SM4_OPCODE_DTOF:
   case SM4_OPCODE_FTOD:
      FOR_EACH_DST0_ENABLED_CHANNEL32(c)
         mkCvt(op, dTy, dst0[c], sTy, src(0, c));
      break;

   case SM4_OPCODE_CUT:
   case SM4_OPCODE_CUT_STREAM:
      mkOp1(OP_RESTART, TYPE_U32, NULL, mkImm(0))->fixed = 1;
      break;
   case SM4_OPCODE_EMIT:
   case SM4_OPCODE_EMIT_STREAM:
      mkOp1(OP_EMIT, TYPE_U32, NULL, mkImm(0))->fixed = 1;
      break;
   case SM4_OPCODE_EMITTHENCUT:
   case SM4_OPCODE_EMITTHENCUT_STREAM:
   {
      Instruction *cut = mkOp1(OP_EMIT, TYPE_U32, NULL,  mkImm(0));
      cut->fixed = 1;
      cut->subOp = NV50_IR_SUBOP_EMIT_RESTART;
   }
      break;

   case SM4_OPCODE_DISCARD:
      info.prop.fp.usesDiscard = TRUE;
      mkOp(OP_DISCARD, TYPE_NONE, NULL)->setPredicate(
         insn->insn.test_nz ? CC_P : CC_NOT_P, src(0, 0));
      break;

   case SM4_OPCODE_CALL:
   case SM4_OPCODE_CALLC:
      assert(!"CALL/CALLC not implemented");
      break;

   case SM4_OPCODE_RET:
      // XXX: the following doesn't work with subroutines / early ret
      if (!haveNextPhase(pos))
         finalizeShader();
      else
         phaseEnded = phase + 1;
      break;

   case SM4_OPCODE_IF:
   {
      BasicBlock *ifClause = new BasicBlock(func);

      bb->cfg.attach(&ifClause->cfg, Graph::Edge::TREE);
      condBBs.push(bb);
      joinBBs.push(bb);

      mkFlow(OP_BRA, NULL, insn->insn.test_nz ? CC_NOT_P : CC_P, src(0, 0));

      setPosition(ifClause, true);
   }
      break;
   case SM4_OPCODE_ELSE:
   {
      BasicBlock *elseClause = new BasicBlock(func);
      BasicBlock *forkPoint = reinterpret_cast<BasicBlock *>(condBBs.pop().u.p);

      forkPoint->cfg.attach(&elseClause->cfg, Graph::Edge::TREE);
      condBBs.push(bb);

      forkPoint->getExit()->asFlow()->target.bb = elseClause;
      if (!bb->isTerminated())
         mkFlow(OP_BRA, NULL, CC_ALWAYS, NULL);

      setPosition(elseClause, true);
   }
      break;
   case SM4_OPCODE_ENDIF:
   {
      BasicBlock *convPoint = new BasicBlock(func);
      BasicBlock *lastBB = reinterpret_cast<BasicBlock *>(condBBs.pop().u.p);
      BasicBlock *forkPoint = reinterpret_cast<BasicBlock *>(joinBBs.pop().u.p);

      if (!bb->isTerminated()) {
         // we only want join if none of the clauses ended with CONT/BREAK/RET
         if (lastBB->getExit()->op == OP_BRA && joinBBs.getSize() < 6)
            insertConvergenceOps(convPoint, forkPoint);
         mkFlow(OP_BRA, convPoint, CC_ALWAYS, NULL);
         bb->cfg.attach(&convPoint->cfg, Graph::Edge::FORWARD);
      }

      if (lastBB->getExit()->op == OP_BRA) {
         lastBB->cfg.attach(&convPoint->cfg, Graph::Edge::FORWARD);
         lastBB->getExit()->asFlow()->target.bb = convPoint;
      }
      setPosition(convPoint, true);
   }
      break;

   case SM4_OPCODE_SWITCH:
   case SM4_OPCODE_CASE:
   case SM4_OPCODE_ENDSWITCH:
      assert(!"SWITCH/CASE/ENDSWITCH not implemented");
      break;

   case SM4_OPCODE_LOOP:
   {
      BasicBlock *loopHeader = new BasicBlock(func);
      BasicBlock *loopBreak = new BasicBlock(func);

      loopBBs.push(loopHeader);
      breakBBs.push(loopBreak);
      if (loopBBs.getSize() > func->loopNestingBound)
         func->loopNestingBound++;

      mkFlow(OP_PREBREAK, loopBreak, CC_ALWAYS, NULL);

      bb->cfg.attach(&loopHeader->cfg, Graph::Edge::TREE);
      setPosition(loopHeader, true);
      mkFlow(OP_PRECONT, loopHeader, CC_ALWAYS, NULL);
   }
      break;
   case SM4_OPCODE_ENDLOOP:
   {
      BasicBlock *loopBB = reinterpret_cast<BasicBlock *>(loopBBs.pop().u.p);

      if (!bb->isTerminated()) {
         mkFlow(OP_CONT, loopBB, CC_ALWAYS, NULL);
         bb->cfg.attach(&loopBB->cfg, Graph::Edge::BACK);
      }
      setPosition(reinterpret_cast<BasicBlock *>(breakBBs.pop().u.p), true);
   }
      break;
   case SM4_OPCODE_BREAK:
   {
      if (bb->isTerminated())
         break;
      BasicBlock *breakBB = reinterpret_cast<BasicBlock *>(breakBBs.peek().u.p);
      mkFlow(OP_BREAK, breakBB, CC_ALWAYS, NULL);
      bb->cfg.attach(&breakBB->cfg, Graph::Edge::CROSS);
   }
      break;
   case SM4_OPCODE_BREAKC:
   {
      BasicBlock *nextBB = new BasicBlock(func);
      BasicBlock *breakBB = reinterpret_cast<BasicBlock *>(breakBBs.peek().u.p);
      CondCode cc = insn->insn.test_nz ? CC_P : CC_NOT_P;
      mkFlow(OP_BREAK, breakBB, cc, src(0, 0));
      bb->cfg.attach(&breakBB->cfg, Graph::Edge::CROSS);
      bb->cfg.attach(&nextBB->cfg, Graph::Edge::FORWARD);
      setPosition(nextBB, true);
   }
      break;
   case SM4_OPCODE_CONTINUE:
   {
      if (bb->isTerminated())
         break;
      BasicBlock *contBB = reinterpret_cast<BasicBlock *>(loopBBs.peek().u.p);
      mkFlow(OP_CONT, contBB, CC_ALWAYS, NULL);
      contBB->explicitCont = true;
      bb->cfg.attach(&contBB->cfg, Graph::Edge::BACK);
   }
      break;
   case SM4_OPCODE_CONTINUEC:
   {
      BasicBlock *nextBB = new BasicBlock(func);
      BasicBlock *contBB = reinterpret_cast<BasicBlock *>(loopBBs.peek().u.p);
      mkFlow(OP_CONT, contBB, insn->insn.test_nz ? CC_P : CC_NOT_P, src(0, 0));
      bb->cfg.attach(&contBB->cfg, Graph::Edge::BACK);
      bb->cfg.attach(&nextBB->cfg, Graph::Edge::FORWARD);
      setPosition(nextBB, true);
   }
      break;

   case SM4_OPCODE_SAMPLE:
   case SM4_OPCODE_SAMPLE_C:
   case SM4_OPCODE_SAMPLE_C_LZ:
   case SM4_OPCODE_SAMPLE_L:
   case SM4_OPCODE_SAMPLE_D:
   case SM4_OPCODE_SAMPLE_B:
      handleSAMPLE(op, dst0);
      break;
   case SM4_OPCODE_LD:
   case SM4_OPCODE_LD_MS:
      handleLOAD(dst0);
      break;

   case SM4_OPCODE_GATHER4:
      assert(!"GATHER4 not implemented\n");
      break;

   case SM4_OPCODE_RESINFO:
      handleQUERY(dst0, TXQ_DIMS);
      break;
   case SM4_OPCODE_SAMPLE_POS:
      handleQUERY(dst0, TXQ_SAMPLE_POSITION);
      break;

   case SM4_OPCODE_NOP:
      mkOp(OP_NOP, TYPE_NONE, NULL);
      break;

   case SM4_OPCODE_HS_DECLS:
      // XXX: any significance ?
      break;
   case SM4_OPCODE_HS_CONTROL_POINT_PHASE:
      phase = 0;
      break;
   case SM4_OPCODE_HS_FORK_PHASE:
      if (phase != 1)
         subPhase = 0;
      phase = 1;
      phaseInstance = (phaseStart == pos) ? (phaseInstance + 1) : 0;
      phaseStart = pos;
      if (info.prop.tp.outputPatchSize < phaseInstCnt[0][subPhase])
         unrollPhase = true;
      break;
   case SM4_OPCODE_HS_JOIN_PHASE:
      if (phase != 2)
         subPhase = 0;
      phase = 2;
      phaseInstance = (phaseStart == pos) ? (phaseInstance + 1) : 0;
      phaseStart = pos;
      if (info.prop.tp.outputPatchSize < phaseInstCnt[1][subPhase])
         unrollPhase = true;
      break;

   default:
      ERROR("SM4_OPCODE_#%u illegal / not supported\n", insn->opcode);
      abort();
      return false;
   }

   for (c = 0; c < nc; ++c) {
      if (nDstOpnds >= 1 && rDst0[c]) {
         if (dst0[c] != rDst0[c])
            mkMov(rDst0[c], dst0[c]);
         saveDst(0, c, rDst0[c]);
      }
      if (nDstOpnds >= 2 && rDst1[c]) {
         if (dst1[c] != rDst1[c])
            mkMov(rDst1[c], dst1[c]);
         saveDst(1, c, rDst1[c]);
      }
   }

   memset(srcPtr, 0, sizeof(srcPtr));
   memset(dstPtr, 0, sizeof(dstPtr));
   memset(vtxBase, 0, sizeof(vtxBase));
   return true;
}

void
Converter::exportOutputs()
{
   for (int i = 0; i < info.numOutputs; ++i) {
      for (int c = 0; c < 4; ++c) {
         if (!oData.exists(i, c))
            continue;
         Symbol *sym = mkSymbol(FILE_SHADER_OUTPUT, 0, TYPE_F32,
                                info.out[i].slot[c] * 4);
         Value *val = oData.load(i, c, NULL);
         if (val)
            mkStore(OP_EXPORT, TYPE_F32, sym, NULL, val);
      }
   }
}

Converter::Converter(Program *p, struct nv50_ir_prog_info *s)
   : tData32(this),
     tData64(this),
     oData(this),
     info(*s),
     sm4(*reinterpret_cast<const sm4_program *>(s->bin.source)),
     prog(p)
{
   memset(srcPtr, 0, sizeof(srcPtr));
   memset(dstPtr, 0, sizeof(dstPtr));
   memset(vtxBase, 0, sizeof(vtxBase));

   memset(interpMode, 0, sizeof(interpMode));

   nrRegVals = nrArrays = arrayVol = 0;

   for (phase = 3; phase > 0; --phase)
      for (unsigned int i = 0; i < PIPE_MAX_SHADER_OUTPUTS; ++i)
         out[phase - 1][i].sn = TGSI_SEMANTIC_COUNT;

   unrollPhase = false;
   phaseStart = 0;
   subPhaseCnt[0] = subPhaseCnt[1] = 0;
}

Converter::~Converter()
{
   if (lData)
      delete[] lData;

   if (subPhaseCnt[0])
      delete[] phaseInstCnt[0];
   if (subPhaseCnt[1])
      delete[] phaseInstCnt[1];
}

bool
Converter::haveNextPhase(unsigned int pos) const
{
   ++pos;
   return (pos < sm4.insns.size()) &&
      (sm4.insns[pos]->opcode == SM4_OPCODE_HS_FORK_PHASE ||
       sm4.insns[pos]->opcode == SM4_OPCODE_HS_JOIN_PHASE);
}

bool
Converter::run()
{
   parseSignature();

   for (unsigned int pos = 0; pos < sm4.dcls.size(); ++pos)
      inspectDeclaration(*sm4.dcls[pos]);

   phaseInstCnt[0] = new unsigned int [subPhaseCnt[0]];
   phaseInstCnt[1] = new unsigned int [subPhaseCnt[1]];
   for (int i = 0; i < subPhaseCnt[0]; ++i)
      phaseInstCnt[0][i] = -1;
   for (int i = 0; i < subPhaseCnt[1]; ++i)
      phaseInstCnt[1][i] = -1;
   // re-increased in handleDeclaration:
   subPhaseCnt[0] = subPhaseCnt[1] = 0;

   allocateValues();
   nrArrays = 0;
   for (unsigned int pos = 0; pos < sm4.dcls.size(); ++pos)
      handleDeclaration(*sm4.dcls[pos]);

   info.io.genUserClip = -1; // no UCPs permitted with SM4 shaders
   info.io.clipDistanceMask = (1 << info.io.clipDistanceMask) - 1;

   info.assignSlots(&info);

   if (sm4.dcls.size() == 0 && sm4.insns.size() == 0)
      return true;

   BasicBlock *entry = new BasicBlock(prog->main);
   BasicBlock *leave = new BasicBlock(prog->main);

   prog->main->setEntry(entry);
   prog->main->setExit(leave);

   setPosition(entry, true);

   entryBBs.push(entry);
   leaveBBs.push(leave);

   if (prog->getType() == Program::TYPE_FRAGMENT) {
      Symbol *sv = mkSysVal(SV_POSITION, 3);
      fragCoord[3] = mkOp1v(OP_RDSV, TYPE_F32, getSSA(), sv);
      mkOp1(OP_RCP, TYPE_F32, fragCoord[3], fragCoord[3]);
   } else
   if (prog->getType() == Program::TYPE_TESSELLATION_EVAL) {
      const int n = (info.prop.tp.domain == PIPE_PRIM_TRIANGLES) ? 3 : 2;
      int c;
      for (c = 0; c < n; ++c)
         domainPt[c] =
            mkOp1v(OP_RDSV, TYPE_F32, getSSA(), mkSysVal(SV_TESS_COORD, c));
      if (c == 2)
         domainPt[2] = loadImm(NULL, 0.0f);
   }

   finalized = false;
   phaseEnded = 0;
   phase = 0;
   subPhase = 0;
   for (unsigned int pos = 0; pos < sm4.insns.size(); ++pos) {
      handleInstruction(pos);
      if (likely(phase == 0) || (phaseEnded < 2))
         continue;
      phaseEnded = 0;
      if (!unrollPhase || !phaseInstanceUsed) {
         ++subPhase;
         continue;
      }
      phaseInstanceUsed = false;
      if (phaseInstance < (phaseInstCnt[phase - 1][subPhase] - 1))
         pos = phaseStart - 1;
      else
         ++subPhase;
   }
   finalizeShader();

   return true;
}

} // anonymous namespace

namespace nv50_ir {

bool
Program::makeFromSM4(struct nv50_ir_prog_info *info)
{
   Converter bld(this, info);
   return bld.run();
}

} // namespace nv50_ir
