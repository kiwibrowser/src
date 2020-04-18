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

#include <limits.h>
#include "util/u_memory.h"
#include "util/u_math.h"
#include "util/u_rect.h"
#include "util/u_surface.h"
#include "util/u_pack_color.h"

#include "lp_scene_queue.h"
#include "lp_debug.h"
#include "lp_fence.h"
#include "lp_perf.h"
#include "lp_query.h"
#include "lp_rast.h"
#include "lp_rast_priv.h"
#include "lp_tile_soa.h"
#include "gallivm/lp_bld_debug.h"
#include "lp_scene.h"
#include "lp_tex_sample.h"


#ifdef DEBUG
int jit_line = 0;
const struct lp_rast_state *jit_state = NULL;
const struct lp_rasterizer_task *jit_task = NULL;
#endif


/**
 * Begin rasterizing a scene.
 * Called once per scene by one thread.
 */
static void
lp_rast_begin( struct lp_rasterizer *rast,
               struct lp_scene *scene )
{

   rast->curr_scene = scene;

   LP_DBG(DEBUG_RAST, "%s\n", __FUNCTION__);

   lp_scene_begin_rasterization( scene );
   lp_scene_bin_iter_begin( scene );
}


static void
lp_rast_end( struct lp_rasterizer *rast )
{
   lp_scene_end_rasterization( rast->curr_scene );

   rast->curr_scene = NULL;

#ifdef DEBUG
   if (0)
      debug_printf("Post render scene: tile unswizzle: %u tile swizzle: %u\n",
                   lp_tile_unswizzle_count, lp_tile_swizzle_count);
#endif
}


/**
 * Begining rasterization of a tile.
 * \param x  window X position of the tile, in pixels
 * \param y  window Y position of the tile, in pixels
 */
static void
lp_rast_tile_begin(struct lp_rasterizer_task *task,
                   const struct cmd_bin *bin)
{
   const struct lp_scene *scene = task->scene;
   enum lp_texture_usage usage;

   LP_DBG(DEBUG_RAST, "%s %d,%d\n", __FUNCTION__, bin->x, bin->y);

   task->bin = bin;
   task->x = bin->x * TILE_SIZE;
   task->y = bin->y * TILE_SIZE;

   /* reset pointers to color tile(s) */
   memset(task->color_tiles, 0, sizeof(task->color_tiles));

   /* get pointer to depth/stencil tile */
   {
      struct pipe_surface *zsbuf = task->scene->fb.zsbuf;
      if (zsbuf) {
         struct llvmpipe_resource *lpt = llvmpipe_resource(zsbuf->texture);

         if (scene->has_depthstencil_clear)
            usage = LP_TEX_USAGE_WRITE_ALL;
         else
            usage = LP_TEX_USAGE_READ_WRITE;

         /* "prime" the tile: convert data from linear to tiled if necessary
          * and update the tile's layout info.
          */
         (void) llvmpipe_get_texture_tile(lpt,
                                          zsbuf->u.tex.first_layer,
                                          zsbuf->u.tex.level,
                                          usage,
                                          task->x,
                                          task->y);
         /* Get actual pointer to the tile data.  Note that depth/stencil
          * data is tiled differently than color data.
          */
         task->depth_tile = lp_rast_get_depth_block_pointer(task,
                                                            task->x,
                                                            task->y);

         assert(task->depth_tile);
      }
      else {
         task->depth_tile = NULL;
      }
   }
}


/**
 * Clear the rasterizer's current color tile.
 * This is a bin command called during bin processing.
 */
