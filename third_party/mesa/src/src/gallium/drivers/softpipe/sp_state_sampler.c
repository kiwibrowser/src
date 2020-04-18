/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
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
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

/* Authors:
 *  Brian Paul
 */

#include "util/u_memory.h"
#include "util/u_inlines.h"

#include "draw/draw_context.h"

#include "sp_context.h"
#include "sp_state.h"
#include "sp_texture.h"
#include "sp_tex_sample.h"
#include "sp_tex_tile_cache.h"


struct sp_sampler {
   struct pipe_sampler_state base;
   struct sp_sampler_variant *variants;
   struct sp_sampler_variant *current;
};

static struct sp_sampler *sp_sampler( struct pipe_sampler_state *sampler )
{
   return (struct sp_sampler *)sampler;
}


static void *
softpipe_create_sampler_state(struct pipe_context *pipe,
                              const struct pipe_sampler_state *sampler)
{
   struct sp_sampler *sp_sampler = CALLOC_STRUCT(sp_sampler);

   sp_sampler->base = *sampler;
   sp_sampler->variants = NULL;

   return (void *)sp_sampler;
}


/**
 * Bind a range [start, start+num-1] of samplers for a shader stage.
 */
static void
softpipe_bind_sampler_states(struct pipe_context *pipe,
                             unsigned shader,
                             unsigned start,
                             unsigned num,
                             void **samplers)
{
   struct softpipe_context *softpipe = softpipe_context(pipe);
   unsigned i;

   assert(shader < PIPE_SHADER_TYPES);
   assert(start + num <= Elements(softpipe->samplers[shader]));

   /* Check for no-op */
   if (start + num <= softpipe->num_samplers[shader] &&
       !memcmp(softpipe->samplers[shader] + start, samplers,
               num * sizeof(void *))) {
      return;
   }

   draw_flush(softpipe->draw);

   /* set the new samplers */
   for (i = 0; i < num; i++) {
      softpipe->samplers[shader][start + i] = samplers[i];
   }

   /* find highest non-null samplers[] entry */
   {
      unsigned j = MAX2(softpipe->num_samplers[shader], start + num);
      while (j > 0 && softpipe->samplers[shader][j - 1] == NULL)
         j--;
      softpipe->num_samplers[shader] = j;
   }

   if (shader == PIPE_SHADER_VERTEX || shader == PIPE_SHADER_GEOMETRY) {
      draw_set_samplers(softpipe->draw,
                        shader,
                        softpipe->samplers[shader],
                        softpipe->num_samplers[shader]);
   }

   softpipe->dirty |= SP_NEW_SAMPLER;
}



static void
softpipe_bind_fragment_sampler_states(struct pipe_context *pipe,
                                      unsigned num, void **samplers)
{
   softpipe_bind_sampler_states(pipe, PIPE_SHADER_FRAGMENT, 0, num, samplers);
}


static void
softpipe_bind_vertex_sampler_states(struct pipe_context *pipe,
                                    unsigned num,
                                    void **samplers)
{
   softpipe_bind_sampler_states(pipe, PIPE_SHADER_VERTEX, 0, num, samplers);
}


static void
softpipe_bind_geometry_sampler_states(struct pipe_context *pipe,
                                      unsigned num,
                                      void **samplers)
{
   softpipe_bind_sampler_states(pipe, PIPE_SHADER_GEOMETRY, 0, num, samplers);
}


static struct pipe_sampler_view *
softpipe_create_sampler_view(struct pipe_context *pipe,
                             struct pipe_resource *resource,
                             const struct pipe_sampler_view *templ)
{
   struct pipe_sampler_view *view = CALLOC_STRUCT(pipe_sampler_view);

   if (view) {
      *view = *templ;
      view->reference.count = 1;
      view->texture = NULL;
      pipe_resource_reference(&view->texture, resource);
      view->context = pipe;
   }

   return view;
}


static void
softpipe_sampler_view_destroy(struct pipe_context *pipe,
                              struct pipe_sampler_view *view)
{
   pipe_resource_reference(&view->texture, NULL);
   FREE(view);
}


static void
softpipe_set_sampler_views(struct pipe_context *pipe,
                           unsigned shader,
                           unsigned start,
                           unsigned num,
                           struct pipe_sampler_view **views)
{
   struct softpipe_context *softpipe = softpipe_context(pipe);
   uint i;

   assert(shader < PIPE_SHADER_TYPES);
   assert(start + num <= Elements(softpipe->sampler_views[shader]));

   /* Check for no-op */
   if (start + num <= softpipe->num_sampler_views[shader] &&
       !memcmp(softpipe->sampler_views[shader] + start, views,
               num * sizeof(struct pipe_sampler_view *))) {
      return;
   }

   draw_flush(softpipe->draw);

   /* set the new sampler views */
   for (i = 0; i < num; i++) {
      pipe_sampler_view_reference(&softpipe->sampler_views[shader][start + i],
                                  views[i]);
      sp_tex_tile_cache_set_sampler_view(softpipe->tex_cache[shader][start + i],
                                         views[i]);
   }

   /* find highest non-null sampler_views[] entry */
   {
      unsigned j = MAX2(softpipe->num_sampler_views[shader], start + num);
      while (j > 0 && softpipe->sampler_views[shader][j - 1] == NULL)
         j--;
      softpipe->num_sampler_views[shader] = j;
   }

   if (shader == PIPE_SHADER_VERTEX || shader == PIPE_SHADER_GEOMETRY) {
      draw_set_sampler_views(softpipe->draw,
                             shader,
                             softpipe->sampler_views[shader],
                             softpipe->num_sampler_views[shader]);
   }

   softpipe->dirty |= SP_NEW_TEXTURE;
}


