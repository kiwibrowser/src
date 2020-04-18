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

#ifndef SHADERS_CACHE_H
#define SHADERS_CACHE_H


struct vg_context;
struct pipe_context;
struct tgsi_token;
struct shaders_cache;

#define _SHADERS_PAINT_BITS            3
#define _SHADERS_IMAGE_BITS            2
#define _SHADERS_COLOR_TRANSFORM_BITS  1
#define _SHADERS_ALPHA_BITS            2
#define _SHADERS_BLEND_BITS            4
#define _SHADERS_MASK_BITS             1
#define _SHADERS_PREMULTIPLY_BITS      2
#define _SHADERS_BW_BITS               1

#define SHADERS_PAINT_SHIFT           (0)
#define SHADERS_IMAGE_SHIFT           (SHADERS_PAINT_SHIFT + _SHADERS_PAINT_BITS)
#define SHADERS_COLOR_TRANSFORM_SHIFT (SHADERS_IMAGE_SHIFT + _SHADERS_IMAGE_BITS)
#define SHADERS_ALPHA_SHIFT           (SHADERS_COLOR_TRANSFORM_SHIFT + _SHADERS_COLOR_TRANSFORM_BITS)
#define SHADERS_BLEND_SHIFT           (SHADERS_ALPHA_SHIFT + _SHADERS_ALPHA_BITS)
#define SHADERS_MASK_SHIFT            (SHADERS_BLEND_SHIFT + _SHADERS_BLEND_BITS)
#define SHADERS_PREMULTIPLY_SHIFT     (SHADERS_MASK_SHIFT + _SHADERS_MASK_BITS)
#define SHADERS_BW_SHIFT              (SHADERS_PREMULTIPLY_SHIFT + _SHADERS_PREMULTIPLY_BITS)

#define _SHADERS_GET_STAGE(stage, id) \
   ((id) & (((1 << _SHADERS_ ## stage ## _BITS) - 1) << SHADERS_ ## stage ## _SHIFT))

#define SHADERS_GET_PAINT_SHADER(id)           _SHADERS_GET_STAGE(PAINT, id)
#define SHADERS_GET_IMAGE_SHADER(id)           _SHADERS_GET_STAGE(IMAGE, id)
#define SHADERS_GET_COLOR_TRANSFORM_SHADER(id) _SHADERS_GET_STAGE(COLOR_TRANSFORM, id)
#define SHADERS_GET_ALPHA_SHADER(id)           _SHADERS_GET_STAGE(ALPHA, id)
#define SHADERS_GET_BLEND_SHADER(id)           _SHADERS_GET_STAGE(BLEND, id)
#define SHADERS_GET_MASK_SHADER(id)            _SHADERS_GET_STAGE(MASK, id)
#define SHADERS_GET_PREMULTIPLY_SHADER(id)     _SHADERS_GET_STAGE(PREMULTIPLY, id)
#define SHADERS_GET_BW_SHADER(id)              _SHADERS_GET_STAGE(BW, id)

enum VegaShaderType {
   VEGA_SOLID_FILL_SHADER         = 1 << SHADERS_PAINT_SHIFT,
   VEGA_LINEAR_GRADIENT_SHADER    = 2 << SHADERS_PAINT_SHIFT,
   VEGA_RADIAL_GRADIENT_SHADER    = 3 << SHADERS_PAINT_SHIFT,
   VEGA_PATTERN_SHADER            = 4 << SHADERS_PAINT_SHIFT,
   VEGA_PAINT_DEGENERATE_SHADER   = 5 << SHADERS_PAINT_SHIFT,

   VEGA_IMAGE_NORMAL_SHADER       = 1 << SHADERS_IMAGE_SHIFT,
   VEGA_IMAGE_MULTIPLY_SHADER     = 2 << SHADERS_IMAGE_SHIFT,
   VEGA_IMAGE_STENCIL_SHADER      = 3 << SHADERS_IMAGE_SHIFT,

   VEGA_COLOR_TRANSFORM_SHADER    = 1 <<  SHADERS_COLOR_TRANSFORM_SHIFT,

   VEGA_ALPHA_NORMAL_SHADER       = 1 <<  SHADERS_ALPHA_SHIFT,
   VEGA_ALPHA_PER_CHANNEL_SHADER  = 2 <<  SHADERS_ALPHA_SHIFT,

   VEGA_BLEND_SRC_SHADER          = 1 << SHADERS_BLEND_SHIFT,
   VEGA_BLEND_SRC_OVER_SHADER     = 2 << SHADERS_BLEND_SHIFT,
   VEGA_BLEND_DST_OVER_SHADER     = 3 << SHADERS_BLEND_SHIFT,
   VEGA_BLEND_SRC_IN_SHADER       = 4 << SHADERS_BLEND_SHIFT,
   VEGA_BLEND_DST_IN_SHADER       = 5 << SHADERS_BLEND_SHIFT,
   VEGA_BLEND_MULTIPLY_SHADER     = 6 << SHADERS_BLEND_SHIFT,
   VEGA_BLEND_SCREEN_SHADER       = 7 << SHADERS_BLEND_SHIFT,
   VEGA_BLEND_DARKEN_SHADER       = 8 << SHADERS_BLEND_SHIFT,
   VEGA_BLEND_LIGHTEN_SHADER      = 9 << SHADERS_BLEND_SHIFT,
   VEGA_BLEND_ADDITIVE_SHADER     = 10<< SHADERS_BLEND_SHIFT,

   VEGA_MASK_SHADER               = 1 << SHADERS_MASK_SHIFT,

   VEGA_PREMULTIPLY_SHADER        = 1 << SHADERS_PREMULTIPLY_SHIFT,
   VEGA_UNPREMULTIPLY_SHADER      = 2 << SHADERS_PREMULTIPLY_SHIFT,

   VEGA_BW_SHADER                 = 1 << SHADERS_BW_SHIFT
};

struct vg_shader {
   void *driver;
   struct tgsi_token *tokens;
   int type;/* PIPE_SHADER_VERTEX, PIPE_SHADER_FRAGMENT */
};

struct shaders_cache *shaders_cache_create(struct vg_context *pipe);
void shaders_cache_destroy(struct shaders_cache *sc);
void *shaders_cache_fill(struct shaders_cache *sc,
                         int shader_key);

struct vg_shader *shader_create_from_text(struct pipe_context *pipe,
                                          const char *txt, int num_tokens,
                                          int type);

void vg_shader_destroy(struct vg_context *ctx, struct vg_shader *shader);



#endif