static void
lp_rast_clear_color(struct lp_rasterizer_task *task,
                    const union lp_rast_cmd_arg arg)
{
   const struct lp_scene *scene = task->scene;
   const uint8_t *clear_color = arg.clear_color;

   unsigned i;

   LP_DBG(DEBUG_RAST, "%s 0x%x,0x%x,0x%x,0x%x\n", __FUNCTION__, 
              clear_color[0],
              clear_color[1],
              clear_color[2],
              clear_color[3]);

   if (clear_color[0] == clear_color[1] &&
       clear_color[1] == clear_color[2] &&
       clear_color[2] == clear_color[3]) {
      /* clear to grayscale value {x, x, x, x} */
      for (i = 0; i < scene->fb.nr_cbufs; i++) {
         uint8_t *ptr =
            lp_rast_get_color_tile_pointer(task, i, LP_TEX_USAGE_WRITE_ALL);
	 memset(ptr, clear_color[0], TILE_SIZE * TILE_SIZE * 4);
      }
   }
   else {
      /* Non-gray color.
       * Note: if the swizzled tile layout changes (see TILE_PIXEL) this code
       * will need to change.  It'll be pretty obvious when clearing no longer
       * works.
       */
      const unsigned chunk = TILE_SIZE / 4;
      for (i = 0; i < scene->fb.nr_cbufs; i++) {
         uint8_t *c =
            lp_rast_get_color_tile_pointer(task, i, LP_TEX_USAGE_WRITE_ALL);
         unsigned j;

         for (j = 0; j < 4 * TILE_SIZE; j++) {
            memset(c, clear_color[0], chunk);
            c += chunk;
            memset(c, clear_color[1], chunk);
            c += chunk;
            memset(c, clear_color[2], chunk);
            c += chunk;
            memset(c, clear_color[3], chunk);
            c += chunk;
         }
      }
   }

   LP_COUNT(nr_color_tile_clear);
}






/**
 * Clear the rasterizer's current z/stencil tile.
 * This is a bin command called during bin processing.
 */
static void
lp_rast_clear_zstencil(struct lp_rasterizer_task *task,
                       const union lp_rast_cmd_arg arg)
{
   const struct lp_scene *scene = task->scene;
   uint32_t clear_value = arg.clear_zstencil.value;
   uint32_t clear_mask = arg.clear_zstencil.mask;
   const unsigned height = TILE_SIZE / TILE_VECTOR_HEIGHT;
   const unsigned width = TILE_SIZE * TILE_VECTOR_HEIGHT;
   const unsigned block_size = scene->zsbuf.blocksize;
   const unsigned dst_stride = scene->zsbuf.stride * TILE_VECTOR_HEIGHT;
   uint8_t *dst;
   unsigned i, j;

   LP_DBG(DEBUG_RAST, "%s: value=0x%08x, mask=0x%08x\n",
           __FUNCTION__, clear_value, clear_mask);

   /*
    * Clear the area of the swizzled depth/depth buffer matching this tile, in
    * stripes of TILE_VECTOR_HEIGHT x TILE_SIZE at a time.
    *
    * The swizzled depth format is such that the depths for
    * TILE_VECTOR_HEIGHT x TILE_VECTOR_WIDTH pixels have consecutive offsets.
    */

   dst = task->depth_tile;

   clear_value &= clear_mask;

   switch (block_size) {
   case 1:
      assert(clear_mask == 0xff);
      memset(dst, (uint8_t) clear_value, height * width);
      break;
   case 2:
      if (clear_mask == 0xffff) {
         for (i = 0; i < height; i++) {
            uint16_t *row = (uint16_t *)dst;
            for (j = 0; j < width; j++)
               *row++ = (uint16_t) clear_value;
            dst += dst_stride;
         }
      }
      else {
         for (i = 0; i < height; i++) {
            uint16_t *row = (uint16_t *)dst;
            for (j = 0; j < width; j++) {
               uint16_t tmp = ~clear_mask & *row;
               *row++ = clear_value | tmp;
            }
            dst += dst_stride;
         }
      }
      break;
   case 4:
      if (clear_mask == 0xffffffff) {
         for (i = 0; i < height; i++) {
            uint32_t *row = (uint32_t *)dst;
            for (j = 0; j < width; j++)
               *row++ = clear_value;
            dst += dst_stride;
         }
      }
      else {
         for (i = 0; i < height; i++) {
            uint32_t *row = (uint32_t *)dst;
            for (j = 0; j < width; j++) {
               uint32_t tmp = ~clear_mask & *row;
               *row++ = clear_value | tmp;
            }
            dst += dst_stride;
         }
      }
      break;
   default:
      assert(0);
      break;
   }
}



