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

#ifndef _TEXT_H
#define _TEXT_H

#include "vg_context.h"
#include "cso_cache/cso_hash.h"

struct vg_font;
struct vg_image;
struct path;

struct vg_font *font_create(VGint glyphCapacityHint);
void font_destroy(struct vg_font *font);

void font_set_glyph_to_path(struct vg_font *font,
                            VGuint glyphIndex,
                            struct path *path,
                            VGboolean isHinted,
                            const VGfloat glyphOrigin[2],
                            const VGfloat escapement[2]);

void font_set_glyph_to_image(struct vg_font *font,
                             VGuint glyphIndex,
                             struct vg_image *image,
                             const VGfloat glyphOrigin[2],
                             const VGfloat escapement[2]);

void font_clear_glyph(struct vg_font *font,
                      VGuint glyphIndex);

void font_draw_glyph(struct vg_font *font,
                     VGuint glyphIndex,
                     VGbitfield paintModes,
                     VGboolean allowAutoHinting);

void font_draw_glyphs(struct vg_font *font,
                      VGint glyphCount,
                      const VGuint *glyphIndices,
                      const VGfloat *adjustments_x,
                      const VGfloat *adjustments_y,
                      VGbitfield paintModes,
                      VGboolean allowAutoHinting);

VGint font_num_glyphs(struct vg_font *font);

#endif /* _TEXT_H */
