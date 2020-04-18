/*
Copyright (C) The Weather Channel, Inc.  2002.  All Rights Reserved.

The Weather Channel (TM) funded Tungsten Graphics to develop the
initial release of the Radeon 8500 driver under the XFree86 license.
This notice must be preserved.

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
*/

/*
 * Authors:
 *   Keith Whitwell <keith@tungstengraphics.com>
 */

#include "main/glheader.h"
#include "main/imports.h"
#include "main/macros.h"
#include "main/context.h"
#include "main/simple_list.h"

#include "radeon_common.h"
#include "r200_context.h"
#include "r200_ioctl.h"
#include "radeon_reg.h"

/* The state atoms will be emitted in the order they appear in the atom list,
 * so this step is important.
 */
#define insert_at_tail_if(atom_list, atom) \
   do { \
      struct radeon_state_atom* current_atom = (atom); \
      if (current_atom->check) \
	 insert_at_tail((atom_list), current_atom); \
   } while(0)

void r200SetUpAtomList( r200ContextPtr rmesa )
{
   int i, mtu;

   mtu = rmesa->radeon.glCtx->Const.MaxTextureUnits;

   make_empty_list(&rmesa->radeon.hw.atomlist);
   rmesa->radeon.hw.atomlist.name = "atom-list";

   insert_at_tail_if( &rmesa->radeon.hw.atomlist, &rmesa->hw.ctx );
   insert_at_tail_if( &rmesa->radeon.hw.atomlist, &rmesa->hw.set );
   insert_at_tail_if( &rmesa->radeon.hw.atomlist, &rmesa->hw.lin );
   insert_at_tail_if( &rmesa->radeon.hw.atomlist, &rmesa->hw.msk );
   insert_at_tail_if( &rmesa->radeon.hw.atomlist, &rmesa->hw.vpt );
   insert_at_tail_if( &rmesa->radeon.hw.atomlist, &rmesa->hw.vtx );
   insert_at_tail_if( &rmesa->radeon.hw.atomlist, &rmesa->hw.vap );
   insert_at_tail_if( &rmesa->radeon.hw.atomlist, &rmesa->hw.vte );
   insert_at_tail_if( &rmesa->radeon.hw.atomlist, &rmesa->hw.msc );
   insert_at_tail_if( &rmesa->radeon.hw.atomlist, &rmesa->hw.cst );
   insert_at_tail_if( &rmesa->radeon.hw.atomlist, &rmesa->hw.zbs );
   insert_at_tail_if( &rmesa->radeon.hw.atomlist, &rmesa->hw.tcl );
   insert_at_tail_if( &rmesa->radeon.hw.atomlist, &rmesa->hw.msl );
   insert_at_tail_if( &rmesa->radeon.hw.atomlist, &rmesa->hw.tcg );
   insert_at_tail_if( &rmesa->radeon.hw.atomlist, &rmesa->hw.grd );
   insert_at_tail_if( &rmesa->radeon.hw.atomlist, &rmesa->hw.fog );
   insert_at_tail_if( &rmesa->radeon.hw.atomlist, &rmesa->hw.tam );
   insert_at_tail_if( &rmesa->radeon.hw.atomlist, &rmesa->hw.tf );
   insert_at_tail_if( &rmesa->radeon.hw.atomlist, &rmesa->hw.atf );
   for (i = 0; i < mtu; ++i)
       insert_at_tail_if( &rmesa->radeon.hw.atomlist, &rmesa->hw.tex[i] );
   for (i = 0; i < mtu; ++i)
       insert_at_tail_if( &rmesa->radeon.hw.atomlist, &rmesa->hw.cube[i] );
   for (i = 0; i < 6; ++i)
       insert_at_tail_if( &rmesa->radeon.hw.atomlist, &rmesa->hw.pix[i] );
   insert_at_tail_if( &rmesa->radeon.hw.atomlist, &rmesa->hw.afs[0] );
   insert_at_tail_if( &rmesa->radeon.hw.atomlist, &rmesa->hw.afs[1] );
   insert_at_tail_if( &rmesa->radeon.hw.atomlist, &rmesa->hw.stp );
   for (i = 0; i < 8; ++i)
       insert_at_tail_if( &rmesa->radeon.hw.atomlist, &rmesa->hw.lit[i] );
   for (i = 0; i < 3 + mtu; ++i)
       insert_at_tail_if( &rmesa->radeon.hw.atomlist, &rmesa->hw.mat[i] );
   insert_at_tail_if( &rmesa->radeon.hw.atomlist, &rmesa->hw.eye );
   insert_at_tail_if( &rmesa->radeon.hw.atomlist, &rmesa->hw.glt );
   for (i = 0; i < 2; ++i)
      insert_at_tail_if( &rmesa->radeon.hw.atomlist, &rmesa->hw.mtl[i] );
   for (i = 0; i < 6; ++i)
       insert_at_tail_if( &rmesa->radeon.hw.atomlist, &rmesa->hw.ucp[i] );
   insert_at_tail_if( &rmesa->radeon.hw.atomlist, &rmesa->hw.spr );
   insert_at_tail_if( &rmesa->radeon.hw.atomlist, &rmesa->hw.ptp );
   insert_at_tail_if( &rmesa->radeon.hw.atomlist, &rmesa->hw.prf );
   insert_at_tail_if( &rmesa->radeon.hw.atomlist, &rmesa->hw.pvs );
   insert_at_tail_if( &rmesa->radeon.hw.atomlist, &rmesa->hw.vpp[0] );
   insert_at_tail_if( &rmesa->radeon.hw.atomlist, &rmesa->hw.vpp[1] );
   insert_at_tail_if( &rmesa->radeon.hw.atomlist, &rmesa->hw.vpi[0] );
   insert_at_tail_if( &rmesa->radeon.hw.atomlist, &rmesa->hw.vpi[1] );
   insert_at_tail_if( &rmesa->radeon.hw.atomlist, &rmesa->hw.sci );
}