/**
 * Convert the color tile from tiled to linear layout.
 * This is generally only done when we're flushing the scene just prior to
 * SwapBuffers.  If we didn't do this here, we'd have to convert the entire
 * tiled color buffer to linear layout in the llvmpipe_texture_unmap()
 * function.  It's better to do it here to take advantage of
 * threading/parallelism.
 * This is a bin command which is stored in all bins.
 */
static void
lp_rast_store_linear_color( struct lp_rasterizer_task *task )
{
   const struct lp_scene *scene = task->scene;
   unsigned buf;

   for (buf = 0; buf < scene->fb.nr_cbufs; buf++) {
      struct pipe_surface *cbuf = scene->fb.cbufs[buf];
      const unsigned layer = cbuf->u.tex.first_layer;
      const unsigned level = cbuf->u.tex.level;
      struct llvmpipe_resource *lpt = llvmpipe_resource(cbuf->texture);

      if (!task->color_tiles[buf])
         continue;

      llvmpipe_unswizzle_cbuf_tile(lpt,
                                   layer,
                                   level,
                                   task->x, task->y,
                                   task->color_tiles[buf]);
   }
}



/**
 * Run the shader on all blocks in a tile.  This is used when a tile is
 * completely contained inside a triangle.
 * This is a bin command called during bin processing.
 */
static void
lp_rast_shade_tile(struct lp_rasterizer_task *task,
                   const union lp_rast_cmd_arg arg)
{
   const struct lp_scene *scene = task->scene;
   const struct lp_rast_shader_inputs *inputs = arg.shade_tile;
   const struct lp_rast_state *state;
   struct lp_fragment_shader_variant *variant;
   const unsigned tile_x = task->x, tile_y = task->y;
   unsigned x, y;

   if (inputs->disable) {
      /* This command was partially binned and has been disabled */
      return;
   }

   LP_DBG(DEBUG_RAST, "%s\n", __FUNCTION__);

   state = task->state;
   assert(state);
   if (!state) {
      return;
   }
   variant = state->variant;

   /* render the whole 64x64 tile in 4x4 chunks */
   for (y = 0; y < TILE_SIZE; y += 4){
      for (x = 0; x < TILE_SIZE; x += 4) {
         uint8_t *color[PIPE_MAX_COLOR_BUFS];
         uint32_t *depth;
         unsigned i;

         /* color buffer */
         for (i = 0; i < scene->fb.nr_cbufs; i++)
            color[i] = lp_rast_get_color_block_pointer(task, i,
                                                       tile_x + x, tile_y + y);

         /* depth buffer */
         depth = lp_rast_get_depth_block_pointer(task, tile_x + x, tile_y + y);

         /* run shader on 4x4 block */
         BEGIN_JIT_CALL(state, task);
         variant->jit_function[RAST_WHOLE]( &state->jit_context,
                                            tile_x + x, tile_y + y,
                                            inputs->frontfacing,
                                            GET_A0(inputs),
                                            GET_DADX(inputs),
                                            GET_DADY(inputs),
                                            color,
                                            depth,
                                            0xffff,
                                            &task->vis_counter);
         END_JIT_CALL();
      }
   }
}


/**
 * Run the shader on all blocks in a tile.  This is used when a tile is
 * completely contained inside a triangle, and the shader is opaque.
 * This is a bin command called during bin processing.
 */
