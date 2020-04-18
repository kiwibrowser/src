/*
 * Copyright © 2012 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "main/teximage.h"
#include "main/fbobject.h"

#include "glsl/ralloc.h"

#include "intel_fbo.h"

#include "brw_blorp.h"
#include "brw_context.h"
#include "brw_eu.h"
#include "brw_state.h"


/**
 * Helper function for handling mirror image blits.
 *
 * If coord0 > coord1, swap them and invert the "mirror" boolean.
 */
static inline void
fixup_mirroring(bool &mirror, GLint &coord0, GLint &coord1)
{
   if (coord0 > coord1) {
      mirror = !mirror;
      GLint tmp = coord0;
      coord0 = coord1;
      coord1 = tmp;
   }
}


/**
 * Adjust {src,dst}_x{0,1} to account for clipping and scissoring of
 * destination coordinates.
 *
 * Return true if there is still blitting to do, false if all pixels got
 * rejected by the clip and/or scissor.
 *
 * For clarity, the nomenclature of this function assumes we are clipping and
 * scissoring the X coordinate; the exact same logic applies for Y
 * coordinates.
 *
 * Note: this function may also be used to account for clipping of source
 * coordinates, by swapping the roles of src and dst.
 */
static inline bool
clip_or_scissor(bool mirror, GLint &src_x0, GLint &src_x1, GLint &dst_x0,
                GLint &dst_x1, GLint fb_xmin, GLint fb_xmax)
{
   /* If we are going to scissor everything away, stop. */
   if (!(fb_xmin < fb_xmax &&
         dst_x0 < fb_xmax &&
         fb_xmin < dst_x1 &&
         dst_x0 < dst_x1)) {
      return false;
   }

   /* Clip the destination rectangle, and keep track of how many pixels we
    * clipped off of the left and right sides of it.
    */
   GLint pixels_clipped_left = 0;
   GLint pixels_clipped_right = 0;
   if (dst_x0 < fb_xmin) {
      pixels_clipped_left = fb_xmin - dst_x0;
      dst_x0 = fb_xmin;
   }
   if (fb_xmax < dst_x1) {
      pixels_clipped_right = dst_x1 - fb_xmax;
      dst_x1 = fb_xmax;
   }

   /* If we are mirrored, then before applying pixels_clipped_{left,right} to
    * the source coordinates, we need to flip them to account for the
    * mirroring.
    */
   if (mirror) {
      GLint tmp = pixels_clipped_left;
      pixels_clipped_left = pixels_clipped_right;
      pixels_clipped_right = tmp;
   }

   /* Adjust the source rectangle to remove the pixels corresponding to those
    * that were clipped/scissored out of the destination rectangle.
    */
   src_x0 += pixels_clipped_left;
   src_x1 -= pixels_clipped_right;

   return true;
}


static struct intel_mipmap_tree *
find_miptree(GLbitfield buffer_bit, struct intel_renderbuffer *irb)
{
   struct intel_mipmap_tree *mt = irb->mt;
   if (buffer_bit == GL_STENCIL_BUFFER_BIT && mt->stencil_mt)
      mt = mt->stencil_mt;
   return mt;
}

void
brw_blorp_blit_miptrees(struct intel_context *intel,
                        struct intel_mipmap_tree *src_mt,
                        unsigned src_level, unsigned src_layer,
                        struct intel_mipmap_tree *dst_mt,
                        unsigned dst_level, unsigned dst_layer,
                        int src_x0, int src_y0,
                        int dst_x0, int dst_y0,
                        int dst_x1, int dst_y1,
                        bool mirror_x, bool mirror_y)
{
   brw_blorp_blit_params params(brw_context(&intel->ctx),
                                src_mt, src_level, src_layer,
                                dst_mt, dst_level, dst_layer,
                                src_x0, src_y0,
                                dst_x0, dst_y0,
                                dst_x1, dst_y1,
                                mirror_x, mirror_y);
   brw_blorp_exec(intel, &params);
}

static void
do_blorp_blit(struct intel_context *intel, GLbitfield buffer_bit,
              struct intel_renderbuffer *src_irb,
              struct intel_renderbuffer *dst_irb,
              GLint srcX0, GLint srcY0,
              GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
              bool mirror_x, bool mirror_y)
{
   /* Find source/dst miptrees */
   struct intel_mipmap_tree *src_mt = find_miptree(buffer_bit, src_irb);
   struct intel_mipmap_tree *dst_mt = find_miptree(buffer_bit, dst_irb);

   /* Get ready to blit.  This includes depth resolving the src and dst
    * buffers if necessary.
    */
   intel_renderbuffer_resolve_depth(intel, src_irb);
   intel_renderbuffer_resolve_depth(intel, dst_irb);

   /* Do the blit */
   brw_blorp_blit_miptrees(intel,
                           src_mt, src_irb->mt_level, src_irb->mt_layer,
                           dst_mt, dst_irb->mt_level, dst_irb->mt_layer,
                           srcX0, srcY0, dstX0, dstY0, dstX1, dstY1,
                           mirror_x, mirror_y);

   intel_renderbuffer_set_needs_hiz_resolve(dst_irb);
   intel_renderbuffer_set_needs_downsample(dst_irb);
}


static bool
formats_match(GLbitfield buffer_bit, struct intel_renderbuffer *src_irb,
              struct intel_renderbuffer *dst_irb)
{
   /* Note: don't just check gl_renderbuffer::Format, because in some cases
    * multiple gl_formats resolve to the same native type in the miptree (for
    * example MESA_FORMAT_X8_Z24 and MESA_FORMAT_S8_Z24), and we can blit
    * between those formats.
    */
   gl_format src_format = find_miptree(buffer_bit, src_irb)->format;
   gl_format dst_format = find_miptree(buffer_bit, dst_irb)->format;

   return _mesa_get_srgb_format_linear(src_format) ==
          _mesa_get_srgb_format_linear(dst_format);
}


static bool
try_blorp_blit(struct intel_context *intel,
               GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
               GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
               GLenum filter, GLbitfield buffer_bit)
{
   struct gl_context *ctx = &intel->ctx;

   /* Sync up the state of window system buffers.  We need to do this before
    * we go looking for the buffers.
    */
   intel_prepare_render(intel);

   const struct gl_framebuffer *read_fb = ctx->ReadBuffer;
   const struct gl_framebuffer *draw_fb = ctx->DrawBuffer;

   /* Detect if the blit needs to be mirrored */
   bool mirror_x = false, mirror_y = false;
   fixup_mirroring(mirror_x, srcX0, srcX1);
   fixup_mirroring(mirror_x, dstX0, dstX1);
   fixup_mirroring(mirror_y, srcY0, srcY1);
   fixup_mirroring(mirror_y, dstY0, dstY1);

   /* Make sure width and height match */
   if (srcX1 - srcX0 != dstX1 - dstX0) return false;
   if (srcY1 - srcY0 != dstY1 - dstY0) return false;

   /* If the destination rectangle needs to be clipped or scissored, do so.
    */
   if (!(clip_or_scissor(mirror_x, srcX0, srcX1, dstX0, dstX1,
                         draw_fb->_Xmin, draw_fb->_Xmax) &&
         clip_or_scissor(mirror_y, srcY0, srcY1, dstY0, dstY1,
                         draw_fb->_Ymin, draw_fb->_Ymax))) {
      /* Everything got clipped/scissored away, so the blit was successful. */
      return true;
   }

   /* If the source rectangle needs to be clipped or scissored, do so. */
   if (!(clip_or_scissor(mirror_x, dstX0, dstX1, srcX0, srcX1,
                         0, read_fb->Width) &&
         clip_or_scissor(mirror_y, dstY0, dstY1, srcY0, srcY1,
                         0, read_fb->Height))) {
      /* Everything got clipped/scissored away, so the blit was successful. */
      return true;
   }

   /* Account for the fact that in the system framebuffer, the origin is at
    * the lower left.
    */
   if (_mesa_is_winsys_fbo(read_fb)) {
      GLint tmp = read_fb->Height - srcY0;
      srcY0 = read_fb->Height - srcY1;
      srcY1 = tmp;
      mirror_y = !mirror_y;
   }
   if (_mesa_is_winsys_fbo(draw_fb)) {
      GLint tmp = draw_fb->Height - dstY0;
      dstY0 = draw_fb->Height - dstY1;
      dstY1 = tmp;
      mirror_y = !mirror_y;
   }

   /* Find buffers */
   struct intel_renderbuffer *src_irb;
   struct intel_renderbuffer *dst_irb;
   switch (buffer_bit) {
   case GL_COLOR_BUFFER_BIT:
      src_irb = intel_renderbuffer(read_fb->_ColorReadBuffer);
      for (unsigned i = 0; i < ctx->DrawBuffer->_NumColorDrawBuffers; ++i) {
         dst_irb = intel_renderbuffer(ctx->DrawBuffer->_ColorDrawBuffers[i]);
         if (dst_irb && !formats_match(buffer_bit, src_irb, dst_irb))
            return false;
      }
      for (unsigned i = 0; i < ctx->DrawBuffer->_NumColorDrawBuffers; ++i) {
         dst_irb = intel_renderbuffer(ctx->DrawBuffer->_ColorDrawBuffers[i]);
         do_blorp_blit(intel, buffer_bit, src_irb, dst_irb, srcX0, srcY0,
                       dstX0, dstY0, dstX1, dstY1, mirror_x, mirror_y);
      }
      break;
   case GL_DEPTH_BUFFER_BIT:
      src_irb =
         intel_renderbuffer(read_fb->Attachment[BUFFER_DEPTH].Renderbuffer);
      dst_irb =
         intel_renderbuffer(draw_fb->Attachment[BUFFER_DEPTH].Renderbuffer);
      if (!formats_match(buffer_bit, src_irb, dst_irb))
         return false;
      do_blorp_blit(intel, buffer_bit, src_irb, dst_irb, srcX0, srcY0,
                    dstX0, dstY0, dstX1, dstY1, mirror_x, mirror_y);
      break;
   case GL_STENCIL_BUFFER_BIT:
      src_irb =
         intel_renderbuffer(read_fb->Attachment[BUFFER_STENCIL].Renderbuffer);
      dst_irb =
         intel_renderbuffer(draw_fb->Attachment[BUFFER_STENCIL].Renderbuffer);
      if (!formats_match(buffer_bit, src_irb, dst_irb))
         return false;
      do_blorp_blit(intel, buffer_bit, src_irb, dst_irb, srcX0, srcY0,
                    dstX0, dstY0, dstX1, dstY1, mirror_x, mirror_y);
      break;
   default:
      assert(false);
   }

   return true;
}

