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

#ifndef SVGA_CONTEXT_H
#define SVGA_CONTEXT_H


#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_state.h"

#include "util/u_double_list.h"

#include "tgsi/tgsi_scan.h"

#include "svga_state.h"
#include "svga_tgsi.h"
#include "svga_hw_reg.h"
#include "svga3d_shaderdefs.h"


struct draw_vertex_shader;
struct draw_fragment_shader;
struct svga_shader_result;
struct SVGACmdMemory;
struct util_bitmask;
struct u_upload_mgr;


struct svga_shader
{
   const struct tgsi_token *tokens;

   struct tgsi_shader_info info;

   struct svga_shader_result *results;

   unsigned id;  /**< for debugging only */
};

struct svga_fragment_shader
{
   struct svga_shader base;

   struct draw_fragment_shader *draw_shader;

   /** Mask of which generic varying variables are read by this shader */
   unsigned generic_inputs;
   /** Table mapping original TGSI generic indexes to low integers */
   int8_t generic_remap_table[MAX_GENERIC_VARYING];
};

struct svga_vertex_shader
{
   struct svga_shader base;

   struct draw_vertex_shader *draw_shader;
};


struct svga_cache_context;
struct svga_tracked_state;

struct svga_blend_state {

   boolean need_white_fragments;

   /* Should be per-render-target:
    */
   struct {
      uint8_t writemask;

      boolean blend_enable;
      uint8_t srcblend;
      uint8_t dstblend;
      uint8_t blendeq;
      
      boolean separate_alpha_blend_enable;
      uint8_t srcblend_alpha;
      uint8_t dstblend_alpha;
      uint8_t blendeq_alpha;

   } rt[1];
};

struct svga_depth_stencil_state {
   unsigned zfunc:8;
   unsigned zenable:1;
   unsigned zwriteenable:1;

   unsigned alphatestenable:1;
   unsigned alphafunc:8;
  
   struct {
      unsigned enabled:1;
      unsigned func:8;
      unsigned fail:8;
      unsigned zfail:8;
      unsigned pass:8;
   } stencil[2];
   
   /* SVGA3D has one ref/mask/writemask triple shared between front &
    * back face stencil.  We really need two:
    */
   unsigned stencil_mask:8;
   unsigned stencil_writemask:8;

   float    alpharef;
};

#define SVGA_UNFILLED_DISABLE 0
#define SVGA_UNFILLED_LINE    1
#define SVGA_UNFILLED_POINT   2

#define SVGA_PIPELINE_FLAG_POINTS   (1<<PIPE_PRIM_POINTS)
#define SVGA_PIPELINE_FLAG_LINES    (1<<PIPE_PRIM_LINES)
#define SVGA_PIPELINE_FLAG_TRIS     (1<<PIPE_PRIM_TRIANGLES)

struct svga_rasterizer_state {
   struct pipe_rasterizer_state templ; /* needed for draw module */

   unsigned shademode:8;
   unsigned cullmode:8;
   unsigned scissortestenable:1;
   unsigned multisampleantialias:1;
   unsigned antialiasedlineenable:1;
   unsigned lastpixel:1;
   unsigned pointsprite:1;

   unsigned linepattern;

   float slopescaledepthbias;
   float depthbias;
   float pointsize;
   
   unsigned hw_unfilled:16;         /* PIPE_POLYGON_MODE_x */

   /** Which prims do we need help for?  Bitmask of (1 << PIPE_PRIM_x) flags */
   unsigned need_pipeline:16;

   /** For debugging: */
   const char* need_pipeline_tris_str;
   const char* need_pipeline_lines_str;
   const char* need_pipeline_points_str;
};

struct svga_sampler_state {
   unsigned mipfilter;
   unsigned magfilter;
   unsigned minfilter;
   unsigned aniso_level;
   float lod_bias;
   unsigned addressu;
   unsigned addressv;
   unsigned addressw;
   unsigned bordercolor;
   unsigned normalized_coords:1;
   unsigned compare_mode:1;
   unsigned compare_func:3;

   unsigned min_lod;
   unsigned view_min_lod;
   unsigned view_max_lod;
};

struct svga_velems_state {
   unsigned count;
   struct pipe_vertex_element velem[PIPE_MAX_ATTRIBS];
};

/* Use to calculate differences between state emitted to hardware and
 * current driver-calculated state.  
 */
