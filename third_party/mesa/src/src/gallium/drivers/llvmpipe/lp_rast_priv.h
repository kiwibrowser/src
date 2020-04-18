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

#ifndef LP_RAST_PRIV_H
#define LP_RAST_PRIV_H

#include "os/os_thread.h"
#include "util/u_format.h"
#include "gallivm/lp_bld_debug.h"
#include "lp_memory.h"
#include "lp_rast.h"
#include "lp_scene.h"
#include "lp_state.h"
#include "lp_texture.h"
#include "lp_tile_soa.h"
#include "lp_limits.h"


/* If we crash in a jitted function, we can examine jit_line and jit_state
 * to get some info.  This is not thread-safe, however.
 */
#ifdef DEBUG

struct lp_rasterizer_task;
extern int jit_line;
extern const struct lp_rast_state *jit_state;
extern const struct lp_rasterizer_task *jit_task;

#define BEGIN_JIT_CALL(state, task)                  \
   do { \
      jit_line = __LINE__; \
      jit_state = state; \
      jit_task = task; \
   } while (0)

#define END_JIT_CALL() \
   do { \
      jit_line = 0; \
      jit_state = NULL; \
   } while (0)

#else

#define BEGIN_JIT_CALL(X, Y)
#define END_JIT_CALL()

#endif


struct lp_rasterizer;
struct cmd_bin;

/**
 * Per-thread rasterization state
 */
struct lp_rasterizer_task
{
   const struct cmd_bin *bin;
   const struct lp_rast_state *state;

   struct lp_scene *scene;
   unsigned x, y;          /**< Pos of this tile in framebuffer, in pixels */

   uint8_t *color_tiles[PIPE_MAX_COLOR_BUFS];
   uint8_t *depth_tile;

   /** "back" pointer */
   struct lp_rasterizer *rast;

   /** "my" index */
   unsigned thread_index;

   /* occlude counter for visiable pixels */
   uint32_t vis_counter;
   struct llvmpipe_query *query;

   pipe_semaphore work_ready;
   pipe_semaphore work_done;
};


/**
 * This is the state required while rasterizing tiles.
 * Note that this contains per-thread information too.
 * The tile size is TILE_SIZE x TILE_SIZE pixels.
 */
struct lp_rasterizer
{
   boolean exit_flag;
   boolean no_rast;  /**< For debugging/profiling */

   /** The incoming queue of scenes ready to rasterize */
   struct lp_scene_queue *full_scenes;

   /** The scene currently being rasterized by the threads */
   struct lp_scene *curr_scene;

   /** A task object for each rasterization thread */
   struct lp_rasterizer_task tasks[LP_MAX_THREADS];

   unsigned num_threads;
   pipe_thread threads[LP_MAX_THREADS];

   /** For synchronizing the rasterization threads */
   pipe_barrier barrier;
};


void
lp_rast_shade_quads_mask(struct lp_rasterizer_task *task,
                         const struct lp_rast_shader_inputs *inputs,
                         unsigned x, unsigned y,
                         unsigned mask);



/**
 * Get the pointer to a 4x4 depth/stencil block.
 * We'll map the z/stencil buffer on demand here.
 * Note that this may be called even when there's no z/stencil buffer - return
 * NULL in that case.
 * \param x, y location of 4x4 block in window coords
 */
static INLINE void *
lp_rast_get_depth_block_pointer(struct lp_rasterizer_task *task,
                                unsigned x, unsigned y)
{
   const struct lp_scene *scene = task->scene;
   void *depth;

   assert(x < scene->tiles_x * TILE_SIZE);
   assert(y < scene->tiles_y * TILE_SIZE);
   assert((x % TILE_VECTOR_WIDTH) == 0);
   assert((y % TILE_VECTOR_HEIGHT) == 0);

   if (!scene->zsbuf.map) {
      /* Either out of memory or no zsbuf.  Can't tell without access
       * to the state.  Just use dummy tile memory, but don't print
       * the oom warning as this most likely because there is no
       * zsbuf.
       */
      return lp_dummy_tile;
   }

   depth = (scene->zsbuf.map +
            scene->zsbuf.stride * y +
            scene->zsbuf.blocksize * x * TILE_VECTOR_HEIGHT);

   assert(lp_check_alignment(depth, 16));
   return depth;
}


/**
 * Get pointer to the swizzled color tile
 */