GLbitfield
brw_blorp_framebuffer(struct intel_context *intel,
                      GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
                      GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
                      GLbitfield mask, GLenum filter)
{
   /* BLORP is not supported before Gen6. */
   if (intel->gen < 6)
      return mask;

   static GLbitfield buffer_bits[] = {
      GL_COLOR_BUFFER_BIT,
      GL_DEPTH_BUFFER_BIT,
      GL_STENCIL_BUFFER_BIT,
   };

   for (unsigned int i = 0; i < ARRAY_SIZE(buffer_bits); ++i) {
      if ((mask & buffer_bits[i]) &&
       try_blorp_blit(intel,
                      srcX0, srcY0, srcX1, srcY1,
                      dstX0, dstY0, dstX1, dstY1,
                      filter, buffer_bits[i])) {
         mask &= ~buffer_bits[i];
      }
   }

   return mask;
}


/**
 * Enum to specify the order of arguments in a sampler message
 */
enum sampler_message_arg
{
   SAMPLER_MESSAGE_ARG_U_FLOAT,
   SAMPLER_MESSAGE_ARG_V_FLOAT,
   SAMPLER_MESSAGE_ARG_U_INT,
   SAMPLER_MESSAGE_ARG_V_INT,
   SAMPLER_MESSAGE_ARG_SI_INT,
   SAMPLER_MESSAGE_ARG_MCS_INT,
   SAMPLER_MESSAGE_ARG_ZERO_INT,
};

/**
 * Generator for WM programs used in BLORP blits.
 *
 * The bulk of the work done by the WM program is to wrap and unwrap the
 * coordinate transformations used by the hardware to store surfaces in
 * memory.  The hardware transforms a pixel location (X, Y, S) (where S is the
 * sample index for a multisampled surface) to a memory offset by the
 * following formulas:
 *
 *   offset = tile(tiling_format, encode_msaa(num_samples, layout, X, Y, S))
 *   (X, Y, S) = decode_msaa(num_samples, layout, detile(tiling_format, offset))
 *
 * For a single-sampled surface, or for a multisampled surface using
 * INTEL_MSAA_LAYOUT_UMS, encode_msaa() and decode_msaa are the identity
 * function:
 *
 *   encode_msaa(1, NONE, X, Y, 0) = (X, Y, 0)
 *   decode_msaa(1, NONE, X, Y, 0) = (X, Y, 0)
 *   encode_msaa(n, UMS, X, Y, S) = (X, Y, S)
 *   decode_msaa(n, UMS, X, Y, S) = (X, Y, S)
 *
 * For a 4x multisampled surface using INTEL_MSAA_LAYOUT_IMS, encode_msaa()
 * embeds the sample number into bit 1 of the X and Y coordinates:
 *
 *   encode_msaa(4, IMS, X, Y, S) = (X', Y', 0)
 *     where X' = (X & ~0b1) << 1 | (S & 0b1) << 1 | (X & 0b1)
 *           Y' = (Y & ~0b1 ) << 1 | (S & 0b10) | (Y & 0b1)
 *   decode_msaa(4, IMS, X, Y, 0) = (X', Y', S)
 *     where X' = (X & ~0b11) >> 1 | (X & 0b1)
 *           Y' = (Y & ~0b11) >> 1 | (Y & 0b1)
 *           S = (Y & 0b10) | (X & 0b10) >> 1
 *
 * For an 8x multisampled surface using INTEL_MSAA_LAYOUT_IMS, encode_msaa()
 * embeds the sample number into bits 1 and 2 of the X coordinate and bit 1 of
 * the Y coordinate:
 *
 *   encode_msaa(8, IMS, X, Y, S) = (X', Y', 0)
 *     where X' = (X & ~0b1) << 2 | (S & 0b100) | (S & 0b1) << 1 | (X & 0b1)
 *           Y' = (Y & ~0b1) << 1 | (S & 0b10) | (Y & 0b1)
 *   decode_msaa(8, IMS, X, Y, 0) = (X', Y', S)
 *     where X' = (X & ~0b111) >> 2 | (X & 0b1)
 *           Y' = (Y & ~0b11) >> 1 | (Y & 0b1)
 *           S = (X & 0b100) | (Y & 0b10) | (X & 0b10) >> 1
 *
 * For X tiling, tile() combines together the low-order bits of the X and Y
 * coordinates in the pattern 0byyyxxxxxxxxx, creating 4k tiles that are 512
 * bytes wide and 8 rows high:
 *
 *   tile(x_tiled, X, Y, S) = A
 *     where A = tile_num << 12 | offset
 *           tile_num = (Y' >> 3) * tile_pitch + (X' >> 9)
 *           offset = (Y' & 0b111) << 9
 *                    | (X & 0b111111111)
 *           X' = X * cpp
 *           Y' = Y + S * qpitch
 *   detile(x_tiled, A) = (X, Y, S)
 *     where X = X' / cpp
 *           Y = Y' % qpitch
 *           S = Y' / qpitch
 *           Y' = (tile_num / tile_pitch) << 3
 *                | (A & 0b111000000000) >> 9
 *           X' = (tile_num % tile_pitch) << 9
 *                | (A & 0b111111111)
 *
 * (In all tiling formulas, cpp is the number of bytes occupied by a single
 * sample ("chars per pixel"), tile_pitch is the number of 4k tiles required
 * to fill the width of the surface, and qpitch is the spacing (in rows)
 * between array slices).
 *
 * For Y tiling, tile() combines together the low-order bits of the X and Y
 * coordinates in the pattern 0bxxxyyyyyxxxx, creating 4k tiles that are 128
 * bytes wide and 32 rows high:
 *
 *   tile(y_tiled, X, Y, S) = A
 *     where A = tile_num << 12 | offset
 *           tile_num = (Y' >> 5) * tile_pitch + (X' >> 7)
 *           offset = (X' & 0b1110000) << 5
 *                    | (Y' & 0b11111) << 4
 *                    | (X' & 0b1111)
 *           X' = X * cpp
 *           Y' = Y + S * qpitch
 *   detile(y_tiled, A) = (X, Y, S)
 *     where X = X' / cpp
 *           Y = Y' % qpitch
 *           S = Y' / qpitch
 *           Y' = (tile_num / tile_pitch) << 5
 *                | (A & 0b111110000) >> 4
 *           X' = (tile_num % tile_pitch) << 7
 *                | (A & 0b111000000000) >> 5
 *                | (A & 0b1111)
 *
 * For W tiling, tile() combines together the low-order bits of the X and Y
 * coordinates in the pattern 0bxxxyyyyxyxyx, creating 4k tiles that are 64
 * bytes wide and 64 rows high (note that W tiling is only used for stencil
 * buffers, which always have cpp = 1 and S=0):
 *
 *   tile(w_tiled, X, Y, S) = A
 *     where A = tile_num << 12 | offset
 *           tile_num = (Y' >> 6) * tile_pitch + (X' >> 6)
 *           offset = (X' & 0b111000) << 6
 *                    | (Y' & 0b111100) << 3
 *                    | (X' & 0b100) << 2
 *                    | (Y' & 0b10) << 2
 *                    | (X' & 0b10) << 1
 *                    | (Y' & 0b1) << 1
 *                    | (X' & 0b1)
 *           X' = X * cpp = X
 *           Y' = Y + S * qpitch
 *   detile(w_tiled, A) = (X, Y, S)
 *     where X = X' / cpp = X'
 *           Y = Y' % qpitch = Y'
 *           S = Y / qpitch = 0
 *           Y' = (tile_num / tile_pitch) << 6
 *                | (A & 0b111100000) >> 3
 *                | (A & 0b1000) >> 2
 *                | (A & 0b10) >> 1
 *           X' = (tile_num % tile_pitch) << 6
 *                | (A & 0b111000000000) >> 6
 *                | (A & 0b10000) >> 2
 *                | (A & 0b100) >> 1
 *                | (A & 0b1)
 *
 * Finally, for a non-tiled surface, tile() simply combines together the X and
 * Y coordinates in the natural way:
 *
 *   tile(untiled, X, Y, S) = A
 *     where A = Y * pitch + X'
 *           X' = X * cpp
 *           Y' = Y + S * qpitch
 *   detile(untiled, A) = (X, Y, S)
 *     where X = X' / cpp
 *           Y = Y' % qpitch
 *           S = Y' / qpitch
 *           X' = A % pitch
 *           Y' = A / pitch
 *
 * (In these formulas, pitch is the number of bytes occupied by a single row
 * of samples).
 */
class brw_blorp_blit_program
{
public:
   brw_blorp_blit_program(struct brw_context *brw,
                          const brw_blorp_blit_prog_key *key);
   ~brw_blorp_blit_program();

   const GLuint *compile(struct brw_context *brw, GLuint *program_size);

   brw_blorp_prog_data prog_data;

private:
   void alloc_regs();
   void alloc_push_const_regs(int base_reg);
   void compute_frag_coords();
   void translate_tiling(bool old_tiled_w, bool new_tiled_w);
   void encode_msaa(unsigned num_samples, intel_msaa_layout layout);
   void decode_msaa(unsigned num_samples, intel_msaa_layout layout);
   void kill_if_outside_dst_rect();
   void translate_dst_to_src();
   void single_to_blend();
   void manual_blend(unsigned num_samples);
   void sample(struct brw_reg dst);
   void texel_fetch(struct brw_reg dst);
   void mcs_fetch();
   void expand_to_32_bits(struct brw_reg src, struct brw_reg dst);
   void texture_lookup(struct brw_reg dst, GLuint msg_type,
                       const sampler_message_arg *args, int num_args);
   void render_target_write();

