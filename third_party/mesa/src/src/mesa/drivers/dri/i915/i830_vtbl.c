/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

#include "i830_context.h"
#include "i830_reg.h"
#include "intel_batchbuffer.h"
#include "intel_mipmap_tree.h"
#include "intel_regions.h"
#include "intel_tris.h"
#include "intel_fbo.h"
#include "intel_buffers.h"
#include "tnl/tnl.h"
#include "tnl/t_context.h"
#include "tnl/t_vertex.h"
#include "swrast_setup/swrast_setup.h"
#include "main/renderbuffer.h"
#include "main/framebuffer.h"
#include "main/fbobject.h"

#define FILE_DEBUG_FLAG DEBUG_STATE

static bool i830_check_vertex_size(struct intel_context *intel,
				   GLuint expected);

#define SZ_TO_HW(sz)  ((sz-2)&0x3)
#define EMIT_SZ(sz)   (EMIT_1F + (sz) - 1)
#define EMIT_ATTR( ATTR, STYLE, V0 )					\
do {									\
   intel->vertex_attrs[intel->vertex_attr_count].attrib = (ATTR);	\
   intel->vertex_attrs[intel->vertex_attr_count].format = (STYLE);	\
   intel->vertex_attr_count++;						\
   v0 |= V0;								\
} while (0)

#define EMIT_PAD( N )							\
do {									\
   intel->vertex_attrs[intel->vertex_attr_count].attrib = 0;		\
   intel->vertex_attrs[intel->vertex_attr_count].format = EMIT_PAD;	\
   intel->vertex_attrs[intel->vertex_attr_count].offset = (N);		\
   intel->vertex_attr_count++;						\
} while (0)


#define VRTX_TEX_SET_FMT(n, x)          ((x)<<((n)*2))
#define TEXBIND_SET(n, x) 		((x)<<((n)*4))

static void
i830_render_prevalidate(struct intel_context *intel)
{
}

