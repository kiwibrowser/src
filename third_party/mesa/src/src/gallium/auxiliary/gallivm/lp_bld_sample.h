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
 * Texture sampling.
 *
 * @author Jose Fonseca <jfonseca@vmware.com>
 */

#ifndef LP_BLD_SAMPLE_H
#define LP_BLD_SAMPLE_H


#include "pipe/p_format.h"
#include "util/u_debug.h"
#include "gallivm/lp_bld.h"
#include "gallivm/lp_bld_type.h"
#include "gallivm/lp_bld_swizzle.h"


struct pipe_resource;
struct pipe_sampler_view;
struct pipe_sampler_state;
struct util_format_description;
struct lp_type;
struct lp_build_context;


/**
 * Helper struct holding all derivatives needed for sampling
 */
struct lp_derivatives
{
   LLVMValueRef ddx_ddy[2];
};


/**
 * Sampler static state.
 *
 * These are the bits of state from pipe_resource and pipe_sampler_state that
 * are embedded in the generated code.
 */
struct lp_sampler_static_state
{
   /* pipe_sampler_view's state */
   enum pipe_format format;
   unsigned swizzle_r:3;     /**< PIPE_SWIZZLE_* */
   unsigned swizzle_g:3;
   unsigned swizzle_b:3;
   unsigned swizzle_a:3;

   /* pipe_texture's state */
   unsigned target:3;        /**< PIPE_TEXTURE_* */
   unsigned pot_width:1;     /**< is the width a power of two? */
   unsigned pot_height:1;
   unsigned pot_depth:1;

   /* pipe_sampler_state's state */
   unsigned wrap_s:3;
   unsigned wrap_t:3;
   unsigned wrap_r:3;
   unsigned min_img_filter:2;
   unsigned min_mip_filter:2;
   unsigned mag_img_filter:2;
   unsigned compare_mode:1;
   unsigned compare_func:3;
   unsigned normalized_coords:1;
   unsigned min_max_lod_equal:1;  /**< min_lod == max_lod ? */
   unsigned lod_bias_non_zero:1;
   unsigned apply_min_lod:1;  /**< min_lod > 0 ? */
   unsigned apply_max_lod:1;  /**< max_lod < last_level ? */

   /* Hacks */
   unsigned force_nearest_s:1;
   unsigned force_nearest_t:1;
};


/**
 * Sampler dynamic state.
 *
 * These are the bits of state from pipe_resource and pipe_sampler_state that
 * are computed in runtime.
 *
 * There are obtained through callbacks, as we don't want to tie the texture
 * sampling code generation logic to any particular texture layout or pipe
 * driver.
 */
struct lp_sampler_dynamic_state
{

   /** Obtain the base texture width (returns int32) */
   LLVMValueRef
   (*width)( const struct lp_sampler_dynamic_state *state,
             struct gallivm_state *gallivm,
             unsigned unit);

   /** Obtain the base texture height (returns int32) */
   LLVMValueRef
   (*height)( const struct lp_sampler_dynamic_state *state,
              struct gallivm_state *gallivm,
              unsigned unit);

   /** Obtain the base texture depth (returns int32) */
   LLVMValueRef
   (*depth)( const struct lp_sampler_dynamic_state *state,
             struct gallivm_state *gallivm,
             unsigned unit);

   /** Obtain the first mipmap level (base level) (returns int32) */
   LLVMValueRef
   (*first_level)( const struct lp_sampler_dynamic_state *state,
                   struct gallivm_state *gallivm,
                   unsigned unit);

   /** Obtain the number of mipmap levels minus one (returns int32) */
   LLVMValueRef
   (*last_level)( const struct lp_sampler_dynamic_state *state,
                  struct gallivm_state *gallivm,
                  unsigned unit);

   /** Obtain stride in bytes between image rows/blocks (returns int32) */
   LLVMValueRef
   (*row_stride)( const struct lp_sampler_dynamic_state *state,
                  struct gallivm_state *gallivm,
                  unsigned unit);

   /** Obtain stride in bytes between image slices (returns int32) */
   LLVMValueRef
   (*img_stride)( const struct lp_sampler_dynamic_state *state,
                  struct gallivm_state *gallivm,
                  unsigned unit);

   /** Obtain pointer to array of pointers to mimpap levels */
   LLVMValueRef
   (*data_ptr)( const struct lp_sampler_dynamic_state *state,
                struct gallivm_state *gallivm,
                unsigned unit);

