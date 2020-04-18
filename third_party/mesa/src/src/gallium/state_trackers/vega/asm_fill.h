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

#ifndef ASM_FILL_H
#define ASM_FILL_H

#include "tgsi/tgsi_ureg.h"

typedef void (* ureg_func)( struct ureg_program *ureg,
                            struct ureg_dst *out,
                            struct ureg_src *in,
                            struct ureg_src *sampler,
                            struct ureg_dst *temp,
                            struct ureg_src *constant);

static INLINE void
solid_fill( struct ureg_program *ureg,
            struct ureg_dst *out,
            struct ureg_src *in,
            struct ureg_src *sampler,
            struct ureg_dst *temp,
            struct ureg_src *constant)
{
   ureg_MOV(ureg, *out, constant[2]);
}

/**
 * Perform frag-coord-to-paint-coord transform.  The transformation is in
 * CONST[4..6].
 */
#define PAINT_TRANSFORM                                                 \
   ureg_MOV(ureg, ureg_writemask(temp[0], TGSI_WRITEMASK_XY), in[0]);   \
   ureg_MOV(ureg,                                                       \
            ureg_writemask(temp[0], TGSI_WRITEMASK_Z),                  \
            ureg_scalar(constant[3], TGSI_SWIZZLE_Y));                  \
   ureg_DP3(ureg, temp[1], constant[4], ureg_src(temp[0]));             \
   ureg_DP3(ureg, temp[2], constant[5], ureg_src(temp[0]));             \
   ureg_DP3(ureg, temp[3], constant[6], ureg_src(temp[0]));             \
   ureg_RCP(ureg, temp[3], ureg_src(temp[3]));                          \
   ureg_MUL(ureg, temp[1], ureg_src(temp[1]), ureg_src(temp[3]));       \
   ureg_MUL(ureg, temp[2], ureg_src(temp[2]), ureg_src(temp[3]));       \
   ureg_MOV(ureg,                                                       \
            ureg_writemask(temp[4], TGSI_WRITEMASK_X),                  \
            ureg_src(temp[1]));                                         \
   ureg_MOV(ureg,                                                       \
            ureg_writemask(temp[4], TGSI_WRITEMASK_Y),                  \
            ureg_src(temp[2]));

static INLINE void
linear_grad( struct ureg_program *ureg,
             struct ureg_dst *out,
             struct ureg_src *in,
             struct ureg_src *sampler,
             struct ureg_dst *temp,
             struct ureg_src *constant)
{
   PAINT_TRANSFORM

   /* grad = DP2((x, y), CONST[2].xy) * CONST[2].z */
   ureg_MUL(ureg, temp[0],
            ureg_scalar(constant[2], TGSI_SWIZZLE_Y),
            ureg_scalar(ureg_src(temp[4]), TGSI_SWIZZLE_Y));
   ureg_MAD(ureg, temp[1],
            ureg_scalar(constant[2], TGSI_SWIZZLE_X),
            ureg_scalar(ureg_src(temp[4]), TGSI_SWIZZLE_X),
            ureg_src(temp[0]));
   ureg_MUL(ureg, temp[2], ureg_src(temp[1]),
            ureg_scalar(constant[2], TGSI_SWIZZLE_Z));

   ureg_TEX(ureg, *out, TGSI_TEXTURE_1D, ureg_src(temp[2]), sampler[0]);
}

static INLINE void
radial_grad( struct ureg_program *ureg,
             struct ureg_dst *out,
             struct ureg_src *in,
             struct ureg_src *sampler,
             struct ureg_dst *temp,
             struct ureg_src *constant)
{
   PAINT_TRANSFORM

   /*
    * Calculate (sqrt(B^2 + AC) - B) / A, where
    *
    *   A is CONST[2].z,
    *   B is DP2((x, y), CONST[2].xy), and
    *   C is DP2((x, y), (x, y)).
    */

