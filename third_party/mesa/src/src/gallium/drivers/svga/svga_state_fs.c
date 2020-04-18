/**********************************************************
 * Copyright 2008-2009 VMware, Inc.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 **********************************************************/

#include "util/u_inlines.h"
#include "pipe/p_defines.h"
#include "util/u_math.h"
#include "util/u_memory.h"
#include "util/u_bitmask.h"
#include "tgsi/tgsi_ureg.h"

#include "svga_context.h"
#include "svga_state.h"
#include "svga_cmd.h"
#include "svga_tgsi.h"

#include "svga_hw_reg.h"



static INLINE int compare_fs_keys( const struct svga_fs_compile_key *a,
                                   const struct svga_fs_compile_key *b )
{
   unsigned keysize_a = svga_fs_key_size( a );
   unsigned keysize_b = svga_fs_key_size( b );

   if (keysize_a != keysize_b) {
      return (int)(keysize_a - keysize_b);
   }
   return memcmp( a, b, keysize_a );
}


static struct svga_shader_result *search_fs_key( struct svga_fragment_shader *fs,
                                                 const struct svga_fs_compile_key *key )
{
   struct svga_shader_result *result = fs->base.results;

   assert(key);

   for ( ; result; result = result->next) {
      if (compare_fs_keys( key, &result->key.fkey ) == 0)
         return result;
   }
   
   return NULL;
}


/**
 * If we fail to compile a fragment shader (because it uses too many
 * registers, for example) we'll use a dummy/fallback shader that
 * simply emits a constant color.
 */
static const struct tgsi_token *
get_dummy_fragment_shader(void)
{
   static const float red[4] = { 1.0, 0.0, 0.0, 0.0 };
   struct ureg_program *ureg;
   const struct tgsi_token *tokens;
   struct ureg_src src;
   struct ureg_dst dst;
   unsigned num_tokens;

   ureg = ureg_create(TGSI_PROCESSOR_FRAGMENT);
   if (!ureg)
      return NULL;

   dst = ureg_DECL_output(ureg, TGSI_SEMANTIC_COLOR, 0);
   src = ureg_DECL_immediate(ureg, red, 4);
   ureg_MOV(ureg, dst, src);
   ureg_END(ureg);

   tokens = ureg_get_tokens(ureg, &num_tokens);

   ureg_destroy(ureg);

   return tokens;
}


static enum pipe_error compile_fs( struct svga_context *svga,
                                   struct svga_fragment_shader *fs,
                                   const struct svga_fs_compile_key *key,
                                   struct svga_shader_result **out_result )
{
   struct svga_shader_result *result;
   enum pipe_error ret = PIPE_ERROR;

   result = svga_translate_fragment_program( fs, key );
   if (result == NULL) {
      /* some problem during translation, try the dummy shader */
      const struct tgsi_token *dummy = get_dummy_fragment_shader();
      if (!dummy) {
         ret = PIPE_ERROR_OUT_OF_MEMORY;
         goto fail;
      }
      debug_printf("Failed to compile fragment shader, using dummy shader instead.\n");
      FREE((void *) fs->base.tokens);
      fs->base.tokens = dummy;
      result = svga_translate_fragment_program(fs, key);
      if (result == NULL) {
         ret = PIPE_ERROR;
         goto fail;
      }
   }

   result->id = util_bitmask_add(svga->fs_bm);
   if(result->id == UTIL_BITMASK_INVALID_INDEX) {
      ret = PIPE_ERROR_OUT_OF_MEMORY;
      goto fail;
   }

   ret = SVGA3D_DefineShader(svga->swc, 
                             result->id,
                             SVGA3D_SHADERTYPE_PS,
                             result->tokens, 
                             result->nr_tokens * sizeof result->tokens[0]);
   if (ret != PIPE_OK)
      goto fail;

   *out_result = result;
   result->next = fs->base.results;
   fs->base.results = result;
   return PIPE_OK;

fail:
   if (result) {
      if (result->id != UTIL_BITMASK_INVALID_INDEX)
         util_bitmask_clear( svga->fs_bm, result->id );
      svga_destroy_shader_result( result );
   }
   return ret;
}


/* SVGA_NEW_TEXTURE_BINDING
 * SVGA_NEW_RAST
 * SVGA_NEW_NEED_SWTNL
 * SVGA_NEW_SAMPLER
 */
static enum pipe_error
make_fs_key(const struct svga_context *svga,
            struct svga_fragment_shader *fs,
            struct svga_fs_compile_key *key)
{
   int i;
   int idx = 0;

   memset(key, 0, sizeof *key);

