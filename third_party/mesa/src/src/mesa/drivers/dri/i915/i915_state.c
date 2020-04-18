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


#include "main/glheader.h"
#include "main/context.h"
#include "main/macros.h"
#include "main/enums.h"
#include "main/fbobject.h"
#include "main/dd.h"
#include "main/state.h"
#include "tnl/tnl.h"
#include "tnl/t_context.h"

#include "drivers/common/driverfuncs.h"

#include "intel_fbo.h"
#include "intel_screen.h"
#include "intel_batchbuffer.h"
#include "intel_buffers.h"

#include "i915_context.h"
#include "i915_reg.h"

#define FILE_DEBUG_FLAG DEBUG_STATE

void
i915_update_stencil(struct gl_context * ctx)
{
   struct i915_context *i915 = I915_CONTEXT(ctx);
   GLuint front_ref, front_writemask, front_mask;
   GLenum front_func, front_fail, front_pass_z_fail, front_pass_z_pass;
   GLuint back_ref, back_writemask, back_mask;
   GLenum back_func, back_fail, back_pass_z_fail, back_pass_z_pass;
   GLuint dirty = 0;

   /* The 915 considers CW to be "front" for two-sided stencil, so choose
    * appropriately.
    */
   /* _NEW_POLYGON | _NEW_STENCIL */
   if (ctx->Polygon.FrontFace == GL_CW) {
      front_ref = ctx->Stencil.Ref[0];
      front_mask = ctx->Stencil.ValueMask[0];
      front_writemask = ctx->Stencil.WriteMask[0];
      front_func = ctx->Stencil.Function[0];
      front_fail = ctx->Stencil.FailFunc[0];
      front_pass_z_fail = ctx->Stencil.ZFailFunc[0];
      front_pass_z_pass = ctx->Stencil.ZPassFunc[0];
      back_ref = ctx->Stencil.Ref[ctx->Stencil._BackFace];
      back_mask = ctx->Stencil.ValueMask[ctx->Stencil._BackFace];
      back_writemask = ctx->Stencil.WriteMask[ctx->Stencil._BackFace];
      back_func = ctx->Stencil.Function[ctx->Stencil._BackFace];
      back_fail = ctx->Stencil.FailFunc[ctx->Stencil._BackFace];
      back_pass_z_fail = ctx->Stencil.ZFailFunc[ctx->Stencil._BackFace];
      back_pass_z_pass = ctx->Stencil.ZPassFunc[ctx->Stencil._BackFace];
   } else {
      front_ref = ctx->Stencil.Ref[ctx->Stencil._BackFace];
      front_mask = ctx->Stencil.ValueMask[ctx->Stencil._BackFace];
      front_writemask = ctx->Stencil.WriteMask[ctx->Stencil._BackFace];
      front_func = ctx->Stencil.Function[ctx->Stencil._BackFace];
      front_fail = ctx->Stencil.FailFunc[ctx->Stencil._BackFace];
      front_pass_z_fail = ctx->Stencil.ZFailFunc[ctx->Stencil._BackFace];
      front_pass_z_pass = ctx->Stencil.ZPassFunc[ctx->Stencil._BackFace];
      back_ref = ctx->Stencil.Ref[0];
      back_mask = ctx->Stencil.ValueMask[0];
      back_writemask = ctx->Stencil.WriteMask[0];
      back_func = ctx->Stencil.Function[0];
      back_fail = ctx->Stencil.FailFunc[0];
      back_pass_z_fail = ctx->Stencil.ZFailFunc[0];
      back_pass_z_pass = ctx->Stencil.ZPassFunc[0];
   }
#define set_ctx_bits(reg, mask, set) do{ \
   GLuint dw = i915->state.Ctx[reg]; \
   dw &= ~(mask); \
   dw |= (set); \
   dirty |= dw != i915->state.Ctx[reg]; \
   i915->state.Ctx[reg] = dw; \
} while(0)

   /* Set front state. */
   set_ctx_bits(I915_CTXREG_STATE4,
                MODE4_ENABLE_STENCIL_TEST_MASK |
                MODE4_ENABLE_STENCIL_WRITE_MASK,
                ENABLE_STENCIL_TEST_MASK |
                ENABLE_STENCIL_WRITE_MASK |
                STENCIL_TEST_MASK(front_mask) |
                STENCIL_WRITE_MASK(front_writemask));

   set_ctx_bits(I915_CTXREG_LIS5,
                S5_STENCIL_REF_MASK |
                S5_STENCIL_TEST_FUNC_MASK |
                S5_STENCIL_FAIL_MASK |
                S5_STENCIL_PASS_Z_FAIL_MASK |
                S5_STENCIL_PASS_Z_PASS_MASK,
                (front_ref << S5_STENCIL_REF_SHIFT) |
                (intel_translate_compare_func(front_func) << S5_STENCIL_TEST_FUNC_SHIFT) |
                (intel_translate_stencil_op(front_fail) << S5_STENCIL_FAIL_SHIFT) |
                (intel_translate_stencil_op(front_pass_z_fail) <<
                 S5_STENCIL_PASS_Z_FAIL_SHIFT) |
                (intel_translate_stencil_op(front_pass_z_pass) <<
                 S5_STENCIL_PASS_Z_PASS_SHIFT));

   /* Set back state if different from front. */
   if (ctx->Stencil._TestTwoSide) {
      set_ctx_bits(I915_CTXREG_BF_STENCIL_OPS,
                   BFO_STENCIL_REF_MASK |
                   BFO_STENCIL_TEST_MASK |
                   BFO_STENCIL_FAIL_MASK |
                   BFO_STENCIL_PASS_Z_FAIL_MASK |
                   BFO_STENCIL_PASS_Z_PASS_MASK,
                   BFO_STENCIL_TWO_SIDE |
                   (back_ref << BFO_STENCIL_REF_SHIFT) |
                   (intel_translate_compare_func(back_func) << BFO_STENCIL_TEST_SHIFT) |
                   (intel_translate_stencil_op(back_fail) << BFO_STENCIL_FAIL_SHIFT) |
                   (intel_translate_stencil_op(back_pass_z_fail) <<
                    BFO_STENCIL_PASS_Z_FAIL_SHIFT) |
                   (intel_translate_stencil_op(back_pass_z_pass) <<
                    BFO_STENCIL_PASS_Z_PASS_SHIFT));

      set_ctx_bits(I915_CTXREG_BF_STENCIL_MASKS,
                   BFM_STENCIL_TEST_MASK_MASK |
                   BFM_STENCIL_WRITE_MASK_MASK,
                   BFM_STENCIL_TEST_MASK(back_mask) |
                   BFM_STENCIL_WRITE_MASK(back_writemask));
   } else {
      set_ctx_bits(I915_CTXREG_BF_STENCIL_OPS,
                   BFO_STENCIL_TWO_SIDE, 0);
   }