   /**
    * Base-2 logarithm of the maximum number of samples that can be blended.
    */
   static const unsigned LOG2_MAX_BLEND_SAMPLES = 3;

   void *mem_ctx;
   struct brw_context *brw;
   const brw_blorp_blit_prog_key *key;
   struct brw_compile func;

   /* Thread dispatch header */
   struct brw_reg R0;

   /* Pixel X/Y coordinates (always in R1). */
   struct brw_reg R1;

   /* Push constants */
   struct brw_reg dst_x0;
   struct brw_reg dst_x1;
   struct brw_reg dst_y0;
   struct brw_reg dst_y1;
   struct {
      struct brw_reg multiplier;
      struct brw_reg offset;
   } x_transform, y_transform;

   /* Data read from texture (4 vec16's per array element) */
   struct brw_reg texture_data[LOG2_MAX_BLEND_SAMPLES + 1];

   /* Auxiliary storage for the contents of the MCS surface.
    *
    * Since the sampler always returns 8 registers worth of data, this is 8
    * registers wide, even though we only use the first 2 registers of it.
    */
   struct brw_reg mcs_data;

   /* X coordinates.  We have two of them so that we can perform coordinate
    * transformations easily.
    */
   struct brw_reg x_coords[2];

   /* Y coordinates.  We have two of them so that we can perform coordinate
    * transformations easily.
    */
   struct brw_reg y_coords[2];

   /* Which element of x_coords and y_coords is currently in use.
    */
   int xy_coord_index;

   /* True if, at the point in the program currently being compiled, the
    * sample index is known to be zero.
    */
   bool s_is_zero;

   /* Register storing the sample index when s_is_zero is false. */
   struct brw_reg sample_index;

   /* Temporaries */
   struct brw_reg t1;
   struct brw_reg t2;

   /* MRF used for sampling and render target writes */
   GLuint base_mrf;
};

brw_blorp_blit_program::brw_blorp_blit_program(
      struct brw_context *brw,
      const brw_blorp_blit_prog_key *key)
   : mem_ctx(ralloc_context(NULL)),
     brw(brw),
     key(key)
{
   brw_init_compile(brw, &func, mem_ctx);
}

brw_blorp_blit_program::~brw_blorp_blit_program()
{
   ralloc_free(mem_ctx);
}

const GLuint *
brw_blorp_blit_program::compile(struct brw_context *brw,
                                GLuint *program_size)
{
   /* Sanity checks */
   if (key->dst_tiled_w && key->rt_samples > 0) {
      /* If the destination image is W tiled and multisampled, then the thread
       * must be dispatched once per sample, not once per pixel.  This is
       * necessary because after conversion between W and Y tiling, there's no
       * guarantee that all samples corresponding to a single pixel will still
       * be together.
       */
      assert(key->persample_msaa_dispatch);
   }

   if (key->blend) {
      /* We are blending, which means we won't have an opportunity to
       * translate the tiling and sample count for the texture surface.  So
       * the surface state for the texture must be configured with the correct
       * tiling and sample count.
       */
      assert(!key->src_tiled_w);
      assert(key->tex_samples == key->src_samples);
      assert(key->tex_layout == key->src_layout);
      assert(key->tex_samples > 0);
   }

   if (key->persample_msaa_dispatch) {
      /* It only makes sense to do persample dispatch if the render target is
       * configured as multisampled.
       */
      assert(key->rt_samples > 0);
   }

   /* Make sure layout is consistent with sample count */
   assert((key->tex_layout == INTEL_MSAA_LAYOUT_NONE) ==
          (key->tex_samples == 0));
   assert((key->rt_layout == INTEL_MSAA_LAYOUT_NONE) ==
          (key->rt_samples == 0));
   assert((key->src_layout == INTEL_MSAA_LAYOUT_NONE) ==
          (key->src_samples == 0));
   assert((key->dst_layout == INTEL_MSAA_LAYOUT_NONE) ==
          (key->dst_samples == 0));

   /* Set up prog_data */
   memset(&prog_data, 0, sizeof(prog_data));
   prog_data.persample_msaa_dispatch = key->persample_msaa_dispatch;

   brw_set_compression_control(&func, BRW_COMPRESSION_NONE);

   alloc_regs();
   compute_frag_coords();

   /* Render target and texture hardware don't support W tiling. */
   const bool rt_tiled_w = false;
   const bool tex_tiled_w = false;

   /* The address that data will be written to is determined by the
    * coordinates supplied to the WM thread and the tiling and sample count of
    * the render target, according to the formula:
    *
    * (X, Y, S) = decode_msaa(rt_samples, detile(rt_tiling, offset))
    *
    * If the actual tiling and sample count of the destination surface are not
    * the same as the configuration of the render target, then these
    * coordinates are wrong and we have to adjust them to compensate for the
    * difference.
    */
   if (rt_tiled_w != key->dst_tiled_w ||
       key->rt_samples != key->dst_samples ||
       key->rt_layout != key->dst_layout) {
      encode_msaa(key->rt_samples, key->rt_layout);
      /* Now (X, Y, S) = detile(rt_tiling, offset) */
      translate_tiling(rt_tiled_w, key->dst_tiled_w);
      /* Now (X, Y, S) = detile(dst_tiling, offset) */
      decode_msaa(key->dst_samples, key->dst_layout);
   }

   /* Now (X, Y, S) = decode_msaa(dst_samples, detile(dst_tiling, offset)).
    *
    * That is: X, Y and S now contain the true coordinates and sample index of
    * the data that the WM thread should output.
    *
    * If we need to kill pixels that are outside the destination rectangle,
    * now is the time to do it.
    */

   if (key->use_kill)
      kill_if_outside_dst_rect();

   /* Next, apply a translation to obtain coordinates in the source image. */
   translate_dst_to_src();

   /* If the source image is not multisampled, then we want to fetch sample
    * number 0, because that's the only sample there is.
    */
   if (key->src_samples == 0)
      s_is_zero = true;

   /* X, Y, and S are now the coordinates of the pixel in the source image
    * that we want to texture from.  Exception: if we are blending, then S is
    * irrelevant, because we are going to fetch all samples.
    */
   if (key->blend) {
      if (brw->intel.gen == 6) {
         /* Gen6 hardware an automatically blend using the SAMPLE message */
         single_to_blend();
         sample(texture_data[0]);
      } else {
         /* Gen7+ hardware doesn't automaticaly blend. */
         manual_blend(key->src_samples);
      }
   } else {
      /* We aren't blending, which means we just want to fetch a single sample
       * from the source surface.  The address that we want to fetch from is
       * related to the X, Y and S values according to the formula:
       *
       * (X, Y, S) = decode_msaa(src_samples, detile(src_tiling, offset)).
       *
       * If the actual tiling and sample count of the source surface are not
       * the same as the configuration of the texture, then we need to adjust
       * the coordinates to compensate for the difference.
       */
      if (tex_tiled_w != key->src_tiled_w ||
          key->tex_samples != key->src_samples ||
          key->tex_layout != key->src_layout) {
         encode_msaa(key->src_samples, key->src_layout);
         /* Now (X, Y, S) = detile(src_tiling, offset) */
         translate_tiling(key->src_tiled_w, tex_tiled_w);
         /* Now (X, Y, S) = detile(tex_tiling, offset) */
         decode_msaa(key->tex_samples, key->tex_layout);
      }

      /* Now (X, Y, S) = decode_msaa(tex_samples, detile(tex_tiling, offset)).
       *
       * In other words: X, Y, and S now contain values which, when passed to
       * the texturing unit, will cause data to be read from the correct
       * memory location.  So we can fetch the texel now.
       */
      if (key->tex_layout == INTEL_MSAA_LAYOUT_CMS)
         mcs_fetch();
      texel_fetch(texture_data[0]);
   }

   /* Finally, write the fetched (or blended) value to the render target and
    * terminate the thread.
    */
   render_target_write();
   return brw_get_program(&func, program_size);
}

void
brw_blorp_blit_program::alloc_push_const_regs(int base_reg)
{
#define CONST_LOC(name) offsetof(brw_blorp_wm_push_constants, name)
#define ALLOC_REG(name) \
   this->name = \
      brw_uw1_reg(BRW_GENERAL_REGISTER_FILE, base_reg, CONST_LOC(name) / 2)

   ALLOC_REG(dst_x0);
   ALLOC_REG(dst_x1);
   ALLOC_REG(dst_y0);
   ALLOC_REG(dst_y1);
   ALLOC_REG(x_transform.multiplier);
   ALLOC_REG(x_transform.offset);
   ALLOC_REG(y_transform.multiplier);
   ALLOC_REG(y_transform.offset);
#undef CONST_LOC
#undef ALLOC_REG
}

void
brw_blorp_blit_program::alloc_regs()
{
   int reg = 0;
   this->R0 = retype(brw_vec8_grf(reg++, 0), BRW_REGISTER_TYPE_UW);
   this->R1 = retype(brw_vec8_grf(reg++, 0), BRW_REGISTER_TYPE_UW);
   prog_data.first_curbe_grf = reg;
   alloc_push_const_regs(reg);
   reg += BRW_BLORP_NUM_PUSH_CONST_REGS;
   for (unsigned i = 0; i < ARRAY_SIZE(texture_data); ++i) {
      this->texture_data[i] =
         retype(vec16(brw_vec8_grf(reg, 0)), key->texture_data_type);
      reg += 8;
   }
   this->mcs_data =
      retype(brw_vec8_grf(reg, 0), BRW_REGISTER_TYPE_UD); reg += 8;
   for (int i = 0; i < 2; ++i) {
      this->x_coords[i]
         = vec16(retype(brw_vec8_grf(reg++, 0), BRW_REGISTER_TYPE_UW));
      this->y_coords[i]
         = vec16(retype(brw_vec8_grf(reg++, 0), BRW_REGISTER_TYPE_UW));
   }
   this->xy_coord_index = 0;
   this->sample_index
      = vec16(retype(brw_vec8_grf(reg++, 0), BRW_REGISTER_TYPE_UW));
   this->t1 = vec16(retype(brw_vec8_grf(reg++, 0), BRW_REGISTER_TYPE_UW));
   this->t2 = vec16(retype(brw_vec8_grf(reg++, 0), BRW_REGISTER_TYPE_UW));

   /* Make sure we didn't run out of registers */
   assert(reg <= GEN7_MRF_HACK_START);

   int mrf = 2;
   this->base_mrf = mrf;
}