   /* B and C */
   ureg_DP2(ureg, temp[0], ureg_src(temp[4]), constant[2]);
   ureg_DP2(ureg, temp[1], ureg_src(temp[4]), ureg_src(temp[4]));

   /* the square root */
   ureg_MUL(ureg, temp[2], ureg_src(temp[0]), ureg_src(temp[0]));
   ureg_MAD(ureg, temp[3], ureg_src(temp[1]),
         ureg_scalar(constant[2], TGSI_SWIZZLE_Z), ureg_src(temp[2]));
   ureg_RSQ(ureg, temp[3], ureg_src(temp[3]));
   ureg_RCP(ureg, temp[3], ureg_src(temp[3]));

   ureg_SUB(ureg, temp[3], ureg_src(temp[3]), ureg_src(temp[0]));
   ureg_RCP(ureg, temp[0], ureg_scalar(constant[2], TGSI_SWIZZLE_Z));
   ureg_MUL(ureg, temp[0], ureg_src(temp[0]), ureg_src(temp[3]));

   ureg_TEX(ureg, *out, TGSI_TEXTURE_1D, ureg_src(temp[0]), sampler[0]);
}


static INLINE void
pattern( struct ureg_program *ureg,
         struct ureg_dst     *out,
         struct ureg_src     *in,
         struct ureg_src     *sampler,
         struct ureg_dst     *temp,
         struct ureg_src     *constant)
{
   PAINT_TRANSFORM

   /* (s, t) = (x / tex_width, y / tex_height) */
   ureg_RCP(ureg, temp[0],
            ureg_swizzle(constant[3],
                         TGSI_SWIZZLE_Z,
                         TGSI_SWIZZLE_W,
                         TGSI_SWIZZLE_Z,
                         TGSI_SWIZZLE_W));
   ureg_MOV(ureg, temp[1], ureg_src(temp[4]));
   ureg_MUL(ureg,
            ureg_writemask(temp[1], TGSI_WRITEMASK_X),
            ureg_src(temp[1]),
            ureg_src(temp[0]));
   ureg_MUL(ureg,
            ureg_writemask(temp[1], TGSI_WRITEMASK_Y),
            ureg_src(temp[1]),
            ureg_src(temp[0]));

   ureg_TEX(ureg, *out, TGSI_TEXTURE_2D, ureg_src(temp[1]), sampler[0]);
}

static INLINE void
paint_degenerate( struct ureg_program *ureg,
                  struct ureg_dst *out,
                  struct ureg_src *in,
                  struct ureg_src *sampler,
                  struct ureg_dst *temp,
                  struct ureg_src *constant)
{
   /* CONST[3].y is 1.0f */
   ureg_MOV(ureg, temp[1], ureg_scalar(constant[3], TGSI_SWIZZLE_Y));
   ureg_TEX(ureg, *out, TGSI_TEXTURE_1D, ureg_src(temp[1]), sampler[0]);
}

static INLINE void
image_normal( struct ureg_program *ureg,
              struct ureg_dst *out,
              struct ureg_src *in,
              struct ureg_src *sampler,
              struct ureg_dst *temp,
              struct ureg_src *constant)
{
   /* store and pass image color in TEMP[1] */
   ureg_TEX(ureg, temp[1], TGSI_TEXTURE_2D, in[1], sampler[3]);
   ureg_MOV(ureg, *out, ureg_src(temp[1]));
}


static INLINE void
image_multiply( struct ureg_program *ureg,
                struct ureg_dst *out,
                struct ureg_src *in,
                struct ureg_src *sampler,
                struct ureg_dst *temp,
                struct ureg_src *constant)
{
   /* store and pass image color in TEMP[1] */
   ureg_TEX(ureg, temp[1], TGSI_TEXTURE_2D, in[1], sampler[3]);
   ureg_MUL(ureg, *out, ureg_src(temp[0]), ureg_src(temp[1]));
}