#undef set_ctx_bits

   if (dirty)
      I915_STATECHANGE(i915, I915_UPLOAD_CTX);
}

static void
i915StencilFuncSeparate(struct gl_context * ctx, GLenum face, GLenum func, GLint ref,
                        GLuint mask)
{
}

static void
i915StencilMaskSeparate(struct gl_context * ctx, GLenum face, GLuint mask)
{
}

static void
i915StencilOpSeparate(struct gl_context * ctx, GLenum face, GLenum fail, GLenum zfail,
                      GLenum zpass)
{
}

static void
i915AlphaFunc(struct gl_context * ctx, GLenum func, GLfloat ref)
{
   struct i915_context *i915 = I915_CONTEXT(ctx);
   int test = intel_translate_compare_func(func);
   GLubyte refByte;
   GLuint dw;

   UNCLAMPED_FLOAT_TO_UBYTE(refByte, ref);

   dw = i915->state.Ctx[I915_CTXREG_LIS6];
   dw &= ~(S6_ALPHA_TEST_FUNC_MASK | S6_ALPHA_REF_MASK);
   dw |= ((test << S6_ALPHA_TEST_FUNC_SHIFT) |
	  (((GLuint) refByte) << S6_ALPHA_REF_SHIFT));
   if (dw != i915->state.Ctx[I915_CTXREG_LIS6]) {
      i915->state.Ctx[I915_CTXREG_LIS6] = dw;
      I915_STATECHANGE(i915, I915_UPLOAD_CTX);
   }
}

/* This function makes sure that the proper enables are
 * set for LogicOp, Independant Alpha Blend, and Blending.
 * It needs to be called from numerous places where we
 * could change the LogicOp or Independant Alpha Blend without subsequent
 * calls to glEnable.
 */
static void
i915EvalLogicOpBlendState(struct gl_context * ctx)
{
   struct i915_context *i915 = I915_CONTEXT(ctx);
   GLuint dw0, dw1;

   dw0 = i915->state.Ctx[I915_CTXREG_LIS5];
   dw1 = i915->state.Ctx[I915_CTXREG_LIS6];

   if (ctx->Color.ColorLogicOpEnabled) {
      dw0 |= S5_LOGICOP_ENABLE;
      dw1 &= ~S6_CBUF_BLEND_ENABLE;
   }
   else {
      dw0 &= ~S5_LOGICOP_ENABLE;

      if (ctx->Color.BlendEnabled) {
         dw1 |= S6_CBUF_BLEND_ENABLE;
      }
      else {
         dw1 &= ~S6_CBUF_BLEND_ENABLE;
      }
   }
   if (dw0 != i915->state.Ctx[I915_CTXREG_LIS5] ||
       dw1 != i915->state.Ctx[I915_CTXREG_LIS6]) {
      i915->state.Ctx[I915_CTXREG_LIS5] = dw0;
      i915->state.Ctx[I915_CTXREG_LIS6] = dw1;

      I915_STATECHANGE(i915, I915_UPLOAD_CTX);
   }
}

static void
i915BlendColor(struct gl_context * ctx, const GLfloat color[4])
{
   struct i915_context *i915 = I915_CONTEXT(ctx);
   GLubyte r, g, b, a;
   GLuint dw;

   DBG("%s\n", __FUNCTION__);
   
   UNCLAMPED_FLOAT_TO_UBYTE(r, color[RCOMP]);
   UNCLAMPED_FLOAT_TO_UBYTE(g, color[GCOMP]);
   UNCLAMPED_FLOAT_TO_UBYTE(b, color[BCOMP]);
   UNCLAMPED_FLOAT_TO_UBYTE(a, color[ACOMP]);

   dw = (a << 24) | (r << 16) | (g << 8) | b;
   if (dw != i915->state.Blend[I915_BLENDREG_BLENDCOLOR1]) {
      i915->state.Blend[I915_BLENDREG_BLENDCOLOR1] = dw;
      I915_STATECHANGE(i915, I915_UPLOAD_BLEND);
   }
}


#define DST_BLND_FACT(f) ((f)<<S6_CBUF_DST_BLEND_FACT_SHIFT)
#define SRC_BLND_FACT(f) ((f)<<S6_CBUF_SRC_BLEND_FACT_SHIFT)
#define DST_ABLND_FACT(f) ((f)<<IAB_DST_FACTOR_SHIFT)
#define SRC_ABLND_FACT(f) ((f)<<IAB_SRC_FACTOR_SHIFT)