static void
i830_render_start(struct intel_context *intel)
{
   struct gl_context *ctx = &intel->ctx;
   struct i830_context *i830 = i830_context(ctx);
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;
   GLbitfield64 index_bitset = tnl->render_inputs_bitset;
   GLuint v0 = _3DSTATE_VFT0_CMD;
   GLuint v2 = _3DSTATE_VFT1_CMD;
   GLuint mcsb1 = 0;

   /* Important:
    */
   VB->AttribPtr[VERT_ATTRIB_POS] = VB->NdcPtr;
   intel->vertex_attr_count = 0;

   /* EMIT_ATTR's must be in order as they tell t_vertex.c how to
    * build up a hardware vertex.
    */
   if (index_bitset & BITFIELD64_RANGE(_TNL_ATTRIB_TEX0, _TNL_NUM_TEX)) {
      EMIT_ATTR(_TNL_ATTRIB_POS, EMIT_4F_VIEWPORT, VFT0_XYZW);
      intel->coloroffset = 4;
   }
   else {
      EMIT_ATTR(_TNL_ATTRIB_POS, EMIT_3F_VIEWPORT, VFT0_XYZ);
      intel->coloroffset = 3;
   }

   if (index_bitset & BITFIELD64_BIT(_TNL_ATTRIB_POINTSIZE)) {
      EMIT_ATTR(_TNL_ATTRIB_POINTSIZE, EMIT_1F, VFT0_POINT_WIDTH);
   }

   EMIT_ATTR(_TNL_ATTRIB_COLOR0, EMIT_4UB_4F_BGRA, VFT0_DIFFUSE);

   intel->specoffset = 0;
   if (index_bitset & (BITFIELD64_BIT(_TNL_ATTRIB_COLOR1) |
                       BITFIELD64_BIT(_TNL_ATTRIB_FOG))) {
      if (index_bitset & BITFIELD64_BIT(_TNL_ATTRIB_COLOR1)) {
         intel->specoffset = intel->coloroffset + 1;
         EMIT_ATTR(_TNL_ATTRIB_COLOR1, EMIT_3UB_3F_BGR, VFT0_SPEC);
      }
      else
         EMIT_PAD(3);

      if (index_bitset & BITFIELD64_BIT(_TNL_ATTRIB_FOG))
         EMIT_ATTR(_TNL_ATTRIB_FOG, EMIT_1UB_1F, VFT0_SPEC);
      else
         EMIT_PAD(1);
   }

   if (index_bitset & BITFIELD64_RANGE(_TNL_ATTRIB_TEX0, _TNL_NUM_TEX)) {
      int i, count = 0;

      for (i = 0; i < I830_TEX_UNITS; i++) {
         if (index_bitset & BITFIELD64_BIT(_TNL_ATTRIB_TEX(i))) {
            GLuint sz = VB->AttribPtr[_TNL_ATTRIB_TEX0 + i]->size;
            GLuint emit;
            GLuint mcs = (i830->state.Tex[i][I830_TEXREG_MCS] &
                          ~TEXCOORDTYPE_MASK);

            switch (sz) {
            case 1:
            case 2:
               emit = EMIT_2F;
               sz = 2;
               mcs |= TEXCOORDTYPE_CARTESIAN;
               break;
            case 3:
               emit = EMIT_3F;
               sz = 3;
               mcs |= TEXCOORDTYPE_VECTOR;
               break;
            case 4:
               emit = EMIT_3F_XYW;
               sz = 3;
               mcs |= TEXCOORDTYPE_HOMOGENEOUS;
               break;
            default:
               continue;
            };


            EMIT_ATTR(_TNL_ATTRIB_TEX0 + i, emit, 0);
            v2 |= VRTX_TEX_SET_FMT(count, SZ_TO_HW(sz));
            mcsb1 |= (count + 8) << (i * 4);

            if (mcs != i830->state.Tex[i][I830_TEXREG_MCS]) {
               I830_STATECHANGE(i830, I830_UPLOAD_TEX(i));
               i830->state.Tex[i][I830_TEXREG_MCS] = mcs;
            }

            count++;
         }
      }

      v0 |= VFT0_TEX_COUNT(count);
   }

   /* Only need to change the vertex emit code if there has been a
    * statechange to a new hardware vertex format:
    */
   if (v0 != i830->state.Ctx[I830_CTXREG_VF] ||
       v2 != i830->state.Ctx[I830_CTXREG_VF2] ||
       mcsb1 != i830->state.Ctx[I830_CTXREG_MCSB1] ||
       index_bitset != i830->last_index_bitset) {
      int k;

      I830_STATECHANGE(i830, I830_UPLOAD_CTX);

      /* Must do this *after* statechange, so as not to affect
       * buffered vertices reliant on the old state:
       */
      intel->vertex_size =
         _tnl_install_attrs(ctx,
                            intel->vertex_attrs,
                            intel->vertex_attr_count,
                            intel->ViewportMatrix.m, 0);

      intel->vertex_size >>= 2;

      i830->state.Ctx[I830_CTXREG_VF] = v0;
      i830->state.Ctx[I830_CTXREG_VF2] = v2;
      i830->state.Ctx[I830_CTXREG_MCSB1] = mcsb1;
      i830->last_index_bitset = index_bitset;

      k = i830_check_vertex_size(intel, intel->vertex_size);
      assert(k);
   }
}

static void
i830_reduced_primitive_state(struct intel_context *intel, GLenum rprim)
{
   struct i830_context *i830 = i830_context(&intel->ctx);
   GLuint st1 = i830->state.Stipple[I830_STPREG_ST1];

   st1 &= ~ST1_ENABLE;

   switch (rprim) {
   case GL_TRIANGLES:
      if (intel->ctx.Polygon.StippleFlag && intel->hw_stipple)
         st1 |= ST1_ENABLE;
      break;
   case GL_LINES:
   case GL_POINTS:
   default:
      break;
   }

   i830->intel.reduced_primitive = rprim;

   if (st1 != i830->state.Stipple[I830_STPREG_ST1]) {
      INTEL_FIREVERTICES(intel);

      I830_STATECHANGE(i830, I830_UPLOAD_STIPPLE);
      i830->state.Stipple[I830_STPREG_ST1] = st1;
   }
}

/* Pull apart the vertex format registers and figure out how large a
 * vertex is supposed to be. 
 */