   /* Only need fragment shader fixup for twoside lighting if doing
    * hwtnl.  Otherwise the draw module does the whole job for us.
    *
    * SVGA_NEW_SWTNL
    */
   if (!svga->state.sw.need_swtnl) {
      /* SVGA_NEW_RAST
       */
      key->light_twoside = svga->curr.rast->templ.light_twoside;
      key->front_ccw = svga->curr.rast->templ.front_ccw;
   }

   /* The blend workaround for simulating logicop xor behaviour
    * requires that the incoming fragment color be white.  This change
    * achieves that by creating a variant of the current fragment
    * shader that overrides all output colors with 1,1,1,1
    *   
    * This will work for most shaders, including those containing
    * TEXKIL and/or depth-write.  However, it will break on the
    * combination of xor-logicop plus alphatest.
    *
    * Ultimately, we could implement alphatest in the shader using
    * texkil prior to overriding the outgoing fragment color.
    *   
    * SVGA_NEW_BLEND
    */
   if (svga->curr.blend->need_white_fragments) {
      key->white_fragments = 1;
   }
   
   /* XXX: want to limit this to the textures that the shader actually
    * refers to.
    *
    * SVGA_NEW_TEXTURE_BINDING | SVGA_NEW_SAMPLER
    */
   for (i = 0; i < svga->curr.num_sampler_views; i++) {
      if (svga->curr.sampler_views[i]) {
         assert(svga->curr.sampler[i]);
         assert(svga->curr.sampler_views[i]->texture);
         key->tex[i].texture_target = svga->curr.sampler_views[i]->texture->target;
         if (!svga->curr.sampler[i]->normalized_coords) {
            key->tex[i].width_height_idx = idx++;
            key->tex[i].unnormalized = TRUE;
            ++key->num_unnormalized_coords;
         }

         key->tex[i].swizzle_r = svga->curr.sampler_views[i]->swizzle_r;
         key->tex[i].swizzle_g = svga->curr.sampler_views[i]->swizzle_g;
         key->tex[i].swizzle_b = svga->curr.sampler_views[i]->swizzle_b;
         key->tex[i].swizzle_a = svga->curr.sampler_views[i]->swizzle_a;
      }
   }
   key->num_textures = svga->curr.num_sampler_views;

   idx = 0;
   for (i = 0; i < svga->curr.num_samplers; ++i) {
      if (svga->curr.sampler[i]) {
         key->tex[i].compare_mode = svga->curr.sampler[i]->compare_mode;
         key->tex[i].compare_func = svga->curr.sampler[i]->compare_func;
      }
   }

   /* sprite coord gen state */
   for (i = 0; i < svga->curr.num_samplers; ++i) {
      key->tex[i].sprite_texgen =
         svga->curr.rast->templ.sprite_coord_enable & (1 << i);
   }

   key->sprite_origin_lower_left = (svga->curr.rast->templ.sprite_coord_mode
                                    == PIPE_SPRITE_COORD_LOWER_LEFT);

   return PIPE_OK;
}



static enum pipe_error
emit_hw_fs(struct svga_context *svga, unsigned dirty)
{
   struct svga_shader_result *result = NULL;
   unsigned id = SVGA3D_INVALID_ID;
   enum pipe_error ret = PIPE_OK;

   struct svga_fragment_shader *fs = svga->curr.fs;
   struct svga_fs_compile_key key;

   /* SVGA_NEW_BLEND
    * SVGA_NEW_TEXTURE_BINDING
    * SVGA_NEW_RAST
    * SVGA_NEW_NEED_SWTNL
    * SVGA_NEW_SAMPLER
    */
   ret = make_fs_key( svga, fs, &key );
   if (ret != PIPE_OK)
      return ret;

   result = search_fs_key( fs, &key );
   if (!result) {
      ret = compile_fs( svga, fs, &key, &result );
      if (ret != PIPE_OK)
         return ret;
   }

   assert (result);
   id = result->id;

   assert(id != SVGA3D_INVALID_ID);

   if (result != svga->state.hw_draw.fs) {
      ret = SVGA3D_SetShader(svga->swc,
                             SVGA3D_SHADERTYPE_PS,
                             id );
      if (ret != PIPE_OK)
         return ret;

      svga->dirty |= SVGA_NEW_FS_RESULT;
      svga->state.hw_draw.fs = result;      
   }

   return PIPE_OK;
}

struct svga_tracked_state svga_hw_fs = 
{
   "fragment shader (hwtnl)",
   (SVGA_NEW_FS |
    SVGA_NEW_TEXTURE_BINDING |
    SVGA_NEW_NEED_SWTNL |
    SVGA_NEW_RAST |
    SVGA_NEW_SAMPLER |
    SVGA_NEW_BLEND),
   emit_hw_fs
};