static GLuint
translate_blend_equation(GLenum mode)
{
   switch (mode) {
   case GL_FUNC_ADD:
      return BLENDFUNC_ADD;
   case GL_MIN:
      return BLENDFUNC_MIN;
   case GL_MAX:
      return BLENDFUNC_MAX;
   case GL_FUNC_SUBTRACT:
      return BLENDFUNC_SUBTRACT;
   case GL_FUNC_REVERSE_SUBTRACT:
      return BLENDFUNC_REVERSE_SUBTRACT;
   default:
      return 0;
   }
}

static void
i915UpdateBlendState(struct gl_context * ctx)
{
   struct i915_context *i915 = I915_CONTEXT(ctx);
   GLuint iab = (i915->state.Blend[I915_BLENDREG_IAB] &
                 ~(IAB_SRC_FACTOR_MASK |
                   IAB_DST_FACTOR_MASK |
                   (BLENDFUNC_MASK << IAB_FUNC_SHIFT) | IAB_ENABLE));

   GLuint lis6 = (i915->state.Ctx[I915_CTXREG_LIS6] &
                  ~(S6_CBUF_SRC_BLEND_FACT_MASK |
                    S6_CBUF_DST_BLEND_FACT_MASK | S6_CBUF_BLEND_FUNC_MASK));

   GLuint eqRGB = ctx->Color.Blend[0].EquationRGB;
   GLuint eqA = ctx->Color.Blend[0].EquationA;
   GLuint srcRGB = ctx->Color.Blend[0].SrcRGB;
   GLuint dstRGB = ctx->Color.Blend[0].DstRGB;
   GLuint srcA = ctx->Color.Blend[0].SrcA;
   GLuint dstA = ctx->Color.Blend[0].DstA;

   if (eqRGB == GL_MIN || eqRGB == GL_MAX) {
      srcRGB = dstRGB = GL_ONE;
   }

   if (eqA == GL_MIN || eqA == GL_MAX) {
      srcA = dstA = GL_ONE;
   }

   lis6 |= SRC_BLND_FACT(intel_translate_blend_factor(srcRGB));
   lis6 |= DST_BLND_FACT(intel_translate_blend_factor(dstRGB));
   lis6 |= translate_blend_equation(eqRGB) << S6_CBUF_BLEND_FUNC_SHIFT;

   iab |= SRC_ABLND_FACT(intel_translate_blend_factor(srcA));
   iab |= DST_ABLND_FACT(intel_translate_blend_factor(dstA));
   iab |= translate_blend_equation(eqA) << IAB_FUNC_SHIFT;

   if (srcA != srcRGB || dstA != dstRGB || eqA != eqRGB)
      iab |= IAB_ENABLE;

   if (iab != i915->state.Blend[I915_BLENDREG_IAB]) {
      i915->state.Blend[I915_BLENDREG_IAB] = iab;
      I915_STATECHANGE(i915, I915_UPLOAD_BLEND);
   }
   if (lis6 != i915->state.Ctx[I915_CTXREG_LIS6]) {
      i915->state.Ctx[I915_CTXREG_LIS6] = lis6;
      I915_STATECHANGE(i915, I915_UPLOAD_CTX);
   }

   /* This will catch a logicop blend equation */
   i915EvalLogicOpBlendState(ctx);
}


static void
i915BlendFuncSeparate(struct gl_context * ctx, GLenum srcRGB,
                      GLenum dstRGB, GLenum srcA, GLenum dstA)
{
   i915UpdateBlendState(ctx);
}


static void
i915BlendEquationSeparate(struct gl_context * ctx, GLenum eqRGB, GLenum eqA)
{
   i915UpdateBlendState(ctx);
}


static void
i915DepthFunc(struct gl_context * ctx, GLenum func)
{
   struct i915_context *i915 = I915_CONTEXT(ctx);
   int test = intel_translate_compare_func(func);
   GLuint dw;

   DBG("%s\n", __FUNCTION__);
   
   dw = i915->state.Ctx[I915_CTXREG_LIS6];
   dw &= ~S6_DEPTH_TEST_FUNC_MASK;
   dw |= test << S6_DEPTH_TEST_FUNC_SHIFT;
   if (dw != i915->state.Ctx[I915_CTXREG_LIS6]) {
      I915_STATECHANGE(i915, I915_UPLOAD_CTX);
      i915->state.Ctx[I915_CTXREG_LIS6] = dw;
   }
}

static void
i915DepthMask(struct gl_context * ctx, GLboolean flag)
{
   struct i915_context *i915 = I915_CONTEXT(ctx);
   GLuint dw;

   DBG("%s flag (%d)\n", __FUNCTION__, flag);

   if (!ctx->DrawBuffer || !ctx->DrawBuffer->Visual.depthBits)
      flag = false;

   dw = i915->state.Ctx[I915_CTXREG_LIS6];
   if (flag && ctx->Depth.Test)
      dw |= S6_DEPTH_WRITE_ENABLE;
   else
      dw &= ~S6_DEPTH_WRITE_ENABLE;
   if (dw != i915->state.Ctx[I915_CTXREG_LIS6]) {
      I915_STATECHANGE(i915, I915_UPLOAD_CTX);
      i915->state.Ctx[I915_CTXREG_LIS6] = dw;
   }
}



/**
 * Update the viewport transformation matrix.  Depends on:
 *  - viewport pos/size
 *  - depthrange
 *  - window pos/size or FBO size
 */