struct svga_state 
{
   const struct svga_blend_state *blend;
   const struct svga_depth_stencil_state *depth;
   const struct svga_rasterizer_state *rast;
   const struct svga_sampler_state *sampler[PIPE_MAX_SAMPLERS];
   const struct svga_velems_state *velems;

   struct pipe_sampler_view *sampler_views[PIPE_MAX_SAMPLERS]; /* or texture ID's? */
   struct svga_fragment_shader *fs;
   struct svga_vertex_shader *vs;

   struct pipe_vertex_buffer vb[PIPE_MAX_ATTRIBS];
   struct pipe_index_buffer ib;
   struct pipe_resource *cb[PIPE_SHADER_TYPES];

   struct pipe_framebuffer_state framebuffer;
   float depthscale;

   /* Hack to limit the number of different render targets between
    * flushes.  Helps avoid blowing out our surface cache in EXA.
    */
   int nr_fbs;

   struct pipe_poly_stipple poly_stipple;
   struct pipe_scissor_state scissor;
   struct pipe_blend_color blend_color;
   struct pipe_stencil_ref stencil_ref;
   struct pipe_clip_state clip;
   struct pipe_viewport_state viewport;

   unsigned num_samplers;
   unsigned num_sampler_views;
   unsigned num_vertex_buffers;
   unsigned reduced_prim;

   struct {
      unsigned flag_1d;
      unsigned flag_srgb;
   } tex_flags;

   boolean any_user_vertex_buffers;
};

struct svga_prescale {
   float translate[4];
   float scale[4];
   boolean enabled;
};


/* Updated by calling svga_update_state( SVGA_STATE_HW_CLEAR )
 */
struct svga_hw_clear_state
{
   struct {
      unsigned x,y,w,h;
   } viewport;

   struct {
      float zmin, zmax;
   } depthrange;
   
   struct pipe_framebuffer_state framebuffer;
   struct svga_prescale prescale;
};

struct svga_hw_view_state
{
   struct pipe_resource *texture;
   struct svga_sampler_view *v;
   unsigned min_lod;
   unsigned max_lod;
   int dirty;
};

/* Updated by calling svga_update_state( SVGA_STATE_HW_DRAW )
 */
struct svga_hw_draw_state
{
   unsigned rs[SVGA3D_RS_MAX];
   unsigned ts[SVGA3D_PIXEL_SAMPLERREG_MAX][SVGA3D_TS_MAX];
   float cb[PIPE_SHADER_TYPES][SVGA3D_CONSTREG_MAX][4];

   struct svga_shader_result *fs;
   struct svga_shader_result *vs;
   struct svga_hw_view_state views[PIPE_MAX_SAMPLERS];

   unsigned num_views;
};


/* Updated by calling svga_update_state( SVGA_STATE_NEED_SWTNL )
 */
struct svga_sw_state
{
   unsigned ve_format[PIPE_MAX_ATTRIBS]; /* NEW_VELEMENT */

   /* which parts we need */
   boolean need_swvfetch;
   boolean need_pipeline;
   boolean need_swtnl;

   /* Flag to make sure that need sw is on while
    * updating state within a swtnl call.
    */
   boolean in_swtnl_draw;
};


/* Queue some state updates (like rss) and submit them to hardware in
 * a single packet.
 */
struct svga_hw_queue;

struct svga_query;

struct svga_context
{
   struct pipe_context pipe;
   struct svga_winsys_context *swc;

   struct {
      boolean no_swtnl;
      boolean force_swtnl;
      boolean use_min_mipmap;

      /* incremented for each shader */
      unsigned shader_id;

      unsigned disable_shader;

      boolean no_line_width;
      boolean force_hw_line_stipple;
   } debug;

   struct {
      struct draw_context *draw;
      struct vbuf_render *backend;
      unsigned hw_prim;
      boolean new_vbuf;
      boolean new_vdecl;
   } swtnl;

   /* Bitmask of used shader IDs */
   struct util_bitmask *fs_bm;
   struct util_bitmask *vs_bm;

   struct {
      unsigned dirty[SVGA_STATE_MAX];

      unsigned texture_timestamp;

      /* 
       */
      struct svga_sw_state          sw;
      struct svga_hw_draw_state     hw_draw;
      struct svga_hw_clear_state    hw_clear;
   } state;

