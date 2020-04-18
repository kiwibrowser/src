/*
 * Copyright 2010 Ben Skeggs
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

#ifndef __NV50_PROG_H__
#define __NV50_PROG_H__

struct nv50_context;

#include "pipe/p_state.h"
#include "pipe/p_shader_tokens.h"

struct nv50_varying {
   uint8_t id; /* tgsi index */
   uint8_t hw; /* hw index, nv50 wants flat FP inputs last */

   unsigned mask   : 4;
   unsigned linear : 1;
   unsigned pad    : 3;

   ubyte sn; /* semantic name */
   ubyte si; /* semantic index */
};

struct nv50_stream_output_state
{
   uint32_t ctrl;
   uint16_t stride[4];
   uint8_t num_attribs[4];
   uint8_t map_size;
   uint8_t map[128];
};

struct nv50_program {
   struct pipe_shader_state pipe;

   ubyte type;
   boolean translated;

   uint32_t *code;
   unsigned code_size;
   unsigned code_base;
   uint32_t *immd;
   unsigned immd_size;
   unsigned parm_size; /* size limit of uniform buffer */
   uint32_t tls_space; /* required local memory per thread */

   ubyte max_gpr; /* REG_ALLOC_TEMP */
   ubyte max_out; /* REG_ALLOC_RESULT or FP_RESULT_COUNT */

   ubyte in_nr;
   ubyte out_nr;
   struct nv50_varying in[16];
   struct nv50_varying out[16];

   struct {
      uint32_t attrs[3]; /* VP_ATTR_EN_0,1 and VP_GP_BUILTIN_ATTR_EN */
      ubyte psiz;        /* output slot of point size */
      ubyte bfc[2];      /* indices into varying for FFC (FP) or BFC (VP) */
      ubyte edgeflag;
      ubyte clpd[2];     /* output slot of clip distance[i]'s 1st component */
      ubyte clpd_nr;
   } vp;

   struct {
      uint32_t flags[2]; /* 0x19a8, 196c */
      uint32_t interp; /* 0x1988 */
      uint32_t colors; /* 0x1904 */
   } fp;

   struct {
      ubyte primid; /* primitive id output register */
      uint8_t vert_count;
      uint8_t prim_type; /* point, line strip or tri strip */
   } gp;

   void *fixups; /* relocation records */

   struct nouveau_heap *mem;

   struct nv50_stream_output_state *so;
};

boolean nv50_program_translate(struct nv50_program *, uint16_t chipset);
boolean nv50_program_upload_code(struct nv50_context *, struct nv50_program *);
void nv50_program_destroy(struct nv50_context *, struct nv50_program *);

#endif /* __NV50_PROG_H__ */