/* In the code that follows, X and Y can be used to quickly refer to the
 * active elements of x_coords and y_coords, and Xp and Yp ("X prime" and "Y
 * prime") to the inactive elements.
 *
 * S can be used to quickly refer to sample_index.
 */
#define X x_coords[xy_coord_index]
#define Y y_coords[xy_coord_index]
#define Xp x_coords[!xy_coord_index]
#define Yp y_coords[!xy_coord_index]
#define S sample_index

/* Quickly swap the roles of (X, Y) and (Xp, Yp).  Saves us from having to do
 * MOVs to transfor (Xp, Yp) to (X, Y) after a coordinate transformation.
 */
#define SWAP_XY_AND_XPYP() xy_coord_index = !xy_coord_index;

/**
 * Emit code to compute the X and Y coordinates of the pixels being rendered
 * by this WM invocation.
 *
 * Assuming the render target is set up for Y tiling, these (X, Y) values are
 * related to the address offset where outputs will be written by the formula:
 *
 *   (X, Y, S) = decode_msaa(detile(offset)).
 *
 * (See brw_blorp_blit_program).
 */
void
brw_blorp_blit_program::compute_frag_coords()
{
   /* R1.2[15:0] = X coordinate of upper left pixel of subspan 0 (pixel 0)
    * R1.3[15:0] = X coordinate of upper left pixel of subspan 1 (pixel 4)
    * R1.4[15:0] = X coordinate of upper left pixel of subspan 2 (pixel 8)
    * R1.5[15:0] = X coordinate of upper left pixel of subspan 3 (pixel 12)
    *
    * Pixels within a subspan are laid out in this arrangement:
    * 0 1
    * 2 3
    *
    * So, to compute the coordinates of each pixel, we need to read every 2nd
    * 16-bit value (vstride=2) from R1, starting at the 4th 16-bit value
    * (suboffset=4), and duplicate each value 4 times (hstride=0, width=4).
    * In other words, the data we want to access is R1.4<2;4,0>UW.
    *
    * Then, we need to add the repeating sequence (0, 1, 0, 1, ...) to the
    * result, since pixels n+1 and n+3 are in the right half of the subspan.
    */
   brw_ADD(&func, X, stride(suboffset(R1, 4), 2, 4, 0), brw_imm_v(0x10101010));

   /* Similarly, Y coordinates for subspans come from R1.2[31:16] through
    * R1.5[31:16], so to get pixel Y coordinates we need to start at the 5th
    * 16-bit value instead of the 4th (R1.5<2;4,0>UW instead of
    * R1.4<2;4,0>UW).
    *
    * And we need to add the repeating sequence (0, 0, 1, 1, ...), since
    * pixels n+2 and n+3 are in the bottom half of the subspan.
    */
   brw_ADD(&func, Y, stride(suboffset(R1, 5), 2, 4, 0), brw_imm_v(0x11001100));

   if (key->persample_msaa_dispatch) {
      switch (key->rt_samples) {
      case 4:
         /* The WM will be run in MSDISPMODE_PERSAMPLE with num_samples == 4.
          * Therefore, subspan 0 will represent sample 0, subspan 1 will
          * represent sample 1, and so on.
          *
          * So we need to populate S with the sequence (0, 0, 0, 0, 1, 1, 1,
          * 1, 2, 2, 2, 2, 3, 3, 3, 3).  The easiest way to do this is to
          * populate a temporary variable with the sequence (0, 1, 2, 3), and
          * then copy from it using vstride=1, width=4, hstride=0.
          */
         brw_MOV(&func, t1, brw_imm_v(0x3210));
         brw_MOV(&func, S, stride(t1, 1, 4, 0));
         break;
      case 8: {
         /* The WM will be run in MSDISPMODE_PERSAMPLE with num_samples == 8.
          * Therefore, subspan 0 will represent sample N (where N is 0 or 4),
          * subspan 1 will represent sample 1, and so on.  We can find the
          * value of N by looking at R0.0 bits 7:6 ("Starting Sample Pair
          * Index") and multiplying by two (since samples are always delivered
          * in pairs).  That is, we compute 2*((R0.0 & 0xc0) >> 6) == (R0.0 &
          * 0xc0) >> 5.
          *
          * Then we need to add N to the sequence (0, 0, 0, 0, 1, 1, 1, 1, 2,
          * 2, 2, 2, 3, 3, 3, 3), which we compute by populating a temporary
          * variable with the sequence (0, 1, 2, 3), and then reading from it
          * using vstride=1, width=4, hstride=0.
          */
         struct brw_reg t1_ud1 = vec1(retype(t1, BRW_REGISTER_TYPE_UD));
         struct brw_reg r0_ud1 = vec1(retype(R0, BRW_REGISTER_TYPE_UD));
         brw_AND(&func, t1_ud1, r0_ud1, brw_imm_ud(0xc0));
         brw_SHR(&func, t1_ud1, t1_ud1, brw_imm_ud(5));
         brw_MOV(&func, t2, brw_imm_v(0x3210));
         brw_ADD(&func, S, retype(t1_ud1, BRW_REGISTER_TYPE_UW),
                 stride(t2, 1, 4, 0));
         break;
      }
      default:
         assert(!"Unrecognized sample count in "
                "brw_blorp_blit_program::compute_frag_coords()");
         break;
      }
      s_is_zero = false;
   } else {
      /* Either the destination surface is single-sampled, or the WM will be
       * run in MSDISPMODE_PERPIXEL (which causes a single fragment dispatch
       * per pixel).  In either case, it's not meaningful to compute a sample
       * value.  Just set it to 0.
       */
      s_is_zero = true;
   }
}

/**
 * Emit code to compensate for the difference between Y and W tiling.
 *
 * This code modifies the X and Y coordinates according to the formula:
 *
 *   (X', Y', S') = detile(new_tiling, tile(old_tiling, X, Y, S))
 *
 * (See brw_blorp_blit_program).
 *
 * It can only translate between W and Y tiling, so new_tiling and old_tiling
 * are booleans where true represents W tiling and false represents Y tiling.
 */
void
brw_blorp_blit_program::translate_tiling(bool old_tiled_w, bool new_tiled_w)
{
   if (old_tiled_w == new_tiled_w)
      return;

   /* In the code that follows, we can safely assume that S = 0, because W
    * tiling formats always use IMS layout.
    */
   assert(s_is_zero);

   if (new_tiled_w) {
      /* Given X and Y coordinates that describe an address using Y tiling,
       * translate to the X and Y coordinates that describe the same address
       * using W tiling.
       *
       * If we break down the low order bits of X and Y, using a
       * single letter to represent each low-order bit:
       *
       *   X = A << 7 | 0bBCDEFGH
       *   Y = J << 5 | 0bKLMNP                                       (1)
       *
       * Then we can apply the Y tiling formula to see the memory offset being
       * addressed:
       *
       *   offset = (J * tile_pitch + A) << 12 | 0bBCDKLMNPEFGH       (2)
       *
       * If we apply the W detiling formula to this memory location, that the
       * corresponding X' and Y' coordinates are:
       *
       *   X' = A << 6 | 0bBCDPFH                                     (3)
       *   Y' = J << 6 | 0bKLMNEG
       *
       * Combining (1) and (3), we see that to transform (X, Y) to (X', Y'),
       * we need to make the following computation:
       *
       *   X' = (X & ~0b1011) >> 1 | (Y & 0b1) << 2 | X & 0b1         (4)
       *   Y' = (Y & ~0b1) << 1 | (X & 0b1000) >> 2 | (X & 0b10) >> 1
       */
      brw_AND(&func, t1, X, brw_imm_uw(0xfff4)); /* X & ~0b1011 */
      brw_SHR(&func, t1, t1, brw_imm_uw(1)); /* (X & ~0b1011) >> 1 */
      brw_AND(&func, t2, Y, brw_imm_uw(1)); /* Y & 0b1 */
      brw_SHL(&func, t2, t2, brw_imm_uw(2)); /* (Y & 0b1) << 2 */
      brw_OR(&func, t1, t1, t2); /* (X & ~0b1011) >> 1 | (Y & 0b1) << 2 */
      brw_AND(&func, t2, X, brw_imm_uw(1)); /* X & 0b1 */
      brw_OR(&func, Xp, t1, t2);
      brw_AND(&func, t1, Y, brw_imm_uw(0xfffe)); /* Y & ~0b1 */
      brw_SHL(&func, t1, t1, brw_imm_uw(1)); /* (Y & ~0b1) << 1 */
      brw_AND(&func, t2, X, brw_imm_uw(8)); /* X & 0b1000 */
      brw_SHR(&func, t2, t2, brw_imm_uw(2)); /* (X & 0b1000) >> 2 */
      brw_OR(&func, t1, t1, t2); /* (Y & ~0b1) << 1 | (X & 0b1000) >> 2 */
      brw_AND(&func, t2, X, brw_imm_uw(2)); /* X & 0b10 */
      brw_SHR(&func, t2, t2, brw_imm_uw(1)); /* (X & 0b10) >> 1 */
      brw_OR(&func, Yp, t1, t2);
      SWAP_XY_AND_XPYP();
   } else {
      /* Applying the same logic as above, but in reverse, we obtain the
       * formulas:
       *
       * X' = (X & ~0b101) << 1 | (Y & 0b10) << 2 | (Y & 0b1) << 1 | X & 0b1
       * Y' = (Y & ~0b11) >> 1 | (X & 0b100) >> 2
       */
      brw_AND(&func, t1, X, brw_imm_uw(0xfffa)); /* X & ~0b101 */
      brw_SHL(&func, t1, t1, brw_imm_uw(1)); /* (X & ~0b101) << 1 */
      brw_AND(&func, t2, Y, brw_imm_uw(2)); /* Y & 0b10 */
      brw_SHL(&func, t2, t2, brw_imm_uw(2)); /* (Y & 0b10) << 2 */
      brw_OR(&func, t1, t1, t2); /* (X & ~0b101) << 1 | (Y & 0b10) << 2 */
      brw_AND(&func, t2, Y, brw_imm_uw(1)); /* Y & 0b1 */
      brw_SHL(&func, t2, t2, brw_imm_uw(1)); /* (Y & 0b1) << 1 */
      brw_OR(&func, t1, t1, t2); /* (X & ~0b101) << 1 | (Y & 0b10) << 2
                                    | (Y & 0b1) << 1 */
      brw_AND(&func, t2, X, brw_imm_uw(1)); /* X & 0b1 */
      brw_OR(&func, Xp, t1, t2);
      brw_AND(&func, t1, Y, brw_imm_uw(0xfffc)); /* Y & ~0b11 */
      brw_SHR(&func, t1, t1, brw_imm_uw(1)); /* (Y & ~0b11) >> 1 */
      brw_AND(&func, t2, X, brw_imm_uw(4)); /* X & 0b100 */
      brw_SHR(&func, t2, t2, brw_imm_uw(2)); /* (X & 0b100) >> 2 */
      brw_OR(&func, Yp, t1, t2);
      SWAP_XY_AND_XPYP();
   }
}