static INLINE void
image_stencil( struct ureg_program *ureg,
               struct ureg_dst *out,
               struct ureg_src *in,
               struct ureg_src *sampler,
               struct ureg_dst *temp,
               struct ureg_src *constant)
{
   /* store and pass image color in TEMP[1] */
   ureg_TEX(ureg, temp[1], TGSI_TEXTURE_2D, in[1], sampler[3]);
   ureg_MOV(ureg, *out, ureg_src(temp[0]));
}

static INLINE void
color_transform( struct ureg_program *ureg,
                 struct ureg_dst *out,
                 struct ureg_src *in,
                 struct ureg_src *sampler,
                 struct ureg_dst *temp,
                 struct ureg_src *constant)
{
   /* note that TEMP[1] may already be used for image color */

   ureg_MAD(ureg, temp[2], ureg_src(temp[0]), constant[0], constant[1]);
   /* clamp to [0.0f, 1.0f] */
   ureg_CLAMP(ureg, temp[2],
              ureg_src(temp[2]),
              ureg_scalar(constant[3], TGSI_SWIZZLE_X),
              ureg_scalar(constant[3], TGSI_SWIZZLE_Y));
   ureg_MOV(ureg, *out, ureg_src(temp[2]));
}

static INLINE void
alpha_normal( struct ureg_program *ureg,
              struct ureg_dst *out,
              struct ureg_src *in,
              struct ureg_src *sampler,
              struct ureg_dst *temp,
              struct ureg_src *constant)
{
   /* save per-channel alpha in TEMP[1] */
   ureg_MOV(ureg, temp[1], ureg_scalar(ureg_src(temp[0]), TGSI_SWIZZLE_W));

   ureg_MOV(ureg, *out, ureg_src(temp[0]));
}

static INLINE void
alpha_per_channel( struct ureg_program *ureg,
                   struct ureg_dst *out,
                   struct ureg_src *in,
                   struct ureg_src *sampler,
                   struct ureg_dst *temp,
                   struct ureg_src *constant)
{
   /* save per-channel alpha in TEMP[1] */
   ureg_MUL(ureg,
            ureg_writemask(temp[1], TGSI_WRITEMASK_W),
            ureg_src(temp[0]),
            ureg_src(temp[1]));
   ureg_MUL(ureg,
            ureg_writemask(temp[1], TGSI_WRITEMASK_XYZ),
            ureg_src(temp[1]),
            ureg_scalar(ureg_src(temp[1]), TGSI_SWIZZLE_W));

   /* update alpha */
   ureg_MOV(ureg,
            ureg_writemask(temp[0], TGSI_WRITEMASK_W),
            ureg_src(temp[1]));
   ureg_MOV(ureg, *out, ureg_src(temp[0]));
}

/**
 * Premultiply src and dst.
 */
static INLINE void
blend_premultiply( struct ureg_program *ureg,
                   struct ureg_src src,
                   struct ureg_src src_channel_alpha,
                   struct ureg_src dst)
{
   /* premultiply src */
   ureg_MUL(ureg,
            ureg_writemask(ureg_dst(src), TGSI_WRITEMASK_XYZ),
            src,
            src_channel_alpha);
   /* premultiply dst */
   ureg_MUL(ureg,
            ureg_writemask(ureg_dst(dst), TGSI_WRITEMASK_XYZ),
            dst,
            ureg_scalar(dst, TGSI_SWIZZLE_W));
}

/**
 * Unpremultiply src.
 */
static INLINE void
blend_unpremultiply( struct ureg_program *ureg,
                     struct ureg_src src,
                     struct ureg_src one,
                     struct ureg_dst temp[1])
{
   /* replace 0.0f by 1.0f before calculating reciprocal */
   ureg_CMP(ureg,
            temp[0],
            ureg_negate(ureg_scalar(src, TGSI_SWIZZLE_W)),
            ureg_scalar(src, TGSI_SWIZZLE_W),
            one);
   ureg_RCP(ureg, temp[0], ureg_src(temp[0]));