   /** Obtain texture min lod (returns float) */
   LLVMValueRef
   (*min_lod)(const struct lp_sampler_dynamic_state *state,
              struct gallivm_state *gallivm, unsigned unit);

   /** Obtain texture max lod (returns float) */
   LLVMValueRef
   (*max_lod)(const struct lp_sampler_dynamic_state *state,
              struct gallivm_state *gallivm, unsigned unit);

   /** Obtain texture lod bias (returns float) */
   LLVMValueRef
   (*lod_bias)(const struct lp_sampler_dynamic_state *state,
               struct gallivm_state *gallivm, unsigned unit);

   /** Obtain texture border color (returns ptr to float[4]) */
   LLVMValueRef
   (*border_color)(const struct lp_sampler_dynamic_state *state,
                   struct gallivm_state *gallivm, unsigned unit);
};


/**
 * Keep all information for sampling code generation in a single place.
 */
struct lp_build_sample_context
{
   struct gallivm_state *gallivm;

   const struct lp_sampler_static_state *static_state;

   struct lp_sampler_dynamic_state *dynamic_state;

   const struct util_format_description *format_desc;

   /* See texture_dims() */
   unsigned dims;

   /** SIMD vector width */
   unsigned vector_width;

   /** regular scalar float type */
   struct lp_type float_type;
   struct lp_build_context float_bld;

   /** float vector type */
   struct lp_build_context float_vec_bld;

   /** regular scalar int type */
   struct lp_type int_type;
   struct lp_build_context int_bld;

   /** Incoming coordinates type and build context */
   struct lp_type coord_type;
   struct lp_build_context coord_bld;

   /** Signed integer coordinates */
   struct lp_type int_coord_type;
   struct lp_build_context int_coord_bld;

   /** Unsigned integer texture size */
   struct lp_type int_size_type;
   struct lp_build_context int_size_bld;

   /** Unsigned integer texture size */
   struct lp_type float_size_type;
   struct lp_build_context float_size_bld;

   /** Output texels type and build context */
   struct lp_type texel_type;
   struct lp_build_context texel_bld;

   /** Float per-quad type */
   struct lp_type perquadf_type;
   struct lp_build_context perquadf_bld;

   /** Int per-quad type */
   struct lp_type perquadi_type;
   struct lp_build_context perquadi_bld;

   /* Common dynamic state values */
   LLVMValueRef row_stride_array;
   LLVMValueRef img_stride_array;
   LLVMValueRef data_array;

   /** Integer vector with texture width, height, depth */
   LLVMValueRef int_size;
};



/**
 * We only support a few wrap modes in lp_build_sample_wrap_linear_int() at
 * this time.  Return whether the given mode is supported by that function.
 */
static INLINE boolean
lp_is_simple_wrap_mode(unsigned mode)
{
   switch (mode) {
   case PIPE_TEX_WRAP_REPEAT:
   case PIPE_TEX_WRAP_CLAMP_TO_EDGE:
      return TRUE;
   default:
      return FALSE;
   }
}


static INLINE void
apply_sampler_swizzle(struct lp_build_sample_context *bld,
                      LLVMValueRef *texel)
{
   unsigned char swizzles[4];

   swizzles[0] = bld->static_state->swizzle_r;
   swizzles[1] = bld->static_state->swizzle_g;
   swizzles[2] = bld->static_state->swizzle_b;
   swizzles[3] = bld->static_state->swizzle_a;

   lp_build_swizzle_soa_inplace(&bld->texel_bld, texel, swizzles);
}


static INLINE unsigned
texture_dims(enum pipe_texture_target tex)
{
   switch (tex) {
   case PIPE_TEXTURE_1D:
      return 1;
   case PIPE_TEXTURE_2D:
   case PIPE_TEXTURE_RECT:
   case PIPE_TEXTURE_CUBE:
      return 2;
   case PIPE_TEXTURE_3D:
      return 3;
   default:
      assert(0 && "bad texture target in texture_dims()");
      return 2;
   }
}


boolean
lp_sampler_wrap_mode_uses_border_color(unsigned mode,
                                       unsigned min_img_filter,
                                       unsigned mag_img_filter);

/**
 * Derive the sampler static state.
 */
void
lp_sampler_static_state(struct lp_sampler_static_state *state,
                        const struct pipe_sampler_view *view,
                        const struct pipe_sampler_state *sampler);