void
intelCalcViewport(struct gl_context * ctx)
{
   struct intel_context *intel = intel_context(ctx);

   if (_mesa_is_winsys_fbo(ctx->DrawBuffer)) {
      _math_matrix_viewport(&intel->ViewportMatrix,
			    ctx->Viewport.X,
			    ctx->DrawBuffer->Height - ctx->Viewport.Y,
			    ctx->Viewport.Width,
			    -ctx->Viewport.Height,
			    ctx->Viewport.Near,
			    ctx->Viewport.Far,
			    1.0);
   } else {
      _math_matrix_viewport(&intel->ViewportMatrix,
			    ctx->Viewport.X,
			    ctx->Viewport.Y,
			    ctx->Viewport.Width,
			    ctx->Viewport.Height,
			    ctx->Viewport.Near,
			    ctx->Viewport.Far,
			    1.0);
   }
}


/** Called from ctx->Driver.Viewport() */
static void
i915Viewport(struct gl_context * ctx,
              GLint x, GLint y, GLsizei width, GLsizei height)
{
   intelCalcViewport(ctx);
}


/** Called from ctx->Driver.DepthRange() */
static void
i915DepthRange(struct gl_context * ctx, GLclampd nearval, GLclampd farval)
{
   intelCalcViewport(ctx);
}


/* =============================================================
 * Polygon stipple
 *
 * The i915 supports a 4x4 stipple natively, GL wants 32x32.
 * Fortunately stipple is usually a repeating pattern.
 */
static void
i915PolygonStipple(struct gl_context * ctx, const GLubyte * mask)
{
   struct i915_context *i915 = I915_CONTEXT(ctx);
   const GLubyte *m;
   GLubyte p[4];
   int i, j, k;
   int active = (ctx->Polygon.StippleFlag &&
                 i915->intel.reduced_primitive == GL_TRIANGLES);
   GLuint newMask;

   if (active) {
      I915_STATECHANGE(i915, I915_UPLOAD_STIPPLE);
      i915->state.Stipple[I915_STPREG_ST1] &= ~ST1_ENABLE;
   }

   /* Use the already unpacked stipple data from the context rather than the
    * uninterpreted mask passed in.
    */
   mask = (const GLubyte *)ctx->PolygonStipple;
   m = mask;

   p[0] = mask[12] & 0xf;
   p[0] |= p[0] << 4;
   p[1] = mask[8] & 0xf;
   p[1] |= p[1] << 4;
   p[2] = mask[4] & 0xf;
   p[2] |= p[2] << 4;
   p[3] = mask[0] & 0xf;
   p[3] |= p[3] << 4;

   for (k = 0; k < 8; k++)
      for (j = 3; j >= 0; j--)
         for (i = 0; i < 4; i++, m++)
            if (*m != p[j]) {
               i915->intel.hw_stipple = 0;
               return;
            }

   newMask = (((p[0] & 0xf) << 0) |
              ((p[1] & 0xf) << 4) |
              ((p[2] & 0xf) << 8) | ((p[3] & 0xf) << 12));


   if (newMask == 0xffff || newMask == 0x0) {
      /* this is needed to make conform pass */
      i915->intel.hw_stipple = 0;
      return;
   }

   i915->state.Stipple[I915_STPREG_ST1] &= ~0xffff;
   i915->state.Stipple[I915_STPREG_ST1] |= newMask;
   i915->intel.hw_stipple = 1;

   if (active)
      i915->state.Stipple[I915_STPREG_ST1] |= ST1_ENABLE;
}


/* =============================================================
 * Hardware clipping
 */
static void
i915Scissor(struct gl_context * ctx, GLint x, GLint y, GLsizei w, GLsizei h)
{
   struct i915_context *i915 = I915_CONTEXT(ctx);
   int x1, y1, x2, y2;

   if (!ctx->DrawBuffer)
      return;

   DBG("%s %d,%d %dx%d\n", __FUNCTION__, x, y, w, h);

   if (_mesa_is_winsys_fbo(ctx->DrawBuffer)) {
      x1 = x;
      y1 = ctx->DrawBuffer->Height - (y + h);
      x2 = x + w - 1;
      y2 = y1 + h - 1;
      DBG("%s %d..%d,%d..%d (inverted)\n", __FUNCTION__, x1, x2, y1, y2);
   }
   else {
      /* FBO - not inverted
       */
      x1 = x;
      y1 = y;
      x2 = x + w - 1;
      y2 = y + h - 1;
      DBG("%s %d..%d,%d..%d (not inverted)\n", __FUNCTION__, x1, x2, y1, y2);
   }
   
   x1 = CLAMP(x1, 0, ctx->DrawBuffer->Width - 1);
   y1 = CLAMP(y1, 0, ctx->DrawBuffer->Height - 1);
   x2 = CLAMP(x2, 0, ctx->DrawBuffer->Width - 1);
   y2 = CLAMP(y2, 0, ctx->DrawBuffer->Height - 1);
   
   DBG("%s %d..%d,%d..%d (clamped)\n", __FUNCTION__, x1, x2, y1, y2);

   I915_STATECHANGE(i915, I915_UPLOAD_BUFFERS);
   i915->state.Buffer[I915_DESTREG_SR1] = (y1 << 16) | (x1 & 0xffff);
   i915->state.Buffer[I915_DESTREG_SR2] = (y2 << 16) | (x2 & 0xffff);
}

static void
i915LogicOp(struct gl_context * ctx, GLenum opcode)
{
   struct i915_context *i915 = I915_CONTEXT(ctx);
   int tmp = intel_translate_logic_op(opcode);

   DBG("%s\n", __FUNCTION__);
   
   I915_STATECHANGE(i915, I915_UPLOAD_CTX);
   i915->state.Ctx[I915_CTXREG_STATE4] &= ~LOGICOP_MASK;
   i915->state.Ctx[I915_CTXREG_STATE4] |= LOGIC_OP_FUNC(tmp);
}