/* Fire a section of the retained (indexed_verts) buffer as a regular
 * primtive.  
 */
void r200EmitVbufPrim( r200ContextPtr rmesa,
                       GLuint primitive,
                       GLuint vertex_nr )
{
   BATCH_LOCALS(&rmesa->radeon);

   assert(!(primitive & R200_VF_PRIM_WALK_IND));
   
   radeonEmitState(&rmesa->radeon);
   
   radeon_print(RADEON_RENDER|RADEON_SWRENDER,RADEON_VERBOSE,
           "%s cmd_used/4: %d prim %x nr %d\n", __FUNCTION__,
           rmesa->store.cmd_used/4, primitive, vertex_nr);
 
   BEGIN_BATCH(3);
   OUT_BATCH_PACKET3_CLIP(R200_CP_CMD_3D_DRAW_VBUF_2, 0);
   OUT_BATCH(primitive | R200_VF_PRIM_WALK_LIST | R200_VF_COLOR_ORDER_RGBA |
	     (vertex_nr << R200_VF_VERTEX_NUMBER_SHIFT));
   END_BATCH();
}

static void r200FireEB(r200ContextPtr rmesa, int vertex_count, int type)
{
	BATCH_LOCALS(&rmesa->radeon);

	if (vertex_count > 0) {
		BEGIN_BATCH(8+2);
		OUT_BATCH_PACKET3_CLIP(R200_CP_CMD_3D_DRAW_INDX_2, 0);
		OUT_BATCH(R200_VF_PRIM_WALK_IND |
			  R200_VF_COLOR_ORDER_RGBA | 
			  ((vertex_count + 0) << 16) |
			  type);

		OUT_BATCH_PACKET3(R200_CP_CMD_INDX_BUFFER, 2);
		OUT_BATCH((0x80 << 24) | (0 << 16) | 0x810);
		OUT_BATCH(rmesa->radeon.tcl.elt_dma_offset);
		OUT_BATCH((vertex_count + 1)/2);
		radeon_cs_write_reloc(rmesa->radeon.cmdbuf.cs,
				      rmesa->radeon.tcl.elt_dma_bo,
				      RADEON_GEM_DOMAIN_GTT, 0, 0);
		END_BATCH();
	}
}