   ureg_MUL(ureg,
            ureg_writemask(ureg_dst(src), TGSI_WRITEMASK_XYZ),
            src,
            ureg_src(temp[0]));
}

/**
 * Emit instructions for the specified blend mode.  Colors will be
 * unpremultiplied.  Two temporary registers are required.
 *
 * The output is written back to src.
 */
static INLINE void
blend_generic(struct ureg_program *ureg,
              VGBlendMode mode,
              struct ureg_src src,
              struct ureg_src src_channel_alpha,
              struct ureg_src dst,
              struct ureg_src one,
              struct ureg_dst temp[2])
{
   struct ureg_dst out;

   blend_premultiply(ureg, src, src_channel_alpha, dst);

   /* blend in-place */
   out = ureg_dst(src);

   switch (mode) {
   case VG_BLEND_SRC:
      ureg_MOV(ureg, out, src);
      break;
   case VG_BLEND_SRC_OVER:
      /* RGBA_out = RGBA_src + (1 - A_src) * RGBA_dst */
      ureg_SUB(ureg, temp[0], one, src_channel_alpha);
      ureg_MAD(ureg, out, ureg_src(temp[0]), dst, src);
      break;
   case VG_BLEND_DST_OVER:
      /* RGBA_out = RGBA_dst + (1 - A_dst) * RGBA_src */
      ureg_SUB(ureg, temp[0], one, ureg_scalar(dst, TGSI_SWIZZLE_W));
      ureg_MAD(ureg, out, ureg_src(temp[0]), src, dst);
      break;
   case VG_BLEND_SRC_IN:
      ureg_MUL(ureg, out, src, ureg_scalar(dst, TGSI_SWIZZLE_W));
      break;
   case VG_BLEND_DST_IN:
      ureg_MUL(ureg, out, dst, src_channel_alpha);
      break;
   case VG_BLEND_MULTIPLY:
      /*
       * RGB_out = (1 - A_dst) * RGB_src + (1 - A_src) * RGB_dst +
       *           RGB_src * RGB_dst
       */
      ureg_MAD(ureg, temp[0],
            ureg_scalar(dst, TGSI_SWIZZLE_W), ureg_negate(src), src);
      ureg_MAD(ureg, temp[1],
            src_channel_alpha, ureg_negate(dst), dst);
      ureg_MAD(ureg, temp[0], src, dst, ureg_src(temp[0]));
      ureg_ADD(ureg, out, ureg_src(temp[0]), ureg_src(temp[1]));
      /* alpha is src over */
      ureg_ADD(ureg, ureg_writemask(out, TGSI_WRITEMASK_W),
            src, ureg_src(temp[1]));
      break;
   case VG_BLEND_SCREEN:
      /* RGBA_out = RGBA_src + (1 - RGBA_src) * RGBA_dst */
      ureg_SUB(ureg, temp[0], one, src);
      ureg_MAD(ureg, out, ureg_src(temp[0]), dst, src);
      break;
   case VG_BLEND_DARKEN:
   case VG_BLEND_LIGHTEN:
      /* src over */
      ureg_SUB(ureg, temp[0], one, src_channel_alpha);
      ureg_MAD(ureg, temp[0], ureg_src(temp[0]), dst, src);
      /* dst over */
      ureg_SUB(ureg, temp[1], one, ureg_scalar(dst, TGSI_SWIZZLE_W));
      ureg_MAD(ureg, temp[1], ureg_src(temp[1]), src, dst);
      /* take min/max for colors */
      if (mode == VG_BLEND_DARKEN) {
         ureg_MIN(ureg, ureg_writemask(out, TGSI_WRITEMASK_XYZ),
               ureg_src(temp[0]), ureg_src(temp[1]));
      }
      else {
         ureg_MAX(ureg, ureg_writemask(out, TGSI_WRITEMASK_XYZ),
               ureg_src(temp[0]), ureg_src(temp[1]));
      }
      break;
   case VG_BLEND_ADDITIVE:
      /* RGBA_out = RGBA_src + RGBA_dst */
      ureg_ADD(ureg, temp[0], src, dst);
      ureg_MIN(ureg, out, ureg_src(temp[0]), one);
      break;
   default:
      assert(0);
      break;
   }