static void
i915CullFaceFrontFace(struct gl_context * ctx, GLenum unused)
{
   struct i915_context *i915 = I915_CONTEXT(ctx);
   GLuint mode, dw;

   DBG("%s %d\n", __FUNCTION__,
       ctx->DrawBuffer ? ctx->DrawBuffer->Name : 0);

   if (!ctx->Polygon.CullFlag) {
      mode = S4_CULLMODE_NONE;
   }
   else if (ctx->Polygon.CullFaceMode != GL_FRONT_AND_BACK) {
      mode = S4_CULLMODE_CW;

      if (ctx->DrawBuffer && _mesa_is_user_fbo(ctx->DrawBuffer))
         mode ^= (S4_CULLMODE_CW ^ S4_CULLMODE_CCW);
      if (ctx->Polygon.CullFaceMode == GL_FRONT)
         mode ^= (S4_CULLMODE_CW ^ S4_CULLMODE_CCW);
      if (ctx->Polygon.FrontFace != GL_CCW)
         mode ^= (S4_CULLMODE_CW ^ S4_CULLMODE_CCW);
   }
   else {
      mode = S4_CULLMODE_BOTH;
   }

   dw = i915->state.Ctx[I915_CTXREG_LIS4];
   dw &= ~S4_CULLMODE_MASK;
   dw |= mode;
   if (dw != i915->state.Ctx[I915_CTXREG_LIS4]) {
      i915->state.Ctx[I915_CTXREG_LIS4] = dw;
      I915_STATECHANGE(i915, I915_UPLOAD_CTX);
   }
}

static void
i915LineWidth(struct gl_context * ctx, GLfloat widthf)
{
   struct i915_context *i915 = I915_CONTEXT(ctx);
   int lis4 = i915->state.Ctx[I915_CTXREG_LIS4] & ~S4_LINE_WIDTH_MASK;
   int width;

   DBG("%s\n", __FUNCTION__);
   
   width = (int) (widthf * 2);
   width = CLAMP(width, 1, 0xf);
   lis4 |= width << S4_LINE_WIDTH_SHIFT;

   if (lis4 != i915->state.Ctx[I915_CTXREG_LIS4]) {
      I915_STATECHANGE(i915, I915_UPLOAD_CTX);
      i915->state.Ctx[I915_CTXREG_LIS4] = lis4;
   }
}

static void
i915PointSize(struct gl_context * ctx, GLfloat size)
{
   struct i915_context *i915 = I915_CONTEXT(ctx);
   int lis4 = i915->state.Ctx[I915_CTXREG_LIS4] & ~S4_POINT_WIDTH_MASK;
   GLint point_size = (int) round(size);

   DBG("%s\n", __FUNCTION__);
   
   point_size = CLAMP(point_size, 1, 255);
   lis4 |= point_size << S4_POINT_WIDTH_SHIFT;

   if (lis4 != i915->state.Ctx[I915_CTXREG_LIS4]) {
      I915_STATECHANGE(i915, I915_UPLOAD_CTX);
      i915->state.Ctx[I915_CTXREG_LIS4] = lis4;
   }
}


static void
i915PointParameterfv(struct gl_context * ctx, GLenum pname, const GLfloat *params)
{
   struct i915_context *i915 = I915_CONTEXT(ctx);

   switch (pname) {
   case GL_POINT_SPRITE_COORD_ORIGIN:
      /* This could be supported, but it would require modifying the fragment
       * program to invert the y component of the texture coordinate by
       * inserting a 'SUB tc.y, {1.0}.xxxx, tc' instruction.
       */
      FALLBACK(&i915->intel, I915_FALLBACK_POINT_SPRITE_COORD_ORIGIN,
	       (params[0] != GL_UPPER_LEFT));
      break;
   }
}

void
i915_update_sprite_point_enable(struct gl_context *ctx)
{
   struct intel_context *intel = intel_context(ctx);
   /* _NEW_PROGRAM */
   struct i915_fragment_program *p =
      (struct i915_fragment_program *) ctx->FragmentProgram._Current;
   const GLbitfield64 inputsRead = p->FragProg.Base.InputsRead;
   struct i915_context *i915 = i915_context(ctx);
   GLuint s4 = i915->state.Ctx[I915_CTXREG_LIS4] & ~S4_VFMT_MASK;
   int i;
   GLuint coord_replace_bits = 0x0;
   GLuint tex_coord_unit_bits = 0x0;

   for (i = 0; i < ctx->Const.MaxTextureCoordUnits; i++) {
      /* _NEW_POINT */
      if (ctx->Point.CoordReplace[i] && ctx->Point.PointSprite)
         coord_replace_bits |= (1 << i);
      if (inputsRead & FRAG_BIT_TEX(i))
         tex_coord_unit_bits |= (1 << i);
   }

   /*
    * Here we can't enable the SPRITE_POINT_ENABLE bit when the mis-match
    * of tex_coord_unit_bits and coord_replace_bits, or this will make all
    * the other non-point-sprite coords(like varying inputs, as we now use
    * tex coord to implement varying inputs) be replaced to value (0, 0)-(1, 1).
    *
    * Thus, do fallback when needed.
    */
   FALLBACK(intel, I915_FALLBACK_COORD_REPLACE,
            coord_replace_bits && coord_replace_bits != tex_coord_unit_bits);

   s4 &= ~S4_SPRITE_POINT_ENABLE;
   s4 |= (coord_replace_bits && coord_replace_bits == tex_coord_unit_bits) ?
         S4_SPRITE_POINT_ENABLE : 0;
   if (s4 != i915->state.Ctx[I915_CTXREG_LIS4]) {
      i915->state.Ctx[I915_CTXREG_LIS4] = s4;
      I915_STATECHANGE(i915, I915_UPLOAD_CTX);
   }
}


/* =============================================================
 * Color masks
 */

