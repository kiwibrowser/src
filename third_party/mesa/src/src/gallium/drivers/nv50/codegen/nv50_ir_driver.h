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

#ifndef __NV50_IR_DRIVER_H__
#define __NV50_IR_DRIVER_H__

#include "pipe/p_shader_tokens.h"

#include "tgsi/tgsi_util.h"
#include "tgsi/tgsi_parse.h"
#include "tgsi/tgsi_scan.h"

/*
 * This struct constitutes linkage information in TGSI terminology.
 *
 * It is created by the code generator and handed to the pipe driver
 * for input/output slot assignment.
 */
struct nv50_ir_varying
{
   uint8_t slot[4]; /* native slots for xyzw (addresses in 32-bit words) */

   unsigned mask     : 4; /* vec4 mask */
   unsigned linear   : 1; /* linearly interpolated if true (and not flat) */
   unsigned flat     : 1;
   unsigned sc       : 1; /* special colour interpolation mode (SHADE_MODEL) */
   unsigned centroid : 1;
   unsigned patch    : 1; /* patch constant value */
   unsigned regular  : 1; /* driver-specific meaning (e.g. input in sreg) */
   unsigned input    : 1; /* indicates direction of system values */
   unsigned oread    : 1; /* true if output is read from parallel TCP */

   ubyte id; /* TGSI register index */
   ubyte sn; /* TGSI semantic name */
   ubyte si; /* TGSI semantic index */
};

#define NV50_PROGRAM_IR_TGSI 0
#define NV50_PROGRAM_IR_SM4  1
#define NV50_PROGRAM_IR_GLSL 2
#define NV50_PROGRAM_IR_LLVM 3

#ifdef DEBUG
# define NV50_IR_DEBUG_BASIC     (1 << 0)
# define NV50_IR_DEBUG_VERBOSE   (2 << 0)
# define NV50_IR_DEBUG_REG_ALLOC (1 << 2)
#else
# define NV50_IR_DEBUG_BASIC     0
# define NV50_IR_DEBUG_VERBOSE   0
# define NV50_IR_DEBUG_REG_ALLOC 0
#endif

#define NV50_SEMANTIC_CLIPDISTANCE  (TGSI_SEMANTIC_COUNT + 0)
#define NV50_SEMANTIC_TEXCOORD      (TGSI_SEMANTIC_COUNT + 1)
#define NV50_SEMANTIC_POINTCOORD    (TGSI_SEMANTIC_COUNT + 2)
#define NV50_SEMANTIC_VIEWPORTINDEX (TGSI_SEMANTIC_COUNT + 4)
#define NV50_SEMANTIC_LAYER         (TGSI_SEMANTIC_COUNT + 5)
#define NV50_SEMANTIC_INVOCATIONID  (TGSI_SEMANTIC_COUNT + 6)
#define NV50_SEMANTIC_TESSFACTOR    (TGSI_SEMANTIC_COUNT + 7)
#define NV50_SEMANTIC_TESSCOORD     (TGSI_SEMANTIC_COUNT + 8)
#define NV50_SEMANTIC_SAMPLEMASK    (TGSI_SEMANTIC_COUNT + 9)
#define NV50_SEMANTIC_COUNT         (TGSI_SEMANTIC_COUNT + 10)

#define NV50_TESS_PART_FRACT_ODD  0
#define NV50_TESS_PART_FRACT_EVEN 1
#define NV50_TESS_PART_POW2       2
#define NV50_TESS_PART_INTEGER    3

#define NV50_PRIM_PATCHES PIPE_PRIM_MAX

struct nv50_ir_prog_symbol
{
   uint32_t label;
   uint32_t offset;
};

struct nv50_ir_prog_info
{
   uint16_t target; /* chipset (0x50, 0x84, 0xc0, ...) */

   uint8_t type; /* PIPE_SHADER */

   uint8_t optLevel; /* optimization level (0 to 3) */
   uint8_t dbgFlags;

