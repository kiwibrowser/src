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

/**
 * Tiling engine.
 *
 * Builds per-tile display lists and executes them on calls to
 * lp_setup_flush().
 */

#include <limits.h>

#include "pipe/p_defines.h"
#include "util/u_framebuffer.h"
#include "util/u_inlines.h"
#include "util/u_memory.h"
#include "util/u_pack_color.h"
#include "draw/draw_pipe.h"
#include "lp_context.h"
#include "lp_memory.h"
#include "lp_scene.h"
#include "lp_texture.h"
#include "lp_debug.h"
#include "lp_fence.h"
#include "lp_query.h"
#include "lp_rast.h"
#include "lp_setup_context.h"
#include "lp_screen.h"
#include "lp_state.h"
#include "state_tracker/sw_winsys.h"

#include "draw/draw_context.h"
#include "draw/draw_vbuf.h"


static boolean set_scene_state( struct lp_setup_context *, enum setup_state,
                             const char *reason);
static boolean try_update_scene_state( struct lp_setup_context *setup );


static void
lp_setup_get_empty_scene(struct lp_setup_context *setup)
{
   assert(setup->scene == NULL);

   setup->scene_idx++;
   setup->scene_idx %= Elements(setup->scenes);

   setup->scene = setup->scenes[setup->scene_idx];

   if (setup->scene->fence) {
      if (LP_DEBUG & DEBUG_SETUP)
         debug_printf("%s: wait for scene %d\n",
                      __FUNCTION__, setup->scene->fence->id);

      lp_fence_wait(setup->scene->fence);
   }

   lp_scene_begin_binning(setup->scene, &setup->fb);
   
}


static void
first_triangle( struct lp_setup_context *setup,
                const float (*v0)[4],
                const float (*v1)[4],
                const float (*v2)[4])
{
   assert(setup->state == SETUP_ACTIVE);
   lp_setup_choose_triangle( setup );
   setup->triangle( setup, v0, v1, v2 );
}

static void
first_line( struct lp_setup_context *setup,
	    const float (*v0)[4],
	    const float (*v1)[4])
{
   assert(setup->state == SETUP_ACTIVE);
   lp_setup_choose_line( setup );
   setup->line( setup, v0, v1 );
}

static void
first_point( struct lp_setup_context *setup,
	     const float (*v0)[4])
{
   assert(setup->state == SETUP_ACTIVE);
   lp_setup_choose_point( setup );
   setup->point( setup, v0 );
}

void lp_setup_reset( struct lp_setup_context *setup )
{
   LP_DBG(DEBUG_SETUP, "%s\n", __FUNCTION__);

   /* Reset derived state */
   setup->constants.stored_size = 0;
   setup->constants.stored_data = NULL;
   setup->fs.stored = NULL;
   setup->dirty = ~0;

   /* no current bin */
   setup->scene = NULL;

   /* Reset some state:
    */
   memset(&setup->clear, 0, sizeof setup->clear);

   /* Have an explicit "start-binning" call and get rid of this
    * pointer twiddling?
    */
   setup->line = first_line;
   setup->point = first_point;
   setup->triangle = first_triangle;
}


/** Rasterize all scene's bins */
static void
lp_setup_rasterize_scene( struct lp_setup_context *setup )
{
   struct lp_scene *scene = setup->scene;
   struct llvmpipe_screen *screen = llvmpipe_screen(scene->pipe->screen);

   lp_scene_end_binning(scene);

   lp_fence_reference(&setup->last_fence, scene->fence);

   if (setup->last_fence)
      setup->last_fence->issued = TRUE;

   pipe_mutex_lock(screen->rast_mutex);
   lp_rast_queue_scene(screen->rast, scene);
   lp_rast_finish(screen->rast);
   pipe_mutex_unlock(screen->rast_mutex);

   lp_scene_end_rasterization(setup->scene);
   lp_setup_reset( setup );

   LP_DBG(DEBUG_SETUP, "%s done \n", __FUNCTION__);
}