void r200FlushElts(struct gl_context *ctx)
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   int nr, elt_used = rmesa->tcl.elt_used;

   radeon_print(RADEON_RENDER, RADEON_VERBOSE, "%s %x %d\n", __FUNCTION__, rmesa->tcl.hw_primitive, elt_used);

   assert( rmesa->radeon.dma.flush == r200FlushElts );
   rmesa->radeon.dma.flush = NULL;

   nr = elt_used / 2;

   radeon_bo_unmap(rmesa->radeon.tcl.elt_dma_bo);

   r200FireEB(rmesa, nr, rmesa->tcl.hw_primitive);

   radeon_bo_unref(rmesa->radeon.tcl.elt_dma_bo);
   rmesa->radeon.tcl.elt_dma_bo = NULL;

   if (R200_ELT_BUF_SZ > elt_used)
     radeonReturnDmaRegion(&rmesa->radeon, R200_ELT_BUF_SZ - elt_used);
}


GLushort *r200AllocEltsOpenEnded( r200ContextPtr rmesa,
				    GLuint primitive,
				    GLuint min_nr )
{
   GLushort *retval;

   radeon_print(RADEON_RENDER, RADEON_VERBOSE, "%s %d prim %x\n", __FUNCTION__, min_nr, primitive);

   assert((primitive & R200_VF_PRIM_WALK_IND));
   
   radeonEmitState(&rmesa->radeon);

   radeonAllocDmaRegion(&rmesa->radeon, &rmesa->radeon.tcl.elt_dma_bo,
			&rmesa->radeon.tcl.elt_dma_offset, R200_ELT_BUF_SZ, 4);
   rmesa->tcl.elt_used = min_nr * 2;

   radeon_bo_map(rmesa->radeon.tcl.elt_dma_bo, 1);
   retval = rmesa->radeon.tcl.elt_dma_bo->ptr + rmesa->radeon.tcl.elt_dma_offset;
   
   assert(!rmesa->radeon.dma.flush);
   rmesa->radeon.glCtx->Driver.NeedFlush |= FLUSH_STORED_VERTICES;
   rmesa->radeon.dma.flush = r200FlushElts;

   return retval;
}

void r200EmitMaxVtxIndex(r200ContextPtr rmesa, int count)
{
   BATCH_LOCALS(&rmesa->radeon);

   BEGIN_BATCH_NO_AUTOSTATE(2);
   OUT_BATCH(CP_PACKET0(R200_SE_VF_MAX_VTX_INDX, 0));
   OUT_BATCH(count);
   END_BATCH();
}

void r200EmitVertexAOS( r200ContextPtr rmesa,
			GLuint vertex_size,
 			struct radeon_bo *bo,
			GLuint offset )
{
   BATCH_LOCALS(&rmesa->radeon);

   radeon_print(RADEON_SWRENDER, RADEON_VERBOSE, "%s:  vertex_size 0x%x offset 0x%x \n",
	      __FUNCTION__, vertex_size, offset);


   BEGIN_BATCH(7);
   OUT_BATCH_PACKET3(R200_CP_CMD_3D_LOAD_VBPNTR, 2);
   OUT_BATCH(1);
   OUT_BATCH(vertex_size | (vertex_size << 8));
   OUT_BATCH_RELOC(offset, bo, offset, RADEON_GEM_DOMAIN_GTT, 0, 0);
   END_BATCH();
}

void r200EmitAOS(r200ContextPtr rmesa, GLuint nr, GLuint offset)
{
   BATCH_LOCALS(&rmesa->radeon);
   uint32_t voffset;
   int sz = 1 + (nr >> 1) * 3 + (nr & 1) * 2;
   int i;
   
   radeon_print(RADEON_RENDER, RADEON_VERBOSE,
           "%s: nr=%d, ofs=0x%08x\n",
           __FUNCTION__, nr, offset);

   BEGIN_BATCH(sz+2+ (nr*2));
   OUT_BATCH_PACKET3(R200_CP_CMD_3D_LOAD_VBPNTR, sz - 1);
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
}
