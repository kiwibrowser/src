 /**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
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

#ifndef I915_CONTEXT_H
#define I915_CONTEXT_H


#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_state.h"

#include "draw/draw_vertex.h"

#include "tgsi/tgsi_scan.h"

#include "util/u_slab.h"
#include "util/u_blitter.h"


struct i915_winsys;
struct i915_winsys_buffer;
struct i915_winsys_batchbuffer;


#define I915_TEX_UNITS 8

#define I915_DYNAMIC_MODES4       0
#define I915_DYNAMIC_DEPTHSCALE_0 1 /* just the header */
#define I915_DYNAMIC_DEPTHSCALE_1 2
#define I915_DYNAMIC_IAB          3
#define I915_DYNAMIC_BC_0         4 /* just the header */
#define I915_DYNAMIC_BC_1         5
#define I915_DYNAMIC_BFO_0        6
#define I915_DYNAMIC_BFO_1        7
#define I915_DYNAMIC_STP_0        8
#define I915_DYNAMIC_STP_1        9
#define I915_DYNAMIC_SC_ENA_0     10
#define I915_DYNAMIC_SC_RECT_0    11
#define I915_DYNAMIC_SC_RECT_1    12
#define I915_DYNAMIC_SC_RECT_2    13
#define I915_MAX_DYNAMIC          14


#define I915_IMMEDIATE_S0         0
#define I915_IMMEDIATE_S1         1
#define I915_IMMEDIATE_S2         2
#define I915_IMMEDIATE_S3         3
#define I915_IMMEDIATE_S4         4
#define I915_IMMEDIATE_S5         5
#define I915_IMMEDIATE_S6         6
#define I915_IMMEDIATE_S7         7
#define I915_MAX_IMMEDIATE        8

/* These must mach the order of LI0_STATE_* bits, as they will be used
 * to generate hardware packets:
 */
#define I915_CACHE_STATIC         0
#define I915_CACHE_DYNAMIC        1 /* handled specially */
#define I915_CACHE_SAMPLER        2
#define I915_CACHE_MAP            3
#define I915_CACHE_PROGRAM        4
#define I915_CACHE_CONSTANTS      5
#define I915_MAX_CACHE            6

#define I915_MAX_CONSTANT  32


/** See constant_flags[] below */
#define I915_CONSTFLAG_USER 0x1f


/**
 * Subclass of pipe_shader_state
 */
struct i915_fragment_shader
{
   struct pipe_shader_state state;

   struct tgsi_shader_info info;

   struct draw_fragment_shader *draw_data;

   uint *decl;
   uint decl_len;

   uint *program;
   uint program_len;

   /**
    * constants introduced during translation.
    * These are placed at the end of the constant buffer and grow toward
    * the beginning (eg: slot 31, 30 29, ...)
    * User-provided constants start at 0.
    * This allows both types of constants to co-exist (until there's too many)
    * and doesn't require regenerating/changing the fragment program to
    * shuffle constants around.
    */
   uint num_constants;
   float constants[I915_MAX_CONSTANT][4];

   /**
    * Status of each constant
    * if I915_CONSTFLAG_PARAM, the value must be taken from the corresponding
    * slot of the user's constant buffer. (set by pipe->set_constant_buffer())
    * Else, the bitmask indicates which components are occupied by immediates.
    */
   ubyte constant_flags[I915_MAX_CONSTANT];

   /**
    * The mapping between generics and hw texture coords.
    * We need to share this between the vertex and fragment stages.
    **/
   int generic_mapping[I915_TEX_UNITS];
};


struct i915_cache_context;

/* Use to calculate differences between state emitted to hardware and
 * current driver-calculated state.  
 */
struct i915_state
{
   unsigned immediate[I915_MAX_IMMEDIATE];
   unsigned dynamic[I915_MAX_DYNAMIC];

   /** number of constants passed in through a constant buffer */
   uint num_user_constants[PIPE_SHADER_TYPES];

   /* texture sampler state */
   unsigned sampler[I915_TEX_UNITS][3];
   unsigned sampler_enable_flags;
   unsigned sampler_enable_nr;
   boolean sampler_srgb[I915_TEX_UNITS];
   int srgb_const_offset;

   /* texture image buffers */
   unsigned texbuffer[I915_TEX_UNITS][2];

   /** Describes the current hardware vertex layout */
   struct vertex_info vertex_info;