static boolean
begin_binning( struct lp_setup_context *setup )
{
   struct lp_scene *scene = setup->scene;
   boolean need_zsload = FALSE;
   boolean ok;
   unsigned i, j;

   assert(scene);
   assert(scene->fence == NULL);

   /* Always create a fence:
    */
   scene->fence = lp_fence_create(MAX2(1, setup->num_threads));
   if (!scene->fence)
      return FALSE;

   /* Initialize the bin flags and x/y coords:
    */
   for (i = 0; i < scene->tiles_x; i++) {
      for (j = 0; j < scene->tiles_y; j++) {
         scene->tile[i][j].x = i;
         scene->tile[i][j].y = j;
      }
   }

   ok = try_update_scene_state(setup);
   if (!ok)
      return FALSE;

   if (setup->fb.zsbuf &&
       ((setup->clear.flags & PIPE_CLEAR_DEPTHSTENCIL) != PIPE_CLEAR_DEPTHSTENCIL) &&
        util_format_is_depth_and_stencil(setup->fb.zsbuf->format))
      need_zsload = TRUE;

   LP_DBG(DEBUG_SETUP, "%s color: %s depth: %s\n", __FUNCTION__,
          (setup->clear.flags & PIPE_CLEAR_COLOR) ? "clear": "load",
          need_zsload ? "clear": "load");

   if (setup->fb.nr_cbufs) {
      if (setup->clear.flags & PIPE_CLEAR_COLOR) {
         ok = lp_scene_bin_everywhere( scene, 
                                       LP_RAST_OP_CLEAR_COLOR, 
                                       setup->clear.color );
         if (!ok)
            return FALSE;
      }
   }

   if (setup->fb.zsbuf) {
      if (setup->clear.flags & PIPE_CLEAR_DEPTHSTENCIL) {
         if (!need_zsload)
            scene->has_depthstencil_clear = TRUE;

         ok = lp_scene_bin_everywhere( scene,
                                       LP_RAST_OP_CLEAR_ZSTENCIL,
                                       lp_rast_arg_clearzs(
                                          setup->clear.zsvalue,
                                          setup->clear.zsmask));
         if (!ok)
            return FALSE;
      }
   }

   if (setup->active_query) {
      ok = lp_scene_bin_everywhere( scene,
                                    LP_RAST_OP_BEGIN_QUERY,
                                    lp_rast_arg_query(setup->active_query) );
      if (!ok)
         return FALSE;
   }

   setup->clear.flags = 0;
   setup->clear.zsmask = 0;
   setup->clear.zsvalue = 0;

   LP_DBG(DEBUG_SETUP, "%s done\n", __FUNCTION__);
   return TRUE;
}


/* This basically bins and then flushes any outstanding full-screen
 * clears.  
 *
 * TODO: fast path for fullscreen clears and no triangles.
 */
static boolean
execute_clears( struct lp_setup_context *setup )
{
   LP_DBG(DEBUG_SETUP, "%s\n", __FUNCTION__);

   return begin_binning( setup );
}

const char *states[] = {
   "FLUSHED",
   "CLEARED",
   "ACTIVE "
};


static boolean
set_scene_state( struct lp_setup_context *setup,
                 enum setup_state new_state,
                 const char *reason)
{
   unsigned old_state = setup->state;

   if (old_state == new_state)
      return TRUE;
   
   if (LP_DEBUG & DEBUG_SCENE) {
      debug_printf("%s old %s new %s%s%s\n",
                   __FUNCTION__,
                   states[old_state],
                   states[new_state],
                   (new_state == SETUP_FLUSHED) ? ": " : "",
                   (new_state == SETUP_FLUSHED) ? reason : "");

      if (new_state == SETUP_FLUSHED && setup->scene)
         lp_debug_draw_bins_by_cmd_length(setup->scene);
   }

   /* wait for a free/empty scene
    */
   if (old_state == SETUP_FLUSHED) 
      lp_setup_get_empty_scene(setup);

   switch (new_state) {
   case SETUP_CLEARED:
      break;

   case SETUP_ACTIVE:
      if (!begin_binning( setup ))
         goto fail;
      break;

   case SETUP_FLUSHED:
      if (old_state == SETUP_CLEARED)
         if (!execute_clears( setup ))
            goto fail;

      lp_setup_rasterize_scene( setup );
      assert(setup->scene == NULL);
      break;

   default:
      assert(0 && "invalid setup state mode");
      goto fail;
   }

   setup->state = new_state;
   return TRUE;

fail:
   if (setup->scene) {
      lp_scene_end_rasterization(setup->scene);
      setup->scene = NULL;
   }

   setup->state = SETUP_FLUSHED;
   lp_setup_reset( setup );
   return FALSE;
}


