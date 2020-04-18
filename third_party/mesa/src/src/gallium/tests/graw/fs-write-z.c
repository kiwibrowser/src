/* Test the writing Z in fragment shader.
 * The red quad should be entirely in front of the blue quad even
 * though the overlap and intersect in Z.
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

#define z0 0.2
#define z01 0.5
#define z1 0.4


static struct vertex vertices[] =
{
   /* left quad: clock-wise, front-facing, red */
   {
      {-0.8, -0.9, z0, 1.0 },
      { 1, 0, 0, 1 }
   },

   {
      { -0.2, -0.9, z0, 1.0 },
      { 1, 0, 0, 1 }
   },

   {
      { 0.2,  0.9, z01, 1.0 },
      { 1, 0, 0, 1 }
   },

   {
      {-0.9,  0.9, z01, 1.0 },
      { 1, 0, 0, 1 }
   },

   /* right quad : counter-clock-wise, back-facing, green */
   {
      { 0.2,  -0.9, z1, 1.0 },
      { 0, 0, 1, -1 }
   },

   {
      { -0.2,  0.8, z1, 1.0 },
      { 0, 0, 1, -1 }
   },

   {
      { 0.9,  0.8, z1, 1.0 },
      { 0, 0, 1, -1 }
   },

   {
      { 0.8, -0.9, z1, 1.0 },
      { 0, 0, 1, -1 }
   },
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
set_fragment_shader(void)
{
   void *handle;
   const char *text =
      "FRAG\n"
      "DCL IN[0], GENERIC, CONSTANT\n"
      "DCL OUT[0], COLOR\n"
      "DCL OUT[1], POSITION\n"
      "DCL TEMP[0]\n"
      "IMM FLT32 {    1.0,     0.0,     0.0,     0.0 }\n"
      "IMM FLT32 {    0.0,     1.0,     0.0,     0.0 }\n"
      "IMM FLT32 {    0.5,     0.4,     0.0,     0.0 }\n"
      " 0: MOV OUT[0], IN[0]\n"    /* front-facing: red */
      " 1: IF IN[0].xxxx :3\n"
      " 2:   MOV OUT[1].z, IMM[2].yyyy\n"   /* red: Z = 0.4 */
      " 3: ELSE :5\n"
      " 4:   MOV OUT[1].z, IMM[2].xxxx\n"   /* blue: Z = 0.5 */
      " 5: ENDIF\n"
      " 6: END\n";

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
   clear_color.f[3] = 1.00;

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

   graw_util_viewport(&info, 0, 0, width, height, -1.0, 1.0);
}
#endif


static void
init(void)
{
   if (!graw_util_create_window(&info, width, height, 1, TRUE))
      exit(1);

   graw_util_default_state(&info, TRUE);

   graw_util_viewport(&info, 0, 0, width, height, -1.0, 1.0);

   set_vertices();
   set_vertex_shader();
   set_fragment_shader();
}


int
main(int argc, char *argv[])
{
   init();

   printf("The red quad should be entirely in front of the blue quad.\n");

   graw_set_display_func(draw);
   /*graw_set_reshape_func(resize);*/
   graw_main_loop();
   return 0;
}