static void
softpipe_set_fragment_sampler_views(struct pipe_context *pipe,
                                    unsigned num,
                                    struct pipe_sampler_view **views)
{
   softpipe_set_sampler_views(pipe, PIPE_SHADER_FRAGMENT, 0, num, views);
}


static void
softpipe_set_vertex_sampler_views(struct pipe_context *pipe,
                                  unsigned num,
                                  struct pipe_sampler_view **views)
{
   softpipe_set_sampler_views(pipe, PIPE_SHADER_VERTEX, 0, num, views);
}


static void
softpipe_set_geometry_sampler_views(struct pipe_context *pipe,
                                    unsigned num,
                                    struct pipe_sampler_view **views)
{
   softpipe_set_sampler_views(pipe, PIPE_SHADER_GEOMETRY, 0, num, views);
}


/**
 * Find/create an sp_sampler_variant object for sampling the given texture,
 * sampler and tex unit.
 *
 * Note that the tex unit is significant.  We can't re-use a sampler
 * variant for multiple texture units because the sampler variant contains
 * the texture object pointer.  If the texture object pointer were stored
 * somewhere outside the sampler variant, we could re-use samplers for
 * multiple texture units.
 */
static struct sp_sampler_variant *
get_sampler_variant( unsigned unit,
                     struct sp_sampler *sampler,
                     struct pipe_sampler_view *view,
                     unsigned processor )
{
   struct softpipe_resource *sp_texture = softpipe_resource(view->texture);
   struct sp_sampler_variant *v = NULL;
   union sp_sampler_key key;

   /* if this fails, widen the key.unit field and update this assertion */
   assert(PIPE_MAX_SAMPLERS <= 16);

   key.bits.target = sp_texture->base.target;
   key.bits.is_pot = sp_texture->pot;
   key.bits.processor = processor;
   key.bits.unit = unit;
   key.bits.swizzle_r = view->swizzle_r;
   key.bits.swizzle_g = view->swizzle_g;
   key.bits.swizzle_b = view->swizzle_b;
   key.bits.swizzle_a = view->swizzle_a;
   key.bits.pad = 0;

   if (sampler->current && 
       key.value == sampler->current->key.value) {
      v = sampler->current;
   }

   if (v == NULL) {
      for (v = sampler->variants; v; v = v->next)
         if (v->key.value == key.value)
            break;

      if (v == NULL) {
         v = sp_create_sampler_variant( &sampler->base, key );
         v->next = sampler->variants;
         sampler->variants = v;
      }
   }
   
   sampler->current = v;
   return v;
}


/**
 * Reset the sampler variants for a shader stage (vert, frag, geom).
 */
static void
reset_sampler_variants(struct softpipe_context *softpipe,
                       unsigned shader,
                       unsigned tgsi_shader,
                       int max_sampler)
{
   int i;

   for (i = 0; i <= max_sampler; i++) {
      if (softpipe->samplers[shader][i]) {
         softpipe->tgsi.samplers_list[shader][i] = 
            get_sampler_variant(i,
                                sp_sampler(softpipe->samplers[shader][i]),
                                softpipe->sampler_views[shader][i],
                                tgsi_shader);

         sp_sampler_variant_bind_view(softpipe->tgsi.samplers_list[shader][i],
                                      softpipe->tex_cache[shader][i],
                                      softpipe->sampler_views[shader][i]);
      }
   }
}


void
softpipe_reset_sampler_variants(struct softpipe_context *softpipe)
{
   /* It's a bit hard to build these samplers ahead of time -- don't
    * really know which samplers are going to be used for vertex and
    * fragment programs.
    */

   /* XXX note: PIPE_SHADER_x != TGSI_PROCESSOR_x (fix that someday) */
   reset_sampler_variants(softpipe,
                          PIPE_SHADER_VERTEX,
                          TGSI_PROCESSOR_VERTEX,
                          softpipe->vs->max_sampler);

   reset_sampler_variants(softpipe,
                          PIPE_SHADER_FRAGMENT,
                          TGSI_PROCESSOR_FRAGMENT,
                          softpipe->fs_variant->info.file_max[TGSI_FILE_SAMPLER]);

   if (softpipe->gs) {
      reset_sampler_variants(softpipe,
                             PIPE_SHADER_GEOMETRY,
                             TGSI_PROCESSOR_GEOMETRY,
                             softpipe->gs->max_sampler);
   }
}


static void
softpipe_delete_sampler_state(struct pipe_context *pipe,
                              void *sampler)
{
   struct sp_sampler *sp_sampler = (struct sp_sampler *)sampler;
   struct sp_sampler_variant *v, *tmp;

   for (v = sp_sampler->variants; v; v = tmp) {
      tmp = v->next;
      sp_sampler_variant_destroy(v);
   }

   FREE( sampler );
}


void
softpipe_init_sampler_funcs(struct pipe_context *pipe)
{
   pipe->create_sampler_state = softpipe_create_sampler_state;
   pipe->bind_fragment_sampler_states  = softpipe_bind_fragment_sampler_states;
   pipe->bind_vertex_sampler_states = softpipe_bind_vertex_sampler_states;
   pipe->bind_geometry_sampler_states = softpipe_bind_geometry_sampler_states;
   pipe->delete_sampler_state = softpipe_delete_sampler_state;

   pipe->set_fragment_sampler_views = softpipe_set_fragment_sampler_views;
   pipe->set_vertex_sampler_views = softpipe_set_vertex_sampler_views;
   pipe->set_geometry_sampler_views = softpipe_set_geometry_sampler_views;

   pipe->create_sampler_view = softpipe_create_sampler_view;
   pipe->sampler_view_destroy = softpipe_sampler_view_destroy;
}

