/*
 * Copyright © 2011 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "brw_context.h"
#include "brw_state.h"
#include "brw_defines.h"
#include "brw_util.h"
#include "main/macros.h"
#include "main/fbobject.h"
#include "intel_batchbuffer.h"

static void
upload_sbe_state(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;
   struct gl_context *ctx = &intel->ctx;
   /* BRW_NEW_FRAGMENT_PROGRAM */
   uint32_t num_outputs = _mesa_bitcount_64(brw->fragment_program->Base.InputsRead);
   /* _NEW_LIGHT */
   bool shade_model_flat = ctx->Light.ShadeModel == GL_FLAT;
   uint32_t dw1, dw10, dw11;
   int i;
   int attr = 0, input_index = 0;
   int urb_entry_read_offset = 1;
   uint16_t attr_overrides[FRAG_ATTRIB_MAX];
   /* _NEW_BUFFERS */
   bool render_to_fbo = _mesa_is_user_fbo(ctx->DrawBuffer);
   uint32_t point_sprite_origin;

   /* FINISHME: Attribute Swizzle Control Mode? */
   dw1 = GEN7_SBE_SWIZZLE_ENABLE | num_outputs << GEN7_SBE_NUM_OUTPUTS_SHIFT;

   /* _NEW_POINT
    *
    * Window coordinates in an FBO are inverted, which means point
    * sprite origin must be inverted.
    */
   if ((ctx->Point.SpriteOrigin == GL_LOWER_LEFT) != render_to_fbo) {
      point_sprite_origin = GEN6_SF_POINT_SPRITE_LOWERLEFT;
   } else {
      point_sprite_origin = GEN6_SF_POINT_SPRITE_UPPERLEFT;
   }
   dw1 |= point_sprite_origin;


   dw10 = 0;
   dw11 = 0;

   /* Create the mapping from the FS inputs we produce to the VS outputs
    * they source from.
    */
   uint32_t max_source_attr = 0;
   for (; attr < FRAG_ATTRIB_MAX; attr++) {
      enum glsl_interp_qualifier interp_qualifier =
         brw->fragment_program->InterpQualifier[attr];
      bool is_gl_Color = attr == FRAG_ATTRIB_COL0 || attr == FRAG_ATTRIB_COL1;

      if (!(brw->fragment_program->Base.InputsRead & BITFIELD64_BIT(attr)))
	 continue;

      if (ctx->Point.PointSprite &&
	  attr >= FRAG_ATTRIB_TEX0 && attr <= FRAG_ATTRIB_TEX7 &&
	  ctx->Point.CoordReplace[attr - FRAG_ATTRIB_TEX0]) {
	 dw10 |= (1 << input_index);
      }

      if (attr == FRAG_ATTRIB_PNTC)
	 dw10 |= (1 << input_index);

      /* flat shading */
      if (interp_qualifier == INTERP_QUALIFIER_FLAT ||
          (shade_model_flat && is_gl_Color &&
           interp_qualifier == INTERP_QUALIFIER_NONE))
         dw11 |= (1 << input_index);

      /* The hardware can only do the overrides on 16 overrides at a
       * time, and the other up to 16 have to be lined up so that the
       * input index = the output index.  We'll need to do some
       * tweaking to make sure that's the case.
       */
      assert(input_index < 16 || attr == input_index);

      /* CACHE_NEW_VS_PROG | _NEW_LIGHT | _NEW_PROGRAM */
      attr_overrides[input_index++] =
         get_attr_override(&brw->vs.prog_data->vue_map,
			   urb_entry_read_offset, attr,
                           ctx->VertexProgram._TwoSideEnabled,
                           &max_source_attr);
   }

   /* From the Ivy Bridge PRM, Volume 2, Part 1, documentation for
    * 3DSTATE_SBE DWord 1 bits 15:11, "Vertex URB Entry Read Length":
    *
    * "This field should be set to the minimum length required to read the
    *  maximum source attribute.  The maximum source attribute is indicated
    *  by the maximum value of the enabled Attribute # Source Attribute if
    *  Attribute Swizzle Enable is set, Number of Output Attributes-1 if
    *  enable is not set.
    *
    *  read_length = ceiling((max_source_attr + 1) / 2)"
    */
   uint32_t urb_entry_read_length = ALIGN(max_source_attr + 1, 2) / 2;
   dw1 |= urb_entry_read_length << GEN7_SBE_URB_ENTRY_READ_LENGTH_SHIFT |
          urb_entry_read_offset << GEN7_SBE_URB_ENTRY_READ_OFFSET_SHIFT;

   for (; input_index < FRAG_ATTRIB_MAX; input_index++)
      attr_overrides[input_index] = 0;

   BEGIN_BATCH(14);
   OUT_BATCH(_3DSTATE_SBE << 16 | (14 - 2));
   OUT_BATCH(dw1);

   /* Output dwords 2 through 9 */
   for (i = 0; i < 8; i++) {
      OUT_BATCH(attr_overrides[i * 2] | attr_overrides[i * 2 + 1] << 16);
   }

   OUT_BATCH(dw10); /* point sprite texcoord bitmask */
   OUT_BATCH(dw11); /* constant interp bitmask */
   OUT_BATCH(0); /* wrapshortest enables 0-7 */
   OUT_BATCH(0); /* wrapshortest enables 8-15 */
   ADVANCE_BATCH();
}