void
lp_setup_flush( struct lp_setup_context *setup,
                struct pipe_fence_handle **fence,
                const char *reason)
{
   set_scene_state( setup, SETUP_FLUSHED, reason );

   if (fence) {
      lp_fence_reference((struct lp_fence **)fence, setup->last_fence);
   }
}


void
lp_setup_bind_framebuffer( struct lp_setup_context *setup,
                           const struct pipe_framebuffer_state *fb )
{
   LP_DBG(DEBUG_SETUP, "%s\n", __FUNCTION__);

   /* Flush any old scene.
    */
   set_scene_state( setup, SETUP_FLUSHED, __FUNCTION__ );

   /*
    * Ensure the old scene is not reused.
    */
   assert(!setup->scene);

   /* Set new state.  This will be picked up later when we next need a
    * scene.
    */
   util_copy_framebuffer_state(&setup->fb, fb);
   setup->framebuffer.x0 = 0;
   setup->framebuffer.y0 = 0;
   setup->framebuffer.x1 = fb->width-1;
   setup->framebuffer.y1 = fb->height-1;
   setup->dirty |= LP_SETUP_NEW_SCISSOR;
}


static boolean
lp_setup_try_clear( struct lp_setup_context *setup,
                    const float *color,
                    double depth,
                    unsigned stencil,
                    unsigned flags )
{
   uint32_t zsmask = 0;
   uint32_t zsvalue = 0;
   union lp_rast_cmd_arg color_arg;
   unsigned i;

   LP_DBG(DEBUG_SETUP, "%s state %d\n", __FUNCTION__, setup->state);

   if (flags & PIPE_CLEAR_COLOR) {
      for (i = 0; i < 4; i++)
         color_arg.clear_color[i] = float_to_ubyte(color[i]);
   }

   if (flags & PIPE_CLEAR_DEPTHSTENCIL) {
      uint32_t zmask = (flags & PIPE_CLEAR_DEPTH) ? ~0 : 0;
      uint32_t smask = (flags & PIPE_CLEAR_STENCIL) ? ~0 : 0;

      zsvalue = util_pack_z_stencil(setup->fb.zsbuf->format,
                                    depth,
                                    stencil);


      zsmask = util_pack_mask_z_stencil(setup->fb.zsbuf->format,
                                        zmask,
                                        smask);

      zsvalue &= zsmask;
   }

   if (setup->state == SETUP_ACTIVE) {
      struct lp_scene *scene = setup->scene;

      /* Add the clear to existing scene.  In the unusual case where
       * both color and depth-stencil are being cleared when there's
       * already been some rendering, we could discard the currently
       * binned scene and start again, but I don't see that as being
       * a common usage.
       */
      if (flags & PIPE_CLEAR_COLOR) {
         if (!lp_scene_bin_everywhere( scene, 
                                       LP_RAST_OP_CLEAR_COLOR,
                                       color_arg ))
            return FALSE;
      }

      if (flags & PIPE_CLEAR_DEPTHSTENCIL) {
         if (!lp_scene_bin_everywhere( scene,
                                       LP_RAST_OP_CLEAR_ZSTENCIL,
                                       lp_rast_arg_clearzs(zsvalue, zsmask) ))
            return FALSE;
      }
   }
   else {
      /* Put ourselves into the 'pre-clear' state, specifically to try
       * and accumulate multiple clears to color and depth_stencil
       * buffers which the app or state-tracker might issue
       * separately.
       */
      set_scene_state( setup, SETUP_CLEARED, __FUNCTION__ );

      setup->clear.flags |= flags;

      if (flags & PIPE_CLEAR_DEPTHSTENCIL) {
         setup->clear.zsmask |= zsmask;
         setup->clear.zsvalue =
            (setup->clear.zsvalue & ~zsmask) | (zsvalue & zsmask);
      }

      if (flags & PIPE_CLEAR_COLOR) {
         memcpy(setup->clear.color.clear_color,
                &color_arg,
                sizeof setup->clear.color.clear_color);
      }
   }
   
   return TRUE;
}