static void
lp_rast_shade_tile_opaque(struct lp_rasterizer_task *task,
                          const union lp_rast_cmd_arg arg)
{
   const struct lp_scene *scene = task->scene;
   unsigned i;

   LP_DBG(DEBUG_RAST, "%s\n", __FUNCTION__);

   assert(task->state);
   if (!task->state) {
      return;
   }

   /* this will prevent converting the layout from tiled to linear */
   for (i = 0; i < scene->fb.nr_cbufs; i++) {
      (void)lp_rast_get_color_tile_pointer(task, i, LP_TEX_USAGE_WRITE_ALL);
   }

   lp_rast_shade_tile(task, arg);
}


/**
 * Compute shading for a 4x4 block of pixels inside a triangle.
 * This is a bin command called during bin processing.
 * \param x  X position of quad in window coords
 * \param y  Y position of quad in window coords
 */
void
lp_rast_shade_quads_mask(struct lp_rasterizer_task *task,
                         const struct lp_rast_shader_inputs *inputs,
                         unsigned x, unsigned y,
                         unsigned mask)
{
   const struct lp_rast_state *state = task->state;
   struct lp_fragment_shader_variant *variant = state->variant;
   const struct lp_scene *scene = task->scene;
   uint8_t *color[PIPE_MAX_COLOR_BUFS];
   void *depth;
   unsigned i;

   assert(state);

   /* Sanity checks */
   assert(x < scene->tiles_x * TILE_SIZE);
   assert(y < scene->tiles_y * TILE_SIZE);
   assert(x % TILE_VECTOR_WIDTH == 0);
   assert(y % TILE_VECTOR_HEIGHT == 0);

   assert((x % 4) == 0);
   assert((y % 4) == 0);

   /* color buffer */
   for (i = 0; i < scene->fb.nr_cbufs; i++) {
      color[i] = lp_rast_get_color_block_pointer(task, i, x, y);
      assert(lp_check_alignment(color[i], 16));
   }

   /* depth buffer */
   depth = lp_rast_get_depth_block_pointer(task, x, y);


   assert(lp_check_alignment(state->jit_context.blend_color, 16));

   /* run shader on 4x4 block */
   BEGIN_JIT_CALL(state, task);
   variant->jit_function[RAST_EDGE_TEST](&state->jit_context,
                                         x, y,
                                         inputs->frontfacing,
                                         GET_A0(inputs),
                                         GET_DADX(inputs),
                                         GET_DADY(inputs),
                                         color,
                                         depth,
                                         mask,
                                         &task->vis_counter);
   END_JIT_CALL();
}



/**
 * Begin a new occlusion query.
 * This is a bin command put in all bins.
 * Called per thread.
 */
static void
lp_rast_begin_query(struct lp_rasterizer_task *task,
                    const union lp_rast_cmd_arg arg)
{
   struct llvmpipe_query *pq = arg.query_obj;

   assert(task->query == NULL);
   task->vis_counter = 0;
   task->query = pq;
}


/**
 * End the current occlusion query.
 * This is a bin command put in all bins.
 * Called per thread.
 */
static void
lp_rast_end_query(struct lp_rasterizer_task *task,
                  const union lp_rast_cmd_arg arg)
{
   assert(task->query);
   if (task->query) {
      task->query->count[task->thread_index] += task->vis_counter;
      task->query = NULL;
   }
}


void
lp_rast_set_state(struct lp_rasterizer_task *task,
                  const union lp_rast_cmd_arg arg)
{
   task->state = arg.state;
}



/**
 * Set top row and left column of the tile's pixels to white.  For debugging.
 */
static void
outline_tile(uint8_t *tile)
{
   const uint8_t val = 0xff;
   unsigned i;

   for (i = 0; i < TILE_SIZE; i++) {
      TILE_PIXEL(tile, i, 0, 0) = val;
      TILE_PIXEL(tile, i, 0, 1) = val;
      TILE_PIXEL(tile, i, 0, 2) = val;
      TILE_PIXEL(tile, i, 0, 3) = val;

      TILE_PIXEL(tile, 0, i, 0) = val;
      TILE_PIXEL(tile, 0, i, 1) = val;
      TILE_PIXEL(tile, 0, i, 2) = val;
      TILE_PIXEL(tile, 0, i, 3) = val;
   }
}


