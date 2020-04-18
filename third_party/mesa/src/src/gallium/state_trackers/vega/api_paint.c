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
#include "paint.h"
#include "api.h"
#include "handle.h"


VGPaint vegaCreatePaint(void)
{
   return paint_to_handle(paint_create(vg_current_context()));
}

void vegaDestroyPaint(VGPaint p)
{
   struct vg_context *ctx = vg_current_context();

   if (p == VG_INVALID_HANDLE) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }

   paint_destroy(handle_to_paint(p));
}

void vegaSetPaint(VGPaint paint, VGbitfield paintModes)
{
   struct vg_context *ctx = vg_current_context();

   if (paint == VG_INVALID_HANDLE) {
      /* restore the default */
      paint = paint_to_handle(ctx->default_paint);
   } else if (!vg_object_is_valid(paint, VG_OBJECT_PAINT)) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }

   if (!(paintModes & ((VG_FILL_PATH|VG_STROKE_PATH)))) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }

   if (paintModes & VG_FILL_PATH) {
      ctx->state.vg.fill_paint = handle_to_paint(paint);
   }
   if (paintModes & VG_STROKE_PATH) {
      ctx->state.vg.stroke_paint = handle_to_paint(paint);
   }

   ctx->state.dirty |= PAINT_DIRTY;
}

VGPaint vegaGetPaint(VGPaintMode paintMode)
{
   struct vg_context *ctx = vg_current_context();
   VGPaint paint = VG_INVALID_HANDLE;

   if (paintMode < VG_STROKE_PATH || paintMode > VG_FILL_PATH) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return VG_INVALID_HANDLE;
   }

   if (paintMode == VG_FILL_PATH)
      paint = paint_to_handle(ctx->state.vg.fill_paint);
   else if (paintMode == VG_STROKE_PATH)
      paint = paint_to_handle(ctx->state.vg.stroke_paint);

   if (paint == paint_to_handle(ctx->default_paint))
      paint = VG_INVALID_HANDLE;

   return paint;
}

void vegaSetColor(VGPaint paint, VGuint rgba)
{
   struct vg_context *ctx = vg_current_context();
   struct vg_paint *p;

   if (paint == VG_INVALID_HANDLE) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }

   if (!vg_object_is_valid(paint, VG_OBJECT_PAINT)) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }

   p = handle_to_paint(paint);
   paint_set_colori(p, rgba);

   if (ctx->state.vg.fill_paint == p ||
       ctx->state.vg.stroke_paint == p)
      ctx->state.dirty |= PAINT_DIRTY;
}

VGuint vegaGetColor(VGPaint paint)
{
   struct vg_context *ctx = vg_current_context();
   struct vg_paint *p;
   VGuint rgba = 0;

   if (paint == VG_INVALID_HANDLE) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return rgba;
   }

   if (!vg_object_is_valid(paint, VG_OBJECT_PAINT)) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return rgba;
   }
   p = handle_to_paint(paint);

   return paint_colori(p);
}

void vegaPaintPattern(VGPaint paint, VGImage pattern)
{
   struct vg_context *ctx = vg_current_context();

   if (paint == VG_INVALID_HANDLE ||
       !vg_context_is_object_valid(ctx, VG_OBJECT_PAINT, paint)) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }

   if (pattern == VG_INVALID_HANDLE) {
      paint_set_type(handle_to_paint(paint), VG_PAINT_TYPE_COLOR);
      return;
   }

   if (!vg_context_is_object_valid(ctx, VG_OBJECT_IMAGE, pattern)) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }


   if (!vg_object_is_valid(paint, VG_OBJECT_PAINT) ||
       !vg_object_is_valid(pattern, VG_OBJECT_IMAGE)) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }
   paint_set_pattern(handle_to_paint(paint),
                     handle_to_image(pattern));
}

