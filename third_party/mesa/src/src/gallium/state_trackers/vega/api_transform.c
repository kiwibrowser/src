/**************************************************************************
 *
 * Copyright 2009 VMware, Inc.  All Rights Reserved.
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
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#include "VG/openvg.h"

#include "vg_context.h"

#include "matrix.h"
#include "api.h"

void vegaLoadIdentity(void)
{
   struct vg_context *ctx = vg_current_context();
   struct  matrix *mat = vg_state_matrix(&ctx->state.vg);
   matrix_load_identity(mat);
}

void vegaLoadMatrix(const VGfloat * m)
{
   struct vg_context *ctx = vg_current_context();
   struct  matrix *mat;

   if (!ctx)
      return;

   if (!m || !is_aligned(m)) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }

   mat = vg_state_matrix(&ctx->state.vg);
   matrix_init(mat, m);
   if (!matrix_is_affine(mat)) {
      if (ctx->state.vg.matrix_mode != VG_MATRIX_IMAGE_USER_TO_SURFACE) {
         matrix_make_affine(mat);
      }
   }
}

void vegaGetMatrix(VGfloat * m)
{
   struct vg_context *ctx = vg_current_context();
   struct matrix *mat;

   if (!ctx)
      return;

   if (!m || !is_aligned(m)) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }

   mat = vg_state_matrix(&ctx->state.vg);
   memcpy(m, mat->m, sizeof(VGfloat)*9);
}

void vegaMultMatrix(const VGfloat * m)
{
   struct vg_context *ctx = vg_current_context();
   struct matrix *dst, src;

   if (!ctx)
      return;

   if (!m || !is_aligned(m)) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }
   matrix_init(&src, m);
   dst = vg_state_matrix(&ctx->state.vg);
   if (!matrix_is_affine(&src)) {
      if (ctx->state.vg.matrix_mode != VG_MATRIX_IMAGE_USER_TO_SURFACE) {
         matrix_make_affine(&src);
      }
   }
   matrix_mult(dst, &src);

}

void vegaTranslate(VGfloat tx, VGfloat ty)
{
   struct vg_context *ctx = vg_current_context();
   struct matrix *dst = vg_state_matrix(&ctx->state.vg);
   matrix_translate(dst, tx, ty);
}

void vegaScale(VGfloat sx, VGfloat sy)
{
   struct vg_context *ctx = vg_current_context();
   struct matrix *dst = vg_state_matrix(&ctx->state.vg);
   matrix_scale(dst, sx, sy);
}

void vegaShear(VGfloat shx, VGfloat shy)
{
   struct vg_context *ctx = vg_current_context();
   struct matrix *dst = vg_state_matrix(&ctx->state.vg);
   matrix_shear(dst, shx, shy);
}

void vegaRotate(VGfloat angle)
{
   struct vg_context *ctx = vg_current_context();
   struct matrix *dst = vg_state_matrix(&ctx->state.vg);
   matrix_rotate(dst, angle);
}