/**
 * Emit code to compensate for the difference between MSAA and non-MSAA
 * surfaces.
 *
 * This code modifies the X and Y coordinates according to the formula:
 *
 *   (X', Y', S') = encode_msaa(num_samples, IMS, X, Y, S)
 *
 * (See brw_blorp_blit_program).
 */
void
brw_blorp_blit_program::encode_msaa(unsigned num_samples,
                                    intel_msaa_layout layout)
{
   switch (layout) {
   case INTEL_MSAA_LAYOUT_NONE:
      /* No translation necessary, and S should already be zero. */
      assert(s_is_zero);
      break;
   case INTEL_MSAA_LAYOUT_CMS:
      /* We can't compensate for compressed layout since at this point in the
       * program we haven't read from the MCS buffer.
       */
      assert(!"Bad layout in encode_msaa");
      break;
   case INTEL_MSAA_LAYOUT_UMS:
      /* No translation necessary. */
      break;
   case INTEL_MSAA_LAYOUT_IMS:
      switch (num_samples) {
      case 4:
         /* encode_msaa(4, IMS, X, Y, S) = (X', Y', 0)
          *   where X' = (X & ~0b1) << 1 | (S & 0b1) << 1 | (X & 0b1)
          *         Y' = (Y & ~0b1) << 1 | (S & 0b10) | (Y & 0b1)
          */
         brw_AND(&func, t1, X, brw_imm_uw(0xfffe)); /* X & ~0b1 */
         if (!s_is_zero) {
            brw_AND(&func, t2, S, brw_imm_uw(1)); /* S & 0b1 */
            brw_OR(&func, t1, t1, t2); /* (X & ~0b1) | (S & 0b1) */
         }
         brw_SHL(&func, t1, t1, brw_imm_uw(1)); /* (X & ~0b1) << 1
                                                   | (S & 0b1) << 1 */
         brw_AND(&func, t2, X, brw_imm_uw(1)); /* X & 0b1 */
         brw_OR(&func, Xp, t1, t2);
         brw_AND(&func, t1, Y, brw_imm_uw(0xfffe)); /* Y & ~0b1 */
         brw_SHL(&func, t1, t1, brw_imm_uw(1)); /* (Y & ~0b1) << 1 */
         if (!s_is_zero) {
            brw_AND(&func, t2, S, brw_imm_uw(2)); /* S & 0b10 */
            brw_OR(&func, t1, t1, t2); /* (Y & ~0b1) << 1 | (S & 0b10) */
         }
         brw_AND(&func, t2, Y, brw_imm_uw(1)); /* Y & 0b1 */
         brw_OR(&func, Yp, t1, t2);
         break;
      case 8:
         /* encode_msaa(8, IMS, X, Y, S) = (X', Y', 0)
          *   where X' = (X & ~0b1) << 2 | (S & 0b100) | (S & 0b1) << 1
          *              | (X & 0b1)
          *         Y' = (Y & ~0b1) << 1 | (S & 0b10) | (Y & 0b1)
          */
         brw_AND(&func, t1, X, brw_imm_uw(0xfffe)); /* X & ~0b1 */
         brw_SHL(&func, t1, t1, brw_imm_uw(2)); /* (X & ~0b1) << 2 */
         if (!s_is_zero) {
            brw_AND(&func, t2, S, brw_imm_uw(4)); /* S & 0b100 */
            brw_OR(&func, t1, t1, t2); /* (X & ~0b1) << 2 | (S & 0b100) */
            brw_AND(&func, t2, S, brw_imm_uw(1)); /* S & 0b1 */
            brw_SHL(&func, t2, t2, brw_imm_uw(1)); /* (S & 0b1) << 1 */
            brw_OR(&func, t1, t1, t2); /* (X & ~0b1) << 2 | (S & 0b100)
                                          | (S & 0b1) << 1 */
         }
         brw_AND(&func, t2, X, brw_imm_uw(1)); /* X & 0b1 */
         brw_OR(&func, Xp, t1, t2);
         brw_AND(&func, t1, Y, brw_imm_uw(0xfffe)); /* Y & ~0b1 */
         brw_SHL(&func, t1, t1, brw_imm_uw(1)); /* (Y & ~0b1) << 1 */
         if (!s_is_zero) {
            brw_AND(&func, t2, S, brw_imm_uw(2)); /* S & 0b10 */
            brw_OR(&func, t1, t1, t2); /* (Y & ~0b1) << 1 | (S & 0b10) */
         }
         brw_AND(&func, t2, Y, brw_imm_uw(1)); /* Y & 0b1 */
         brw_OR(&func, Yp, t1, t2);
         break;
      }
      SWAP_XY_AND_XPYP();
      s_is_zero = true;
      break;
   }
}

/**
 * Emit code to compensate for the difference between MSAA and non-MSAA
 * surfaces.
 *
 * This code modifies the X and Y coordinates according to the formula:
 *
 *   (X', Y', S) = decode_msaa(num_samples, IMS, X, Y, S)
 *
 * (See brw_blorp_blit_program).
 */
void
brw_blorp_blit_program::decode_msaa(unsigned num_samples,
                                    intel_msaa_layout layout)
{
   switch (layout) {
   case INTEL_MSAA_LAYOUT_NONE:
      /* No translation necessary, and S should already be zero. */
      assert(s_is_zero);
      break;
   case INTEL_MSAA_LAYOUT_CMS:
      /* We can't compensate for compressed layout since at this point in the
       * program we don't have access to the MCS buffer.
       */
      assert(!"Bad layout in encode_msaa");
      break;
   case INTEL_MSAA_LAYOUT_UMS:
      /* No translation necessary. */
      break;
   case INTEL_MSAA_LAYOUT_IMS:
      assert(s_is_zero);
      switch (num_samples) {
      case 4:
         /* decode_msaa(4, IMS, X, Y, 0) = (X', Y', S)
          *   where X' = (X & ~0b11) >> 1 | (X & 0b1)
          *         Y' = (Y & ~0b11) >> 1 | (Y & 0b1)
          *         S = (Y & 0b10) | (X & 0b10) >> 1
          */
         brw_AND(&func, t1, X, brw_imm_uw(0xfffc)); /* X & ~0b11 */
         brw_SHR(&func, t1, t1, brw_imm_uw(1)); /* (X & ~0b11) >> 1 */
         brw_AND(&func, t2, X, brw_imm_uw(1)); /* X & 0b1 */
         brw_OR(&func, Xp, t1, t2);
         brw_AND(&func, t1, Y, brw_imm_uw(0xfffc)); /* Y & ~0b11 */
         brw_SHR(&func, t1, t1, brw_imm_uw(1)); /* (Y & ~0b11) >> 1 */
         brw_AND(&func, t2, Y, brw_imm_uw(1)); /* Y & 0b1 */
         brw_OR(&func, Yp, t1, t2);
         brw_AND(&func, t1, Y, brw_imm_uw(2)); /* Y & 0b10 */
         brw_AND(&func, t2, X, brw_imm_uw(2)); /* X & 0b10 */
         brw_SHR(&func, t2, t2, brw_imm_uw(1)); /* (X & 0b10) >> 1 */
         brw_OR(&func, S, t1, t2);
         break;
      case 8:
         /* decode_msaa(8, IMS, X, Y, 0) = (X', Y', S)
          *   where X' = (X & ~0b111) >> 2 | (X & 0b1)
          *         Y' = (Y & ~0b11) >> 1 | (Y & 0b1)
          *         S = (X & 0b100) | (Y & 0b10) | (X & 0b10) >> 1
          */
         brw_AND(&func, t1, X, brw_imm_uw(0xfff8)); /* X & ~0b111 */
         brw_SHR(&func, t1, t1, brw_imm_uw(2)); /* (X & ~0b111) >> 2 */
         brw_AND(&func, t2, X, brw_imm_uw(1)); /* X & 0b1 */
         brw_OR(&func, Xp, t1, t2);
         brw_AND(&func, t1, Y, brw_imm_uw(0xfffc)); /* Y & ~0b11 */
         brw_SHR(&func, t1, t1, brw_imm_uw(1)); /* (Y & ~0b11) >> 1 */
         brw_AND(&func, t2, Y, brw_imm_uw(1)); /* Y & 0b1 */
         brw_OR(&func, Yp, t1, t2);
         brw_AND(&func, t1, X, brw_imm_uw(4)); /* X & 0b100 */
         brw_AND(&func, t2, Y, brw_imm_uw(2)); /* Y & 0b10 */
         brw_OR(&func, t1, t1, t2); /* (X & 0b100) | (Y & 0b10) */
         brw_AND(&func, t2, X, brw_imm_uw(2)); /* X & 0b10 */
         brw_SHR(&func, t2, t2, brw_imm_uw(1)); /* (X & 0b10) >> 1 */
         brw_OR(&func, S, t1, t2);
         break;
      }
      s_is_zero = false;
      SWAP_XY_AND_XPYP();
      break;
   }
}