void
lp_setup_clear( struct lp_setup_context *setup,
                const float *color,
                double depth,
                unsigned stencil,
                unsigned flags )
{
   if (!lp_setup_try_clear( setup, color, depth, stencil, flags )) {
      lp_setup_flush(setup, NULL, __FUNCTION__);

      if (!lp_setup_try_clear( setup, color, depth, stencil, flags ))
         assert(0);
   }
}





void 
lp_setup_set_triangle_state( struct lp_setup_context *setup,
                             unsigned cull_mode,
                             boolean ccw_is_frontface,
                             boolean scissor,
                             boolean gl_rasterization_rules)
{
   LP_DBG(DEBUG_SETUP, "%s\n", __FUNCTION__);

   setup->ccw_is_frontface = ccw_is_frontface;
   setup->cullmode = cull_mode;
   setup->triangle = first_triangle;
   setup->pixel_offset = gl_rasterization_rules ? 0.5f : 0.0f;

   if (setup->scissor_test != scissor) {
      setup->dirty |= LP_SETUP_NEW_SCISSOR;
      setup->scissor_test = scissor;
   }
}

void 
lp_setup_set_line_state( struct lp_setup_context *setup,
			 float line_width)
{
   LP_DBG(DEBUG_SETUP, "%s\n", __FUNCTION__);

   setup->line_width = line_width;
}

void 
lp_setup_set_point_state( struct lp_setup_context *setup,
                          float point_size,                          
                          boolean point_size_per_vertex,
                          uint sprite_coord_enable,
                          uint sprite_coord_origin)
{
   LP_DBG(DEBUG_SETUP, "%s\n", __FUNCTION__);

   setup->point_size = point_size;
   setup->sprite_coord_enable = sprite_coord_enable;
   setup->sprite_coord_origin = sprite_coord_origin;
   setup->point_size_per_vertex = point_size_per_vertex;
}

void
lp_setup_set_setup_variant( struct lp_setup_context *setup,
			    const struct lp_setup_variant *variant)
{
   LP_DBG(DEBUG_SETUP, "%s\n", __FUNCTION__);
   
   setup->setup.variant = variant;
}

void
lp_setup_set_fs_variant( struct lp_setup_context *setup,
                         struct lp_fragment_shader_variant *variant)
{
   LP_DBG(DEBUG_SETUP, "%s %p\n", __FUNCTION__,
          variant);
   /* FIXME: reference count */

   setup->fs.current.variant = variant;
   setup->dirty |= LP_SETUP_NEW_FS;
}

void
lp_setup_set_fs_constants(struct lp_setup_context *setup,
                          struct pipe_resource *buffer)
{
   LP_DBG(DEBUG_SETUP, "%s %p\n", __FUNCTION__, (void *) buffer);

   pipe_resource_reference(&setup->constants.current, buffer);

   setup->dirty |= LP_SETUP_NEW_CONSTANTS;
}


void
lp_setup_set_alpha_ref_value( struct lp_setup_context *setup,
                              float alpha_ref_value )
{
   LP_DBG(DEBUG_SETUP, "%s %f\n", __FUNCTION__, alpha_ref_value);

   if(setup->fs.current.jit_context.alpha_ref_value != alpha_ref_value) {
      setup->fs.current.jit_context.alpha_ref_value = alpha_ref_value;
      setup->dirty |= LP_SETUP_NEW_FS;
   }
}

void
lp_setup_set_stencil_ref_values( struct lp_setup_context *setup,
                                 const ubyte refs[2] )
{
   LP_DBG(DEBUG_SETUP, "%s %d %d\n", __FUNCTION__, refs[0], refs[1]);

   if (setup->fs.current.jit_context.stencil_ref_front != refs[0] ||
       setup->fs.current.jit_context.stencil_ref_back != refs[1]) {
      setup->fs.current.jit_context.stencil_ref_front = refs[0];
      setup->fs.current.jit_context.stencil_ref_back = refs[1];
      setup->dirty |= LP_SETUP_NEW_FS;
   }
}