static bool
i830_check_vertex_size(struct intel_context *intel, GLuint expected)
{
   struct i830_context *i830 = i830_context(&intel->ctx);
   int vft0 = i830->state.Ctx[I830_CTXREG_VF];
   int vft1 = i830->state.Ctx[I830_CTXREG_VF2];
   int nrtex = (vft0 & VFT0_TEX_COUNT_MASK) >> VFT0_TEX_COUNT_SHIFT;
   int i, sz = 0;

   switch (vft0 & VFT0_XYZW_MASK) {
   case VFT0_XY:
      sz = 2;
      break;
   case VFT0_XYZ:
      sz = 3;
      break;
   case VFT0_XYW:
      sz = 3;
      break;
   case VFT0_XYZW:
      sz = 4;
      break;
   default:
      fprintf(stderr, "no xyzw specified\n");
      return 0;
   }

   if (vft0 & VFT0_SPEC)
      sz++;
   if (vft0 & VFT0_DIFFUSE)
      sz++;
   if (vft0 & VFT0_DEPTH_OFFSET)
      sz++;
   if (vft0 & VFT0_POINT_WIDTH)
      sz++;

   for (i = 0; i < nrtex; i++) {
      switch (vft1 & VFT1_TEX0_MASK) {
      case TEXCOORDFMT_2D:
         sz += 2;
         break;
      case TEXCOORDFMT_3D:
         sz += 3;
         break;
      case TEXCOORDFMT_4D:
         sz += 4;
         break;
      case TEXCOORDFMT_1D:
         sz += 1;
         break;
      }
      vft1 >>= VFT1_TEX1_SHIFT;
   }

   if (sz != expected)
      fprintf(stderr, "vertex size mismatch %d/%d\n", sz, expected);

   return sz == expected;
}

static void
i830_emit_invarient_state(struct intel_context *intel)
{
   BATCH_LOCALS;

   BEGIN_BATCH(29);

   OUT_BATCH(_3DSTATE_DFLT_DIFFUSE_CMD);
   OUT_BATCH(0);

   OUT_BATCH(_3DSTATE_DFLT_SPEC_CMD);
   OUT_BATCH(0);

   OUT_BATCH(_3DSTATE_DFLT_Z_CMD);
   OUT_BATCH(0);

   OUT_BATCH(_3DSTATE_FOG_MODE_CMD);
   OUT_BATCH(FOGFUNC_ENABLE |
             FOG_LINEAR_CONST | FOGSRC_INDEX_Z | ENABLE_FOG_DENSITY);
   OUT_BATCH(0);
   OUT_BATCH(0);


   OUT_BATCH(_3DSTATE_MAP_TEX_STREAM_CMD |
             MAP_UNIT(0) |
             DISABLE_TEX_STREAM_BUMP |
             ENABLE_TEX_STREAM_COORD_SET |
             TEX_STREAM_COORD_SET(0) |
             ENABLE_TEX_STREAM_MAP_IDX | TEX_STREAM_MAP_IDX(0));
   OUT_BATCH(_3DSTATE_MAP_TEX_STREAM_CMD |
             MAP_UNIT(1) |
             DISABLE_TEX_STREAM_BUMP |
             ENABLE_TEX_STREAM_COORD_SET |
             TEX_STREAM_COORD_SET(1) |
             ENABLE_TEX_STREAM_MAP_IDX | TEX_STREAM_MAP_IDX(1));
   OUT_BATCH(_3DSTATE_MAP_TEX_STREAM_CMD |
             MAP_UNIT(2) |
             DISABLE_TEX_STREAM_BUMP |
             ENABLE_TEX_STREAM_COORD_SET |
             TEX_STREAM_COORD_SET(2) |
             ENABLE_TEX_STREAM_MAP_IDX | TEX_STREAM_MAP_IDX(2));
   OUT_BATCH(_3DSTATE_MAP_TEX_STREAM_CMD |
             MAP_UNIT(3) |
             DISABLE_TEX_STREAM_BUMP |
             ENABLE_TEX_STREAM_COORD_SET |
             TEX_STREAM_COORD_SET(3) |
             ENABLE_TEX_STREAM_MAP_IDX | TEX_STREAM_MAP_IDX(3));

   OUT_BATCH(_3DSTATE_MAP_COORD_TRANSFORM);
   OUT_BATCH(DISABLE_TEX_TRANSFORM | TEXTURE_SET(0));
   OUT_BATCH(_3DSTATE_MAP_COORD_TRANSFORM);
   OUT_BATCH(DISABLE_TEX_TRANSFORM | TEXTURE_SET(1));
   OUT_BATCH(_3DSTATE_MAP_COORD_TRANSFORM);
   OUT_BATCH(DISABLE_TEX_TRANSFORM | TEXTURE_SET(2));
   OUT_BATCH(_3DSTATE_MAP_COORD_TRANSFORM);
   OUT_BATCH(DISABLE_TEX_TRANSFORM | TEXTURE_SET(3));

   OUT_BATCH(_3DSTATE_VERTEX_TRANSFORM);
   OUT_BATCH(DISABLE_VIEWPORT_TRANSFORM | DISABLE_PERSPECTIVE_DIVIDE);

   OUT_BATCH(_3DSTATE_W_STATE_CMD);
   OUT_BATCH(MAGIC_W_STATE_DWORD1);
   OUT_BATCH(0x3f800000 /* 1.0 in IEEE float */ );


   OUT_BATCH(_3DSTATE_COLOR_FACTOR_CMD);
   OUT_BATCH(0x80808080);       /* .5 required in alpha for GL_DOT3_RGBA_EXT */

   ADVANCE_BATCH();
}