/**
 * Emit code that kills pixels whose X and Y coordinates are outside the
 * boundary of the rectangle defined by the push constants (dst_x0, dst_y0,
 * dst_x1, dst_y1).
 */
void
brw_blorp_blit_program::kill_if_outside_dst_rect()
{
   struct brw_reg f0 = brw_flag_reg();
   struct brw_reg g1 = retype(brw_vec1_grf(1, 7), BRW_REGISTER_TYPE_UW);
   struct brw_reg null16 = vec16(retype(brw_null_reg(), BRW_REGISTER_TYPE_UW));

   brw_CMP(&func, null16, BRW_CONDITIONAL_GE, X, dst_x0);
   brw_CMP(&func, null16, BRW_CONDITIONAL_GE, Y, dst_y0);
   brw_CMP(&func, null16, BRW_CONDITIONAL_L, X, dst_x1);
   brw_CMP(&func, null16, BRW_CONDITIONAL_L, Y, dst_y1);

   brw_set_predicate_control(&func, BRW_PREDICATE_NONE);
   brw_push_insn_state(&func);
   brw_set_mask_control(&func, BRW_MASK_DISABLE);
   brw_AND(&func, g1, f0, g1);
   brw_pop_insn_state(&func);
}

/**
 * Emit code to translate from destination (X, Y) coordinates to source (X, Y)
 * coordinates.
 */
void
brw_blorp_blit_program::translate_dst_to_src()
{
   brw_MUL(&func, Xp, X, x_transform.multiplier);
   brw_MUL(&func, Yp, Y, y_transform.multiplier);
   brw_ADD(&func, Xp, Xp, x_transform.offset);
   brw_ADD(&func, Yp, Yp, y_transform.offset);
   SWAP_XY_AND_XPYP();
}

/**
 * Emit code to transform the X and Y coordinates as needed for blending
 * together the different samples in an MSAA texture.
 */
void
brw_blorp_blit_program::single_to_blend()
{
   /* When looking up samples in an MSAA texture using the SAMPLE message,
    * Gen6 requires the texture coordinates to be odd integers (so that they
    * correspond to the center of a 2x2 block representing the four samples
    * that maxe up a pixel).  So we need to multiply our X and Y coordinates
    * each by 2 and then add 1.
    */
   brw_SHL(&func, t1, X, brw_imm_w(1));
   brw_SHL(&func, t2, Y, brw_imm_w(1));
   brw_ADD(&func, Xp, t1, brw_imm_w(1));
   brw_ADD(&func, Yp, t2, brw_imm_w(1));
   SWAP_XY_AND_XPYP();
}


/**
 * Count the number of trailing 1 bits in the given value.  For example:
 *
 * count_trailing_one_bits(0) == 0
 * count_trailing_one_bits(7) == 3
 * count_trailing_one_bits(11) == 2
 */
inline int count_trailing_one_bits(unsigned value)
{
#if defined(__GNUC__) && ((__GNUC__ * 100 + __GNUC_MINOR__) >= 304) /* gcc 3.4 or later */
   return __builtin_ctz(~value);
#else
   return _mesa_bitcount(value & ~(value + 1));
#endif
}


void
brw_blorp_blit_program::manual_blend(unsigned num_samples)
{
   if (key->tex_layout == INTEL_MSAA_LAYOUT_CMS)
      mcs_fetch();

   /* We add together samples using a binary tree structure, e.g. for 4x MSAA:
    *
    *   result = ((sample[0] + sample[1]) + (sample[2] + sample[3])) / 4
    *
    * This ensures that when all samples have the same value, no numerical
    * precision is lost, since each addition operation always adds two equal
    * values, and summing two equal floating point values does not lose
    * precision.
    *
    * We perform this computation by treating the texture_data array as a
    * stack and performing the following operations:
    *
    * - push sample 0 onto stack
    * - push sample 1 onto stack
    * - add top two stack entries
    * - push sample 2 onto stack
    * - push sample 3 onto stack
    * - add top two stack entries
    * - add top two stack entries
    * - divide top stack entry by 4
    *
    * Note that after pushing sample i onto the stack, the number of add
    * operations we do is equal to the number of trailing 1 bits in i.  This
    * works provided the total number of samples is a power of two, which it
    * always is for i965.
    *
    * For integer formats, we replace the add operations with average
    * operations and skip the final division.
    */
   typedef struct brw_instruction *(*brw_op2_ptr)(struct brw_compile *,
                                                  struct brw_reg,
                                                  struct brw_reg,
                                                  struct brw_reg);
   brw_op2_ptr combine_op =
      key->texture_data_type == BRW_REGISTER_TYPE_F ? brw_ADD : brw_AVG;
   unsigned stack_depth = 0;
   for (unsigned i = 0; i < num_samples; ++i) {
      assert(stack_depth == _mesa_bitcount(i)); /* Loop invariant */

      /* Push sample i onto the stack */
      assert(stack_depth < ARRAY_SIZE(texture_data));
      if (i == 0) {
         s_is_zero = true;
      } else {
         s_is_zero = false;
         brw_MOV(&func, S, brw_imm_uw(i));
      }
      texel_fetch(texture_data[stack_depth++]);

      if (i == 0 && key->tex_layout == INTEL_MSAA_LAYOUT_CMS) {
         /* The Ivy Bridge PRM, Vol4 Part1 p27 (Multisample Control Surface)
          * suggests an optimization:
          *
          *     "A simple optimization with probable large return in
          *     performance is to compare the MCS value to zero (indicating
          *     all samples are on sample slice 0), and sample only from
          *     sample slice 0 using ld2dss if MCS is zero."
          *
          * Note that in the case where the MCS value is zero, sampling from
          * sample slice 0 using ld2dss and sampling from sample 0 using
          * ld2dms are equivalent (since all samples are on sample slice 0).
          * Since we have already sampled from sample 0, all we need to do is
          * skip the remaining fetches and averaging if MCS is zero.
          */
         brw_CMP(&func, vec16(brw_null_reg()), BRW_CONDITIONAL_NZ,
                 mcs_data, brw_imm_ud(0));
         brw_IF(&func, BRW_EXECUTE_16);
      }

      /* Do count_trailing_one_bits(i) times */
      for (int j = count_trailing_one_bits(i); j-- > 0; ) {
         assert(stack_depth >= 2);
         --stack_depth;

         /* TODO: should use a smaller loop bound for non_RGBA formats */
         for (int k = 0; k < 4; ++k) {
            combine_op(&func, offset(texture_data[stack_depth - 1], 2*k),
                       offset(vec8(texture_data[stack_depth - 1]), 2*k),
                       offset(vec8(texture_data[stack_depth]), 2*k));
         }
      }
   }

   /* We should have just 1 sample on the stack now. */
   assert(stack_depth == 1);

   if (key->texture_data_type == BRW_REGISTER_TYPE_F) {
      /* Scale the result down by a factor of num_samples */
      /* TODO: should use a smaller loop bound for non-RGBA formats */
      for (int j = 0; j < 4; ++j) {
         brw_MUL(&func, offset(texture_data[0], 2*j),
                 offset(vec8(texture_data[0]), 2*j),
                 brw_imm_f(1.0/num_samples));
      }
   }

   if (key->tex_layout == INTEL_MSAA_LAYOUT_CMS)
      brw_ENDIF(&func);
}

/**
 * Emit code to look up a value in the texture using the SAMPLE message (which
 * does blending of MSAA surfaces).
 */
void
brw_blorp_blit_program::sample(struct brw_reg dst)
{
   static const sampler_message_arg args[2] = {
      SAMPLER_MESSAGE_ARG_U_FLOAT,
      SAMPLER_MESSAGE_ARG_V_FLOAT
   };

   texture_lookup(dst, GEN5_SAMPLER_MESSAGE_SAMPLE, args, ARRAY_SIZE(args));
}

/**
 * Emit code to look up a value in the texture using the SAMPLE_LD message
 * (which does a simple texel fetch).
 */
void
brw_blorp_blit_program::texel_fetch(struct brw_reg dst)
{
   static const sampler_message_arg gen6_args[5] = {
      SAMPLER_MESSAGE_ARG_U_INT,
      SAMPLER_MESSAGE_ARG_V_INT,
      SAMPLER_MESSAGE_ARG_ZERO_INT, /* R */
      SAMPLER_MESSAGE_ARG_ZERO_INT, /* LOD */
      SAMPLER_MESSAGE_ARG_SI_INT
   };
   static const sampler_message_arg gen7_ld_args[3] = {
      SAMPLER_MESSAGE_ARG_U_INT,
      SAMPLER_MESSAGE_ARG_ZERO_INT, /* LOD */
      SAMPLER_MESSAGE_ARG_V_INT
   };
   static const sampler_message_arg gen7_ld2dss_args[3] = {
      SAMPLER_MESSAGE_ARG_SI_INT,
      SAMPLER_MESSAGE_ARG_U_INT,
      SAMPLER_MESSAGE_ARG_V_INT
   };
   static const sampler_message_arg gen7_ld2dms_args[4] = {
      SAMPLER_MESSAGE_ARG_SI_INT,
      SAMPLER_MESSAGE_ARG_MCS_INT,
      SAMPLER_MESSAGE_ARG_U_INT,
      SAMPLER_MESSAGE_ARG_V_INT
   };

   switch (brw->intel.gen) {
   case 6:
      texture_lookup(dst, GEN5_SAMPLER_MESSAGE_SAMPLE_LD, gen6_args,
                     s_is_zero ? 2 : 5);
      break;
   case 7:
      switch (key->tex_layout) {
      case INTEL_MSAA_LAYOUT_IMS:
         /* From the Ivy Bridge PRM, Vol4 Part1 p72 (Multisampled Surface Storage
          * Format):
          *
          *     If this field is MSFMT_DEPTH_STENCIL
          *     [a.k.a. INTEL_MSAA_LAYOUT_IMS], the only sampling engine
          *     messages allowed are "ld2dms", "resinfo", and "sampleinfo".
          *
          * So fall through to emit the same message as we use for
          * INTEL_MSAA_LAYOUT_CMS.
          */
      case INTEL_MSAA_LAYOUT_CMS:
         texture_lookup(dst, GEN7_SAMPLER_MESSAGE_SAMPLE_LD2DMS,
                        gen7_ld2dms_args, ARRAY_SIZE(gen7_ld2dms_args));
         break;
      case INTEL_MSAA_LAYOUT_UMS:
         texture_lookup(dst, GEN7_SAMPLER_MESSAGE_SAMPLE_LD2DSS,
                        gen7_ld2dss_args, ARRAY_SIZE(gen7_ld2dss_args));
         break;
      case INTEL_MSAA_LAYOUT_NONE:
         assert(s_is_zero);
         texture_lookup(dst, GEN5_SAMPLER_MESSAGE_SAMPLE_LD, gen7_ld_args,
                        ARRAY_SIZE(gen7_ld_args));
         break;
      }
      break;
   default:
      assert(!"Should not get here.");
      break;
   };
}