/**
 * Draw grid of gray lines at 16-pixel intervals across the tile to
 * show the sub-tile boundaries.  For debugging.
 */
static void
outline_subtiles(uint8_t *tile)
{
   const uint8_t val = 0x80;
   const unsigned step = 16;
   unsigned i, j;

   for (i = 0; i < TILE_SIZE; i += step) {
      for (j = 0; j < TILE_SIZE; j++) {
         TILE_PIXEL(tile, i, j, 0) = val;
         TILE_PIXEL(tile, i, j, 1) = val;
         TILE_PIXEL(tile, i, j, 2) = val;
         TILE_PIXEL(tile, i, j, 3) = val;

         TILE_PIXEL(tile, j, i, 0) = val;
         TILE_PIXEL(tile, j, i, 1) = val;
         TILE_PIXEL(tile, j, i, 2) = val;
         TILE_PIXEL(tile, j, i, 3) = val;
      }
   }

   outline_tile(tile);
}



/**
 * Called when we're done writing to a color tile.
 */
static void
lp_rast_tile_end(struct lp_rasterizer_task *task)
{
#ifdef DEBUG
   if (LP_DEBUG & (DEBUG_SHOW_SUBTILES | DEBUG_SHOW_TILES)) {
      const struct lp_scene *scene = task->scene;
      unsigned buf;

      for (buf = 0; buf < scene->fb.nr_cbufs; buf++) {
         uint8_t *color = lp_rast_get_color_block_pointer(task, buf,
                                                          task->x, task->y);

         if (LP_DEBUG & DEBUG_SHOW_SUBTILES)
            outline_subtiles(color);
         else if (LP_DEBUG & DEBUG_SHOW_TILES)
            outline_tile(color);
      }
   }
#else
   (void) outline_subtiles;
#endif

   lp_rast_store_linear_color(task);

   if (task->query) {
      union lp_rast_cmd_arg dummy = {0};
      lp_rast_end_query(task, dummy);
   }

   /* debug */
   memset(task->color_tiles, 0, sizeof(task->color_tiles));
   task->depth_tile = NULL;

   task->bin = NULL;
}

static lp_rast_cmd_func dispatch[LP_RAST_OP_MAX] =
{
   lp_rast_clear_color,
   lp_rast_clear_zstencil,
   lp_rast_triangle_1,
   lp_rast_triangle_2,
   lp_rast_triangle_3,
   lp_rast_triangle_4,
   lp_rast_triangle_5,
   lp_rast_triangle_6,
   lp_rast_triangle_7,
   lp_rast_triangle_8,
   lp_rast_triangle_3_4,
   lp_rast_triangle_3_16,
   lp_rast_triangle_4_16,
   lp_rast_shade_tile,
   lp_rast_shade_tile_opaque,
   lp_rast_begin_query,
   lp_rast_end_query,
   lp_rast_set_state,
};


static void
do_rasterize_bin(struct lp_rasterizer_task *task,
                 const struct cmd_bin *bin)
{
   const struct cmd_block *block;
   unsigned k;

   if (0)
      lp_debug_bin(bin);

   for (block = bin->head; block; block = block->next) {
      for (k = 0; k < block->count; k++) {
         dispatch[block->cmd[k]]( task, block->arg[k] );
      }
   }
}



/**
 * Rasterize commands for a single bin.
 * \param x, y  position of the bin's tile in the framebuffer
 * Must be called between lp_rast_begin() and lp_rast_end().
 * Called per thread.
 */