   blend_unpremultiply(ureg, src, one, temp);
}

#define BLEND_GENERIC(mode) \
   do { \
      ureg_TEX(ureg, temp[2], TGSI_TEXTURE_2D, in[0], sampler[2]);         \
      blend_generic(ureg, (mode), ureg_src(temp[0]), ureg_src(temp[1]),    \
                    ureg_src(temp[2]),                                     \
                    ureg_scalar(constant[3], TGSI_SWIZZLE_Y), temp + 3);   \
      ureg_MOV(ureg, *out, ureg_src(temp[0]));                             \
   } while (0)

static INLINE void
blend_src( struct ureg_program *ureg,
           struct ureg_dst *out,
           struct ureg_src *in,
           struct ureg_src *sampler,
           struct ureg_dst *temp,
           struct ureg_src *constant)
{
   BLEND_GENERIC(VG_BLEND_SRC);
}

static INLINE void
blend_src_over( struct ureg_program *ureg,
                struct ureg_dst *out,
                struct ureg_src *in,
                struct ureg_src *sampler,
                struct ureg_dst *temp,
                struct ureg_src *constant)
{
   BLEND_GENERIC(VG_BLEND_SRC_OVER);
}

static INLINE void
blend_dst_over( struct ureg_program *ureg,
                struct ureg_dst *out,
                struct ureg_src *in,
                struct ureg_src *sampler,
                struct ureg_dst *temp,
                struct ureg_src *constant)
{
   BLEND_GENERIC(VG_BLEND_DST_OVER);
}

static INLINE void
blend_src_in( struct ureg_program *ureg,
              struct ureg_dst *out,
              struct ureg_src *in,
              struct ureg_src *sampler,
              struct ureg_dst *temp,
              struct ureg_src *constant)
{
   BLEND_GENERIC(VG_BLEND_SRC_IN);
}

static INLINE void
blend_dst_in( struct ureg_program *ureg,
              struct ureg_dst *out,
              struct ureg_src *in,
              struct ureg_src *sampler,
              struct ureg_dst *temp,
              struct ureg_src *constant)
{
   BLEND_GENERIC(VG_BLEND_DST_IN);
}

static INLINE void
blend_multiply( struct ureg_program *ureg,
                struct ureg_dst *out,
                struct ureg_src *in,
                struct ureg_src *sampler,
                struct ureg_dst *temp,
                struct ureg_src *constant)
{
   BLEND_GENERIC(VG_BLEND_MULTIPLY);
}

static INLINE void
blend_screen( struct ureg_program *ureg,
              struct ureg_dst     *out,
              struct ureg_src     *in,
              struct ureg_src     *sampler,
              struct ureg_dst     *temp,
              struct ureg_src     *constant)
{
   BLEND_GENERIC(VG_BLEND_SCREEN);
}

static INLINE void
blend_darken( struct ureg_program *ureg,
              struct ureg_dst     *out,
              struct ureg_src     *in,
              struct ureg_src     *sampler,
              struct ureg_dst     *temp,
              struct ureg_src     *constant)
{
   BLEND_GENERIC(VG_BLEND_DARKEN);
}

static INLINE void
blend_lighten( struct ureg_program *ureg,
               struct ureg_dst     *out,
               struct ureg_src     *in,
               struct ureg_src     *sampler,
               struct ureg_dst *temp,
               struct ureg_src     *constant)
{
   BLEND_GENERIC(VG_BLEND_LIGHTEN);
}

static INLINE void
blend_additive( struct ureg_program *ureg,
                struct ureg_dst *out,
                struct ureg_src *in,
                struct ureg_src *sampler,
                struct ureg_dst *temp,
                struct ureg_src *constant)
{
   BLEND_GENERIC(VG_BLEND_ADDITIVE);
}

