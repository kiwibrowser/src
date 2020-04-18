/* Display a cleared blue window.  This demo has no dependencies on
 * any utility code, just the graw interface and gallium.
 */

#include "state_tracker/graw.h"
#include "pipe/p_screen.h"
#include "pipe/p_context.h"
#include "pipe/p_shader_tokens.h"
#include "pipe/p_state.h"
#include "pipe/p_defines.h"

#include "util/u_debug.h"       /* debug_dump_surface_bmp() */
#include "util/u_inlines.h"
#include "util/u_memory.h"      /* Offset() */
#include "util/u_draw_quad.h"
#include "util/u_box.h"    

#include <stdio.h>

enum pipe_format formats[] = {
   PIPE_FORMAT_R8G8B8A8_UNORM,
   PIPE_FORMAT_B8G8R8A8_UNORM,
   PIPE_FORMAT_NONE
};

static const int WIDTH = 300;
static const int HEIGHT = 300;

static struct pipe_screen *screen = NULL;
static struct pipe_context *ctx = NULL;
static struct pipe_resource *rttex = NULL;
static struct pipe_resource *samptex = NULL;
static struct pipe_surface *surf = NULL;
static struct pipe_sampler_view *sv = NULL;
static void *sampler = NULL;
static void *window = NULL;

struct vertex {
   float position[4];
   float color[4];
};

static struct vertex vertices[] =
{
   { { 0.9, -0.9, 0.0, 1.0 },
     { 1, 0, 0, 1 } },

   { { 0.9,  0.9, 0.0, 1.0 },
     { 1, 1, 0, 1 } },

   { {-0.9,  0.9, 0.0, 1.0 },
     { 0, 1, 0, 1 } },

   { {-0.9,  -0.9, 0.0, 1.0 },
     { 0, 0, 0, 1 } },
};




static void set_viewport( float x, float y,
                          float width, float height,
                          float near, float far)
{
   float z = far;
   float half_width = (float)width / 2.0f;
   float half_height = (float)height / 2.0f;
   float half_depth = ((float)far - (float)near) / 2.0f;
   struct pipe_viewport_state vp;

   vp.scale[0] = half_width;
   vp.scale[1] = half_height;
   vp.scale[2] = half_depth;
   vp.scale[3] = 1.0f;

   vp.translate[0] = half_width + x;
   vp.translate[1] = half_height + y;
   vp.translate[2] = half_depth + z;
   vp.translate[3] = 0.0f;

   ctx->set_viewport_state( ctx, &vp );
}

static void set_vertices( void )
{
   struct pipe_vertex_element ve[2];
   struct pipe_vertex_buffer vbuf;
   void *handle;

   memset(ve, 0, sizeof ve);

   ve[0].src_offset = Offset(struct vertex, position);
   ve[0].src_format = PIPE_FORMAT_R32G32B32A32_FLOAT;
   ve[1].src_offset = Offset(struct vertex, color);
   ve[1].src_format = PIPE_FORMAT_R32G32B32A32_FLOAT;

   handle = ctx->create_vertex_elements_state(ctx, 2, ve);
   ctx->bind_vertex_elements_state(ctx, handle);


   vbuf.stride = sizeof( struct vertex );
   vbuf.buffer_offset = 0;
   vbuf.buffer = pipe_buffer_create_with_data(ctx,
                                              PIPE_BIND_VERTEX_BUFFER,
                                              PIPE_USAGE_STATIC,
                                              sizeof(vertices),
                                              vertices);

   ctx->set_vertex_buffers(ctx, 1, &vbuf);
}

static void set_vertex_shader( void )
{
   void *handle;
   const char *text =
      "VERT\n"
      "DCL IN[0]\n"
      "DCL IN[1]\n"
      "DCL OUT[0], POSITION\n"
      "DCL OUT[1], GENERIC[0]\n"
      "  0: MOV OUT[1], IN[1]\n"
      "  1: MOV OUT[0], IN[0]\n"
      "  2: END\n";

   handle = graw_parse_vertex_shader(ctx, text);
   ctx->bind_vs_state(ctx, handle);
}

static void set_fragment_shader( void )
{
   void *handle;
   const char *text =
      "FRAG\n"
      "DCL IN[0], GENERIC[0], PERSPECTIVE\n"
      "DCL OUT[0], COLOR\n"
      "DCL TEMP[0]\n"
      "DCL SAMP[0]\n"
      "DCL RES[0], 2D, FLOAT\n"
      "  0: SAMPLE TEMP[0], IN[0], RES[0], SAMP[0]\n"
      "  1: MOV OUT[0], TEMP[0]\n"
      "  2: END\n";

   handle = graw_parse_fragment_shader(ctx, text);
   ctx->bind_fs_state(ctx, handle);
}