static void
rasterize_bin(struct lp_rasterizer_task *task,
              const struct cmd_bin *bin )
{
   lp_rast_tile_begin( task, bin );

   do_rasterize_bin(task, bin);

   lp_rast_tile_end(task);


   /* Debug/Perf flags:
    */
   if (bin->head->count == 1) {
      if (bin->head->cmd[0] == LP_RAST_OP_SHADE_TILE_OPAQUE)
         LP_COUNT(nr_pure_shade_opaque_64);
      else if (bin->head->cmd[0] == LP_RAST_OP_SHADE_TILE)
         LP_COUNT(nr_pure_shade_64);
   }
}


/* An empty bin is one that just loads the contents of the tile and
 * stores them again unchanged.  This typically happens when bins have
 * been flushed for some reason in the middle of a frame, or when
 * incremental updates are being made to a render target.
 * 
 * Try to avoid doing pointless work in this case.
 */
static boolean
is_empty_bin( const struct cmd_bin *bin )
{
   return bin->head == NULL;
}


/**
 * Rasterize/execute all bins within a scene.
 * Called per thread.
 */
static void
rasterize_scene(struct lp_rasterizer_task *task,
                struct lp_scene *scene)
{
   task->scene = scene;

   if (!task->rast->no_rast) {
      /* loop over scene bins, rasterize each */
#if 0
      {
         unsigned i, j;
         for (i = 0; i < scene->tiles_x; i++) {
            for (j = 0; j < scene->tiles_y; j++) {
               struct cmd_bin *bin = lp_scene_get_bin(scene, i, j);
               rasterize_bin(task, bin, i, j);
            }
         }
      }
#else
      {
         struct cmd_bin *bin;

         assert(scene);
         while ((bin = lp_scene_bin_iter_next(scene))) {
            if (!is_empty_bin( bin ))
               rasterize_bin(task, bin);
         }
      }
#endif
   }


   if (scene->fence) {
      lp_fence_signal(scene->fence);
   }

   task->scene = NULL;
}


/**
 * Called by setup module when it has something for us to render.
 */
void
lp_rast_queue_scene( struct lp_rasterizer *rast,
                     struct lp_scene *scene)
{
   LP_DBG(DEBUG_SETUP, "%s\n", __FUNCTION__);

   if (rast->num_threads == 0) {
      /* no threading */

      lp_rast_begin( rast, scene );

      rasterize_scene( &rast->tasks[0], scene );

      lp_rast_end( rast );

      rast->curr_scene = NULL;
   }
   else {
      /* threaded rendering! */
      unsigned i;

      lp_scene_enqueue( rast->full_scenes, scene );

      /* signal the threads that there's work to do */
      for (i = 0; i < rast->num_threads; i++) {
         pipe_semaphore_signal(&rast->tasks[i].work_ready);
      }
   }

   LP_DBG(DEBUG_SETUP, "%s done \n", __FUNCTION__);
}


void
lp_rast_finish( struct lp_rasterizer *rast )
{
   if (rast->num_threads == 0) {
      /* nothing to do */
   }
   else {
      int i;

      /* wait for work to complete */
      for (i = 0; i < rast->num_threads; i++) {
         pipe_semaphore_wait(&rast->tasks[i].work_done);
      }
   }
}


/**
 * This is the thread's main entrypoint.
 * It's a simple loop:
 *   1. wait for work
 *   2. do work
 *   3. signal that we're done
 */