static INLINE void
mask( struct ureg_program *ureg,
      struct ureg_dst *out,
      struct ureg_src *in,
      struct ureg_src *sampler,
      struct ureg_dst *temp,
      struct ureg_src *constant)
{
   ureg_TEX(ureg, temp[1], TGSI_TEXTURE_2D, in[0], sampler[1]);
   ureg_MUL(ureg, ureg_writemask(temp[0], TGSI_WRITEMASK_W),
            ureg_scalar(ureg_src(temp[0]), TGSI_SWIZZLE_W),
            ureg_scalar(ureg_src(temp[1]), TGSI_SWIZZLE_W));
   ureg_MOV(ureg, *out, ureg_src(temp[0]));
}

static INLINE void
premultiply( struct ureg_program *ureg,
                struct ureg_dst *out,
                struct ureg_src *in,
                struct ureg_src *sampler,
                struct ureg_dst *temp,
                struct ureg_src *constant)
{
   ureg_MUL(ureg,
            ureg_writemask(temp[0], TGSI_WRITEMASK_XYZ),
            ureg_src(temp[0]),
            ureg_scalar(ureg_src(temp[0]), TGSI_SWIZZLE_W));
}

static INLINE void
unpremultiply( struct ureg_program *ureg,
                struct ureg_dst *out,
                struct ureg_src *in,
                struct ureg_src *sampler,
                struct ureg_dst *temp,
                struct ureg_src *constant)
{
   ureg_TEX(ureg, temp[0], TGSI_TEXTURE_2D, in[0], sampler[1]);
}


static INLINE void
color_bw( struct ureg_program *ureg,
                struct ureg_dst *out,
                struct ureg_src *in,
                struct ureg_src *sampler,
                struct ureg_dst *temp,
                struct ureg_src *constant)
{
   ureg_ADD(ureg, temp[1],
            ureg_scalar(constant[3], TGSI_SWIZZLE_Y),
            ureg_scalar(constant[3], TGSI_SWIZZLE_Y));
   ureg_RCP(ureg, temp[2], ureg_src(temp[1]));
   ureg_ADD(ureg, temp[1],
            ureg_scalar(constant[3], TGSI_SWIZZLE_Y),
            ureg_src(temp[2]));
   ureg_ADD(ureg, ureg_writemask(temp[2], TGSI_WRITEMASK_X),
            ureg_scalar(ureg_src(temp[0]), TGSI_SWIZZLE_X),
            ureg_scalar(ureg_src(temp[0]), TGSI_SWIZZLE_Y));
   ureg_ADD(ureg, ureg_writemask(temp[2], TGSI_WRITEMASK_X),
            ureg_scalar(ureg_src(temp[0]), TGSI_SWIZZLE_Z),
            ureg_scalar(ureg_src(temp[0]), TGSI_SWIZZLE_X));
   ureg_SGE(ureg,
            ureg_writemask(temp[0], TGSI_WRITEMASK_XYZ),
            ureg_scalar(ureg_src(temp[2]), TGSI_SWIZZLE_X),
            ureg_src(temp[1]));
  ureg_SGE(ureg,
           ureg_writemask(temp[0], TGSI_WRITEMASK_W),
           ureg_scalar(ureg_src(temp[0]), TGSI_SWIZZLE_W),
           ureg_scalar(ureg_src(temp[2]), TGSI_SWIZZLE_Y));
  ureg_MOV(ureg, *out, ureg_src(temp[0]));
}


struct shader_asm_info {
   VGint id;
   ureg_func func;

   VGboolean needs_position;

   VGint start_const;
   VGint num_consts;

   VGint start_sampler;
   VGint num_samplers;

   VGint start_temp;
   VGint num_temps;
};