static void
i915ColorMask(struct gl_context * ctx,
              GLboolean r, GLboolean g, GLboolean b, GLboolean a)
{
   struct i915_context *i915 = I915_CONTEXT(ctx);
   GLuint tmp = i915->state.Ctx[I915_CTXREG_LIS5] & ~S5_WRITEDISABLE_MASK;

   DBG("%s r(%d) g(%d) b(%d) a(%d)\n", __FUNCTION__, r, g, b,
       a);

   if (!r)
      tmp |= S5_WRITEDISABLE_RED;
   if (!g)
      tmp |= S5_WRITEDISABLE_GREEN;
   if (!b)
      tmp |= S5_WRITEDISABLE_BLUE;
   if (!a)
      tmp |= S5_WRITEDISABLE_ALPHA;

   if (tmp != i915->state.Ctx[I915_CTXREG_LIS5]) {
      I915_STATECHANGE(i915, I915_UPLOAD_CTX);
      i915->state.Ctx[I915_CTXREG_LIS5] = tmp;
   }
}

static void
update_specular(struct gl_context * ctx)
{
   /* A hack to trigger the rebuild of the fragment program.
    */
   intel_context(ctx)->NewGLState |= _NEW_TEXTURE;
}

static void
i915LightModelfv(struct gl_context * ctx, GLenum pname, const GLfloat * param)
{
   DBG("%s\n", __FUNCTION__);
   
   if (pname == GL_LIGHT_MODEL_COLOR_CONTROL) {
      update_specular(ctx);
   }
}

static void
i915ShadeModel(struct gl_context * ctx, GLenum mode)
{
   struct i915_context *i915 = I915_CONTEXT(ctx);
   I915_STATECHANGE(i915, I915_UPLOAD_CTX);

   if (mode == GL_SMOOTH) {
      i915->state.Ctx[I915_CTXREG_LIS4] &= ~(S4_FLATSHADE_ALPHA |
                                             S4_FLATSHADE_COLOR |
                                             S4_FLATSHADE_SPECULAR);
   }
   else {
      i915->state.Ctx[I915_CTXREG_LIS4] |= (S4_FLATSHADE_ALPHA |
                                            S4_FLATSHADE_COLOR |
                                            S4_FLATSHADE_SPECULAR);
   }
}

/* =============================================================
 * Fog
 *
 * This empty function remains because _mesa_init_driver_state calls
 * dd_function_table::Fogfv unconditionally.  We have to have some function
 * there so that it doesn't try to call a NULL pointer.
 */
static void
i915Fogfv(struct gl_context * ctx, GLenum pname, const GLfloat * param)
{
   (void) ctx;
   (void) pname;
   (void) param;
}

/* =============================================================
 */

static void
i915Enable(struct gl_context * ctx, GLenum cap, GLboolean state)
{
   struct i915_context *i915 = I915_CONTEXT(ctx);
   GLuint dw;

   switch (cap) {
   case GL_TEXTURE_2D:
      break;

   case GL_LIGHTING:
   case GL_COLOR_SUM:
      update_specular(ctx);
      break;

   case GL_ALPHA_TEST:
      dw = i915->state.Ctx[I915_CTXREG_LIS6];
      if (state)
         dw |= S6_ALPHA_TEST_ENABLE;
      else
         dw &= ~S6_ALPHA_TEST_ENABLE;
      if (dw != i915->state.Ctx[I915_CTXREG_LIS6]) {
	 i915->state.Ctx[I915_CTXREG_LIS6] = dw;
	 I915_STATECHANGE(i915, I915_UPLOAD_CTX);
      }
      break;

   case GL_BLEND:
      i915EvalLogicOpBlendState(ctx);
      break;

   case GL_COLOR_LOGIC_OP:
      i915EvalLogicOpBlendState(ctx);

      /* Logicop doesn't seem to work at 16bpp:
       */
      if (ctx->Visual.rgbBits == 16)
         FALLBACK(&i915->intel, I915_FALLBACK_LOGICOP, state);
      break;

   case GL_FRAGMENT_PROGRAM_ARB:
      break;

   case GL_DITHER:
      dw = i915->state.Ctx[I915_CTXREG_LIS5];
      if (state)
         dw |= S5_COLOR_DITHER_ENABLE;
      else
         dw &= ~S5_COLOR_DITHER_ENABLE;
      if (dw != i915->state.Ctx[I915_CTXREG_LIS5]) {
	 i915->state.Ctx[I915_CTXREG_LIS5] = dw;
	 I915_STATECHANGE(i915, I915_UPLOAD_CTX);
      }
      break;

   case GL_DEPTH_TEST:
      dw = i915->state.Ctx[I915_CTXREG_LIS6];

      if (!ctx->DrawBuffer || !ctx->DrawBuffer->Visual.depthBits)
	 state = false;

      if (state)
         dw |= S6_DEPTH_TEST_ENABLE;
      else
         dw &= ~S6_DEPTH_TEST_ENABLE;
      if (dw != i915->state.Ctx[I915_CTXREG_LIS6]) {
	 i915->state.Ctx[I915_CTXREG_LIS6] = dw;
	 I915_STATECHANGE(i915, I915_UPLOAD_CTX);
      }

      i915DepthMask(ctx, ctx->Depth.Mask);
      break;

   case GL_SCISSOR_TEST:
      I915_STATECHANGE(i915, I915_UPLOAD_BUFFERS);
      if (state)
         i915->state.Buffer[I915_DESTREG_SENABLE] =
            (_3DSTATE_SCISSOR_ENABLE_CMD | ENABLE_SCISSOR_RECT);
      else
         i915->state.Buffer[I915_DESTREG_SENABLE] =
            (_3DSTATE_SCISSOR_ENABLE_CMD | DISABLE_SCISSOR_RECT);
      break;

   case GL_LINE_SMOOTH:
      dw = i915->state.Ctx[I915_CTXREG_LIS4];
      if (state)
         dw |= S4_LINE_ANTIALIAS_ENABLE;
      else
         dw &= ~S4_LINE_ANTIALIAS_ENABLE;
      if (dw != i915->state.Ctx[I915_CTXREG_LIS4]) {
	 i915->state.Ctx[I915_CTXREG_LIS4] = dw;
	 I915_STATECHANGE(i915, I915_UPLOAD_CTX);
      }
      break;

   case GL_CULL_FACE:
      i915CullFaceFrontFace(ctx, 0);
      break;

   case GL_STENCIL_TEST:
      if (!ctx->DrawBuffer || !ctx->DrawBuffer->Visual.stencilBits)
	 state = false;

      dw = i915->state.Ctx[I915_CTXREG_LIS5];
      if (state)
	 dw |= (S5_STENCIL_TEST_ENABLE | S5_STENCIL_WRITE_ENABLE);
      else
	 dw &= ~(S5_STENCIL_TEST_ENABLE | S5_STENCIL_WRITE_ENABLE);
      if (dw != i915->state.Ctx[I915_CTXREG_LIS5]) {
	 i915->state.Ctx[I915_CTXREG_LIS5] = dw;
	 I915_STATECHANGE(i915, I915_UPLOAD_CTX);
      }
      break;

   case GL_POLYGON_STIPPLE:
      /* The stipple command worked on my 855GM box, but not my 845G.
       * I'll do more testing later to find out exactly which hardware
       * supports it.  Disabled for now.
       */
      if (i915->intel.hw_stipple &&
          i915->intel.reduced_primitive == GL_TRIANGLES) {
         I915_STATECHANGE(i915, I915_UPLOAD_STIPPLE);
         if (state)
            i915->state.Stipple[I915_STPREG_ST1] |= ST1_ENABLE;
         else
            i915->state.Stipple[I915_STPREG_ST1] &= ~ST1_ENABLE;
      }
      break;

   case GL_POLYGON_SMOOTH:
      break;

   case GL_POINT_SPRITE:
      /* Handle it at i915_update_sprite_point_enable () */
      break;

   case GL_POINT_SMOOTH:
      break;

   default:
      ;
   }
}


