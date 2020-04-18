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

#include "shaders_cache.h"

#include "vg_context.h"

#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_shader_tokens.h"

#include "tgsi/tgsi_build.h"
#include "tgsi/tgsi_dump.h"
#include "tgsi/tgsi_parse.h"
#include "tgsi/tgsi_util.h"
#include "tgsi/tgsi_text.h"

#include "util/u_memory.h"
#include "util/u_math.h"
#include "util/u_debug.h"
#include "cso_cache/cso_hash.h"
#include "cso_cache/cso_context.h"

#include "VG/openvg.h"

#include "asm_fill.h"

/* Essentially we construct an ubber-shader based on the state
 * of the pipeline. The stages are:
 * 1) Paint generation (color/gradient/pattern)
 * 2) Image composition (normal/multiply/stencil)
 * 3) Color transform
 * 4) Per-channel alpha generation
 * 5) Extended blend (multiply/screen/darken/lighten)
 * 6) Mask
 * 7) Premultiply/Unpremultiply
 * 8) Color transform (to black and white)
 */
#define SHADER_STAGES 8

struct cached_shader {
   void *driver_shader;
   struct pipe_shader_state state;
};

struct shaders_cache {
   struct vg_context *pipe;

   struct cso_hash *hash;
};


static INLINE struct tgsi_token *tokens_from_assembly(const char *txt, int num_tokens)
{
   struct tgsi_token *tokens;

   tokens = (struct tgsi_token *) MALLOC(num_tokens * sizeof(tokens[0]));

   tgsi_text_translate(txt, tokens, num_tokens);

#if DEBUG_SHADERS
   tgsi_dump(tokens, 0);
#endif

   return tokens;
}

/*
static const char max_shader_preamble[] =
   "FRAG\n"
   "DCL IN[0], POSITION, LINEAR\n"
   "DCL IN[1], GENERIC[0], PERSPECTIVE\n"
   "DCL OUT[0], COLOR, CONSTANT\n"
   "DCL CONST[0..9], CONSTANT\n"
   "DCL TEMP[0..9], CONSTANT\n"
   "DCL SAMP[0..9], CONSTANT\n";

   max_shader_preamble strlen == 175
*/
#define MAX_PREAMBLE 175

static INLINE VGint range_min(VGint min, VGint current)
{
   if (min < 0)
      min = current;
   else
      min = MIN2(min, current);
   return min;
}

static INLINE VGint range_max(VGint max, VGint current)
{
   return MAX2(max, current);
}

static void *
combine_shaders(const struct shader_asm_info *shaders[SHADER_STAGES], int num_shaders,
                struct pipe_context *pipe,
                struct pipe_shader_state *shader)
{
   VGboolean declare_input = VG_FALSE;
   VGint start_const   = -1, end_const   = 0;
   VGint start_temp    = -1, end_temp    = 0;
   VGint start_sampler = -1, end_sampler = 0;
   VGint i, current_shader = 0;
   VGint num_consts, num_temps, num_samplers;
   struct ureg_program *ureg;
   struct ureg_src in[2];
   struct ureg_src *sampler = NULL;
   struct ureg_src *constant = NULL;
   struct ureg_dst out, *temp = NULL;
   void *p = NULL;

   for (i = 0; i < num_shaders; ++i) {
      if (shaders[i]->num_consts)
         start_const = range_min(start_const, shaders[i]->start_const);
      if (shaders[i]->num_temps)
         start_temp = range_min(start_temp, shaders[i]->start_temp);
      if (shaders[i]->num_samplers)
         start_sampler = range_min(start_sampler, shaders[i]->start_sampler);

      end_const = range_max(end_const, shaders[i]->start_const +
                            shaders[i]->num_consts);
      end_temp = range_max(end_temp, shaders[i]->start_temp +
                            shaders[i]->num_temps);
      end_sampler = range_max(end_sampler, shaders[i]->start_sampler +
                            shaders[i]->num_samplers);
      if (shaders[i]->needs_position)
         declare_input = VG_TRUE;
   }
   /* if they're still unitialized, initialize them */
   if (start_const < 0)
      start_const = 0;
   if (start_temp < 0)
      start_temp = 0;
   if (start_sampler < 0)
       start_sampler = 0;

   num_consts   = end_const   - start_const;
   num_temps    = end_temp    - start_temp;
   num_samplers = end_sampler - start_sampler;

   ureg = ureg_create(TGSI_PROCESSOR_FRAGMENT);
   if (!ureg)
       return NULL;

   if (declare_input) {
      in[0] = ureg_DECL_fs_input(ureg,
                                 TGSI_SEMANTIC_POSITION,
                                 0,
                                 TGSI_INTERPOLATE_LINEAR);
      in[1] = ureg_DECL_fs_input(ureg,
                                 TGSI_SEMANTIC_GENERIC,
                                 0,
                                 TGSI_INTERPOLATE_PERSPECTIVE);
   }

