/* Test the TGSI_SEMANTIC_POSITION fragment shader input.
 * Plus properties for upper-left vs. lower-left origin and
 * center integer vs. half-integer;
 */

#include <stdio.h>

#include "graw_util.h"


static int width = 300;
static int height = 300;

static struct graw_info info;

struct vertex {
   float position[4];
   float color[4];
};

/* Note: the upper-left vertex is pushed to the left a bit to
 * make sure we can spot upside-down rendering.
 */
static struct vertex vertices[] =
{
   {
      {-0.95, -0.95, 0.5, 1.0 },
      { 0, 0, 0, 1 }
   },

   {
      { 0.85, -0.95, 0.5, 1.0 },
      { 0, 0, 0, 1 }
   },

   {
      { 0.95,  0.95, 0.5, 1.0 },
      { 0, 0, 0, 1 }
   },

   {
      {-0.95,  0.95, 0.5, 1.0 },
      { 0, 0, 0, 1 }
   }
};

#define NUM_VERTS (sizeof(vertices) / sizeof(vertices[0]))


static void
set_vertices(void)
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
                                              sizeof(vertices),
                                              vertices);

   info.ctx->set_vertex_buffers(info.ctx, 1, &vbuf);
}


static void
set_vertex_shader(void)
{
   void *handle;
   const char *text =
      "VERT\n"
      "DCL IN[0]\n"
      "DCL IN[1]\n"
      "DCL OUT[0], POSITION\n"
      "DCL OUT[1], GENERIC[0]\n"
      "  0: MOV OUT[0], IN[0]\n"
      "  1: MOV OUT[1], IN[1]\n"
      "  2: END\n";

   handle = graw_parse_vertex_shader(info.ctx, text);
   info.ctx->bind_vs_state(info.ctx, handle);
}


static void
set_fragment_shader(int mode)
{
   void *handle;

   const char *origin_upper_left_text =
      "FRAG\n"
      "PROPERTY FS_COORD_ORIGIN UPPER_LEFT\n"  /* upper-left = black corner */
      "DCL IN[0], POSITION, LINEAR\n"
      "DCL OUT[0], COLOR\n"
      "DCL TEMP[0]\n"
      "IMM FLT32 { 0.003333, 0.003333, 1.0, 1.0 }\n"
      "IMM FLT32 { 0.0, 300.0, 0.0, 0.0 }\n"
      " 0: MOV TEMP[0], IN[0] \n"
      " 1: MOV TEMP[0].zw, IMM[1].xxxx \n"
      " 2: MUL OUT[0], TEMP[0], IMM[0] \n"
      " 3: END\n";

   const char *origin_lower_left_text =
      "FRAG\n"
      "PROPERTY FS_COORD_ORIGIN LOWER_LEFT\n"  /* lower-left = black corner */
      "DCL IN[0], POSITION, LINEAR\n"
      "DCL OUT[0], COLOR\n"
      "DCL TEMP[0]\n"
      "IMM FLT32 { 0.003333, 0.003333, 1.0, 1.0 }\n"
      "IMM FLT32 { 0.0, 300.0, 0.0, 0.0 }\n"
      " 0: MOV TEMP[0], IN[0] \n"
      " 1: MOV TEMP[0].zw, IMM[1].xxxx \n"
      " 2: MUL OUT[0], TEMP[0], IMM[0] \n"
      " 3: END\n";

   /* Test fragcoord center integer vs. half integer */
   const char *center_integer_text =
      "FRAG\n"
      "PROPERTY FS_COORD_PIXEL_CENTER INTEGER \n"       /* pixels are black */
      "DCL IN[0], POSITION, LINEAR \n"
      "DCL OUT[0], COLOR \n"
      "DCL TEMP[0] \n"
      "IMM FLT32 { 0.003333, 0.003333, 1.0, 1.0 } \n"
      "IMM FLT32 { 0.0, 300.0, 0.0, 0.0 } \n"
      "0: FRC TEMP[0], IN[0]  \n"
      "1: MOV TEMP[0].zw, IMM[1].xxxx \n"
      "2: MOV OUT[0], TEMP[0] \n"
      "3: END \n";

   const char *center_half_integer_text =
      "FRAG\n"
      "PROPERTY FS_COORD_PIXEL_CENTER HALF_INTEGER \n"  /* pixels are olive colored */
      "DCL IN[0], POSITION, LINEAR \n"
      "DCL OUT[0], COLOR \n"
      "DCL TEMP[0] \n"
      "IMM FLT32 { 0.003333, 0.003333, 1.0, 1.0 } \n"
      "IMM FLT32 { 0.0, 300.0, 0.0, 0.0 } \n"
      "0: FRC TEMP[0], IN[0]  \n"
      "1: MOV TEMP[0].zw, IMM[1].xxxx \n"
      "2: MOV OUT[0], TEMP[0] \n"
      "3: END \n";

   const char *text;

   if (mode == 0)
      text = origin_upper_left_text;
   else if (mode == 1)
      text = origin_lower_left_text;
   else if (mode == 2)
      text = center_integer_text;
   else
      text = center_half_integer_text;

   handle = graw_parse_fragment_shader(info.ctx, text);
   info.ctx->bind_fs_state(info.ctx, handle);
}


static void
draw(void)
{
   union pipe_color_union clear_color;

   clear_color.f[0] = 0.25;
   clear_color.f[1] = 0.25;
   clear_color.f[2] = 0.25;
   clear_color.f[3] = 1.0;

   info.ctx->clear(info.ctx,
              PIPE_CLEAR_COLOR | PIPE_CLEAR_DEPTHSTENCIL,
              &clear_color, 1.0, 0);
   util_draw_arrays(info.ctx, PIPE_PRIM_QUADS, 0, NUM_VERTS);
   info.ctx->flush(info.ctx, NULL);

#if 0
   /* At the moment, libgraw leaks out/makes available some of the
    * symbols from gallium/auxiliary, including these debug helpers.
    * Will eventually want to bless some of these paths, and lock the
    * others down so they aren't accessible from test programs.
    *
    * This currently just happens to work on debug builds - a release
    * build will probably fail to link here:
    */
   debug_dump_surface_bmp(info.ctx, "result.bmp", surf);
#endif

   graw_util_flush_front(&info);
}


#if 0
static void
resize(int w, int h)
{
   width = w;
   height = h;

   set_viewport(0, 0, width, height, 30, 1000);
}
#endif


static void
init(int mode)
{
   if (!graw_util_create_window(&info, width, height, 1, TRUE))
      exit(1);

   graw_util_default_state(&info, TRUE);

   graw_util_viewport(&info, 0, 0, width, height, -1.0, 1.0);

   set_vertices();
   set_vertex_shader();
   set_fragment_shader(mode);
}


int
main(int argc, char *argv[])
{
   int mode = argc > 1 ? atoi(argv[1]) : 0;

   switch (mode) {
   default:
   case 0:
      printf("frag coord origin upper-left (lower-left = black)\n");
      break;
   case 1:
      printf("frag coord origin lower-left (upper-left = black)\n");
      break;
   case 2:
      printf("frag coord center integer (all pixels black)\n");
      break;
   case 3:
      printf("frag coord center half-integer (all pixels olive color)\n");
      break;
   }

   init(mode);

   graw_set_display_func(draw);
   /*graw_set_reshape_func(resize);*/
   graw_main_loop();
   return 0;
}