static PIPE_THREAD_ROUTINE( thread_function, init_data )
{
   struct lp_rasterizer_task *task = (struct lp_rasterizer_task *) init_data;
   struct lp_rasterizer *rast = task->rast;
   boolean debug = false;

   while (1) {
      /* wait for work */
      if (debug)
         debug_printf("thread %d waiting for work\n", task->thread_index);
      pipe_semaphore_wait(&task->work_ready);

      if (rast->exit_flag)
         break;

      if (task->thread_index == 0) {
         /* thread[0]:
          *  - get next scene to rasterize
          *  - map the framebuffer surfaces
          */
         lp_rast_begin( rast, 
                        lp_scene_dequeue( rast->full_scenes, TRUE ) );
      }

      /* Wait for all threads to get here so that threads[1+] don't
       * get a null rast->curr_scene pointer.
       */
      pipe_barrier_wait( &rast->barrier );

      /* do work */
      if (debug)
         debug_printf("thread %d doing work\n", task->thread_index);

      rasterize_scene(task,
                      rast->curr_scene);
      
      /* wait for all threads to finish with this scene */
      pipe_barrier_wait( &rast->barrier );

      /* XXX: shouldn't be necessary:
       */
      if (task->thread_index == 0) {
         lp_rast_end( rast );
      }

      /* signal done with work */
      if (debug)
         debug_printf("thread %d done working\n", task->thread_index);

      pipe_semaphore_signal(&task->work_done);
   }

   return NULL;
}


/**
 * Initialize semaphores and spawn the threads.
 */
static void
create_rast_threads(struct lp_rasterizer *rast)
{
   unsigned i;

   /* NOTE: if num_threads is zero, we won't use any threads */
   for (i = 0; i < rast->num_threads; i++) {
      pipe_semaphore_init(&rast->tasks[i].work_ready, 0);
      pipe_semaphore_init(&rast->tasks[i].work_done, 0);
      rast->threads[i] = pipe_thread_create(thread_function,
                                            (void *) &rast->tasks[i]);
   }
}



/**
 * Create new lp_rasterizer.  If num_threads is zero, don't create any
 * new threads, do rendering synchronously.
 * \param num_threads  number of rasterizer threads to create
 */
struct lp_rasterizer *
lp_rast_create( unsigned num_threads )
{
   struct lp_rasterizer *rast;
   unsigned i;

   rast = CALLOC_STRUCT(lp_rasterizer);
   if (!rast) {
      goto no_rast;
   }

   rast->full_scenes = lp_scene_queue_create();
   if (!rast->full_scenes) {
      goto no_full_scenes;
   }

   for (i = 0; i < Elements(rast->tasks); i++) {
      struct lp_rasterizer_task *task = &rast->tasks[i];
      task->rast = rast;
      task->thread_index = i;
   }

   rast->num_threads = num_threads;

   rast->no_rast = debug_get_bool_option("LP_NO_RAST", FALSE);

   create_rast_threads(rast);

   /* for synchronizing rasterization threads */
   pipe_barrier_init( &rast->barrier, rast->num_threads );

   memset(lp_swizzled_cbuf, 0, sizeof lp_swizzled_cbuf);

   memset(lp_dummy_tile, 0, sizeof lp_dummy_tile);

   return rast;

no_full_scenes:
   FREE(rast);
no_rast:
   return NULL;
}


/* Shutdown:
 */
void lp_rast_destroy( struct lp_rasterizer *rast )
{
   unsigned i;

   /* Set exit_flag and signal each thread's work_ready semaphore.
    * Each thread will be woken up, notice that the exit_flag is set and
    * break out of its main loop.  The thread will then exit.
    */
   rast->exit_flag = TRUE;
   for (i = 0; i < rast->num_threads; i++) {
      pipe_semaphore_signal(&rast->tasks[i].work_ready);
   }

   /* Wait for threads to terminate before cleaning up per-thread data */
   for (i = 0; i < rast->num_threads; i++) {
      pipe_thread_wait(rast->threads[i]);
   }

   /* Clean up per-thread data */
   for (i = 0; i < rast->num_threads; i++) {
      pipe_semaphore_destroy(&rast->tasks[i].work_ready);
      pipe_semaphore_destroy(&rast->tasks[i].work_done);
   }

   /* for synchronizing rasterization threads */
   pipe_barrier_destroy( &rast->barrier );

   lp_scene_queue_destroy(rast->full_scenes);

   FREE(rast);
}


/** Return number of rasterization threads */
unsigned
lp_rast_get_num_threads( struct lp_rasterizer *rast )
{
   return rast->num_threads;
}