   /* we always have a color output */
   out = ureg_DECL_output(ureg, TGSI_SEMANTIC_COLOR, 0);

   if (num_consts >= 1) {
      constant = (struct ureg_src *) malloc(sizeof(struct ureg_src) * end_const);
      for (i = start_const; i < end_const; i++) {
         constant[i] = ureg_DECL_constant(ureg, i);
      }

   }

   if (num_temps >= 1) {
      temp = (struct ureg_dst *) malloc(sizeof(struct ureg_dst) * end_temp);
      for (i = start_temp; i < end_temp; i++) {
         temp[i] = ureg_DECL_temporary(ureg);
      }
   }

   if (num_samplers >= 1) {
      sampler = (struct ureg_src *) malloc(sizeof(struct ureg_src) * end_sampler);
      for (i = start_sampler; i < end_sampler; i++) {
         sampler[i] = ureg_DECL_sampler(ureg, i);
      }
   }

   while (current_shader < num_shaders) {
      if ((current_shader + 1) == num_shaders) {
         shaders[current_shader]->func(ureg,
                                       &out,
                                       in,
                                       sampler,
                                       temp,
                                       constant);
      } else {
         shaders[current_shader]->func(ureg,
                                      &temp[0],
                                      in,
                                      sampler,
                                      temp,
                                      constant);
      }
      current_shader++;
   }

   ureg_END(ureg);

   shader->tokens = ureg_finalize(ureg);
   if(!shader->tokens)
      return NULL;

   p = pipe->create_fs_state(pipe, shader);

   if (num_temps >= 1) {
      for (i = start_temp; i < end_temp; i++) {
         ureg_release_temporary(ureg, temp[i]);
      }
   }

   ureg_destroy(ureg);

   if (temp)
      free(temp);
   if (constant)
      free(constant);
   if (sampler)
      free(sampler);

   return p;
}

static void *
create_shader(struct pipe_context *pipe,
              int id,
              struct pipe_shader_state *shader)
{
   int idx = 0, sh;
   const struct shader_asm_info * shaders[SHADER_STAGES];

   /* first stage */
   sh = SHADERS_GET_PAINT_SHADER(id);
   switch (sh << SHADERS_PAINT_SHIFT) {
   case VEGA_SOLID_FILL_SHADER:
   case VEGA_LINEAR_GRADIENT_SHADER:
   case VEGA_RADIAL_GRADIENT_SHADER:
   case VEGA_PATTERN_SHADER:
   case VEGA_PAINT_DEGENERATE_SHADER:
      shaders[idx] = &shaders_paint_asm[(sh >> SHADERS_PAINT_SHIFT) - 1];
      assert(shaders[idx]->id == sh);
      idx++;
      break;
   default:
      break;
   }

   /* second stage */
   sh = SHADERS_GET_IMAGE_SHADER(id);
   switch (sh) {
   case VEGA_IMAGE_NORMAL_SHADER:
   case VEGA_IMAGE_MULTIPLY_SHADER:
   case VEGA_IMAGE_STENCIL_SHADER:
      shaders[idx] = &shaders_image_asm[(sh >> SHADERS_IMAGE_SHIFT) - 1];
      assert(shaders[idx]->id == sh);
      idx++;
      break;
   default:
      break;
   }

   /* sanity check */
   assert(idx == ((!sh || sh == VEGA_IMAGE_NORMAL_SHADER) ? 1 : 2));

   /* third stage */
   sh = SHADERS_GET_COLOR_TRANSFORM_SHADER(id);
   switch (sh) {
   case VEGA_COLOR_TRANSFORM_SHADER:
      shaders[idx] = &shaders_color_transform_asm[
         (sh >> SHADERS_COLOR_TRANSFORM_SHIFT) - 1];
      assert(shaders[idx]->id == sh);
      idx++;
      break;
   default:
      break;
   }

   /* fourth stage */
   sh = SHADERS_GET_ALPHA_SHADER(id);
   switch (sh) {
   case VEGA_ALPHA_NORMAL_SHADER:
   case VEGA_ALPHA_PER_CHANNEL_SHADER:
      shaders[idx] = &shaders_alpha_asm[
         (sh >> SHADERS_ALPHA_SHIFT) - 1];
      assert(shaders[idx]->id == sh);
      idx++;
      break;
   default:
      break;
   }