   /* static state (dst/depth buffer state) */
   struct i915_winsys_buffer *cbuf_bo;
   unsigned cbuf_flags;
   struct i915_winsys_buffer *depth_bo;
   unsigned depth_flags;
   unsigned dst_buf_vars;
   uint32_t draw_offset;
   uint32_t draw_size;
   uint32_t target_fixup_format;
   uint32_t fixup_swizzle;

   unsigned id;			/* track lost context events */
};

struct i915_blend_state {
   unsigned iab;
   unsigned modes4;
   unsigned LIS5;
   unsigned LIS6;
};

struct i915_depth_stencil_state {
   unsigned stencil_modes4;
   unsigned bfo[2];
   unsigned stencil_LIS5;
   unsigned depth_LIS6;
};

struct i915_rasterizer_state {
   struct pipe_rasterizer_state templ;

   unsigned light_twoside : 1;
   unsigned st;
   enum interp_mode color_interp;

   unsigned LIS4;
   unsigned LIS7;
   unsigned sc[1];

   union { float f; unsigned u; } ds[2];
};

struct i915_sampler_state {
   struct pipe_sampler_state templ;
   unsigned state[3];
   unsigned minlod;
   unsigned maxlod;
};

struct i915_velems_state {
   unsigned count;
   struct pipe_vertex_element velem[PIPE_MAX_ATTRIBS];
};


struct i915_context {
   struct pipe_context base;

   struct i915_winsys *iws;

   struct draw_context *draw;

   /* The most recent drawing state as set by the driver:
    */
   const struct i915_blend_state           *blend;
   const struct i915_sampler_state         *sampler[PIPE_MAX_SAMPLERS];
   struct pipe_sampler_state *vertex_samplers[PIPE_MAX_SAMPLERS];
   const struct i915_depth_stencil_state   *depth_stencil;
   const struct i915_rasterizer_state      *rasterizer;

   struct i915_fragment_shader *fs;

   struct pipe_blend_color blend_color;
   struct pipe_stencil_ref stencil_ref;
   struct pipe_clip_state clip;
   struct pipe_resource *constants[PIPE_SHADER_TYPES];
   struct pipe_framebuffer_state framebuffer;
   struct pipe_poly_stipple poly_stipple;
   struct pipe_scissor_state scissor;
   struct pipe_sampler_view *fragment_sampler_views[PIPE_MAX_SAMPLERS];
   struct pipe_sampler_view *vertex_sampler_views[PIPE_MAX_SAMPLERS];
   struct pipe_viewport_state viewport;
   struct pipe_index_buffer index_buffer;

   unsigned dirty;

   struct pipe_resource *mapped_vs_tex[PIPE_MAX_SAMPLERS];
   struct i915_winsys_buffer* mapped_vs_tex_buffer[PIPE_MAX_SAMPLERS];

   unsigned num_samplers;
   unsigned num_fragment_sampler_views;
   unsigned num_vertex_samplers;
   unsigned num_vertex_sampler_views;

   struct i915_winsys_batchbuffer *batch;

   /** Vertex buffer */
   struct i915_winsys_buffer *vbo;
   size_t vbo_offset;
   unsigned vbo_flushed;

   struct i915_state current;
   unsigned hardware_dirty;
   unsigned immediate_dirty : I915_MAX_IMMEDIATE;
   unsigned dynamic_dirty : I915_MAX_DYNAMIC;
   unsigned static_dirty : 4;
   unsigned flush_dirty : 2;

   struct i915_winsys_buffer *validation_buffers[2 + 1 + I915_TEX_UNITS];
   int num_validation_buffers;

   struct util_slab_mempool transfer_pool;
   struct util_slab_mempool texture_transfer_pool;

   /* state for tracking flushes */
   int last_fired_vertices;
   int fired_vertices;
   int queued_vertices;

   /** blitter/hw-clear */
   struct blitter_context* blitter;

   /** State tracking needed by u_blitter for save/restore. */
   void *saved_fs;
   void (*saved_bind_fs_state)(struct pipe_context *pipe, void *shader);
   void *saved_vs;
   struct pipe_clip_state saved_clip;
   struct i915_velems_state *saved_velems;
   unsigned saved_nr_vertex_buffers;
   struct pipe_vertex_buffer saved_vertex_buffers[PIPE_MAX_ATTRIBS];
   unsigned saved_nr_samplers;
   void *saved_samplers[PIPE_MAX_SAMPLERS];
   void (*saved_bind_sampler_states)(struct pipe_context *pipe,
                                     unsigned num, void **sampler);
   unsigned saved_nr_sampler_views;
   struct pipe_sampler_view *saved_sampler_views[PIPE_MAX_SAMPLERS];
   void (*saved_set_sampler_views)(struct pipe_context *pipe,
                                   unsigned num, struct pipe_sampler_view **views);
};