#define emit( intel, state, size )			\
   intel_batchbuffer_data(intel, state, size, false)

static GLuint
get_dirty(struct i830_hw_state *state)
{
   return state->active & ~state->emitted;
}

static GLuint
get_state_size(struct i830_hw_state *state)
{
   GLuint dirty = get_dirty(state);
   GLuint sz = 0;
   GLuint i;

   if (dirty & I830_UPLOAD_INVARIENT)
      sz += 40 * sizeof(int);

   if (dirty & I830_UPLOAD_RASTER_RULES)
      sz += sizeof(state->RasterRules);

   if (dirty & I830_UPLOAD_CTX)
      sz += sizeof(state->Ctx);

   if (dirty & I830_UPLOAD_BUFFERS)
      sz += sizeof(state->Buffer);

   if (dirty & I830_UPLOAD_STIPPLE)
      sz += sizeof(state->Stipple);

   for (i = 0; i < I830_TEX_UNITS; i++) {
      if ((dirty & I830_UPLOAD_TEX(i)))
         sz += sizeof(state->Tex[i]);

      if (dirty & I830_UPLOAD_TEXBLEND(i))
         sz += state->TexBlendWordsUsed[i] * 4;
   }

   return sz;
}


/* Push the state into the sarea and/or texture memory.
 */