static INLINE uint8_t *
lp_rast_get_color_tile_pointer(struct lp_rasterizer_task *task,
                               unsigned buf, enum lp_texture_usage usage)
{
   const struct lp_scene *scene = task->scene;

   assert(task->x < scene->tiles_x * TILE_SIZE);
   assert(task->y < scene->tiles_y * TILE_SIZE);
   assert(task->x % TILE_SIZE == 0);
   assert(task->y % TILE_SIZE == 0);
   assert(buf < scene->fb.nr_cbufs);

   if (!task->color_tiles[buf]) {
      struct pipe_surface *cbuf = scene->fb.cbufs[buf];
      struct llvmpipe_resource *lpt;
      assert(cbuf);
      lpt = llvmpipe_resource(cbuf->texture);
      task->color_tiles[buf] = lp_swizzled_cbuf[task->thread_index][buf];

      if (usage != LP_TEX_USAGE_WRITE_ALL) {
         llvmpipe_swizzle_cbuf_tile(lpt,
                                    cbuf->u.tex.first_layer,
                                    cbuf->u.tex.level,
                                    task->x, task->y,
                                    task->color_tiles[buf]);
      }
   }

   return task->color_tiles[buf];
}


/**
 * Get the pointer to a 4x4 color block (within a 64x64 tile).
 * We'll map the color buffer on demand here.
 * Note that this may be called even when there's no color buffers - return
 * NULL in that case.
 * \param x, y location of 4x4 block in window coords
 */
static INLINE uint8_t *
lp_rast_get_color_block_pointer(struct lp_rasterizer_task *task,
                                unsigned buf, unsigned x, unsigned y)
{
   unsigned px, py, pixel_offset;
   uint8_t *color;

   assert(x < task->scene->tiles_x * TILE_SIZE);
   assert(y < task->scene->tiles_y * TILE_SIZE);
   assert((x % TILE_VECTOR_WIDTH) == 0);
   assert((y % TILE_VECTOR_HEIGHT) == 0);

   color = lp_rast_get_color_tile_pointer(task, buf, LP_TEX_USAGE_READ_WRITE);
   assert(color);

   px = x % TILE_SIZE;
   py = y % TILE_SIZE;
   pixel_offset = tile_pixel_offset(px, py, 0);

   color = color + pixel_offset;

   assert(lp_check_alignment(color, 16));
   return color;
}



/**
 * Shade all pixels in a 4x4 block.  The fragment code omits the
 * triangle in/out tests.
 * \param x, y location of 4x4 block in window coords
 */
static INLINE void
lp_rast_shade_quads_all( struct lp_rasterizer_task *task,
                         const struct lp_rast_shader_inputs *inputs,
                         unsigned x, unsigned y )
{
   const struct lp_scene *scene = task->scene;
   const struct lp_rast_state *state = task->state;
   struct lp_fragment_shader_variant *variant = state->variant;
   uint8_t *color[PIPE_MAX_COLOR_BUFS];
   void *depth;
   unsigned i;

   /* color buffer */
   for (i = 0; i < scene->fb.nr_cbufs; i++)
      color[i] = lp_rast_get_color_block_pointer(task, i, x, y);

   depth = lp_rast_get_depth_block_pointer(task, x, y);

   /* run shader on 4x4 block */
   BEGIN_JIT_CALL(state, task);
   variant->jit_function[RAST_WHOLE]( &state->jit_context,
                                      x, y,
                                      inputs->frontfacing,
                                      GET_A0(inputs),
                                      GET_DADX(inputs),
                                      GET_DADY(inputs),
                                      color,
                                      depth,
                                      0xffff,
                                      &task->vis_counter );
   END_JIT_CALL();
}

void lp_rast_triangle_1( struct lp_rasterizer_task *, 
                         const union lp_rast_cmd_arg );
void lp_rast_triangle_2( struct lp_rasterizer_task *, 
                         const union lp_rast_cmd_arg );
void lp_rast_triangle_3( struct lp_rasterizer_task *, 
                         const union lp_rast_cmd_arg );
void lp_rast_triangle_4( struct lp_rasterizer_task *, 
                         const union lp_rast_cmd_arg );
void lp_rast_triangle_5( struct lp_rasterizer_task *, 
                         const union lp_rast_cmd_arg );
void lp_rast_triangle_6( struct lp_rasterizer_task *, 
                         const union lp_rast_cmd_arg );
void lp_rast_triangle_7( struct lp_rasterizer_task *, 
                         const union lp_rast_cmd_arg );
void lp_rast_triangle_8( struct lp_rasterizer_task *, 
                         const union lp_rast_cmd_arg );

void lp_rast_triangle_3_4(struct lp_rasterizer_task *,
			  const union lp_rast_cmd_arg );

void lp_rast_triangle_3_16( struct lp_rasterizer_task *, 
                            const union lp_rast_cmd_arg );

void lp_rast_triangle_4_16( struct lp_rasterizer_task *, 
                            const union lp_rast_cmd_arg );

void
lp_rast_set_state(struct lp_rasterizer_task *task,
                  const union lp_rast_cmd_arg arg);
 
void
lp_debug_bin( const struct cmd_bin *bin );

#endif
