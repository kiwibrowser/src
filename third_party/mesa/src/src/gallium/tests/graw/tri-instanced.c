/*
 * Test draw instancing.
 */

#include <stdio.h>
#include <string.h>

#include "state_tracker/graw.h"
#include "pipe/p_screen.h"
#include "pipe/p_context.h"
#include "pipe/p_state.h"
#include "pipe/p_defines.h"

#include "util/u_memory.h"      /* Offset() */
#include "util/u_draw_quad.h"
#include "util/u_inlines.h"


enum pipe_format formats[] = {
   PIPE_FORMAT_R8G8B8A8_UNORM,
   PIPE_FORMAT_B8G8R8A8_UNORM,
   PIPE_FORMAT_NONE
};

static const int WIDTH = 300;
static const int HEIGHT = 300;

static struct pipe_screen *screen = NULL;
static struct pipe_context *ctx = NULL;
static struct pipe_surface *surf = NULL;
static struct pipe_resource *tex = NULL;
static void *window = NULL;

struct vertex {
   float position[4];
   float color[4];
};


static int draw_elements = 0;


/**
 * Vertex data.
 * Each vertex has three attributes: position, color and translation.
 * The translation attribute is a per-instance attribute.  See
 * "instance_divisor" below.
 */
static struct vertex vertices[4] =
{
   {
      { 0.0f, -0.3f, 0.0f, 1.0f },  /* pos */
      { 1.0f, 0.0f, 0.0f, 1.0f }    /* color */
   },
   {
      { -0.2f, 0.3f, 0.0f, 1.0f },
      { 0.0f, 1.0f, 0.0f, 1.0f }
   },
   {
      { 0.2f, 0.3f, 0.0f, 1.0f },
      { 0.0f, 0.0f, 1.0f, 1.0f }
   }
};


#define NUM_INST 5

static float inst_data[NUM_INST][4] =
{
   { -0.50f, 0.4f, 0.0f, 0.0f },
   { -0.25f, 0.1f, 0.0f, 0.0f },
   { 0.00f, 0.2f, 0.0f, 0.0f },
   { 0.25f, 0.1f, 0.0f, 0.0f },
   { 0.50f, 0.3f, 0.0f, 0.0f }
};


static ushort indices[3] = { 0, 2, 1 };


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
   struct pipe_vertex_element ve[3];
   struct pipe_vertex_buffer vbuf[2];
   struct pipe_index_buffer ibuf;
   void *handle;

   memset(ve, 0, sizeof ve);

   /* pos */
   ve[0].src_offset = Offset(struct vertex, position);
   ve[0].src_format = PIPE_FORMAT_R32G32B32A32_FLOAT;
   ve[0].vertex_buffer_index = 0;

   /* color */
   ve[1].src_offset = Offset(struct vertex, color);
   ve[1].src_format = PIPE_FORMAT_R32G32B32A32_FLOAT;
   ve[1].vertex_buffer_index = 0;

   /* per-instance info */
   ve[2].src_offset = 0;
   ve[2].src_format = PIPE_FORMAT_R32G32B32A32_FLOAT;
   ve[2].vertex_buffer_index = 1;
   ve[2].instance_divisor = 1;

   handle = ctx->create_vertex_elements_state(ctx, 3, ve);
   ctx->bind_vertex_elements_state(ctx, handle);


   /* vertex data */
   vbuf[0].stride = sizeof( struct vertex );
   vbuf[0].buffer_offset = 0;
   vbuf[0].buffer = pipe_buffer_create_with_data(ctx,
                                                 PIPE_BIND_VERTEX_BUFFER,
                                                 PIPE_USAGE_STATIC,
                                                 sizeof(vertices),
                                                 vertices);

   /* instance data */
   vbuf[1].stride = sizeof( inst_data[0] );
   vbuf[1].buffer_offset = 0;
   vbuf[1].buffer = pipe_buffer_create_with_data(ctx,
                                                 PIPE_BIND_VERTEX_BUFFER,
                                                 PIPE_USAGE_STATIC,
                                                 sizeof(inst_data),
                                                 inst_data);

   ctx->set_vertex_buffers(ctx, 2, vbuf);

   /* index data */
   ibuf.buffer = pipe_buffer_create_with_data(ctx,
                                              PIPE_BIND_INDEX_BUFFER,
                                              PIPE_USAGE_STATIC,
                                              sizeof(indices),
                                              indices);
   ibuf.offset = 0;
   ibuf.index_size = 2;

   ctx->set_index_buffer(ctx, &ibuf);

}

static void set_vertex_shader( void )
{
   void *handle;
   const char *text =
      "VERT\n"
      "DCL IN[0]\n"
      "DCL IN[1]\n"
      "DCL IN[2]\n"
      "DCL OUT[0], POSITION\n"
      "DCL OUT[1], COLOR\n"
      "  0: MOV OUT[1], IN[1]\n"
      "  1: ADD OUT[0], IN[0], IN[2]\n"  /* add instance pos to vertex pos */
      "  2: END\n";

   handle = graw_parse_vertex_shader(ctx, text);
   ctx->bind_vs_state(ctx, handle);
}

static void set_fragment_shader( void )
{
   void *handle;
   const char *text =
      "FRAG\n"
      "DCL IN[0], COLOR, LINEAR\n"
      "DCL OUT[0], COLOR\n"
      "  0: MOV OUT[0], IN[0]\n"
      "  1: END\n";

   handle = graw_parse_fragment_shader(ctx, text);
   ctx->bind_fs_state(ctx, handle);
}


static void draw( void )
{
   union pipe_color_union clear_color = { {1,0,1,1} };
   struct pipe_draw_info info;

   ctx->clear(ctx, PIPE_CLEAR_COLOR, &clear_color, 0, 0);

   util_draw_init_info(&info);
   info.indexed = (draw_elements != 0);
   info.mode = PIPE_PRIM_TRIANGLES;
   info.start = 0;
   info.count = 3;
   /* draw NUM_INST triangles */
   info.instance_count = NUM_INST;

   ctx->draw_vbo(ctx, &info);

   ctx->flush(ctx, NULL);

   graw_save_surface_to_file(ctx, surf, NULL);

   screen->flush_frontbuffer(screen, tex, 0, 0, window);
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
   
   tex = screen->resource_create(screen,
                                 &templat);
   if (tex == NULL)
      exit(4);

   surf_tmpl.format = templat.format;
   surf_tmpl.usage = PIPE_BIND_RENDER_TARGET;
   surf_tmpl.u.tex.level = 0;
   surf_tmpl.u.tex.first_layer = 0;
   surf_tmpl.u.tex.last_layer = 0;
   surf = ctx->create_surface(ctx, tex, &surf_tmpl);
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
   set_vertices();
   set_vertex_shader();
   set_fragment_shader();
}


static void options(int argc, char *argv[])
{
   int i;

   for (i = 1; i < argc;) {
      if (graw_parse_args(&i, argc, argv)) {
         continue;
      }
      if (strcmp(argv[i], "-e") == 0) {
         draw_elements = 1;
         i++;
      }
      else {
         i++;
      }
   }
   if (draw_elements)
      printf("Using pipe_context::draw_elements_instanced()\n");
   else
      printf("Using pipe_context::draw_arrays_instanced()\n");
}


int main( int argc, char *argv[] )
{
   options(argc, argv);

   init();

   graw_set_display_func( draw );
   graw_main_loop();
   return 0;
}