static void
i830_emit_state(struct intel_context *intel)
{
   struct i830_context *i830 = i830_context(&intel->ctx);
   struct i830_hw_state *state = &i830->state;
   int i, count;
   GLuint dirty;
   drm_intel_bo *aper_array[3 + I830_TEX_UNITS];
   int aper_count;
   GET_CURRENT_CONTEXT(ctx);
   BATCH_LOCALS;

   /* We don't hold the lock at this point, so want to make sure that
    * there won't be a buffer wrap between the state emits and the primitive
    * emit header.
    *
    * It might be better to talk about explicit places where
    * scheduling is allowed, rather than assume that it is whenever a
    * batchbuffer fills up.
    */
   intel_batchbuffer_require_space(intel,
				   get_state_size(state) + INTEL_PRIM_EMIT_SIZE,
				   false);
   count = 0;
 again:
   aper_count = 0;
   dirty = get_dirty(state);

   aper_array[aper_count++] = intel->batch.bo;
   if (dirty & I830_UPLOAD_BUFFERS) {
      aper_array[aper_count++] = state->draw_region->bo;
      if (state->depth_region)
         aper_array[aper_count++] = state->depth_region->bo;
   }

   for (i = 0; i < I830_TEX_UNITS; i++)
     if (dirty & I830_UPLOAD_TEX(i)) {
	if (state->tex_buffer[i]) {
	   aper_array[aper_count++] = state->tex_buffer[i];
	}
     }

   if (dri_bufmgr_check_aperture_space(aper_array, aper_count)) {
       if (count == 0) {
	   count++;
	   intel_batchbuffer_flush(intel);
	   goto again;
       } else {
	   _mesa_error(ctx, GL_OUT_OF_MEMORY, "i830 emit state");
	   assert(0);
       }
   }


   /* Do this here as we may have flushed the batchbuffer above,
    * causing more state to be dirty!
    */
   dirty = get_dirty(state);
   state->emitted |= dirty;
   assert(get_dirty(state) == 0);

   if (dirty & I830_UPLOAD_INVARIENT) {
      DBG("I830_UPLOAD_INVARIENT:\n");
      i830_emit_invarient_state(intel);
   }

   if (dirty & I830_UPLOAD_RASTER_RULES) {
      DBG("I830_UPLOAD_RASTER_RULES:\n");
      emit(intel, state->RasterRules, sizeof(state->RasterRules));
   }

   if (dirty & I830_UPLOAD_CTX) {
      DBG("I830_UPLOAD_CTX:\n");
      emit(intel, state->Ctx, sizeof(state->Ctx));

   }

   if (dirty & I830_UPLOAD_BUFFERS) {
      GLuint count = 15;

      DBG("I830_UPLOAD_BUFFERS:\n");

      if (state->depth_region)
          count += 3;

      BEGIN_BATCH(count);
      OUT_BATCH(state->Buffer[I830_DESTREG_CBUFADDR0]);
      OUT_BATCH(state->Buffer[I830_DESTREG_CBUFADDR1]);
      OUT_RELOC(state->draw_region->bo,
		I915_GEM_DOMAIN_RENDER, I915_GEM_DOMAIN_RENDER, 0);

      if (state->depth_region) {
         OUT_BATCH(state->Buffer[I830_DESTREG_DBUFADDR0]);
         OUT_BATCH(state->Buffer[I830_DESTREG_DBUFADDR1]);
         OUT_RELOC(state->depth_region->bo,
		   I915_GEM_DOMAIN_RENDER, I915_GEM_DOMAIN_RENDER, 0);
      }

      OUT_BATCH(state->Buffer[I830_DESTREG_DV0]);
      OUT_BATCH(state->Buffer[I830_DESTREG_DV1]);
      OUT_BATCH(state->Buffer[I830_DESTREG_SENABLE]);
      OUT_BATCH(state->Buffer[I830_DESTREG_SR0]);
      OUT_BATCH(state->Buffer[I830_DESTREG_SR1]);
      OUT_BATCH(state->Buffer[I830_DESTREG_SR2]);

      assert(state->Buffer[I830_DESTREG_DRAWRECT0] != MI_NOOP);
      OUT_BATCH(state->Buffer[I830_DESTREG_DRAWRECT0]);
      OUT_BATCH(state->Buffer[I830_DESTREG_DRAWRECT1]);
      OUT_BATCH(state->Buffer[I830_DESTREG_DRAWRECT2]);
      OUT_BATCH(state->Buffer[I830_DESTREG_DRAWRECT3]);
      OUT_BATCH(state->Buffer[I830_DESTREG_DRAWRECT4]);
      OUT_BATCH(state->Buffer[I830_DESTREG_DRAWRECT5]);
      ADVANCE_BATCH();
   }
   
   if (dirty & I830_UPLOAD_STIPPLE) {
      DBG("I830_UPLOAD_STIPPLE:\n");
      emit(intel, state->Stipple, sizeof(state->Stipple));
   }

   for (i = 0; i < I830_TEX_UNITS; i++) {
      if ((dirty & I830_UPLOAD_TEX(i))) {
         DBG("I830_UPLOAD_TEX(%d):\n", i);

         BEGIN_BATCH(I830_TEX_SETUP_SIZE + 1);
         OUT_BATCH(state->Tex[i][I830_TEXREG_TM0LI]);

	 OUT_RELOC(state->tex_buffer[i],
		   I915_GEM_DOMAIN_SAMPLER, 0,
		   state->tex_offset[i]);

         OUT_BATCH(state->Tex[i][I830_TEXREG_TM0S1]);
         OUT_BATCH(state->Tex[i][I830_TEXREG_TM0S2]);
         OUT_BATCH(state->Tex[i][I830_TEXREG_TM0S3]);
         OUT_BATCH(state->Tex[i][I830_TEXREG_TM0S4]);
         OUT_BATCH(state->Tex[i][I830_TEXREG_MCS]);
         OUT_BATCH(state->Tex[i][I830_TEXREG_CUBE]);

         ADVANCE_BATCH();
      }

      if (dirty & I830_UPLOAD_TEXBLEND(i)) {
         DBG("I830_UPLOAD_TEXBLEND(%d): %d words\n", i,
             state->TexBlendWordsUsed[i]);
         emit(intel, state->TexBlend[i], state->TexBlendWordsUsed[i] * 4);
      }
   }

   assert(get_dirty(state) == 0);
}

static void
i830_destroy_context(struct intel_context *intel)
{
   GLuint i;
   struct i830_context *i830 = i830_context(&intel->ctx);

   intel_region_release(&i830->state.draw_region);
   intel_region_release(&i830->state.depth_region);

   for (i = 0; i < I830_TEX_UNITS; i++) {
      if (i830->state.tex_buffer[i] != NULL) {
	 drm_intel_bo_unreference(i830->state.tex_buffer[i]);
	 i830->state.tex_buffer[i] = NULL;
      }
   }

   _tnl_free_vertices(&intel->ctx);
}