/* paint types */
static const struct shader_asm_info shaders_paint_asm[] = {
   {VEGA_SOLID_FILL_SHADER, solid_fill,
    VG_FALSE, 2, 1, 0, 0, 0, 0},
   {VEGA_LINEAR_GRADIENT_SHADER, linear_grad,
    VG_TRUE,  2, 5, 0, 1, 0, 5},
   {VEGA_RADIAL_GRADIENT_SHADER, radial_grad,
    VG_TRUE,  2, 5, 0, 1, 0, 5},
   {VEGA_PATTERN_SHADER, pattern,
    VG_TRUE,  3, 4, 0, 1, 0, 5},
   {VEGA_PAINT_DEGENERATE_SHADER, paint_degenerate,
    VG_FALSE,  3, 1, 0, 1, 0, 2}
};

/* image draw modes */
static const struct shader_asm_info shaders_image_asm[] = {
   {VEGA_IMAGE_NORMAL_SHADER, image_normal,
    VG_TRUE,  0, 0, 3, 1, 0, 2},
   {VEGA_IMAGE_MULTIPLY_SHADER, image_multiply,
    VG_TRUE,  0, 0, 3, 1, 0, 2},
   {VEGA_IMAGE_STENCIL_SHADER, image_stencil,
    VG_TRUE,  0, 0, 3, 1, 0, 2}
};

static const struct shader_asm_info shaders_color_transform_asm[] = {
   {VEGA_COLOR_TRANSFORM_SHADER, color_transform,
    VG_FALSE, 0, 4, 0, 0, 0, 3}
};

static const struct shader_asm_info shaders_alpha_asm[] = {
   {VEGA_ALPHA_NORMAL_SHADER, alpha_normal,
    VG_FALSE, 0, 0, 0, 0, 0, 2},
   {VEGA_ALPHA_PER_CHANNEL_SHADER, alpha_per_channel,
    VG_FALSE, 0, 0, 0, 0, 0, 2}
};

/* extra blend modes */
static const struct shader_asm_info shaders_blend_asm[] = {
#define BLEND_ASM_INFO(id, func) { (id), (func), VG_TRUE, 3, 1, 2, 1, 0, 5 }
   BLEND_ASM_INFO(VEGA_BLEND_SRC_SHADER, blend_src),
   BLEND_ASM_INFO(VEGA_BLEND_SRC_OVER_SHADER, blend_src_over),
   BLEND_ASM_INFO(VEGA_BLEND_DST_OVER_SHADER, blend_dst_over),
   BLEND_ASM_INFO(VEGA_BLEND_SRC_IN_SHADER, blend_src_in),
   BLEND_ASM_INFO(VEGA_BLEND_DST_IN_SHADER, blend_dst_in),
   BLEND_ASM_INFO(VEGA_BLEND_MULTIPLY_SHADER, blend_multiply),
   BLEND_ASM_INFO(VEGA_BLEND_SCREEN_SHADER, blend_screen),
   BLEND_ASM_INFO(VEGA_BLEND_DARKEN_SHADER, blend_darken),
   BLEND_ASM_INFO(VEGA_BLEND_LIGHTEN_SHADER, blend_lighten),
   BLEND_ASM_INFO(VEGA_BLEND_ADDITIVE_SHADER, blend_additive)
#undef BLEND_ASM_INFO
};

static const struct shader_asm_info shaders_mask_asm[] = {
   {VEGA_MASK_SHADER, mask,
    VG_TRUE,  0, 0, 1, 1, 0, 2}
};

/* premultiply */
static const struct shader_asm_info shaders_premultiply_asm[] = {
   {VEGA_PREMULTIPLY_SHADER, premultiply,
    VG_FALSE,  0, 0, 0, 0, 0, 1},
   {VEGA_UNPREMULTIPLY_SHADER, unpremultiply,
    VG_FALSE,  0, 0, 0, 0, 0, 1},
};

/* color transform to black and white */
static const struct shader_asm_info shaders_bw_asm[] = {
   {VEGA_BW_SHADER, color_bw,
    VG_FALSE,  3, 1, 0, 0, 0, 3},
};

#endif