static void draw( void )
{
   union pipe_color_union clear_color = { {.5,.5,.5,1} };

   ctx->clear(ctx, PIPE_CLEAR_COLOR, &clear_color, 0, 0);
   util_draw_arrays(ctx, PIPE_PRIM_QUADS, 0, 4);
   ctx->flush(ctx, NULL);

   graw_save_surface_to_file(ctx, surf, NULL);

   screen->flush_frontbuffer(screen, rttex, 0, 0, window);
}

#define SIZE 16

static void init_tex( void )
{ 
   struct pipe_sampler_view sv_template;
   struct pipe_sampler_state sampler_desc;
   struct pipe_resource templat;
   struct pipe_box box;
   ubyte tex2d[SIZE][SIZE][4];
   int s, t;

#if (SIZE != 2)
   for (s = 0; s < SIZE; s++) {
      for (t = 0; t < SIZE; t++) {
         if (0) {
            int x = (s ^ t) & 1;
	    tex2d[t][s][0] = (x) ? 0 : 63;
	    tex2d[t][s][1] = (x) ? 0 : 128;
	    tex2d[t][s][2] = 0;
	    tex2d[t][s][3] = 0xff;
         }
         else {
            int x = ((s ^ t) >> 2) & 1;
	    tex2d[t][s][0] = s*255/(SIZE-1);
	    tex2d[t][s][1] = t*255/(SIZE-1);
	    tex2d[t][s][2] = (x) ? 0 : 128;
	    tex2d[t][s][3] = 0xff;
         }
      }
   }
#else
   tex2d[0][0][0] = 0;
   tex2d[0][0][1] = 255;
   tex2d[0][0][2] = 255;
   tex2d[0][0][3] = 0;

   tex2d[0][1][0] = 0;
   tex2d[0][1][1] = 0;
   tex2d[0][1][2] = 255;
   tex2d[0][1][3] = 255;

   tex2d[1][0][0] = 255;
   tex2d[1][0][1] = 255;
   tex2d[1][0][2] = 0;
   tex2d[1][0][3] = 255;

   tex2d[1][1][0] = 255;
   tex2d[1][1][1] = 0;
   tex2d[1][1][2] = 0;
   tex2d[1][1][3] = 255;
#endif

   templat.target = PIPE_TEXTURE_2D;
   templat.format = PIPE_FORMAT_B8G8R8A8_UNORM;
   templat.width0 = SIZE;
   templat.height0 = SIZE;
   templat.depth0 = 1;
   templat.last_level = 0;
   templat.nr_samples = 1;
   templat.bind = PIPE_BIND_SAMPLER_VIEW;

   
   samptex = screen->resource_create(screen,
                                 &templat);
   if (samptex == NULL)
      exit(4);

   u_box_2d(0,0,SIZE,SIZE, &box);

   ctx->transfer_inline_write(ctx,
                              samptex,
                              0,
                              PIPE_TRANSFER_WRITE,
                              &box,
                              tex2d,
                              sizeof tex2d[0],
                              sizeof tex2d);

   /* Possibly read back & compare against original data:
    */
   if (0)
   {
      struct pipe_transfer *t;
      uint32_t *ptr;
      t = pipe_get_transfer(ctx, samptex,
                            0, 0, /* level, layer */
                            PIPE_TRANSFER_READ,
                            0, 0, SIZE, SIZE); /* x, y, width, height */

      ptr = ctx->transfer_map(ctx, t);

      if (memcmp(ptr, tex2d, sizeof tex2d) != 0) {
         assert(0);
         exit(9);
      }

      ctx->transfer_unmap(ctx, t);

      ctx->transfer_destroy(ctx, t);
   }

   memset(&sv_template, 0, sizeof sv_template);
   sv_template.format = samptex->format;
   sv_template.texture = samptex;
   sv_template.swizzle_r = 0;
   sv_template.swizzle_g = 1;
   sv_template.swizzle_b = 2;
   sv_template.swizzle_a = 3;
   sv = ctx->create_sampler_view(ctx, samptex, &sv_template);
   if (sv == NULL)
      exit(5);

   ctx->set_fragment_sampler_views(ctx, 1, &sv);
   

   memset(&sampler_desc, 0, sizeof sampler_desc);
   sampler_desc.wrap_s = PIPE_TEX_WRAP_REPEAT;
   sampler_desc.wrap_t = PIPE_TEX_WRAP_REPEAT;
   sampler_desc.wrap_r = PIPE_TEX_WRAP_REPEAT;
   sampler_desc.min_img_filter = PIPE_TEX_FILTER_NEAREST;
   sampler_desc.min_mip_filter = PIPE_TEX_MIPFILTER_NONE;
   sampler_desc.mag_img_filter = PIPE_TEX_FILTER_NEAREST;
   sampler_desc.compare_mode = PIPE_TEX_COMPARE_NONE;
   sampler_desc.compare_func = 0;
   sampler_desc.normalized_coords = 1;
   sampler_desc.max_anisotropy = 0;
   
   sampler = ctx->create_sampler_state(ctx, &sampler_desc);
   if (sampler == NULL)
      exit(6);

   ctx->bind_fragment_sampler_states(ctx, 1, &sampler);
   
}