void
lp_setup_set_blend_color( struct lp_setup_context *setup,
                          const struct pipe_blend_color *blend_color )
{
   LP_DBG(DEBUG_SETUP, "%s\n", __FUNCTION__);

   assert(blend_color);

   if(memcmp(&setup->blend_color.current, blend_color, sizeof *blend_color) != 0) {
      memcpy(&setup->blend_color.current, blend_color, sizeof *blend_color);
      setup->dirty |= LP_SETUP_NEW_BLEND_COLOR;
   }
}


void
lp_setup_set_scissor( struct lp_setup_context *setup,
                      const struct pipe_scissor_state *scissor )
{
   LP_DBG(DEBUG_SETUP, "%s\n", __FUNCTION__);

   assert(scissor);

   setup->scissor.x0 = scissor->minx;
   setup->scissor.x1 = scissor->maxx-1;
   setup->scissor.y0 = scissor->miny;
   setup->scissor.y1 = scissor->maxy-1;
   setup->dirty |= LP_SETUP_NEW_SCISSOR;
}


void 
lp_setup_set_flatshade_first( struct lp_setup_context *setup,
                              boolean flatshade_first )
{
   setup->flatshade_first = flatshade_first;
}


void 
lp_setup_set_vertex_info( struct lp_setup_context *setup,
                          struct vertex_info *vertex_info )
{
   /* XXX: just silently holding onto the pointer:
    */
   setup->vertex_info = vertex_info;
}


/**
 * Called during state validation when LP_NEW_SAMPLER_VIEW is set.
 */
void
lp_setup_set_fragment_sampler_views(struct lp_setup_context *setup,
                                    unsigned num,
                                    struct pipe_sampler_view **views)
{
   unsigned i;

   LP_DBG(DEBUG_SETUP, "%s\n", __FUNCTION__);

   assert(num <= PIPE_MAX_SAMPLERS);

   for (i = 0; i < PIPE_MAX_SAMPLERS; i++) {
      struct pipe_sampler_view *view = i < num ? views[i] : NULL;

      if (view) {
         struct pipe_resource *tex = view->texture;
         struct llvmpipe_resource *lp_tex = llvmpipe_resource(tex);
         struct lp_jit_texture *jit_tex;
         jit_tex = &setup->fs.current.jit_context.textures[i];
         jit_tex->width = tex->width0;
         jit_tex->height = tex->height0;
         jit_tex->depth = tex->depth0;
         jit_tex->first_level = view->u.tex.first_level;
         jit_tex->last_level = tex->last_level;

         /* We're referencing the texture's internal data, so save a
          * reference to it.
          */
         pipe_resource_reference(&setup->fs.current_tex[i], tex);

         if (!lp_tex->dt) {
            /* regular texture - setup array of mipmap level pointers */
            int j;
            for (j = view->u.tex.first_level; j <= tex->last_level; j++) {
               jit_tex->data[j] =
                  llvmpipe_get_texture_image_all(lp_tex, j, LP_TEX_USAGE_READ,
                                                 LP_TEX_LAYOUT_LINEAR);
               jit_tex->row_stride[j] = lp_tex->row_stride[j];
               jit_tex->img_stride[j] = lp_tex->img_stride[j];

               if ((LP_PERF & PERF_TEX_MEM) ||
		   !jit_tex->data[j]) {
                  /* out of memory - use dummy tile memory */
                  jit_tex->data[j] = lp_dummy_tile;
                  jit_tex->width = TILE_SIZE/8;
                  jit_tex->height = TILE_SIZE/8;
                  jit_tex->depth = 1;
                  jit_tex->first_level = 0;
                  jit_tex->last_level = 0;
                  jit_tex->row_stride[j] = 0;
                  jit_tex->img_stride[j] = 0;
               }
            }
         }
         else {
            /* display target texture/surface */
            /*
             * XXX: Where should this be unmapped?
             */
            struct llvmpipe_screen *screen = llvmpipe_screen(tex->screen);
            struct sw_winsys *winsys = screen->winsys;
            jit_tex->data[0] = winsys->displaytarget_map(winsys, lp_tex->dt,
							 PIPE_TRANSFER_READ);
            jit_tex->row_stride[0] = lp_tex->row_stride[0];
            jit_tex->img_stride[0] = lp_tex->img_stride[0];
            assert(jit_tex->data[0]);
         }
      }
   }