/* A flag for each state_tracker state object:
 */
#define I915_NEW_VIEWPORT      0x1
#define I915_NEW_RASTERIZER    0x2
#define I915_NEW_FS            0x4
#define I915_NEW_BLEND         0x8
#define I915_NEW_CLIP          0x10
#define I915_NEW_SCISSOR       0x20
#define I915_NEW_STIPPLE       0x40
#define I915_NEW_FRAMEBUFFER   0x80
#define I915_NEW_ALPHA_TEST    0x100
#define I915_NEW_DEPTH_STENCIL 0x200
#define I915_NEW_SAMPLER       0x400
#define I915_NEW_SAMPLER_VIEW  0x800
#define I915_NEW_VS_CONSTANTS  0x1000
#define I915_NEW_FS_CONSTANTS  0x2000
#define I915_NEW_GS_CONSTANTS  0x4000
#define I915_NEW_VBO           0x8000
#define I915_NEW_VS            0x10000


/* Driver's internally generated state flags:
 */
#define I915_NEW_VERTEX_FORMAT    0x10000


/* Dirty flags for hardware emit
 */
#define I915_HW_STATIC            (1<<I915_CACHE_STATIC)
#define I915_HW_DYNAMIC           (1<<I915_CACHE_DYNAMIC)
#define I915_HW_SAMPLER           (1<<I915_CACHE_SAMPLER)
#define I915_HW_MAP               (1<<I915_CACHE_MAP)
#define I915_HW_PROGRAM           (1<<I915_CACHE_PROGRAM)
#define I915_HW_CONSTANTS         (1<<I915_CACHE_CONSTANTS)
#define I915_HW_IMMEDIATE         (1<<(I915_MAX_CACHE+0))
#define I915_HW_INVARIANT         (1<<(I915_MAX_CACHE+1))
#define I915_HW_FLUSH             (1<<(I915_MAX_CACHE+1))

/* hw flush handling */
#define I915_FLUSH_CACHE		1
#define I915_PIPELINE_FLUSH		2

/* split up static state */
#define I915_DST_BUF_COLOR              1
#define I915_DST_BUF_DEPTH              2
#define I915_DST_VARS                   4
#define I915_DST_RECT                   8

static INLINE
void i915_set_flush_dirty(struct i915_context *i915, unsigned flush)
{
   i915->hardware_dirty |= I915_HW_FLUSH;
   i915->flush_dirty |= flush;
}


/***********************************************************************
 * i915_prim_emit.c: 
 */
struct draw_stage *i915_draw_render_stage( struct i915_context *i915 );


/***********************************************************************
 * i915_prim_vbuf.c: 
 */
struct draw_stage *i915_draw_vbuf_stage( struct i915_context *i915 );


/***********************************************************************
 * i915_state.c:
 */
void i915_prepare_vertex_sampling(struct i915_context *i915);
void i915_cleanup_vertex_sampling(struct i915_context *i915);



/***********************************************************************
 * i915_state_emit.c: 
 */
void i915_emit_hardware_state(struct i915_context *i915 );



/***********************************************************************
 * i915_clear.c: 
 */
void i915_clear_blitter(struct pipe_context *pipe, unsigned buffers,
                        const union pipe_color_union *color,
                        double depth, unsigned stencil);
void i915_clear_render(struct pipe_context *pipe, unsigned buffers,
                       const union pipe_color_union *color,
                       double depth, unsigned stencil);
void i915_clear_emit(struct pipe_context *pipe, unsigned buffers,
                     const union pipe_color_union *color,
                     double depth, unsigned stencil,
                     unsigned destx, unsigned desty, unsigned width, unsigned height);


/***********************************************************************
 * 
 */
void i915_init_state_functions( struct i915_context *i915 );
void i915_init_fixup_state_functions( struct i915_context *i915 );
void i915_init_flush_functions( struct i915_context *i915 );
void i915_init_string_functions( struct i915_context *i915 );


/************************************************************************
 * i915_context.c
 */
struct pipe_context *i915_create_context(struct pipe_screen *screen,
					 void *priv);


/***********************************************************************
 * Inline conversion functions.  These are better-typed than the
 * macros used previously:
 */
static INLINE struct i915_context *
i915_context( struct pipe_context *pipe )
{
   return (struct i915_context *)pipe;
}


#endif