void
lp_build_lod_selector(struct lp_build_sample_context *bld,
                      unsigned unit,
                      const struct lp_derivatives *derivs,
                      LLVMValueRef lod_bias, /* optional */
                      LLVMValueRef explicit_lod, /* optional */
                      unsigned mip_filter,
                      LLVMValueRef *out_lod_ipart,
                      LLVMValueRef *out_lod_fpart);

void
lp_build_nearest_mip_level(struct lp_build_sample_context *bld,
                           unsigned unit,
                           LLVMValueRef lod,
                           LLVMValueRef *level_out);

void
lp_build_linear_mip_levels(struct lp_build_sample_context *bld,
                           unsigned unit,
                           LLVMValueRef lod_ipart,
                           LLVMValueRef *lod_fpart_inout,
                           LLVMValueRef *level0_out,
                           LLVMValueRef *level1_out);

LLVMValueRef
lp_build_get_mipmap_level(struct lp_build_sample_context *bld,
                          LLVMValueRef level);


void
lp_build_mipmap_level_sizes(struct lp_build_sample_context *bld,
                            LLVMValueRef ilevel,
                            LLVMValueRef *out_size_vec,
                            LLVMValueRef *row_stride_vec,
                            LLVMValueRef *img_stride_vec);


void
lp_build_extract_image_sizes(struct lp_build_sample_context *bld,
                             struct lp_type size_type,
                             struct lp_type coord_type,
                             LLVMValueRef size,
                             LLVMValueRef *out_width,
                             LLVMValueRef *out_height,
                             LLVMValueRef *out_depth);


void
lp_build_unnormalized_coords(struct lp_build_sample_context *bld,
                             LLVMValueRef flt_size,
                             LLVMValueRef *s,
                             LLVMValueRef *t,
                             LLVMValueRef *r);


void
lp_build_cube_lookup(struct lp_build_sample_context *bld,
                     LLVMValueRef s,
                     LLVMValueRef t,
                     LLVMValueRef r,
                     LLVMValueRef *face,
                     LLVMValueRef *face_s,
                     LLVMValueRef *face_t);


void
lp_build_sample_partial_offset(struct lp_build_context *bld,
                               unsigned block_length,
                               LLVMValueRef coord,
                               LLVMValueRef stride,
                               LLVMValueRef *out_offset,
                               LLVMValueRef *out_i);


void
lp_build_sample_offset(struct lp_build_context *bld,
                       const struct util_format_description *format_desc,
                       LLVMValueRef x,
                       LLVMValueRef y,
                       LLVMValueRef z,
                       LLVMValueRef y_stride,
                       LLVMValueRef z_stride,
                       LLVMValueRef *out_offset,
                       LLVMValueRef *out_i,
                       LLVMValueRef *out_j);


void
lp_build_sample_soa(struct gallivm_state *gallivm,
                    const struct lp_sampler_static_state *static_state,
                    struct lp_sampler_dynamic_state *dynamic_state,
                    struct lp_type fp_type,
                    unsigned unit,
                    unsigned num_coords,
                    const LLVMValueRef *coords,
                    const struct lp_derivatives *derivs,
                    LLVMValueRef lod_bias,
                    LLVMValueRef explicit_lod,
                    LLVMValueRef texel_out[4]);


void
lp_build_coord_repeat_npot_linear(struct lp_build_sample_context *bld,
                                  LLVMValueRef coord_f,
                                  LLVMValueRef length_i,
                                  LLVMValueRef length_f,
                                  LLVMValueRef *coord0_i,
                                  LLVMValueRef *weight_f);


void
lp_build_size_query_soa(struct gallivm_state *gallivm,
                        const struct lp_sampler_static_state *static_state,
                        struct lp_sampler_dynamic_state *dynamic_state,
                        struct lp_type int_type,
                        unsigned unit,
                        LLVMValueRef explicit_lod,
                        LLVMValueRef *sizes_out);

void
lp_build_sample_nop(struct gallivm_state *gallivm, 
                    struct lp_type type,
                    unsigned num_coords,
                    const LLVMValueRef *coords,
                    LLVMValueRef texel_out[4]);


LLVMValueRef
lp_build_minify(struct lp_build_context *bld,
                LLVMValueRef base_size,
                LLVMValueRef level);


#endif /* LP_BLD_SAMPLE_H */
