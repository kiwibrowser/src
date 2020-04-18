/* Display a cleared blue window.  This demo has no dependencies on
 * any utility code, just the graw interface and gallium.
 */

#include <stdio.h>
#include "graw_util.h"

static struct graw_info info;

static const int WIDTH = 300;
static const int HEIGHT = 300;


struct vertex {
   float position[4];
   float color[4];
};

static boolean FlatShade = FALSE;


static struct vertex vertices[3] =
{
   {
      { 0.0f, -0.9f, 0.0f, 1.0f },
      { 1.0f, 0.0f, 0.0f, 1.0f }
   },
   {
      { -0.9f, 0.9f, 0.0f, 1.0f },
      { 0.0f, 1.0f, 0.0f, 1.0f }
   },
   {
      { 0.9f, 0.9f, 0.0f, 1.0f },
      { 0.0f, 0.0f, 1.0f, 1.0f }
   }
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
      "DCL OUT[1], COLOR\n"
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
      "DCL IN[0], COLOR, LINEAR\n"
      "DCL OUT[0], COLOR\n"
      "  0: MOV OUT[0], IN[0]\n"
      "  1: END\n";

   handle = graw_parse_fragment_shader(info.ctx, text);
   info.ctx->bind_fs_state(info.ctx, handle);
}


static void draw( void )
{
   union pipe_color_union clear_color = { {1,0,1,1} };

   info.ctx->clear(info.ctx, PIPE_CLEAR_COLOR, &clear_color, 0, 0);
   util_draw_arrays(info.ctx, PIPE_PRIM_TRIANGLES, 0, 3);
   info.ctx->flush(info.ctx, NULL);

   graw_save_surface_to_file(info.ctx, info.color_surf[0], NULL);

   graw_util_flush_front(&info);
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
      rasterizer.flatshade = FlatShade;
      rasterizer.depth_clip = 1;
      handle = info.ctx->create_rasterizer_state(info.ctx, &rasterizer);
      info.ctx->bind_rasterizer_state(info.ctx, handle);
   }


   graw_util_viewport(&info, 0, 0, WIDTH, HEIGHT, 30, 1000);

   set_vertices();
   set_vertex_shader();
   set_fragment_shader();
}

static void args(int argc, char *argv[])
{
   int i;

   for (i = 1; i < argc; ) {
      if (graw_parse_args(&i, argc, argv)) {
         /* ok */
      }
      else if (strcmp(argv[i], "-f") == 0) {
         FlatShade = TRUE;
         i++;
      }
      else {
         printf("Invalid arg %s\n", argv[i]);
         exit(1);
      }
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