static uint32_t i830_render_target_format_for_mesa_format[MESA_FORMAT_COUNT] =
{
   [MESA_FORMAT_ARGB8888] = DV_PF_8888,
   [MESA_FORMAT_XRGB8888] = DV_PF_8888,
   [MESA_FORMAT_RGB565] = DV_PF_565,
   [MESA_FORMAT_ARGB1555] = DV_PF_1555,
   [MESA_FORMAT_ARGB4444] = DV_PF_4444,
};

static bool
i830_render_target_supported(struct intel_context *intel,
			     struct gl_renderbuffer *rb)
{
   gl_format format = rb->Format;

   if (format == MESA_FORMAT_S8_Z24 ||
       format == MESA_FORMAT_X8_Z24 ||
       format == MESA_FORMAT_Z16) {
      return true;
   }

   return i830_render_target_format_for_mesa_format[format] != 0;
}

static void
i830_set_draw_region(struct intel_context *intel,
                     struct intel_region *color_regions[],
                     struct intel_region *depth_region,
		     GLuint num_regions)
{
   struct i830_context *i830 = i830_context(&intel->ctx);
   struct gl_context *ctx = &intel->ctx;
   struct gl_renderbuffer *rb = ctx->DrawBuffer->_ColorDrawBuffers[0];
   struct intel_renderbuffer *irb = intel_renderbuffer(rb);
   struct gl_renderbuffer *drb;
   struct intel_renderbuffer *idrb = NULL;
   GLuint value;
   struct i830_hw_state *state = &i830->state;
   uint32_t draw_x, draw_y;

   if (state->draw_region != color_regions[0]) {
      intel_region_reference(&state->draw_region, color_regions[0]);
   }
   if (state->depth_region != depth_region) {
      intel_region_reference(&state->depth_region, depth_region);
   }

   /*
    * Set stride/cpp values
    */
   i915_set_buf_info_for_region(&state->Buffer[I830_DESTREG_CBUFADDR0],
				color_regions[0], BUF_3D_ID_COLOR_BACK);

   i915_set_buf_info_for_region(&state->Buffer[I830_DESTREG_DBUFADDR0],
				depth_region, BUF_3D_ID_DEPTH);

   /*
    * Compute/set I830_DESTREG_DV1 value
    */
   value = (DSTORG_HORT_BIAS(0x8) |     /* .5 */
            DSTORG_VERT_BIAS(0x8) | DEPTH_IS_Z);    /* .5 */

   if (irb != NULL) {
      value |= i830_render_target_format_for_mesa_format[intel_rb_format(irb)];
   }

   if (depth_region && depth_region->cpp == 4) {
      value |= DEPTH_FRMT_24_FIXED_8_OTHER;
   }
   else {
      value |= DEPTH_FRMT_16_FIXED;
   }
   state->Buffer[I830_DESTREG_DV1] = value;

   drb = ctx->DrawBuffer->Attachment[BUFFER_DEPTH].Renderbuffer;
   if (!drb)
      drb = ctx->DrawBuffer->Attachment[BUFFER_STENCIL].Renderbuffer;

   if (drb)
      idrb = intel_renderbuffer(drb);

   /* We set up the drawing rectangle to be offset into the color
    * region's location in the miptree.  If it doesn't match with
    * depth's offsets, we can't render to it.
    *
    * (Well, not actually true -- the hw grew a bit to let depth's
    * offset get forced to 0,0.  We may want to use that if people are
    * hitting that case.  Also, some configurations may be supportable
    * by tweaking the start offset of the buffers around, which we
    * can't do in general due to tiling)
    */
   FALLBACK(intel, I830_FALLBACK_DRAW_OFFSET,
	    idrb && irb && (idrb->draw_x != irb->draw_x ||
			    idrb->draw_y != irb->draw_y));

   if (irb) {
      draw_x = irb->draw_x;
      draw_y = irb->draw_y;
   } else if (idrb) {
      draw_x = idrb->draw_x;
      draw_y = idrb->draw_y;
   } else {
      draw_x = 0;
      draw_y = 0;
   }

   state->Buffer[I830_DESTREG_DRAWRECT0] = _3DSTATE_DRAWRECT_INFO;
   state->Buffer[I830_DESTREG_DRAWRECT1] = 0;
   state->Buffer[I830_DESTREG_DRAWRECT2] = (draw_y << 16) | draw_x;
   state->Buffer[I830_DESTREG_DRAWRECT3] =
      ((ctx->DrawBuffer->Width + draw_x - 1) & 0xffff) |
      ((ctx->DrawBuffer->Height + draw_y - 1) << 16);
   state->Buffer[I830_DESTREG_DRAWRECT4] = (draw_y << 16) | draw_x;
   state->Buffer[I830_DESTREG_DRAWRECT5] = MI_NOOP;

   I830_STATECHANGE(i830, I830_UPLOAD_BUFFERS);
}