   setup->dirty |= LP_SETUP_NEW_FS;
}


/**
 * Called during state validation when LP_NEW_SAMPLER is set.
 */
void
lp_setup_set_fragment_sampler_state(struct lp_setup_context *setup,
                                    unsigned num,
                                    struct pipe_sampler_state **samplers)
{
   unsigned i;

   LP_DBG(DEBUG_SETUP, "%s\n", __FUNCTION__);

   assert(num <= PIPE_MAX_SAMPLERS);

   for (i = 0; i < PIPE_MAX_SAMPLERS; i++) {
      const struct pipe_sampler_state *sampler = i < num ? samplers[i] : NULL;

      if (sampler) {
         struct lp_jit_texture *jit_tex;
         jit_tex = &setup->fs.current.jit_context.textures[i];

         jit_tex->min_lod = sampler->min_lod;
         jit_tex->max_lod = sampler->max_lod;
         jit_tex->lod_bias = sampler->lod_bias;
         COPY_4V(jit_tex->border_color, sampler->border_color.f);
      }
   }

   setup->dirty |= LP_SETUP_NEW_FS;
}


/**
 * Is the given texture referenced by any scene?
 * Note: we have to check all scenes including any scenes currently
 * being rendered and the current scene being built.
 */
unsigned
lp_setup_is_resource_referenced( const struct lp_setup_context *setup,
                                const struct pipe_resource *texture )
{
   unsigned i;

   /* check the render targets */
   for (i = 0; i < setup->fb.nr_cbufs; i++) {
      if (setup->fb.cbufs[i]->texture == texture)
         return LP_REFERENCED_FOR_READ | LP_REFERENCED_FOR_WRITE;
   }
   if (setup->fb.zsbuf && setup->fb.zsbuf->texture == texture) {
      return LP_REFERENCED_FOR_READ | LP_REFERENCED_FOR_WRITE;
   }

   /* check textures referenced by the scene */
   for (i = 0; i < Elements(setup->scenes); i++) {
      if (lp_scene_is_resource_referenced(setup->scenes[i], texture)) {
         return LP_REFERENCED_FOR_READ;
      }
   }

   return LP_UNREFERENCED;
}


/**
 * Called by vbuf code when we're about to draw something.
 */
static boolean
try_update_scene_state( struct lp_setup_context *setup )
{
   boolean new_scene = (setup->fs.stored == NULL);
   struct lp_scene *scene = setup->scene;

   assert(scene);

   if(setup->dirty & LP_SETUP_NEW_BLEND_COLOR) {
      uint8_t *stored;
      unsigned i, j;

      stored = lp_scene_alloc_aligned(scene, 4 * 16, 16);
      if (!stored) {
         assert(!new_scene);
         return FALSE;
      }

      /* smear each blend color component across 16 ubyte elements */
      for (i = 0; i < 4; ++i) {
         uint8_t c = float_to_ubyte(setup->blend_color.current.color[i]);
         for (j = 0; j < 16; ++j)
            stored[i*16 + j] = c;
      }

      setup->blend_color.stored = stored;
      setup->fs.current.jit_context.blend_color = setup->blend_color.stored;
      setup->dirty |= LP_SETUP_NEW_FS;
   }

   if(setup->dirty & LP_SETUP_NEW_CONSTANTS) {
      struct pipe_resource *buffer = setup->constants.current;

      if(buffer) {
         unsigned current_size = buffer->width0;
         const void *current_data = llvmpipe_resource_data(buffer);

         /* TODO: copy only the actually used constants? */

         if(setup->constants.stored_size != current_size ||
            !setup->constants.stored_data ||
            memcmp(setup->constants.stored_data,
                   current_data,
                   current_size) != 0) {
            void *stored;

            stored = lp_scene_alloc(scene, current_size);
            if (!stored) {
               assert(!new_scene);
               return FALSE;
            }

            memcpy(stored,
                   current_data,
                   current_size);
            setup->constants.stored_size = current_size;
            setup->constants.stored_data = stored;
         }
      }
      else {
         setup->constants.stored_size = 0;
         setup->constants.stored_data = NULL;
      }

      setup->fs.current.jit_context.constants = setup->constants.stored_data;
      setup->dirty |= LP_SETUP_NEW_FS;
   }


   if (setup->dirty & LP_SETUP_NEW_FS) {
      if (!setup->fs.stored ||
          memcmp(setup->fs.stored,
                 &setup->fs.current,
                 sizeof setup->fs.current) != 0)
      {
         struct lp_rast_state *stored;
         uint i;
         
         /* The fs state that's been stored in the scene is different from
          * the new, current state.  So allocate a new lp_rast_state object
          * and append it to the bin's setup data buffer.
          */
         stored = (struct lp_rast_state *) lp_scene_alloc(scene, sizeof *stored);
         if (!stored) {
            assert(!new_scene);
            return FALSE;
         }

         memcpy(stored,
                &setup->fs.current,
                sizeof setup->fs.current);
         setup->fs.stored = stored;
         
         /* The scene now references the textures in the rasterization
          * state record.  Note that now.
          */
         for (i = 0; i < Elements(setup->fs.current_tex); i++) {
            if (setup->fs.current_tex[i]) {
               if (!lp_scene_add_resource_reference(scene,
                                                    setup->fs.current_tex[i],
                                                    new_scene)) {
                  assert(!new_scene);
                  return FALSE;
               }
            }
         }
      }
   }

   if (setup->dirty & LP_SETUP_NEW_SCISSOR) {
      setup->draw_region = setup->framebuffer;
      if (setup->scissor_test) {
         u_rect_possible_intersection(&setup->scissor,
                                      &setup->draw_region);
      }
   }
                                      
   setup->dirty = 0;

   assert(setup->fs.stored);
   return TRUE;
}

