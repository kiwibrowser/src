/**************************************************************************

Copyright 2000, 2001 ATI Technologies Inc., Ontario, Canada, and
                     VA Linux Systems Inc., Fremont, California.

All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Kevin E. Martin <martin@valinux.com>
 *   Gareth Hughes <gareth@valinux.com>
 *   Keith Whitwell <keith@tungstengraphics.com>
 */

#include <sched.h>
#include <errno.h>

#include "main/attrib.h"
#include "main/bufferobj.h"
#include "swrast/swrast.h"

#include "main/glheader.h"
#include "main/imports.h"
#include "main/simple_list.h"

#include "radeon_context.h"
#include "radeon_common.h"
#include "radeon_ioctl.h"

#define STANDALONE_MMIO

#define RADEON_TIMEOUT             512
#define RADEON_IDLE_RETRY           16


/* =============================================================
 * Kernel command buffer handling
 */

/* The state atoms will be emitted in the order they appear in the atom list,
 * so this step is important.
 */
void radeonSetUpAtomList( r100ContextPtr rmesa )
{
   int i, mtu = rmesa->radeon.glCtx->Const.MaxTextureUnits;

   make_empty_list(&rmesa->radeon.hw.atomlist);
   rmesa->radeon.hw.atomlist.name = "atom-list";

   insert_at_tail(&rmesa->radeon.hw.atomlist, &rmesa->hw.ctx);
   insert_at_tail(&rmesa->radeon.hw.atomlist, &rmesa->hw.set);
   insert_at_tail(&rmesa->radeon.hw.atomlist, &rmesa->hw.lin);
   insert_at_tail(&rmesa->radeon.hw.atomlist, &rmesa->hw.msk);
   insert_at_tail(&rmesa->radeon.hw.atomlist, &rmesa->hw.vpt);
   insert_at_tail(&rmesa->radeon.hw.atomlist, &rmesa->hw.tcl);
   insert_at_tail(&rmesa->radeon.hw.atomlist, &rmesa->hw.msc);
   for (i = 0; i < mtu; ++i) {
       insert_at_tail(&rmesa->radeon.hw.atomlist, &rmesa->hw.tex[i]);
       insert_at_tail(&rmesa->radeon.hw.atomlist, &rmesa->hw.txr[i]);
       insert_at_tail(&rmesa->radeon.hw.atomlist, &rmesa->hw.cube[i]);
   }
   insert_at_tail(&rmesa->radeon.hw.atomlist, &rmesa->hw.zbs);
   insert_at_tail(&rmesa->radeon.hw.atomlist, &rmesa->hw.mtl);
   for (i = 0; i < 3 + mtu; ++i)
      insert_at_tail(&rmesa->radeon.hw.atomlist, &rmesa->hw.mat[i]);
   for (i = 0; i < 8; ++i)
      insert_at_tail(&rmesa->radeon.hw.atomlist, &rmesa->hw.lit[i]);
   for (i = 0; i < 6; ++i)
      insert_at_tail(&rmesa->radeon.hw.atomlist, &rmesa->hw.ucp[i]);
   insert_at_tail(&rmesa->radeon.hw.atomlist, &rmesa->hw.stp);
   insert_at_tail(&rmesa->radeon.hw.atomlist, &rmesa->hw.eye);
   insert_at_tail(&rmesa->radeon.hw.atomlist, &rmesa->hw.grd);
   insert_at_tail(&rmesa->radeon.hw.atomlist, &rmesa->hw.fog);
   insert_at_tail(&rmesa->radeon.hw.atomlist, &rmesa->hw.glt);
}

static void radeonEmitScissor(r100ContextPtr rmesa)
{
    BATCH_LOCALS(&rmesa->radeon);
    if (rmesa->radeon.state.scissor.enabled) {
        BEGIN_BATCH(6);
        OUT_BATCH(CP_PACKET0(RADEON_PP_CNTL, 0));
        OUT_BATCH(rmesa->hw.ctx.cmd[CTX_PP_CNTL] | RADEON_SCISSOR_ENABLE);
        OUT_BATCH(CP_PACKET0(RADEON_RE_TOP_LEFT, 0));
        OUT_BATCH((rmesa->radeon.state.scissor.rect.y1 << 16) |
                  rmesa->radeon.state.scissor.rect.x1);
        OUT_BATCH(CP_PACKET0(RADEON_RE_WIDTH_HEIGHT, 0));
        OUT_BATCH(((rmesa->radeon.state.scissor.rect.y2) << 16) |
                  (rmesa->radeon.state.scissor.rect.x2));
        END_BATCH();
    } else {
        BEGIN_BATCH(2);
        OUT_BATCH(CP_PACKET0(RADEON_PP_CNTL, 0));
        OUT_BATCH(rmesa->hw.ctx.cmd[CTX_PP_CNTL] & ~RADEON_SCISSOR_ENABLE);
        END_BATCH();
    }
}