   struct {
      int16_t maxGPR;     /* may be -1 if none used */
      int16_t maxOutput;
      uint32_t tlsSpace;  /* required local memory per thread */
      uint32_t *code;
      uint32_t codeSize;
      uint8_t sourceRep;  /* NV50_PROGRAM_IR */
      const void *source;
      void *relocData;
      struct nv50_ir_prog_symbol *syms;
      uint16_t numSyms;
   } bin;

   struct nv50_ir_varying sv[PIPE_MAX_SHADER_INPUTS];
   struct nv50_ir_varying in[PIPE_MAX_SHADER_INPUTS];
   struct nv50_ir_varying out[PIPE_MAX_SHADER_OUTPUTS];
   uint8_t numInputs;
   uint8_t numOutputs;
   uint8_t numPatchConstants; /* also included in numInputs/numOutputs */
   uint8_t numSysVals;

   struct {
      uint32_t *buf;    /* for IMMEDIATE_ARRAY */
      uint16_t bufSize; /* size of immediate array */
      uint16_t count;   /* count of inline immediates */
      uint32_t *data;   /* inline immediate data */
      uint8_t *type;    /* for each vec4 (128 bit) */
   } immd;

   union {
      struct {
         uint32_t inputMask[4]; /* mask of attributes read (1 bit per scalar) */
      } vp;
      struct {
         uint8_t inputPatchSize;
         uint8_t outputPatchSize;
         uint8_t partitioning;    /* PIPE_TESS_PART */
         int8_t winding;          /* +1 (clockwise) / -1 (counter-clockwise) */
         uint8_t domain;          /* PIPE_PRIM_{QUADS,TRIANGLES,LINES} */
         uint8_t outputPrim;      /* PIPE_PRIM_{TRIANGLES,LINES,POINTS} */
      } tp;
      struct {
         uint8_t inputPrim;
         uint8_t outputPrim;
         unsigned instanceCount;
         unsigned maxVertices;
      } gp;
      struct {
         unsigned numColourResults;
         boolean writesDepth;
         boolean earlyFragTests;
         boolean separateFragData;
         boolean usesDiscard;
      } fp;
   } prop;

   struct {
      uint8_t clipDistance;      /* index of first clip distance output */
      uint8_t clipDistanceMask;  /* mask of clip distances defined */
      uint8_t cullDistanceMask;  /* clip distance mode (1 bit per output) */
      int8_t genUserClip;        /* request user clip planes for ClipVertex */
      uint16_t ucpBase;          /* base address for UCPs */
      uint8_t ucpBinding;        /* constant buffer index of UCP data */
      uint8_t pointSize;         /* output index for PointSize */
      uint8_t instanceId;        /* system value index of InstanceID */
      uint8_t vertexId;          /* system value index of VertexID */
      uint8_t edgeFlagIn;
      uint8_t edgeFlagOut;
      uint8_t fragDepth;         /* output index of FragDepth */
      uint8_t sampleMask;        /* output index of SampleMask */
      uint8_t backFaceColor[2];  /* input/output indices of back face colour */
      uint8_t globalAccess;      /* 1 for read, 2 for wr, 3 for rw */
   } io;

   /* driver callback to assign input/output locations */
   int (*assignSlots)(struct nv50_ir_prog_info *);

   void *driverPriv;
};

#ifdef __cplusplus
extern "C" {
#endif

extern int nv50_ir_generate_code(struct nv50_ir_prog_info *);

extern void nv50_ir_relocate_code(void *relocData, uint32_t *code,
                                  uint32_t codePos,
                                  uint32_t libPos,
                                  uint32_t dataPos);

/* obtain code that will be shared among programs */
extern void nv50_ir_get_target_library(uint32_t chipset,
                                       const uint32_t **code, uint32_t *size);

#ifdef __cplusplus
}
#endif

#endif // __NV50_IR_DRIVER_H__