void
brw_blorp_blit_program::mcs_fetch()
{
   static const sampler_message_arg gen7_ld_mcs_args[2] = {
      SAMPLER_MESSAGE_ARG_U_INT,
      SAMPLER_MESSAGE_ARG_V_INT
   };
   texture_lookup(vec16(mcs_data), GEN7_SAMPLER_MESSAGE_SAMPLE_LD_MCS,
                  gen7_ld_mcs_args, ARRAY_SIZE(gen7_ld_mcs_args));
}

void
brw_blorp_blit_program::expand_to_32_bits(struct brw_reg src,
                                          struct brw_reg dst)
{
   brw_MOV(&func, vec8(dst), vec8(src));
   brw_set_compression_control(&func, BRW_COMPRESSION_2NDHALF);
   brw_MOV(&func, offset(vec8(dst), 1), suboffset(vec8(src), 8));
   brw_set_compression_control(&func, BRW_COMPRESSION_NONE);
}

void
brw_blorp_blit_program::texture_lookup(struct brw_reg dst,
                                       GLuint msg_type,
                                       const sampler_message_arg *args,
                                       int num_args)
{
   struct brw_reg mrf =
      retype(vec16(brw_message_reg(base_mrf)), BRW_REGISTER_TYPE_UD);
   for (int arg = 0; arg < num_args; ++arg) {
      switch (args[arg]) {
      case SAMPLER_MESSAGE_ARG_U_FLOAT:
         expand_to_32_bits(X, retype(mrf, BRW_REGISTER_TYPE_F));
         break;
      case SAMPLER_MESSAGE_ARG_V_FLOAT:
         expand_to_32_bits(Y, retype(mrf, BRW_REGISTER_TYPE_F));
         break;
      case SAMPLER_MESSAGE_ARG_U_INT:
         expand_to_32_bits(X, mrf);
         break;
      case SAMPLER_MESSAGE_ARG_V_INT:
         expand_to_32_bits(Y, mrf);
         break;
      case SAMPLER_MESSAGE_ARG_SI_INT:
         /* Note: on Gen7, this code may be reached with s_is_zero==true
          * because in Gen7's ld2dss message, the sample index is the first
          * argument.  When this happens, we need to move a 0 into the
          * appropriate message register.
          */
         if (s_is_zero)
            brw_MOV(&func, mrf, brw_imm_ud(0));
         else
            expand_to_32_bits(S, mrf);
         break;
      case SAMPLER_MESSAGE_ARG_MCS_INT:
         switch (key->tex_layout) {
         case INTEL_MSAA_LAYOUT_CMS:
            brw_MOV(&func, mrf, mcs_data);
            break;
         case INTEL_MSAA_LAYOUT_IMS:
            /* When sampling from an IMS surface, MCS data is not relevant,
             * and the hardware ignores it.  So don't bother populating it.
             */
            break;
         default:
            /* We shouldn't be trying to send MCS data with any other
             * layouts.
             */
            assert (!"Unsupported layout for MCS data");
            break;
         }
         break;
      case SAMPLER_MESSAGE_ARG_ZERO_INT:
         brw_MOV(&func, mrf, brw_imm_ud(0));
         break;
      }
      mrf.nr += 2;
   }

   brw_SAMPLE(&func,
              retype(dst, BRW_REGISTER_TYPE_UW) /* dest */,
              base_mrf /* msg_reg_nr */,
              brw_message_reg(base_mrf) /* src0 */,
              BRW_BLORP_TEXTURE_BINDING_TABLE_INDEX,
              0 /* sampler */,
              WRITEMASK_XYZW,
              msg_type,
              8 /* response_length.  TODO: should be smaller for non-RGBA formats? */,
              mrf.nr - base_mrf /* msg_length */,
              0 /* header_present */,
              BRW_SAMPLER_SIMD_MODE_SIMD16,
              BRW_SAMPLER_RETURN_FORMAT_FLOAT32);
}

#undef X
#undef Y
#undef U
#undef V
#undef S
#undef SWAP_XY_AND_XPYP

void
brw_blorp_blit_program::render_target_write()
{
   struct brw_reg mrf_rt_write =
      retype(vec16(brw_message_reg(base_mrf)), key->texture_data_type);
   int mrf_offset = 0;

   /* If we may have killed pixels, then we need to send R0 and R1 in a header
    * so that the render target knows which pixels we killed.
    */
   bool use_header = key->use_kill;
   if (use_header) {
      /* Copy R0/1 to MRF */
      brw_MOV(&func, retype(mrf_rt_write, BRW_REGISTER_TYPE_UD),
              retype(R0, BRW_REGISTER_TYPE_UD));
      mrf_offset += 2;
   }

   /* Copy texture data to MRFs */
   for (int i = 0; i < 4; ++i) {
      /* E.g. mov(16) m2.0<1>:f r2.0<8;8,1>:f { Align1, H1 } */
      brw_MOV(&func, offset(mrf_rt_write, mrf_offset),
              offset(vec8(texture_data[0]), 2*i));
      mrf_offset += 2;
   }

   /* Now write to the render target and terminate the thread */
   brw_fb_WRITE(&func,
                16 /* dispatch_width */,
                base_mrf /* msg_reg_nr */,
                mrf_rt_write /* src0 */,
                BRW_DATAPORT_RENDER_TARGET_WRITE_SIMD16_SINGLE_SOURCE,
                BRW_BLORP_RENDERBUFFER_BINDING_TABLE_INDEX,
                mrf_offset /* msg_length.  TODO: Should be smaller for non-RGBA formats. */,
                0 /* response_length */,
                true /* eot */,
                use_header);
}


void
brw_blorp_coord_transform_params::setup(GLuint src0, GLuint dst0, GLuint dst1,
                                        bool mirror)
{
   if (!mirror) {
      /* When not mirroring a coordinate (say, X), we need:
       *   x' - src_x0 = x - dst_x0
       * Therefore:
       *   x' = 1*x + (src_x0 - dst_x0)
       */
      multiplier = 1;
      offset = src0 - dst0;
   } else {
      /* When mirroring X we need:
       *   x' - src_x0 = dst_x1 - x - 1
       * Therefore:
       *   x' = -1*x + (src_x0 + dst_x1 - 1)
       */
      multiplier = -1;
      offset = src0 + dst1 - 1;
   }
}


/**
 * Determine which MSAA layout the GPU pipeline should be configured for,
 * based on the chip generation, the number of samples, and the true layout of
 * the image in memory.
 */
inline intel_msaa_layout
compute_msaa_layout_for_pipeline(struct brw_context *brw, unsigned num_samples,
                                 intel_msaa_layout true_layout)
{
   if (num_samples <= 1) {
      /* When configuring the GPU for non-MSAA, we can still accommodate IMS
       * format buffers, by transforming coordinates appropriately.
       */
      assert(true_layout == INTEL_MSAA_LAYOUT_NONE ||
             true_layout == INTEL_MSAA_LAYOUT_IMS);
      return INTEL_MSAA_LAYOUT_NONE;
   } else {
      assert(true_layout != INTEL_MSAA_LAYOUT_NONE);
   }

   /* Prior to Gen7, all MSAA surfaces use IMS layout. */
   if (brw->intel.gen == 6) {
      assert(true_layout == INTEL_MSAA_LAYOUT_IMS);
   }

   return true_layout;
}