/* Fire a section of the retained (indexed_verts) buffer as a regular
 * primtive.
 */
extern void radeonEmitVbufPrim( r100ContextPtr rmesa,
				GLuint vertex_format,
				GLuint primitive,
				GLuint vertex_nr )
{
   BATCH_LOCALS(&rmesa->radeon);

   assert(!(primitive & RADEON_CP_VC_CNTL_PRIM_WALK_IND));

   radeonEmitState(&rmesa->radeon);
   radeonEmitScissor(rmesa);

#if RADEON_OLD_PACKETS
   BEGIN_BATCH(8);
   OUT_BATCH_PACKET3_CLIP(RADEON_CP_PACKET3_3D_RNDR_GEN_INDX_PRIM, 3);
   OUT_BATCH(rmesa->ioctl.vertex_offset);

   OUT_BATCH(vertex_nr);
   OUT_BATCH(vertex_format);
   OUT_BATCH(primitive |  RADEON_CP_VC_CNTL_PRIM_WALK_LIST |
	     RADEON_CP_VC_CNTL_COLOR_ORDER_RGBA |
	     RADEON_CP_VC_CNTL_VTX_FMT_RADEON_MODE |
	     (vertex_nr << RADEON_CP_VC_CNTL_NUM_SHIFT));

   radeon_cs_write_reloc(rmesa->radeon.cmdbuf.cs,
			 rmesa->ioctl.bo,
			 RADEON_GEM_DOMAIN_GTT,
			 0, 0);

   END_BATCH();

#else
   BEGIN_BATCH(4);
   OUT_BATCH_PACKET3_CLIP(RADEON_CP_PACKET3_3D_DRAW_VBUF, 1);
   OUT_BATCH(vertex_format);
   OUT_BATCH(primitive |
	     RADEON_CP_VC_CNTL_PRIM_WALK_LIST |
	     RADEON_CP_VC_CNTL_COLOR_ORDER_RGBA |
	     RADEON_CP_VC_CNTL_MAOS_ENABLE |
	     RADEON_CP_VC_CNTL_VTX_FMT_RADEON_MODE |
	     (vertex_nr << RADEON_CP_VC_CNTL_NUM_SHIFT));
   END_BATCH();
#endif
}

void radeonFlushElts( struct gl_context *ctx )
{
   r100ContextPtr rmesa = R100_CONTEXT(ctx);
   BATCH_LOCALS(&rmesa->radeon);
   int nr;
   uint32_t *cmd = (uint32_t *)(rmesa->radeon.cmdbuf.cs->packets + rmesa->tcl.elt_cmd_start);
   int dwords = (rmesa->radeon.cmdbuf.cs->section_ndw - rmesa->radeon.cmdbuf.cs->section_cdw);

   if (RADEON_DEBUG & RADEON_IOCTL)
      fprintf(stderr, "%s\n", __FUNCTION__);

   assert( rmesa->radeon.dma.flush == radeonFlushElts );
   rmesa->radeon.dma.flush = NULL;

   nr = rmesa->tcl.elt_used;

#if RADEON_OLD_PACKETS
   dwords -= 2;
#endif

#if RADEON_OLD_PACKETS
   cmd[1] |= (dwords + 3) << 16;
   cmd[5] |= nr << RADEON_CP_VC_CNTL_NUM_SHIFT;
#else
   cmd[1] |= (dwords + 2) << 16;
   cmd[3] |= nr << RADEON_CP_VC_CNTL_NUM_SHIFT;
#endif

   rmesa->radeon.cmdbuf.cs->cdw += dwords;
   rmesa->radeon.cmdbuf.cs->section_cdw += dwords;

#if RADEON_OLD_PACKETS
   radeon_cs_write_reloc(rmesa->radeon.cmdbuf.cs,
			 rmesa->ioctl.bo,
			 RADEON_GEM_DOMAIN_GTT,
			 0, 0);
#endif

   END_BATCH();

   if (RADEON_DEBUG & RADEON_SYNC) {
      fprintf(stderr, "%s: Syncing\n", __FUNCTION__);
      radeonFinish( rmesa->radeon.glCtx );
   }

}