/**
 * Update the hardware state for drawing into a window or framebuffer object.
 *
 * Called by glDrawBuffer, glBindFramebufferEXT, MakeCurrent, and other
 * places within the driver.
 *
 * Basically, this needs to be called any time the current framebuffer
 * changes, the renderbuffers change, or we need to draw into different
 * color buffers.
 */
static void
i830_update_draw_buffer(struct intel_context *intel)
{
   struct gl_context *ctx = &intel->ctx;
   struct gl_framebuffer *fb = ctx->DrawBuffer;
   struct intel_region *colorRegions[MAX_DRAW_BUFFERS], *depthRegion = NULL;
   struct intel_renderbuffer *irbDepth = NULL, *irbStencil = NULL;

   if (!fb) {
      /* this can happen during the initial context initialization */
      return;
   }

   irbDepth = intel_get_renderbuffer(fb, BUFFER_DEPTH);
   irbStencil = intel_get_renderbuffer(fb, BUFFER_STENCIL);

   /* Do this here, not core Mesa, since this function is called from
    * many places within the driver.
    */
   if (ctx->NewState & _NEW_BUFFERS) {
      /* this updates the DrawBuffer->_NumColorDrawBuffers fields, etc */
      _mesa_update_framebuffer(ctx);
      /* this updates the DrawBuffer's Width/Height if it's a FBO */
      _mesa_update_draw_buffer_bounds(ctx);
   }

   if (fb->_Status != GL_FRAMEBUFFER_COMPLETE_EXT) {
      /* this may occur when we're called by glBindFrameBuffer() during
       * the process of someone setting up renderbuffers, etc.
       */
      /*_mesa_debug(ctx, "DrawBuffer: incomplete user FBO\n");*/
      return;
   }

   /* How many color buffers are we drawing into?
    *
    * If there are zero buffers or the buffer is too big, don't configure any
    * regions for hardware drawing.  We'll fallback to software below.  Not
    * having regions set makes some of the software fallback paths faster.
    */
   if ((fb->Width > ctx->Const.MaxRenderbufferSize)
       || (fb->Height > ctx->Const.MaxRenderbufferSize)
       || (fb->_NumColorDrawBuffers == 0)) {
      /* writing to 0  */
      colorRegions[0] = NULL;
   }
   else if (fb->_NumColorDrawBuffers > 1) {
       int i;
       struct intel_renderbuffer *irb;

       for (i = 0; i < fb->_NumColorDrawBuffers; i++) {
           irb = intel_renderbuffer(fb->_ColorDrawBuffers[i]);
           colorRegions[i] = (irb && irb->mt) ? irb->mt->region : NULL;
       }
   }
   else {
      /* Get the intel_renderbuffer for the single colorbuffer we're drawing
       * into.
       */
      if (_mesa_is_winsys_fbo(fb)) {
	 /* drawing to window system buffer */
	 if (fb->_ColorDrawBufferIndexes[0] == BUFFER_FRONT_LEFT)
	    colorRegions[0] = intel_get_rb_region(fb, BUFFER_FRONT_LEFT);
	 else
	    colorRegions[0] = intel_get_rb_region(fb, BUFFER_BACK_LEFT);
      }
      else {
	 /* drawing to user-created FBO */
	 struct intel_renderbuffer *irb;
	 irb = intel_renderbuffer(fb->_ColorDrawBuffers[0]);
	 colorRegions[0] = (irb && irb->mt->region) ? irb->mt->region : NULL;
      }
   }

   if (!colorRegions[0]) {
      FALLBACK(intel, INTEL_FALLBACK_DRAW_BUFFER, true);
   }
   else {
      FALLBACK(intel, INTEL_FALLBACK_DRAW_BUFFER, false);
   }

   /* Check for depth fallback. */
   if (irbDepth && irbDepth->mt) {
      FALLBACK(intel, INTEL_FALLBACK_DEPTH_BUFFER, false);
      depthRegion = irbDepth->mt->region;
   } else if (irbDepth && !irbDepth->mt) {
      FALLBACK(intel, INTEL_FALLBACK_DEPTH_BUFFER, true);
      depthRegion = NULL;
   } else { /* !irbDepth */
      /* No fallback is needed because there is no depth buffer. */
      FALLBACK(intel, INTEL_FALLBACK_DEPTH_BUFFER, false);
      depthRegion = NULL;
   }

   /* Check for stencil fallback. */
   if (irbStencil && irbStencil->mt) {
      assert(intel_rb_format(irbStencil) == MESA_FORMAT_S8_Z24);
      FALLBACK(intel, INTEL_FALLBACK_STENCIL_BUFFER, false);
   } else if (irbStencil && !irbStencil->mt) {
      FALLBACK(intel, INTEL_FALLBACK_STENCIL_BUFFER, true);
   } else { /* !irbStencil */
      /* No fallback is needed because there is no stencil buffer. */
      FALLBACK(intel, INTEL_FALLBACK_STENCIL_BUFFER, false);
   }

   /* If we have a (packed) stencil buffer attached but no depth buffer,
    * we still need to set up the shared depth/stencil state so we can use it.
    */
   if (depthRegion == NULL && irbStencil && irbStencil->mt
       && intel_rb_format(irbStencil) == MESA_FORMAT_S8_Z24) {
      depthRegion = irbStencil->mt->region;
   }

   /*
    * Update depth and stencil test state
    */
   ctx->Driver.Enable(ctx, GL_DEPTH_TEST, ctx->Depth.Test);
   ctx->Driver.Enable(ctx, GL_STENCIL_TEST,
		      (ctx->Stencil.Enabled && fb->Visual.stencilBits > 0));

   intel->vtbl.set_draw_region(intel, colorRegions, depthRegion,
                               fb->_NumColorDrawBuffers);
   intel->NewGLState |= _NEW_BUFFERS;

   /* update viewport since it depends on window size */
   intelCalcViewport(ctx);

   /* Set state we know depends on drawable parameters:
    */
   ctx->Driver.Scissor(ctx, ctx->Scissor.X, ctx->Scissor.Y,
		       ctx->Scissor.Width, ctx->Scissor.Height);

   ctx->Driver.DepthRange(ctx, ctx->Viewport.Near, ctx->Viewport.Far);

   /* Update culling direction which changes depending on the
    * orientation of the buffer:
    */
   ctx->Driver.FrontFace(ctx, ctx->Polygon.FrontFace);
}