brw_blorp_blit_params::brw_blorp_blit_params(struct brw_context *brw,
                                             struct intel_mipmap_tree *src_mt,
                                             unsigned src_level, unsigned src_layer,
                                             struct intel_mipmap_tree *dst_mt,
                                             unsigned dst_level, unsigned dst_layer,
                                             GLuint src_x0, GLuint src_y0,
                                             GLuint dst_x0, GLuint dst_y0,
                                             GLuint dst_x1, GLuint dst_y1,
                                             bool mirror_x, bool mirror_y)
{
   src.set(brw, src_mt, src_level, src_layer);
   dst.set(brw, dst_mt, dst_level, dst_layer);

   /* If we are blitting from sRGB to linear or vice versa, we still want the
    * blit to be a direct copy, so we need source and destination to use the
    * same format.  However, we want the destination sRGB/linear state to be
    * correct (so that sRGB blending is used when doing an MSAA resolve to an
    * sRGB surface, and linear blending is used when doing an MSAA resolve to
    * a linear surface).  Since blorp blits don't support any format
    * conversion (except between sRGB and linear), we can accomplish this by
    * simply setting up the source to use the same format as the destination.
    */
   assert(_mesa_get_srgb_format_linear(src_mt->format) ==
          _mesa_get_srgb_format_linear(dst_mt->format));
   src.brw_surfaceformat = dst.brw_surfaceformat;

   use_wm_prog = true;
   memset(&wm_prog_key, 0, sizeof(wm_prog_key));

   /* texture_data_type indicates the register type that should be used to
    * manipulate texture data.
    */
   switch (_mesa_get_format_datatype(src_mt->format)) {
   case GL_UNSIGNED_NORMALIZED:
   case GL_SIGNED_NORMALIZED:
   case GL_FLOAT:
      wm_prog_key.texture_data_type = BRW_REGISTER_TYPE_F;
      break;
   case GL_UNSIGNED_INT:
      if (src_mt->format == MESA_FORMAT_S8) {
         /* We process stencil as though it's an unsigned normalized color */
         wm_prog_key.texture_data_type = BRW_REGISTER_TYPE_F;
      } else {
         wm_prog_key.texture_data_type = BRW_REGISTER_TYPE_UD;
      }
      break;
   case GL_INT:
      wm_prog_key.texture_data_type = BRW_REGISTER_TYPE_D;
      break;
   default:
      assert(!"Unrecognized blorp format");
      break;
   }

   if (brw->intel.gen > 6) {
      /* Gen7's rendering hardware only supports the IMS layout for depth and
       * stencil render targets.  Blorp always maps its destination surface as
       * a color render target (even if it's actually a depth or stencil
       * buffer).  So if the destination is IMS, we'll have to map it as a
       * single-sampled texture and interleave the samples ourselves.
       */
      if (dst_mt->msaa_layout == INTEL_MSAA_LAYOUT_IMS)
         dst.num_samples = 0;
   }

   if (dst.map_stencil_as_y_tiled && dst.num_samples > 1) {
      /* If the destination surface is a W-tiled multisampled stencil buffer
       * that we're mapping as Y tiled, then we need to arrange for the WM
       * program to run once per sample rather than once per pixel, because
       * the memory layout of related samples doesn't match between W and Y
       * tiling.
       */
      wm_prog_key.persample_msaa_dispatch = true;
   }

   if (src.num_samples > 0 && dst.num_samples > 1) {
      /* We are blitting from a multisample buffer to a multisample buffer, so
       * we must preserve samples within a pixel.  This means we have to
       * arrange for the WM program to run once per sample rather than once
       * per pixel.
       */
      wm_prog_key.persample_msaa_dispatch = true;
   }

   /* The render path must be configured to use the same number of samples as
    * the destination buffer.
    */
   num_samples = dst.num_samples;

   GLenum base_format = _mesa_get_format_base_format(src_mt->format);
   if (base_format != GL_DEPTH_COMPONENT && /* TODO: what about depth/stencil? */
       base_format != GL_STENCIL_INDEX &&
       src_mt->num_samples > 1 && dst_mt->num_samples <= 1) {
      /* We are downsampling a color buffer, so blend. */
      wm_prog_key.blend = true;
   }

   /* src_samples and dst_samples are the true sample counts */
   wm_prog_key.src_samples = src_mt->num_samples;
   wm_prog_key.dst_samples = dst_mt->num_samples;

   /* tex_samples and rt_samples are the sample counts that are set up in
    * SURFACE_STATE.
    */
   wm_prog_key.tex_samples = src.num_samples;
   wm_prog_key.rt_samples  = dst.num_samples;

   /* tex_layout and rt_layout indicate the MSAA layout the GPU pipeline will
    * use to access the source and destination surfaces.
    */
   wm_prog_key.tex_layout =
      compute_msaa_layout_for_pipeline(brw, src.num_samples, src.msaa_layout);
   wm_prog_key.rt_layout =
      compute_msaa_layout_for_pipeline(brw, dst.num_samples, dst.msaa_layout);

   /* src_layout and dst_layout indicate the true MSAA layout used by src and
    * dst.
    */
   wm_prog_key.src_layout = src_mt->msaa_layout;
   wm_prog_key.dst_layout = dst_mt->msaa_layout;

   wm_prog_key.src_tiled_w = src.map_stencil_as_y_tiled;
   wm_prog_key.dst_tiled_w = dst.map_stencil_as_y_tiled;
   x0 = wm_push_consts.dst_x0 = dst_x0;
   y0 = wm_push_consts.dst_y0 = dst_y0;
   x1 = wm_push_consts.dst_x1 = dst_x1;
   y1 = wm_push_consts.dst_y1 = dst_y1;
   wm_push_consts.x_transform.setup(src_x0, dst_x0, dst_x1, mirror_x);
   wm_push_consts.y_transform.setup(src_y0, dst_y0, dst_y1, mirror_y);

   if (dst.num_samples <= 1 && dst_mt->num_samples > 1) {
      /* We must expand the rectangle we send through the rendering pipeline,
       * to account for the fact that we are mapping the destination region as
       * single-sampled when it is in fact multisampled.  We must also align
       * it to a multiple of the multisampling pattern, because the
       * differences between multisampled and single-sampled surface formats
       * will mean that pixels are scrambled within the multisampling pattern.
       * TODO: what if this makes the coordinates too large?
       *
       * Note: this only works if the destination surface uses the IMS layout.
       * If it's UMS, then we have no choice but to set up the rendering
       * pipeline as multisampled.
       */
      assert(dst_mt->msaa_layout == INTEL_MSAA_LAYOUT_IMS);
      switch (dst_mt->num_samples) {
      case 4:
         x0 = ROUND_DOWN_TO(x0 * 2, 4);
         y0 = ROUND_DOWN_TO(y0 * 2, 4);
         x1 = ALIGN(x1 * 2, 4);
         y1 = ALIGN(y1 * 2, 4);
         break;
      case 8:
         x0 = ROUND_DOWN_TO(x0 * 4, 8);
         y0 = ROUND_DOWN_TO(y0 * 2, 4);
         x1 = ALIGN(x1 * 4, 8);
         y1 = ALIGN(y1 * 2, 4);
         break;
      default:
         assert(!"Unrecognized sample count in brw_blorp_blit_params ctor");
         break;
      }
      wm_prog_key.use_kill = true;
   }

   if (dst.map_stencil_as_y_tiled) {
      /* We must modify the rectangle we send through the rendering pipeline
       * (and the size and x/y offset of the destination surface), to account
       * for the fact that we are mapping it as Y-tiled when it is in fact
       * W-tiled.
       *
       * Both Y tiling and W tiling can be understood as organizations of
       * 32-byte sub-tiles; within each 32-byte sub-tile, the layout of pixels
       * is different, but the layout of the 32-byte sub-tiles within the 4k
       * tile is the same (8 sub-tiles across by 16 sub-tiles down, in
       * column-major order).  In Y tiling, the sub-tiles are 16 bytes wide
       * and 2 rows high; in W tiling, they are 8 bytes wide and 4 rows high.
       *
       * Therefore, to account for the layout differences within the 32-byte
       * sub-tiles, we must expand the rectangle so the X coordinates of its
       * edges are multiples of 8 (the W sub-tile width), and its Y
       * coordinates of its edges are multiples of 4 (the W sub-tile height).
       * Then we need to scale the X and Y coordinates of the rectangle to
       * account for the differences in aspect ratio between the Y and W
       * sub-tiles.  We need to modify the layer width and height similarly.
       *
       * A correction needs to be applied when MSAA is in use: since
       * INTEL_MSAA_LAYOUT_IMS uses an interleaving pattern whose height is 4,
       * we need to align the Y coordinates to multiples of 8, so that when
       * they are divided by two they are still multiples of 4.
       *
       * Note: Since the x/y offset of the surface will be applied using the
       * SURFACE_STATE command packet, it will be invisible to the swizzling
       * code in the shader; therefore it needs to be in a multiple of the
       * 32-byte sub-tile size.  Fortunately it is, since the sub-tile is 8
       * pixels wide and 4 pixels high (when viewed as a W-tiled stencil
       * buffer), and the miplevel alignment used for stencil buffers is 8
       * pixels horizontally and either 4 or 8 pixels vertically (see
       * intel_horizontal_texture_alignment_unit() and
       * intel_vertical_texture_alignment_unit()).
       *
       * Note: Also, since the SURFACE_STATE command packet can only apply
       * offsets that are multiples of 4 pixels horizontally and 2 pixels
       * vertically, it is important that the offsets will be multiples of
       * these sizes after they are converted into Y-tiled coordinates.
       * Fortunately they will be, since we know from above that the offsets
       * are a multiple of the 32-byte sub-tile size, and in Y-tiled
       * coordinates the sub-tile is 16 pixels wide and 2 pixels high.
       *
       * TODO: what if this makes the coordinates (or the texture size) too
       * large?
       */
      const unsigned x_align = 8, y_align = dst.num_samples != 0 ? 8 : 4;
      x0 = ROUND_DOWN_TO(x0, x_align) * 2;
      y0 = ROUND_DOWN_TO(y0, y_align) / 2;
      x1 = ALIGN(x1, x_align) * 2;
      y1 = ALIGN(y1, y_align) / 2;
      dst.width = ALIGN(dst.width, x_align) * 2;
      dst.height = ALIGN(dst.height, y_align) / 2;
      dst.x_offset *= 2;
      dst.y_offset /= 2;
      wm_prog_key.use_kill = true;
   }

   if (src.map_stencil_as_y_tiled) {
      /* We must modify the size and x/y offset of the source surface to
       * account for the fact that we are mapping it as Y-tiled when it is in
       * fact W tiled.
       *
       * See the comments above concerning x/y offset alignment for the
       * destination surface.
       *
       * TODO: what if this makes the texture size too large?
       */
      const unsigned x_align = 8, y_align = src.num_samples != 0 ? 8 : 4;
      src.width = ALIGN(src.width, x_align) * 2;
      src.height = ALIGN(src.height, y_align) / 2;
      src.x_offset *= 2;
      src.y_offset /= 2;
   }
}

uint32_t
brw_blorp_blit_params::get_wm_prog(struct brw_context *brw,
                                   brw_blorp_prog_data **prog_data) const
{
   uint32_t prog_offset;
   if (!brw_search_cache(&brw->cache, BRW_BLORP_BLIT_PROG,
                         &this->wm_prog_key, sizeof(this->wm_prog_key),
                         &prog_offset, prog_data)) {
      brw_blorp_blit_program prog(brw, &this->wm_prog_key);
      GLuint program_size;
      const GLuint *program = prog.compile(brw, &program_size);
      brw_upload_cache(&brw->cache, BRW_BLORP_BLIT_PROG,
                       &this->wm_prog_key, sizeof(this->wm_prog_key),
                       program, program_size,
                       &prog.prog_data, sizeof(prog.prog_data),
                       &prog_offset, prog_data);
   }
   return prog_offset;
}