boolean
lp_setup_update_state( struct lp_setup_context *setup,
                       boolean update_scene )
{
   /* Some of the 'draw' pipeline stages may have changed some driver state.
    * Make sure we've processed those state changes before anything else.
    *
    * XXX this is the only place where llvmpipe_context is used in the
    * setup code.  This may get refactored/changed...
    */
   {
      struct llvmpipe_context *lp = llvmpipe_context(setup->pipe);
      if (lp->dirty) {
         llvmpipe_update_derived(lp);
      }

      if (lp->setup->dirty) {
         llvmpipe_update_setup(lp);
      }

      assert(setup->setup.variant);

      /* Will probably need to move this somewhere else, just need  
       * to know about vertex shader point size attribute.
       */
      setup->psize = lp->psize_slot;

      assert(lp->dirty == 0);

      assert(lp->setup_variant.key.size == 
	     setup->setup.variant->key.size);

      assert(memcmp(&lp->setup_variant.key,
		    &setup->setup.variant->key,
		    setup->setup.variant->key.size) == 0);
   }

   if (update_scene && setup->state != SETUP_ACTIVE) {
      if (!set_scene_state( setup, SETUP_ACTIVE, __FUNCTION__ ))
         return FALSE;
   }

   /* Only call into update_scene_state() if we already have a
    * scene:
    */
   if (update_scene && setup->scene) {
      assert(setup->state == SETUP_ACTIVE);

      if (try_update_scene_state(setup))
         return TRUE;

      /* Update failed, try to restart the scene.
       *
       * Cannot call lp_setup_flush_and_restart() directly here
       * because of potential recursion.
       */
      if (!set_scene_state(setup, SETUP_FLUSHED, __FUNCTION__))
         return FALSE;

      if (!set_scene_state(setup, SETUP_ACTIVE, __FUNCTION__))
         return FALSE;

      if (!setup->scene)
         return FALSE;

      return try_update_scene_state(setup);
   }

   return TRUE;
}



/* Only caller is lp_setup_vbuf_destroy()
 */
void 
lp_setup_destroy( struct lp_setup_context *setup )
{
   uint i;

   lp_setup_reset( setup );

   util_unreference_framebuffer_state(&setup->fb);

   for (i = 0; i < Elements(setup->fs.current_tex); i++) {
      pipe_resource_reference(&setup->fs.current_tex[i], NULL);
   }

   pipe_resource_reference(&setup->constants.current, NULL);

   /* free the scenes in the 'empty' queue */
   for (i = 0; i < Elements(setup->scenes); i++) {
      struct lp_scene *scene = setup->scenes[i];

      if (scene->fence)
         lp_fence_wait(scene->fence);

      lp_scene_destroy(scene);
   }

   lp_fence_reference(&setup->last_fence, NULL);

   FREE( setup );
}


