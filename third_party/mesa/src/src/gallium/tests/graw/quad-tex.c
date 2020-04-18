/* Display a cleared blue window.  This demo has no dependencies on
 * any utility code, just the graw interface and gallium.
 */

#include "graw_util.h"

static const int WIDTH = 300;
static const int HEIGHT = 300;

static struct graw_info info;


static struct pipe_resource *texture = NULL;
static struct pipe_sampler_view *sv = NULL;
static void *sampler = NULL;

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

   handle = info.ctx->create_vertex_elements_state(info.ctx, 2, ve);
   info.ctx->bind_vertex_elements_state(info.ctx, handle);


   vbuf.stride = sizeof( struct vertex );
   vbuf.buffer_offset = 0;
   vbuf.buffer = pipe_buffer_create_with_data(info.ctx,
                                              PIPE_BIND_VERTEX_BUFFER,
                                              PIPE_USAGE_STATIC,
                                              sizeof(vertices),
                                              vertices);

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
   union pipe_color_union clear_color = { {.5,.5,.5,1} };

   info.ctx->clear(info.ctx, PIPE_CLEAR_COLOR, &clear_color, 0, 0);
   util_draw_arrays(info.ctx, PIPE_PRIM_QUADS, 0, 4);
   info.ctx->flush(info.ctx, NULL);

   graw_save_surface_to_file(info.ctx, info.color_surf[0], NULL);

   graw_util_flush_front(&info);
}


#define SIZE 16

static void init_tex( void )
{ 
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

   texture = graw_util_create_tex2d(&info, SIZE, SIZE, 
                                    PIPE_FORMAT_B8G8R8A8_UNORM, tex2d);

   sv = graw_util_create_simple_sampler_view(&info, texture);
   info.ctx->set_fragment_sampler_views(info.ctx, 1, &sv);

   sampler = graw_util_create_simple_sampler(&info, 
                                             PIPE_TEX_WRAP_REPEAT,
                                             PIPE_TEX_FILTER_NEAREST);
   info.ctx->bind_fragment_sampler_states(info.ctx, 1, &sampler);
}


static void init( void )
{
   if (!graw_util_create_window(&info, WIDTH, HEIGHT, 1, FALSE))
      exit(1);

   graw_util_default_state(&info, FALSE);

   {
      struct pipe_rasterizer_state rasterizer;
      void *handle;
      memset(&rasterizer, 0, sizeof rasterizer);
      rasterizer.cull_face = PIPE_FACE_NONE;
      rasterizer.gl_rasterization_rules = 1;
      rasterizer.depth_clip = 1;
      handle = info.ctx->create_rasterizer_state(info.ctx, &rasterizer);
      info.ctx->bind_rasterizer_state(info.ctx, handle);
   }

   graw_util_viewport(&info, 0, 0, WIDTH, HEIGHT, 30, 1000);

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