static void
i915_init_packets(struct i915_context *i915)
{
   /* Zero all state */
   memset(&i915->state, 0, sizeof(i915->state));


   {
      I915_STATECHANGE(i915, I915_UPLOAD_CTX);
      I915_STATECHANGE(i915, I915_UPLOAD_BLEND);
      /* Probably don't want to upload all this stuff every time one 
       * piece changes.
       */
      i915->state.Ctx[I915_CTXREG_LI] = (_3DSTATE_LOAD_STATE_IMMEDIATE_1 |
                                         I1_LOAD_S(2) |
                                         I1_LOAD_S(4) |
                                         I1_LOAD_S(5) | I1_LOAD_S(6) | (3));
      i915->state.Ctx[I915_CTXREG_LIS2] = 0;
      i915->state.Ctx[I915_CTXREG_LIS4] = 0;
      i915->state.Ctx[I915_CTXREG_LIS5] = 0;

      if (i915->intel.ctx.Visual.rgbBits == 16)
         i915->state.Ctx[I915_CTXREG_LIS5] |= S5_COLOR_DITHER_ENABLE;


      i915->state.Ctx[I915_CTXREG_LIS6] = (S6_COLOR_WRITE_ENABLE |
                                           (2 << S6_TRISTRIP_PV_SHIFT));

      i915->state.Ctx[I915_CTXREG_STATE4] = (_3DSTATE_MODES_4_CMD |
                                             ENABLE_LOGIC_OP_FUNC |
                                             LOGIC_OP_FUNC(LOGICOP_COPY) |
                                             ENABLE_STENCIL_TEST_MASK |
                                             STENCIL_TEST_MASK(0xff) |
                                             ENABLE_STENCIL_WRITE_MASK |
                                             STENCIL_WRITE_MASK(0xff));

      i915->state.Blend[I915_BLENDREG_IAB] =
         (_3DSTATE_INDEPENDENT_ALPHA_BLEND_CMD | IAB_MODIFY_ENABLE |
          IAB_MODIFY_FUNC | IAB_MODIFY_SRC_FACTOR | IAB_MODIFY_DST_FACTOR);

      i915->state.Blend[I915_BLENDREG_BLENDCOLOR0] =
         _3DSTATE_CONST_BLEND_COLOR_CMD;
      i915->state.Blend[I915_BLENDREG_BLENDCOLOR1] = 0;

      i915->state.Ctx[I915_CTXREG_BF_STENCIL_MASKS] =
	 _3DSTATE_BACKFACE_STENCIL_MASKS |
	 BFM_ENABLE_STENCIL_TEST_MASK |
	 BFM_ENABLE_STENCIL_WRITE_MASK |
	 (0xff << BFM_STENCIL_WRITE_MASK_SHIFT) |
	 (0xff << BFM_STENCIL_TEST_MASK_SHIFT);
      i915->state.Ctx[I915_CTXREG_BF_STENCIL_OPS] =
	 _3DSTATE_BACKFACE_STENCIL_OPS |
	 BFO_ENABLE_STENCIL_REF |
	 BFO_ENABLE_STENCIL_FUNCS |
	 BFO_ENABLE_STENCIL_TWO_SIDE;
   }

   {
      I915_STATECHANGE(i915, I915_UPLOAD_STIPPLE);
      i915->state.Stipple[I915_STPREG_ST0] = _3DSTATE_STIPPLE;
   }

   {
      i915->state.Buffer[I915_DESTREG_DV0] = _3DSTATE_DST_BUF_VARS_CMD;

      /* scissor */
      i915->state.Buffer[I915_DESTREG_SENABLE] =
         (_3DSTATE_SCISSOR_ENABLE_CMD | DISABLE_SCISSOR_RECT);
      i915->state.Buffer[I915_DESTREG_SR0] = _3DSTATE_SCISSOR_RECT_0_CMD;
      i915->state.Buffer[I915_DESTREG_SR1] = 0;
      i915->state.Buffer[I915_DESTREG_SR2] = 0;
   }

   i915->state.RasterRules[I915_RASTER_RULES] = _3DSTATE_RASTER_RULES_CMD |
      ENABLE_POINT_RASTER_RULE |
      OGL_POINT_RASTER_RULE |
      ENABLE_LINE_STRIP_PROVOKE_VRTX |
      ENABLE_TRI_FAN_PROVOKE_VRTX |
      LINE_STRIP_PROVOKE_VRTX(1) |
      TRI_FAN_PROVOKE_VRTX(2) | ENABLE_TEXKILL_3D_4D | TEXKILL_4D;

#if 0
   {
      I915_STATECHANGE(i915, I915_UPLOAD_DEFAULTS);
      i915->state.Default[I915_DEFREG_C0] = _3DSTATE_DEFAULT_DIFFUSE;
      i915->state.Default[I915_DEFREG_C1] = 0;
      i915->state.Default[I915_DEFREG_S0] = _3DSTATE_DEFAULT_SPECULAR;
      i915->state.Default[I915_DEFREG_S1] = 0;
      i915->state.Default[I915_DEFREG_Z0] = _3DSTATE_DEFAULT_Z;
      i915->state.Default[I915_DEFREG_Z1] = 0;
   }
#endif


   /* These will be emitted every at the head of every buffer, unless
    * we get hardware contexts working.
    */
   i915->state.active = (I915_UPLOAD_PROGRAM |
                         I915_UPLOAD_STIPPLE |
                         I915_UPLOAD_CTX |
                         I915_UPLOAD_BLEND |
                         I915_UPLOAD_BUFFERS |
			 I915_UPLOAD_INVARIENT |
			 I915_UPLOAD_RASTER_RULES);
}