GLushort *radeonAllocEltsOpenEnded( r100ContextPtr rmesa,
				    GLuint vertex_format,
				    GLuint primitive,
				    GLuint min_nr )
{
   GLushort *retval;
   int align_min_nr;
   BATCH_LOCALS(&rmesa->radeon);

   if (RADEON_DEBUG & RADEON_IOCTL)
      fprintf(stderr, "%s %d prim %x\n", __FUNCTION__, min_nr, primitive);

   assert((primitive & RADEON_CP_VC_CNTL_PRIM_WALK_IND));

   radeonEmitState(&rmesa->radeon);
   radeonEmitScissor(rmesa);

   rmesa->tcl.elt_cmd_start = rmesa->radeon.cmdbuf.cs->cdw;

   /* round up min_nr to align the state */
   align_min_nr = (min_nr + 1) & ~1;

#if RADEON_OLD_PACKETS
   BEGIN_BATCH_NO_AUTOSTATE(2+ELTS_BUFSZ(align_min_nr)/4);
   OUT_BATCH_PACKET3_CLIP(RADEON_CP_PACKET3_3D_RNDR_GEN_INDX_PRIM, 0);
   OUT_BATCH(rmesa->ioctl.vertex_offset);
   OUT_BATCH(rmesa->ioctl.vertex_max);
   OUT_BATCH(vertex_format);
   OUT_BATCH(primitive |
	     RADEON_CP_VC_CNTL_PRIM_WALK_IND |
	     RADEON_CP_VC_CNTL_COLOR_ORDER_RGBA |
	     RADEON_CP_VC_CNTL_VTX_FMT_RADEON_MODE);
#else
   BEGIN_BATCH_NO_AUTOSTATE(ELTS_BUFSZ(align_min_nr)/4);
   OUT_BATCH_PACKET3_CLIP(RADEON_CP_PACKET3_DRAW_INDX, 0);
   OUT_BATCH(vertex_format);
   OUT_BATCH(primitive |
	     RADEON_CP_VC_CNTL_PRIM_WALK_IND |
	     RADEON_CP_VC_CNTL_COLOR_ORDER_RGBA |
	     RADEON_CP_VC_CNTL_MAOS_ENABLE |
	     RADEON_CP_VC_CNTL_VTX_FMT_RADEON_MODE);
#endif


   rmesa->tcl.elt_cmd_offset = rmesa->radeon.cmdbuf.cs->cdw;
   rmesa->tcl.elt_used = min_nr;

   retval = (GLushort *)(rmesa->radeon.cmdbuf.cs->packets + rmesa->tcl.elt_cmd_offset);

   if (RADEON_DEBUG & RADEON_RENDER)
      fprintf(stderr, "%s: header prim %x \n",
	      __FUNCTION__, primitive);

   assert(!rmesa->radeon.dma.flush);
   rmesa->radeon.glCtx->Driver.NeedFlush |= FLUSH_STORED_VERTICES;
   rmesa->radeon.dma.flush = radeonFlushElts;

   return retval;
}

void radeonEmitVertexAOS( r100ContextPtr rmesa,
			  GLuint vertex_size,
			  struct radeon_bo *bo,
			  GLuint offset )
{
#if RADEON_OLD_PACKETS
   rmesa->ioctl.vertex_offset = offset;
   rmesa->ioctl.bo = bo;
#else
   BATCH_LOCALS(&rmesa->radeon);

   if (RADEON_DEBUG & (RADEON_PRIMS|DEBUG_IOCTL))
      fprintf(stderr, "%s:  vertex_size 0x%x offset 0x%x \n",
	      __FUNCTION__, vertex_size, offset);

   BEGIN_BATCH(7);
   OUT_BATCH_PACKET3(RADEON_CP_PACKET3_3D_LOAD_VBPNTR, 2);
   OUT_BATCH(1);
   OUT_BATCH(vertex_size | (vertex_size << 8));
   OUT_BATCH_RELOC(offset, bo, offset, RADEON_GEM_DOMAIN_GTT, 0, 0);
   END_BATCH();

#endif
}