/* This isn't really handled at the moment.
 */
static void
i830_new_batch(struct intel_context *intel)
{
   struct i830_context *i830 = i830_context(&intel->ctx);
   i830->state.emitted = 0;
}

static void 
i830_assert_not_dirty( struct intel_context *intel )
{
   struct i830_context *i830 = i830_context(&intel->ctx);
   assert(!get_dirty(&i830->state));
   (void) i830;
}

static void
i830_invalidate_state(struct intel_context *intel, GLuint new_state)
{
   struct gl_context *ctx = &intel->ctx;

   _swsetup_InvalidateState(ctx, new_state);
   _tnl_InvalidateState(ctx, new_state);
   _tnl_invalidate_vertex_state(ctx, new_state);

   if (new_state & _NEW_LIGHT)
      i830_update_provoking_vertex(&intel->ctx);
}

static bool
i830_is_hiz_depth_format(struct intel_context *intel, gl_format format)
{
   return false;
}

void
i830InitVtbl(struct i830_context *i830)
{
   i830->intel.vtbl.check_vertex_size = i830_check_vertex_size;
   i830->intel.vtbl.destroy = i830_destroy_context;
   i830->intel.vtbl.emit_state = i830_emit_state;
   i830->intel.vtbl.new_batch = i830_new_batch;
   i830->intel.vtbl.reduced_primitive_state = i830_reduced_primitive_state;
   i830->intel.vtbl.set_draw_region = i830_set_draw_region;
   i830->intel.vtbl.update_draw_buffer = i830_update_draw_buffer;
   i830->intel.vtbl.update_texture_state = i830UpdateTextureState;
   i830->intel.vtbl.render_start = i830_render_start;
   i830->intel.vtbl.render_prevalidate = i830_render_prevalidate;
   i830->intel.vtbl.assert_not_dirty = i830_assert_not_dirty;
   i830->intel.vtbl.finish_batch = intel_finish_vb;
   i830->intel.vtbl.invalidate_state = i830_invalidate_state;
   i830->intel.vtbl.render_target_supported = i830_render_target_supported;
   i830->intel.vtbl.is_hiz_depth_format = i830_is_hiz_depth_format;
}
