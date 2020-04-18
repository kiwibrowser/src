/**************************************************************************
 *
 * Copyright 2009 VMware, Inc.
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
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

/**
 * @file
 * C - JIT interfaces
 *
 * @author Jose Fonseca <jfonseca@vmware.com>
 */

#ifndef LP_JIT_H
#define LP_JIT_H


#include "gallivm/lp_bld_struct.h"

#include "pipe/p_state.h"
#include "lp_texture.h"


struct lp_fragment_shader_variant;
struct llvmpipe_screen;


struct lp_jit_texture
{
   uint32_t width;
   uint32_t height;
   uint32_t depth;
   uint32_t first_level;
   uint32_t last_level;
   uint32_t row_stride[LP_MAX_TEXTURE_LEVELS];
   uint32_t img_stride[LP_MAX_TEXTURE_LEVELS];
   const void *data[LP_MAX_TEXTURE_LEVELS];
   /* sampler state, actually */
   float min_lod;
   float max_lod;
   float lod_bias;
   float border_color[4];
};


enum {
   LP_JIT_TEXTURE_WIDTH = 0,
   LP_JIT_TEXTURE_HEIGHT,
   LP_JIT_TEXTURE_DEPTH,
   LP_JIT_TEXTURE_FIRST_LEVEL,
   LP_JIT_TEXTURE_LAST_LEVEL,
   LP_JIT_TEXTURE_ROW_STRIDE,
   LP_JIT_TEXTURE_IMG_STRIDE,
   LP_JIT_TEXTURE_DATA,
   LP_JIT_TEXTURE_MIN_LOD,
   LP_JIT_TEXTURE_MAX_LOD,
   LP_JIT_TEXTURE_LOD_BIAS,
   LP_JIT_TEXTURE_BORDER_COLOR,
   LP_JIT_TEXTURE_NUM_FIELDS  /* number of fields above */
};



/**
 * This structure is passed directly to the generated fragment shader.
 *
 * It contains the derived state.
 *
 * Changes here must be reflected in the lp_jit_context_* macros and
 * lp_jit_init_types function. Changes to the ordering should be avoided.
 *
 * Only use types with a clear size and padding here, in particular prefer the
 * stdint.h types to the basic integer types.
 */
struct lp_jit_context
{
   const float *constants;

   float alpha_ref_value;

   uint32_t stencil_ref_front, stencil_ref_back;

   /* FIXME: store (also?) in floats */
   uint8_t *blend_color;

   struct lp_jit_texture textures[PIPE_MAX_SAMPLERS];
};


/**
 * These enum values must match the position of the fields in the
 * lp_jit_context struct above.
 */
enum {
   LP_JIT_CTX_CONSTANTS = 0,
   LP_JIT_CTX_ALPHA_REF,
   LP_JIT_CTX_STENCIL_REF_FRONT,
   LP_JIT_CTX_STENCIL_REF_BACK,
   LP_JIT_CTX_BLEND_COLOR,
   LP_JIT_CTX_TEXTURES,
   LP_JIT_CTX_COUNT
};


#define lp_jit_context_constants(_gallivm, _ptr) \
   lp_build_struct_get(_gallivm, _ptr, LP_JIT_CTX_CONSTANTS, "constants")

#define lp_jit_context_alpha_ref_value(_gallivm, _ptr) \
   lp_build_struct_get(_gallivm, _ptr, LP_JIT_CTX_ALPHA_REF, "alpha_ref_value")

#define lp_jit_context_stencil_ref_front_value(_gallivm, _ptr) \
   lp_build_struct_get(_gallivm, _ptr, LP_JIT_CTX_STENCIL_REF_FRONT, "stencil_ref_front")

#define lp_jit_context_stencil_ref_back_value(_gallivm, _ptr) \
   lp_build_struct_get(_gallivm, _ptr, LP_JIT_CTX_STENCIL_REF_BACK, "stencil_ref_back")

#define lp_jit_context_blend_color(_gallivm, _ptr) \
   lp_build_struct_get(_gallivm, _ptr, LP_JIT_CTX_BLEND_COLOR, "blend_color")

#define lp_jit_context_textures(_gallivm, _ptr) \
   lp_build_struct_get_ptr(_gallivm, _ptr, LP_JIT_CTX_TEXTURES, "textures")



typedef void
(*lp_jit_frag_func)(const struct lp_jit_context *context,
                    uint32_t x,
                    uint32_t y,
                    uint32_t facing,
                    const void *a0,
                    const void *dadx,
                    const void *dady,
                    uint8_t **color,
                    void *depth,
                    uint32_t mask,
                    uint32_t *counter);


void
lp_jit_screen_cleanup(struct llvmpipe_screen *screen);


void
lp_jit_screen_init(struct llvmpipe_screen *screen);


void
lp_jit_init_types(struct lp_fragment_shader_variant *lp);


#endif /* LP_JIT_H */