void radeonEmitAOS( r100ContextPtr rmesa,
		    GLuint nr,
		    GLuint offset )
{
#if RADEON_OLD_PACKETS
   assert( nr == 1 );
   rmesa->ioctl.bo = rmesa->radeon.tcl.aos[0].bo;
   rmesa->ioctl.vertex_offset =
     (rmesa->radeon.tcl.aos[0].offset + offset * rmesa->radeon.tcl.aos[0].stride * 4);
   rmesa->ioctl.vertex_max = rmesa->radeon.tcl.aos[0].count;
#else
   BATCH_LOCALS(&rmesa->radeon);
   uint32_t voffset;
   //   int sz = AOS_BUFSZ(nr);
   int sz = 1 + (nr >> 1) * 3 + (nr & 1) * 2;
   int i;

   if (RADEON_DEBUG & RADEON_IOCTL)
      fprintf(stderr, "%s\n", __FUNCTION__);

   BEGIN_BATCH(sz+2+(nr * 2));
   OUT_BATCH_PACKET3(RADEON_CP_PACKET3_3D_LOAD_VBPNTR, sz - 1);
   OUT_BATCH(nr);

   {
      for (i = 0; i + 1 < nr; i += 2) {
	 OUT_BATCH((rmesa->radeon.tcl.aos[i].components << 0) |
		   (rmesa->radeon.tcl.aos[i].stride << 8) |
		   (rmesa->radeon.tcl.aos[i + 1].components << 16) |
		   (rmesa->radeon.tcl.aos[i + 1].stride << 24));

	 voffset =  rmesa->radeon.tcl.aos[i + 0].offset +
	    offset * 4 * rmesa->radeon.tcl.aos[i + 0].stride;
	 OUT_BATCH(voffset);
	 voffset =  rmesa->radeon.tcl.aos[i + 1].offset +
	    offset * 4 * rmesa->radeon.tcl.aos[i + 1].stride;
	 OUT_BATCH(voffset);
      }

      if (nr & 1) {
	 OUT_BATCH((rmesa->radeon.tcl.aos[nr - 1].components << 0) |
		   (rmesa->radeon.tcl.aos[nr - 1].stride << 8));
	 voffset =  rmesa->radeon.tcl.aos[nr - 1].offset +
	    offset * 4 * rmesa->radeon.tcl.aos[nr - 1].stride;
	 OUT_BATCH(voffset);
      }
      for (i = 0; i + 1 < nr; i += 2) {
	 voffset =  rmesa->radeon.tcl.aos[i + 0].offset +
	    offset * 4 * rmesa->radeon.tcl.aos[i + 0].stride;
	 radeon_cs_write_reloc(rmesa->radeon.cmdbuf.cs,
			       rmesa->radeon.tcl.aos[i+0].bo,
			       RADEON_GEM_DOMAIN_GTT,
			       0, 0);
	 voffset =  rmesa->radeon.tcl.aos[i + 1].offset +
	    offset * 4 * rmesa->radeon.tcl.aos[i + 1].stride;
	 radeon_cs_write_reloc(rmesa->radeon.cmdbuf.cs,
			       rmesa->radeon.tcl.aos[i+1].bo,
			       RADEON_GEM_DOMAIN_GTT,
			       0, 0);
      }
      if (nr & 1) {
	 voffset =  rmesa->radeon.tcl.aos[nr - 1].offset +
	    offset * 4 * rmesa->radeon.tcl.aos[nr - 1].stride;
	 radeon_cs_write_reloc(rmesa->radeon.cmdbuf.cs,
			       rmesa->radeon.tcl.aos[nr-1].bo,
			       RADEON_GEM_DOMAIN_GTT,
			       0, 0);
      }
   }
   END_BATCH();

#endif
}

/* ================================================================
 * Buffer clear
 */
#define RADEON_MAX_CLEARS	256

static void radeonClear( struct gl_context *ctx, GLbitfield mask )
{
   r100ContextPtr rmesa = R100_CONTEXT(ctx);
   GLuint hwmask, swmask;
   GLuint hwbits = BUFFER_BIT_FRONT_LEFT | BUFFER_BIT_BACK_LEFT |
                   BUFFER_BIT_DEPTH | BUFFER_BIT_STENCIL |
                   BUFFER_BIT_COLOR0;

   if (mask & (BUFFER_BIT_FRONT_LEFT | BUFFER_BIT_FRONT_RIGHT)) {
      rmesa->radeon.front_buffer_dirty = GL_TRUE;
   }

   if ( RADEON_DEBUG & RADEON_IOCTL ) {
      fprintf( stderr, "radeonClear\n");
   }

   radeon_firevertices(&rmesa->radeon);

   hwmask = mask & hwbits;
   swmask = mask & ~hwbits;

   if ( swmask ) {
      if (RADEON_DEBUG & RADEON_FALLBACKS)
	 fprintf(stderr, "%s: swrast clear, mask: %x\n", __FUNCTION__, swmask);
      _swrast_Clear( ctx, swmask );
   }

   if ( !hwmask )
      return;

   radeonUserClear(ctx, hwmask);
}

void radeonInitIoctlFuncs( struct gl_context *ctx )
{
    ctx->Driver.Clear = radeonClear;
    ctx->Driver.Finish = radeonFinish;
    ctx->Driver.Flush = radeonFlush;
}