   struct svga_state curr;      /* state from the state tracker */
   unsigned dirty;              /* statechanges since last update_state() */

   struct {
      unsigned rendertargets:1;
      unsigned texture_samplers:1;
   } rebind;

   struct u_upload_mgr *upload_ib;
   struct u_upload_mgr *upload_vb;
   struct svga_hwtnl *hwtnl;

   /** The occlusion query currently in progress */
   struct svga_query *sq;

   /** List of buffers with queued transfers */
   struct list_head dirty_buffers;
};

/* A flag for each state_tracker state object:
 */
#define SVGA_NEW_BLEND               0x1
#define SVGA_NEW_DEPTH_STENCIL       0x2
#define SVGA_NEW_RAST                0x4
#define SVGA_NEW_SAMPLER             0x8
#define SVGA_NEW_TEXTURE             0x10
#define SVGA_NEW_VBUFFER             0x20
#define SVGA_NEW_VELEMENT            0x40
#define SVGA_NEW_FS                  0x80
#define SVGA_NEW_VS                  0x100
#define SVGA_NEW_FS_CONST_BUFFER     0x200
#define SVGA_NEW_VS_CONST_BUFFER     0x400
#define SVGA_NEW_FRAME_BUFFER        0x800
#define SVGA_NEW_STIPPLE             0x1000
#define SVGA_NEW_SCISSOR             0x2000
#define SVGA_NEW_BLEND_COLOR         0x4000
#define SVGA_NEW_CLIP                0x8000
#define SVGA_NEW_VIEWPORT            0x10000
#define SVGA_NEW_PRESCALE            0x20000
#define SVGA_NEW_REDUCED_PRIMITIVE   0x40000
#define SVGA_NEW_TEXTURE_BINDING     0x80000
#define SVGA_NEW_NEED_PIPELINE       0x100000
#define SVGA_NEW_NEED_SWVFETCH       0x200000
#define SVGA_NEW_NEED_SWTNL          0x400000
#define SVGA_NEW_FS_RESULT           0x800000
#define SVGA_NEW_VS_RESULT           0x1000000
#define SVGA_NEW_TEXTURE_FLAGS       0x4000000
#define SVGA_NEW_STENCIL_REF         0x8000000





/***********************************************************************
 * svga_clear.c: 
 */
void svga_clear(struct pipe_context *pipe, 
                unsigned buffers,
                const union pipe_color_union *color,
                double depth,
                unsigned stencil);


/***********************************************************************
 * svga_screen_texture.c: 
 */
void svga_mark_surfaces_dirty(struct svga_context *svga);




void svga_init_state_functions( struct svga_context *svga );
void svga_init_flush_functions( struct svga_context *svga );
void svga_init_string_functions( struct svga_context *svga );
void svga_init_blit_functions(struct svga_context *svga);

void svga_init_blend_functions( struct svga_context *svga );
void svga_init_depth_stencil_functions( struct svga_context *svga );
void svga_init_misc_functions( struct svga_context *svga );
void svga_init_rasterizer_functions( struct svga_context *svga );
void svga_init_sampler_functions( struct svga_context *svga );
void svga_init_fs_functions( struct svga_context *svga );
void svga_init_vs_functions( struct svga_context *svga );
void svga_init_vertex_functions( struct svga_context *svga );
void svga_init_constbuffer_functions( struct svga_context *svga );
void svga_init_draw_functions( struct svga_context *svga );
void svga_init_query_functions( struct svga_context *svga );
void svga_init_surface_functions(struct svga_context *svga);

void svga_cleanup_vertex_state( struct svga_context *svga );
void svga_cleanup_tss_binding( struct svga_context *svga );
void svga_cleanup_framebuffer( struct svga_context *svga );

void svga_context_flush( struct svga_context *svga,
                         struct pipe_fence_handle **pfence );

void svga_hwtnl_flush_retry( struct svga_context *svga );
void svga_hwtnl_flush_buffer( struct svga_context *svga,
                              struct pipe_resource *buffer );

void svga_surfaces_flush(struct svga_context *svga);

struct pipe_context *
svga_context_create(struct pipe_screen *screen,
		    void *priv);


/***********************************************************************
 * Inline conversion functions.  These are better-typed than the
 * macros used previously:
 */
static INLINE struct svga_context *
svga_context( struct pipe_context *pipe )
{
   return (struct svga_context *)pipe;
}



#endif
