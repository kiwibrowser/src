/* Test sRGB texturing.
 */

#include "graw_util.h"


static const int WIDTH = 600;
static const int HEIGHT = 300;

static struct graw_info info;

static struct pipe_resource *texture;
static struct pipe_sampler_view *linear_sv, *srgb_sv;


struct vertex {
   float position[4];
   float color[4];
};

static struct vertex vertices1[] =
{
   { { -0.1, -0.9, 0.0, 1.0 },
     { 1, 1, 0, 1 } },

   { { -0.1,  0.9, 0.0, 1.0 },
     { 1, 0, 0, 1 } },

   { {-0.9,  0.9, 0.0, 1.0 },
     { 0, 0, 0, 1 } },

   { {-0.9,  -0.9, 0.0, 1.0 },
     { 0, 1, 0, 1 } },
};


static struct vertex vertices2[] =
{
   { { 0.9, -0.9, 0.0, 1.0 },
     { 1, 1, 0, 1 } },

   { { 0.9,  0.9, 0.0, 1.0 },
     { 1, 0, 0, 1 } },

   { { 0.1,  0.9, 0.0, 1.0 },
     { 0, 0, 0, 1 } },

   { { 0.1,  -0.9, 0.0, 1.0 },
     { 0, 1, 0, 1 } },
};




static void
set_vertices(struct vertex *verts, unsigned num_verts)
{
   struct pipe_vertex_element ve[2];
   struct pipe_vertex_buffer vbuf;
   void *handle;

   memset(ve, 0, sizeof ve);

   ve[0].src_offset = Offset(struct vertex, position);
   ve[0].src_format = PIPE_FORMAT_R32G32B32A32_FLOAT;
   ve[1].src_offset = Offset(struct vertex, color);
   ve[1].src_format = PIPE_FORMAT_R32G32B32A32_FLOAT;

   handle = info.ctx->create_vertex_elements_state(info.ctx, 2, ve);
   info.ctx->bind_vertex_elements_state(info.ctx, handle);


   vbuf.stride = sizeof(struct vertex);
   vbuf.buffer_offset = 0;
   vbuf.buffer = pipe_buffer_create_with_data(info.ctx,
                                              PIPE_BIND_VERTEX_BUFFER,
                                              PIPE_USAGE_STATIC,
                                              num_verts * sizeof(struct vertex),
                                              verts);

   info.ctx->set_vertex_buffers(info.ctx, 1, &vbuf);
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

   handle = graw_parse_vertex_shader(info.ctx, text);
   info.ctx->bind_vs_state(info.ctx, handle);
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
      "  0: TXP TEMP[0], IN[0], SAMP[0], 2D\n"
      "  1: MOV OUT[0], TEMP[0]\n"
      "  2: END\n";

   handle = graw_parse_fragment_shader(info.ctx, text);
   info.ctx->bind_fs_state(info.ctx, handle);
}


static void draw( void )
{
   union pipe_color_union clear_color;

   clear_color.f[0] = 0.5;
   clear_color.f[1] = 0.5;
   clear_color.f[2] = 0.5;
   clear_color.f[3] = 1.0;

   info.ctx->clear(info.ctx, PIPE_CLEAR_COLOR, &clear_color, 0, 0);

   info.ctx->set_fragment_sampler_views(info.ctx, 1, &linear_sv);
   set_vertices(vertices1, 4);
   util_draw_arrays(info.ctx, PIPE_PRIM_QUADS, 0, 4);

   info.ctx->set_fragment_sampler_views(info.ctx, 1, &srgb_sv);
   set_vertices(vertices2, 4);
   util_draw_arrays(info.ctx, PIPE_PRIM_QUADS, 0, 4);

   info.ctx->flush(info.ctx, NULL);

   graw_util_flush_front(&info);
}


static void init_tex( void )
{ 
#define SIZE 64
   ubyte tex2d[SIZE][SIZE][4];
   int s, t;

   for (s = 0; s < SIZE; s++) {
      for (t = 0; t < SIZE; t++) {
         tex2d[t][s][0] = 0;
         tex2d[t][s][1] = s * 255 / SIZE;
         tex2d[t][s][2] = t * 255 / SIZE;
         tex2d[t][s][3] = 255;
      }
   }

   texture = graw_util_create_tex2d(&info, SIZE, SIZE, 
                                    PIPE_FORMAT_B8G8R8A8_UNORM, tex2d);

   {
      void *sampler;
      sampler = graw_util_create_simple_sampler(&info,
                                                PIPE_TEX_WRAP_REPEAT,
                                                PIPE_TEX_FILTER_NEAREST);
      info.ctx->bind_fragment_sampler_states(info.ctx, 1, &sampler);
   }

   /* linear sampler view */
   {
      struct pipe_sampler_view sv_temp;
      memset(&sv_temp, 0, sizeof sv_temp);
      sv_temp.format = PIPE_FORMAT_B8G8R8A8_UNORM;
      sv_temp.texture = texture;
      sv_temp.swizzle_r = PIPE_SWIZZLE_RED;
      sv_temp.swizzle_g = PIPE_SWIZZLE_GREEN;
      sv_temp.swizzle_b = PIPE_SWIZZLE_BLUE;
      sv_temp.swizzle_a = PIPE_SWIZZLE_ALPHA;
      linear_sv = info.ctx->create_sampler_view(info.ctx, texture, &sv_temp);
      if (linear_sv == NULL)
         exit(0);
   }

   /* srgb sampler view */
   {
      struct pipe_sampler_view sv_temp;
      memset(&sv_temp, 0, sizeof sv_temp);
      sv_temp.format = PIPE_FORMAT_B8G8R8A8_SRGB;
      sv_temp.texture = texture;
      sv_temp.swizzle_r = PIPE_SWIZZLE_RED;
      sv_temp.swizzle_g = PIPE_SWIZZLE_GREEN;
      sv_temp.swizzle_b = PIPE_SWIZZLE_BLUE;
      sv_temp.swizzle_a = PIPE_SWIZZLE_ALPHA;
      srgb_sv = info.ctx->create_sampler_view(info.ctx, texture, &sv_temp);
      if (srgb_sv == NULL)
         exit(0);
   }
#undef SIZE
}

static void init( void )
{
   if (!graw_util_create_window(&info, WIDTH, HEIGHT, 1, FALSE))
      exit(1);

   graw_util_default_state(&info, FALSE);
   
   graw_util_viewport(&info, 0, 0, WIDTH, HEIGHT, 30, 10000);

   init_tex();

   set_vertex_shader();
   set_fragment_shader();
}


int main( int argc, char *argv[] )
{
   init();

   graw_set_display_func( draw );
   graw_main_loop();
   return 0;
}