/**
 * Create a new primitive tiling engine.  Plug it into the backend of
 * the draw module.  Currently also creates a rasterizer to use with
 * it.
 */
struct lp_setup_context *
lp_setup_create( struct pipe_context *pipe,
                 struct draw_context *draw )
{
   struct llvmpipe_screen *screen = llvmpipe_screen(pipe->screen);
   struct lp_setup_context *setup;
   unsigned i;

   setup = CALLOC_STRUCT(lp_setup_context);
   if (!setup) {
      goto no_setup;
   }

   lp_setup_init_vbuf(setup);
   
   /* Used only in update_state():
    */
   setup->pipe = pipe;


   setup->num_threads = screen->num_threads;
   setup->vbuf = draw_vbuf_stage(draw, &setup->base);
   if (!setup->vbuf) {
      goto no_vbuf;
   }

   draw_set_rasterize_stage(draw, setup->vbuf);
   draw_set_render(draw, &setup->base);

   /* create some empty scenes */
   for (i = 0; i < MAX_SCENES; i++) {
      setup->scenes[i] = lp_scene_create( pipe );
      if (!setup->scenes[i]) {
         goto no_scenes;
      }
   }

   setup->triangle = first_triangle;
   setup->line     = first_line;
   setup->point    = first_point;
   
   setup->dirty = ~0;

   return setup;

no_scenes:
   for (i = 0; i < MAX_SCENES; i++) {
      if (setup->scenes[i]) {
         lp_scene_destroy(setup->scenes[i]);
      }
   }

   setup->vbuf->destroy(setup->vbuf);
no_vbuf:
   FREE(setup);
no_setup:
   return NULL;
}


/**
 * Put a BeginQuery command into all bins.
 */
void
lp_setup_begin_query(struct lp_setup_context *setup,
                     struct llvmpipe_query *pq)
{
   /* init the query to its beginning state */
   assert(setup->active_query == NULL);

   set_scene_state(setup, SETUP_ACTIVE, "begin_query");
   
   setup->active_query = pq;

   if (setup->scene) {
      if (!lp_scene_bin_everywhere(setup->scene,
                                   LP_RAST_OP_BEGIN_QUERY,
                                   lp_rast_arg_query(pq))) {

         if (!lp_setup_flush_and_restart(setup))
            return;

         if (!lp_scene_bin_everywhere(setup->scene,
                                      LP_RAST_OP_BEGIN_QUERY,
                                      lp_rast_arg_query(pq))) {
            return;
         }
      }
   }
}


/**
 * Put an EndQuery command into all bins.
 */
void
lp_setup_end_query(struct lp_setup_context *setup, struct llvmpipe_query *pq)
{
   union lp_rast_cmd_arg dummy = { 0 };

   set_scene_state(setup, SETUP_ACTIVE, "end_query");

   assert(setup->active_query == pq);
   setup->active_query = NULL;

   /* Setup will automatically re-issue any query which carried over a
    * scene boundary, and the rasterizer automatically "ends" queries
    * which are active at the end of a scene, so there is no need to
    * retry this commands on failure.
    */
   if (setup->scene) {
      /* pq->fence should be the fence of the *last* scene which
       * contributed to the query result.
       */
      lp_fence_reference(&pq->fence, setup->scene->fence);

      if (!lp_scene_bin_everywhere(setup->scene,
                                   LP_RAST_OP_END_QUERY,
                                   dummy)) {
         lp_setup_flush(setup, NULL, __FUNCTION__);
      }
   }
   else {
      lp_fence_reference(&pq->fence, setup->last_fence);
   }
}


boolean
lp_setup_flush_and_restart(struct lp_setup_context *setup)
{
   if (0) debug_printf("%s\n", __FUNCTION__);

   assert(setup->state == SETUP_ACTIVE);

   if (!set_scene_state(setup, SETUP_FLUSHED, __FUNCTION__))
      return FALSE;
   
   if (!lp_setup_update_state(setup, TRUE))
      return FALSE;

   return TRUE;
}


