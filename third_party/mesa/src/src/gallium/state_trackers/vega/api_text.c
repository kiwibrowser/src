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
#include "text.h"
#include "api.h"
#include "handle.h"

#include "util/u_memory.h"

#ifdef OPENVG_VERSION_1_1

VGFont vegaCreateFont(VGint glyphCapacityHint)
{
   struct vg_context *ctx = vg_current_context();

   if (glyphCapacityHint < 0) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return VG_INVALID_HANDLE;
   }

   return font_to_handle(font_create(glyphCapacityHint));
}

void vegaDestroyFont(VGFont f)
{
   struct vg_font *font = handle_to_font(f);
   struct vg_context *ctx = vg_current_context();

   if (f == VG_INVALID_HANDLE) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }
   if (!vg_object_is_valid(f, VG_OBJECT_FONT)) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }

   font_destroy(font);
}

void vegaSetGlyphToPath(VGFont font,
                        VGuint glyphIndex,
                        VGPath path,
                        VGboolean isHinted,
                        const VGfloat glyphOrigin[2],
                        const VGfloat escapement[2])
{
   struct vg_context *ctx = vg_current_context();
   struct path *pathObj;
   struct vg_font *f;

   if (font == VG_INVALID_HANDLE ||
       !vg_context_is_object_valid(ctx, VG_OBJECT_FONT, font)) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }
   if (!glyphOrigin || !escapement ||
       !is_aligned(glyphOrigin) || !is_aligned(escapement)) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }
   if (path != VG_INVALID_HANDLE &&
       !vg_context_is_object_valid(ctx, VG_OBJECT_PATH, path)) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }

   pathObj = handle_to_path(path);
   f = handle_to_font(font);

   font_set_glyph_to_path(f, glyphIndex, pathObj,
         isHinted, glyphOrigin, escapement);
}

void vegaSetGlyphToImage(VGFont font,
                         VGuint glyphIndex,
                         VGImage image,
                         const VGfloat glyphOrigin[2],
                         const VGfloat escapement[2])
{
   struct vg_context *ctx = vg_current_context();
   struct vg_image *img_obj;
   struct vg_font *f;

   if (font == VG_INVALID_HANDLE ||
       !vg_context_is_object_valid(ctx, VG_OBJECT_FONT, font)) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }
   if (!glyphOrigin || !escapement ||
       !is_aligned(glyphOrigin) || !is_aligned(escapement)) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }
   if (image != VG_INVALID_HANDLE &&
       !vg_context_is_object_valid(ctx, VG_OBJECT_IMAGE, image)) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }

   img_obj = handle_to_image(image);
   f = handle_to_font(font);

   font_set_glyph_to_image(f, glyphIndex, img_obj, glyphOrigin, escapement);
}

void vegaClearGlyph(VGFont font,
                    VGuint glyphIndex)
{
   struct vg_context *ctx = vg_current_context();
   struct vg_font *f;

   if (font == VG_INVALID_HANDLE) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }

   f = handle_to_font(font);

   font_clear_glyph(f, glyphIndex);
}

void vegaDrawGlyph(VGFont font,
                   VGuint glyphIndex,
                   VGbitfield paintModes,
                   VGboolean allowAutoHinting)
{
   struct vg_context *ctx = vg_current_context();
   struct vg_font *f;

   if (font == VG_INVALID_HANDLE) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }
   if (paintModes & (~(VG_STROKE_PATH|VG_FILL_PATH))) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }
   f = handle_to_font(font);

   font_draw_glyph(f, glyphIndex, paintModes, allowAutoHinting);
}

void vegaDrawGlyphs(VGFont font,
                    VGint glyphCount,
                    const VGuint *glyphIndices,
                    const VGfloat *adjustments_x,
                    const VGfloat *adjustments_y,
                    VGbitfield paintModes,
                    VGboolean allowAutoHinting)
{
   struct vg_context *ctx = vg_current_context();
   struct vg_font *f;

   if (font == VG_INVALID_HANDLE) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }
   if (glyphCount <= 0) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }
   if (!glyphIndices || !is_aligned(glyphIndices)) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }
   if ((adjustments_x && !is_aligned(adjustments_x)) ||
       (adjustments_y && !is_aligned(adjustments_y))) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }
   if (paintModes & (~(VG_STROKE_PATH|VG_FILL_PATH))) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }

   f = handle_to_font(font);

   font_draw_glyphs(f, glyphCount, glyphIndices,
         adjustments_x, adjustments_y, paintModes, allowAutoHinting);
}

#endif /* OPENVG_VERSION_1_1 */