void
i915_update_provoking_vertex(struct gl_context * ctx)
{
   struct i915_context *i915 = I915_CONTEXT(ctx);

   I915_STATECHANGE(i915, I915_UPLOAD_CTX);
   i915->state.Ctx[I915_CTXREG_LIS6] &= ~(S6_TRISTRIP_PV_MASK);

   I915_STATECHANGE(i915, I915_UPLOAD_RASTER_RULES);
   i915->state.RasterRules[I915_RASTER_RULES] &= ~(LINE_STRIP_PROVOKE_VRTX_MASK |
						   TRI_FAN_PROVOKE_VRTX_MASK);

   /* _NEW_LIGHT */
   if (ctx->Light.ProvokingVertex == GL_LAST_VERTEX_CONVENTION) {
      i915->state.RasterRules[I915_RASTER_RULES] |= (LINE_STRIP_PROVOKE_VRTX(1) |
						     TRI_FAN_PROVOKE_VRTX(2));
      i915->state.Ctx[I915_CTXREG_LIS6] |= (2 << S6_TRISTRIP_PV_SHIFT);
   } else {
      i915->state.RasterRules[I915_RASTER_RULES] |= (LINE_STRIP_PROVOKE_VRTX(0) |
						     TRI_FAN_PROVOKE_VRTX(1));
      i915->state.Ctx[I915_CTXREG_LIS6] |= (0 << S6_TRISTRIP_PV_SHIFT);
    }
}

/* Fallback to swrast for select and feedback.
 */
static void
i915RenderMode(struct gl_context *ctx, GLenum mode)
{
   struct intel_context *intel = intel_context(ctx);
   FALLBACK(intel, INTEL_FALLBACK_RENDERMODE, (mode != GL_RENDER));
}

void
i915InitStateFunctions(struct dd_function_table *functions)
{
   functions->AlphaFunc = i915AlphaFunc;
   functions->BlendColor = i915BlendColor;
   functions->BlendEquationSeparate = i915BlendEquationSeparate;
   functions->BlendFuncSeparate = i915BlendFuncSeparate;
   functions->ColorMask = i915ColorMask;
   functions->CullFace = i915CullFaceFrontFace;
   functions->DepthFunc = i915DepthFunc;
   functions->DepthMask = i915DepthMask;
   functions->Enable = i915Enable;
   functions->Fogfv = i915Fogfv;
   functions->FrontFace = i915CullFaceFrontFace;
   functions->LightModelfv = i915LightModelfv;
   functions->LineWidth = i915LineWidth;
   functions->LogicOpcode = i915LogicOp;
   functions->PointSize = i915PointSize;
   functions->PointParameterfv = i915PointParameterfv;
   functions->PolygonStipple = i915PolygonStipple;
   functions->RenderMode = i915RenderMode;
   functions->Scissor = i915Scissor;
   functions->ShadeModel = i915ShadeModel;
   functions->StencilFuncSeparate = i915StencilFuncSeparate;
   functions->StencilMaskSeparate = i915StencilMaskSeparate;
   functions->StencilOpSeparate = i915StencilOpSeparate;
   functions->DepthRange = i915DepthRange;
   functions->Viewport = i915Viewport;
}


void
i915InitState(struct i915_context *i915)
{
   struct gl_context *ctx = &i915->intel.ctx;

   i915_init_packets(i915);

   _mesa_init_driver_state(ctx);
}