   /* fifth stage */
   sh = SHADERS_GET_BLEND_SHADER(id);
   switch (sh) {
   case VEGA_BLEND_SRC_SHADER:
   case VEGA_BLEND_SRC_OVER_SHADER:
   case VEGA_BLEND_DST_OVER_SHADER:
   case VEGA_BLEND_SRC_IN_SHADER:
   case VEGA_BLEND_DST_IN_SHADER:
   case VEGA_BLEND_MULTIPLY_SHADER:
   case VEGA_BLEND_SCREEN_SHADER:
   case VEGA_BLEND_DARKEN_SHADER:
   case VEGA_BLEND_LIGHTEN_SHADER:
   case VEGA_BLEND_ADDITIVE_SHADER:
      shaders[idx] = &shaders_blend_asm[(sh >> SHADERS_BLEND_SHIFT) - 1];
      assert(shaders[idx]->id == sh);
      idx++;
      break;
   default:
      break;
   }

   /* sixth stage */
   sh = SHADERS_GET_MASK_SHADER(id);
   switch (sh) {
   case VEGA_MASK_SHADER:
      shaders[idx] = &shaders_mask_asm[(sh >> SHADERS_MASK_SHIFT) - 1];
      assert(shaders[idx]->id == sh);
      idx++;
      break;
   default:
      break;
   }

   /* seventh stage */
   sh = SHADERS_GET_PREMULTIPLY_SHADER(id);
   switch (sh) {
   case VEGA_PREMULTIPLY_SHADER:
   case VEGA_UNPREMULTIPLY_SHADER:
      shaders[idx] = &shaders_premultiply_asm[
         (sh >> SHADERS_PREMULTIPLY_SHIFT) - 1];
      assert(shaders[idx]->id == sh);
      idx++;
      break;
   default:
      break;
   }

   /* eighth stage */
   sh = SHADERS_GET_BW_SHADER(id);
   switch (sh) {
   case VEGA_BW_SHADER:
      shaders[idx] = &shaders_bw_asm[(sh >> SHADERS_BW_SHIFT) - 1];
      assert(shaders[idx]->id == sh);
      idx++;
      break;
   default:
      break;
   }

   return combine_shaders(shaders, idx, pipe, shader);
}

/*************************************************/

struct shaders_cache * shaders_cache_create(struct vg_context *vg)
{
   struct shaders_cache *sc = CALLOC_STRUCT(shaders_cache);

   sc->pipe = vg;
   sc->hash = cso_hash_create();

   return sc;
}

void shaders_cache_destroy(struct shaders_cache *sc)
{
   struct cso_hash_iter iter = cso_hash_first_node(sc->hash);

   while (!cso_hash_iter_is_null(iter)) {
      struct cached_shader *cached =
         (struct cached_shader *)cso_hash_iter_data(iter);
      cso_delete_fragment_shader(sc->pipe->cso_context,
                                 cached->driver_shader);
      iter = cso_hash_erase(sc->hash, iter);
   }

   cso_hash_delete(sc->hash);
   FREE(sc);
}

void * shaders_cache_fill(struct shaders_cache *sc,
                          int shader_key)
{
   VGint key = shader_key;
   struct cached_shader *cached;
   struct cso_hash_iter iter = cso_hash_find(sc->hash, key);

   if (cso_hash_iter_is_null(iter)) {
      cached = CALLOC_STRUCT(cached_shader);
      cached->driver_shader = create_shader(sc->pipe->pipe, key, &cached->state);

      cso_hash_insert(sc->hash, key, cached);

      return cached->driver_shader;
   }

   cached = (struct cached_shader *)cso_hash_iter_data(iter);

   assert(cached->driver_shader);
   return cached->driver_shader;
}

struct vg_shader * shader_create_from_text(struct pipe_context *pipe,
                                           const char *txt, int num_tokens,
                                           int type)
{
   struct vg_shader *shader = (struct vg_shader *)MALLOC(
      sizeof(struct vg_shader));
   struct tgsi_token *tokens = tokens_from_assembly(txt, num_tokens);
   struct pipe_shader_state state;

   debug_assert(type == PIPE_SHADER_VERTEX ||
                type == PIPE_SHADER_FRAGMENT);

   state.tokens = tokens;
   memset(&state.stream_output, 0, sizeof(state.stream_output));
   shader->type = type;
   shader->tokens = tokens;

   if (type == PIPE_SHADER_FRAGMENT)
      shader->driver = pipe->create_fs_state(pipe, &state);
   else
      shader->driver = pipe->create_vs_state(pipe, &state);
   return shader;
}

void vg_shader_destroy(struct vg_context *ctx, struct vg_shader *shader)
{
   if (shader->type == PIPE_SHADER_FRAGMENT)
      cso_delete_fragment_shader(ctx->cso_context, shader->driver);
   else
      cso_delete_vertex_shader(ctx->cso_context, shader->driver);
   FREE(shader->tokens);
   FREE(shader);
}