static void init( void )
{
   struct pipe_framebuffer_state fb;
   struct pipe_resource templat;
   struct pipe_surface surf_tmpl;
   int i;

   /* It's hard to say whether window or screen should be created
    * first.  Different environments would prefer one or the other.
    *
    * Also, no easy way of querying supported formats if the screen
    * cannot be created first.
    */
   for (i = 0; formats[i] != PIPE_FORMAT_NONE; i++) {
      screen = graw_create_window_and_screen(0, 0, 300, 300,
                                             formats[i],
                                             &window);
      if (window && screen)
         break;
   }
   if (!screen || !window) {
      fprintf(stderr, "Unable to create window\n");
      exit(1);
   }

   ctx = screen->context_create(screen, NULL);
   if (ctx == NULL)
      exit(3);

   templat.target = PIPE_TEXTURE_2D;
   templat.format = formats[i];
   templat.width0 = WIDTH;
   templat.height0 = HEIGHT;
   templat.depth0 = 1;
   templat.array_size = 1;
   templat.last_level = 0;
   templat.nr_samples = 1;
   templat.bind = (PIPE_BIND_RENDER_TARGET |
                   PIPE_BIND_DISPLAY_TARGET);
   
   rttex = screen->resource_create(screen,
                                 &templat);
   if (rttex == NULL)
      exit(4);

   surf_tmpl.format = templat.format;
   surf_tmpl.usage = PIPE_BIND_RENDER_TARGET;
   surf_tmpl.u.tex.level = 0;
   surf_tmpl.u.tex.first_layer = 0;
   surf_tmpl.u.tex.last_layer = 0;
   surf = ctx->create_surface(ctx, rttex, &surf_tmpl);
   if (surf == NULL)
      exit(5);

   memset(&fb, 0, sizeof fb);
   fb.nr_cbufs = 1;
   fb.width = WIDTH;
   fb.height = HEIGHT;
   fb.cbufs[0] = surf;

   ctx->set_framebuffer_state(ctx, &fb);
   
   {
      struct pipe_blend_state blend;
      void *handle;
      memset(&blend, 0, sizeof blend);
      blend.rt[0].colormask = PIPE_MASK_RGBA;
      handle = ctx->create_blend_state(ctx, &blend);
      ctx->bind_blend_state(ctx, handle);
   }

   {
      struct pipe_depth_stencil_alpha_state depthstencil;
      void *handle;
      memset(&depthstencil, 0, sizeof depthstencil);
      handle = ctx->create_depth_stencil_alpha_state(ctx, &depthstencil);
      ctx->bind_depth_stencil_alpha_state(ctx, handle);
   }

   {
      struct pipe_rasterizer_state rasterizer;
      void *handle;
      memset(&rasterizer, 0, sizeof rasterizer);
      rasterizer.cull_face = PIPE_FACE_NONE;
      rasterizer.gl_rasterization_rules = 1;
      rasterizer.depth_clip = 1;
      handle = ctx->create_rasterizer_state(ctx, &rasterizer);
      ctx->bind_rasterizer_state(ctx, handle);
   }

   set_viewport(0, 0, WIDTH, HEIGHT, 30, 1000);

   init_tex();

   set_vertices();
   set_vertex_shader();
   set_fragment_shader();
}

static void args(int argc, char *argv[])
{
   int i;

   for (i = 1; i < argc;) {
      if (graw_parse_args(&i, argc, argv)) {
         continue;
      }
      exit(1);
   }
}


int main( int argc, char *argv[] )
{
   args(argc, argv);
   init();

   graw_set_display_func( draw );
   graw_main_loop();
   return 0;
}