const struct brw_tracked_state gen7_sbe_state = {
   .dirty = {
      .mesa  = (_NEW_BUFFERS |
		_NEW_LIGHT |
		_NEW_POINT |
		_NEW_PROGRAM),
      .brw   = (BRW_NEW_CONTEXT |
		BRW_NEW_FRAGMENT_PROGRAM),
      .cache = CACHE_NEW_VS_PROG
   },
   .emit = upload_sbe_state,
};

static void
upload_sf_state(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;
   struct gl_context *ctx = &intel->ctx;
   uint32_t dw1, dw2, dw3;
   float point_size;
   /* _NEW_BUFFERS */
   bool render_to_fbo = _mesa_is_user_fbo(brw->intel.ctx.DrawBuffer);
   bool multisampled_fbo = ctx->DrawBuffer->Visual.samples > 1;

   dw1 = GEN6_SF_STATISTICS_ENABLE |
         GEN6_SF_VIEWPORT_TRANSFORM_ENABLE;

   /* _NEW_BUFFERS */
   dw1 |= (brw_depthbuffer_format(brw) << GEN7_SF_DEPTH_BUFFER_SURFACE_FORMAT_SHIFT);

   /* _NEW_POLYGON */
   if ((ctx->Polygon.FrontFace == GL_CCW) ^ render_to_fbo)
      dw1 |= GEN6_SF_WINDING_CCW;

   if (ctx->Polygon.OffsetFill)
       dw1 |= GEN6_SF_GLOBAL_DEPTH_OFFSET_SOLID;

   if (ctx->Polygon.OffsetLine)
       dw1 |= GEN6_SF_GLOBAL_DEPTH_OFFSET_WIREFRAME;

   if (ctx->Polygon.OffsetPoint)
       dw1 |= GEN6_SF_GLOBAL_DEPTH_OFFSET_POINT;

   switch (ctx->Polygon.FrontMode) {
   case GL_FILL:
       dw1 |= GEN6_SF_FRONT_SOLID;
       break;

   case GL_LINE:
       dw1 |= GEN6_SF_FRONT_WIREFRAME;
       break;

   case GL_POINT:
       dw1 |= GEN6_SF_FRONT_POINT;
       break;

   default:
       assert(0);
       break;
   }

   switch (ctx->Polygon.BackMode) {
   case GL_FILL:
       dw1 |= GEN6_SF_BACK_SOLID;
       break;

   case GL_LINE:
       dw1 |= GEN6_SF_BACK_WIREFRAME;
       break;

   case GL_POINT:
       dw1 |= GEN6_SF_BACK_POINT;
       break;

   default:
       assert(0);
       break;
   }

   dw2 = 0;

   if (ctx->Polygon.CullFlag) {
      switch (ctx->Polygon.CullFaceMode) {
      case GL_FRONT:
	 dw2 |= GEN6_SF_CULL_FRONT;
	 break;
      case GL_BACK:
	 dw2 |= GEN6_SF_CULL_BACK;
	 break;
      case GL_FRONT_AND_BACK:
	 dw2 |= GEN6_SF_CULL_BOTH;
	 break;
      default:
	 assert(0);
	 break;
      }
   } else {
      dw2 |= GEN6_SF_CULL_NONE;
   }

   /* _NEW_SCISSOR */
   if (ctx->Scissor.Enabled)
      dw2 |= GEN6_SF_SCISSOR_ENABLE;

   /* _NEW_LINE */
   {
      uint32_t line_width_u3_7 = U_FIXED(CLAMP(ctx->Line.Width, 0.0, 7.99), 7);
      /* TODO: line width of 0 is not allowed when MSAA enabled */
      if (line_width_u3_7 == 0)
         line_width_u3_7 = 1;
      dw2 |= line_width_u3_7 << GEN6_SF_LINE_WIDTH_SHIFT;
   }
   if (ctx->Line.SmoothFlag) {
      dw2 |= GEN6_SF_LINE_AA_ENABLE;
      dw2 |= GEN6_SF_LINE_END_CAP_WIDTH_1_0;
   }
   if (ctx->Line.StippleFlag && intel->is_haswell) {
      dw2 |= HSW_SF_LINE_STIPPLE_ENABLE;
   }
   /* _NEW_MULTISAMPLE */
   if (multisampled_fbo && ctx->Multisample.Enabled)
      dw2 |= GEN6_SF_MSRAST_ON_PATTERN;

   /* FINISHME: Last Pixel Enable?  Vertex Sub Pixel Precision Select?
    */

   dw3 = GEN6_SF_LINE_AA_MODE_TRUE;

   /* _NEW_PROGRAM | _NEW_POINT */
   if (!(ctx->VertexProgram.PointSizeEnabled || ctx->Point._Attenuated))
      dw3 |= GEN6_SF_USE_STATE_POINT_WIDTH;

   /* Clamp to ARB_point_parameters user limits */
   point_size = CLAMP(ctx->Point.Size, ctx->Point.MinSize, ctx->Point.MaxSize);

   /* Clamp to the hardware limits and convert to fixed point */
   dw3 |= U_FIXED(CLAMP(point_size, 0.125, 255.875), 3);

   /* _NEW_LIGHT */
   if (ctx->Light.ProvokingVertex != GL_FIRST_VERTEX_CONVENTION) {
      dw3 |=
	 (2 << GEN6_SF_TRI_PROVOKE_SHIFT) |
	 (2 << GEN6_SF_TRIFAN_PROVOKE_SHIFT) |
	 (1 << GEN6_SF_LINE_PROVOKE_SHIFT);
   } else {
      dw3 |= (1 << GEN6_SF_TRIFAN_PROVOKE_SHIFT);
   }

   BEGIN_BATCH(7);
   OUT_BATCH(_3DSTATE_SF << 16 | (7 - 2));
   OUT_BATCH(dw1);
   OUT_BATCH(dw2);
   OUT_BATCH(dw3);
   OUT_BATCH_F(ctx->Polygon.OffsetUnits * 2); /* constant.  copied from gen4 */
   OUT_BATCH_F(ctx->Polygon.OffsetFactor); /* scale */
   OUT_BATCH_F(0.0); /* XXX: global depth offset clamp */
   ADVANCE_BATCH();
}

const struct brw_tracked_state gen7_sf_state = {
   .dirty = {
      .mesa  = (_NEW_LIGHT |
		_NEW_PROGRAM |
		_NEW_POLYGON |
		_NEW_LINE |
		_NEW_SCISSOR |
		_NEW_BUFFERS |
		_NEW_POINT |
                _NEW_MULTISAMPLE),
      .brw   = BRW_NEW_CONTEXT,
      .cache = CACHE_NEW_VS_PROG
   },
   .emit = upload_sf_state,
};
