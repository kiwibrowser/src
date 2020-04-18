/**************************************************************************
 *
 * Copyright 2010 LunarG, Inc.  All Rights Reserved.
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

#include "util/u_memory.h"
#include "cso_cache/cso_hash.h"

#include "text.h"
#include "image.h"
#include "path.h"

#ifdef OPENVG_VERSION_1_1

struct vg_font {
   struct vg_object base;
   struct cso_hash *glyphs;
};

struct vg_glyph {
   struct vg_object *object; /* it could be NULL */
   VGboolean is_hinted;
   VGfloat glyph_origin[2];
   VGfloat escapement[2];
};

static VGboolean del_glyph(struct vg_font *font,
                           VGuint glyphIndex)
{
   struct vg_glyph *glyph;

   glyph = (struct vg_glyph *)
      cso_hash_take(font->glyphs, (unsigned) glyphIndex);
   if (glyph)
      FREE(glyph);

   return (glyph != NULL);
}

static void add_glyph(struct vg_font *font,
                      VGuint glyphIndex,
                      struct vg_object *obj,
                      VGboolean isHinted,
                      const VGfloat glyphOrigin[2],
                      const VGfloat escapement[2])
{
   struct vg_glyph *glyph;

   /* remove the existing one */
   del_glyph(font, glyphIndex);

   glyph = CALLOC_STRUCT(vg_glyph);
   glyph->object = obj;
   glyph->is_hinted = isHinted;
   memcpy(glyph->glyph_origin, glyphOrigin, sizeof(glyph->glyph_origin));
   memcpy(glyph->escapement, escapement, sizeof(glyph->glyph_origin));

   cso_hash_insert(font->glyphs, (unsigned) glyphIndex, glyph);
}

static struct vg_glyph *get_glyph(struct vg_font *font,
                                  VGuint glyphIndex)
{
   struct cso_hash_iter iter;

   iter = cso_hash_find(font->glyphs, (unsigned) glyphIndex);
   return (struct vg_glyph *) cso_hash_iter_data(iter);
}

static void vg_render_glyph(struct vg_context *ctx,
                            struct vg_glyph *glyph,
                            VGbitfield paintModes,
                            VGboolean allowAutoHinting)
{
   if (glyph->object && paintModes) {
      struct vg_state *state = &ctx->state.vg;
      struct matrix m;

      m = state->glyph_user_to_surface_matrix;
      matrix_translate(&m,
            state->glyph_origin[0].f - glyph->glyph_origin[0],
            state->glyph_origin[1].f - glyph->glyph_origin[1]);

      if (glyph->object->type == VG_OBJECT_PATH) {
         path_render((struct path *) glyph->object, paintModes, &m);
      }
      else {
         assert(glyph->object->type == VG_OBJECT_IMAGE);
         image_draw((struct vg_image *) glyph->object, &m);
      }
   }
}

static void vg_advance_glyph(struct vg_context *ctx,
                             struct vg_glyph *glyph,
                             VGfloat adjustment_x,
                             VGfloat adjustment_y,
                             VGboolean last)
{
   struct vg_value *glyph_origin = ctx->state.vg.glyph_origin;

   glyph_origin[0].f += glyph->escapement[0] + adjustment_x;
   glyph_origin[1].f += glyph->escapement[1] + adjustment_y;

   if (last) {
      glyph_origin[0].i = float_to_int_floor(glyph_origin[0].f);
      glyph_origin[1].i = float_to_int_floor(glyph_origin[1].f);
   }
}

struct vg_font *font_create(VGint glyphCapacityHint)
{
   struct vg_context *ctx = vg_current_context();
   struct vg_font *font;

   font = CALLOC_STRUCT(vg_font);
   vg_init_object(&font->base, ctx, VG_OBJECT_FONT);
   font->glyphs = cso_hash_create();

   vg_context_add_object(ctx, &font->base);

   return font;
}

void font_destroy(struct vg_font *font)
{
   struct vg_context *ctx = vg_current_context();
   struct cso_hash_iter iter;

   vg_context_remove_object(ctx, &font->base);

   iter = cso_hash_first_node(font->glyphs);
   while (!cso_hash_iter_is_null(iter)) {
      struct vg_glyph *glyph = (struct vg_glyph *) cso_hash_iter_data(iter);
      FREE(glyph);
      iter = cso_hash_iter_next(iter);
   }
   cso_hash_delete(font->glyphs);

   FREE(font);
}

void font_set_glyph_to_path(struct vg_font *font,
                            VGuint glyphIndex,
                            struct path *path,
                            VGboolean isHinted,
                            const VGfloat glyphOrigin[2],
                            const VGfloat escapement[2])
{
   add_glyph(font, glyphIndex, (struct vg_object *) path,
         isHinted, glyphOrigin, escapement);
}

void font_set_glyph_to_image(struct vg_font *font,
                             VGuint glyphIndex,
                             struct vg_image *image,
                             const VGfloat glyphOrigin[2],
                             const VGfloat escapement[2])
{
   add_glyph(font, glyphIndex, (struct vg_object *) image,
         VG_TRUE, glyphOrigin, escapement);
}

void font_clear_glyph(struct vg_font *font,
                      VGuint glyphIndex)
{
   if (!del_glyph(font, glyphIndex)) {
      struct vg_context *ctx = vg_current_context();
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
   }
}

void font_draw_glyph(struct vg_font *font,
                     VGuint glyphIndex,
                     VGbitfield paintModes,
                     VGboolean allowAutoHinting)
{
   struct vg_context *ctx = vg_current_context();
   struct vg_glyph *glyph;

   glyph = get_glyph(font, glyphIndex);
   if (!glyph) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }

   vg_render_glyph(ctx, glyph, paintModes, allowAutoHinting);
   vg_advance_glyph(ctx, glyph, 0.0f, 0.0f, VG_TRUE);
}

void font_draw_glyphs(struct vg_font *font,
                      VGint glyphCount,
                      const VGuint *glyphIndices,
                      const VGfloat *adjustments_x,
                      const VGfloat *adjustments_y,
                      VGbitfield paintModes,
                      VGboolean allowAutoHinting)
{
   struct vg_context *ctx = vg_current_context();
   VGint i;

   for (i = 0; i < glyphCount; ++i) {
      if (!get_glyph(font, glyphIndices[i])) {
         vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
         return;
      }
   }

   for (i = 0; i < glyphCount; ++i) {
      struct vg_glyph *glyph;
      VGfloat adj_x, adj_y;

      glyph = get_glyph(font, glyphIndices[i]);

      vg_render_glyph(ctx, glyph, paintModes, allowAutoHinting);

      adj_x = (adjustments_x) ? adjustments_x[i] : 0.0f;
      adj_y = (adjustments_y) ? adjustments_y[i] : 0.0f;
      vg_advance_glyph(ctx, glyph, adj_x, adj_y, (i == glyphCount - 1));
   }
}

VGint font_num_glyphs(struct vg_font *font)
{
   return cso_hash_size(font->glyphs);
}

#endif /* OPENVG_VERSION_1_1 */
