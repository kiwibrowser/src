/*
 * Copyright © 2011 Intel Corporation
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

#ifndef BRW_PROGRAM_H
#define BRW_PROGRAM_H

/**
 * Sampler information needed by VS, WM, and GS program cache keys.
 */
struct brw_sampler_prog_key_data {
   /**
    * EXT_texture_swizzle and DEPTH_TEXTURE_MODE swizzles.
    */
   uint16_t swizzles[MAX_SAMPLERS];

   uint16_t gl_clamp_mask[3];

   /**
    * YUV conversions, needed for the GL_MESA_ycbcr extension.
    */
   uint16_t yuvtex_mask;
   uint16_t yuvtex_swap_mask; /**< UV swaped */
};

void brw_populate_sampler_prog_key_data(struct gl_context *ctx,
				        const struct gl_program *prog,
				        struct brw_sampler_prog_key_data *key);
bool brw_debug_recompile_sampler_key(const struct brw_sampler_prog_key_data *old_key,
                                     const struct brw_sampler_prog_key_data *key);

#endif
