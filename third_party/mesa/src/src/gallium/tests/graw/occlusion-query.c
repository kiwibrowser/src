/* Test gallium occlusion queries.
 */

#include <stdio.h>

#include "graw_util.h"


static int width = 300;
static int height = 300;

/* expected results of occlusion test (depndsd on window size) */
static int expected1 = (int) ((300 * 0.9) * (300 * 0.9));
static int expected2 = 420;


static struct graw_info info;

struct vertex {
   float position[4];
   float color[4];
};

#define z0 0.2
#define z1 0.6

static struct vertex obj1_vertices[4] =
{
   {
      {-0.9, -0.9, z0, 1.0 },
      { 1, 0, 0, 1 }
   },

   {
      { 0.9, -0.9, z0, 1.0 },
      { 1, 0, 0, 1 }
   },

   {
      { 0.9,  0.9, z0, 1.0 },
      { 1, 0, 0, 1 }
   },

   {
      {-0.9,  0.9, z0, 1.0 },
      { 1, 0, 0, 1 }
   }
};

static struct vertex obj2_vertices[4] = 
{
   {
      { -0.2,  -0.2, z1, 1.0 },
      { 0, 0, 1, 1 }
   },

   {
      { 0.95, -0.2, z1, 1.0 },
      { 0, 0, 1, 1 }
   },

   {
      { 0.95,  0.2, z1, 1.0 },
      { 0, 0, 1, 1 }
   },

   {
      { -0.2, 0.2, z1, 1.0 },
      { 0, 0, 1, 1 }
   },
};

#define NUM_VERTS 4



static void
set_vertices(struct vertex *vertices, unsigned bytes)
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
                                              bytes,
                                              vertices);

   info.ctx->set_vertex_buffers(info.ctx, 1, &vbuf);
}


static void
set_vertex_shader(struct graw_info *info)
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

   handle = graw_parse_vertex_shader(info->ctx, text);
   if (!handle) {
      debug_printf("Failed to parse vertex shader\n");
      return;
   }
   info->ctx->bind_vs_state(info->ctx, handle);
}


static void
set_fragment_shader(struct graw_info *info)
{
   void *handle;
   const char *text =
      "FRAG\n"
      "DCL IN[0], GENERIC, LINEAR\n"
      "DCL OUT[0], COLOR\n"
      " 0: MOV OUT[0], IN[0]\n"
      " 1: END\n";

   handle = graw_parse_fragment_shader(info->ctx, text);
   if (!handle) {
      debug_printf("Failed to parse fragment shader\n");
      return;
   }
   info->ctx->bind_fs_state(info->ctx, handle);
}


static void
draw(void)
{
   int expected1_min = (int) (expected1 * 0.95);
   int expected1_max = (int) (expected1 * 1.05);
   int expected2_min = (int) (expected2 * 0.95);
   int expected2_max = (int) (expected2 * 1.05);

   union pipe_color_union clear_color;

   struct pipe_query *q1, *q2;
   uint64_t res1, res2;

   clear_color.f[0] = 0.25;
   clear_color.f[1] = 0.25;
   clear_color.f[2] = 0.25;
   clear_color.f[3] = 1.00;

   info.ctx->clear(info.ctx,
                   PIPE_CLEAR_COLOR | PIPE_CLEAR_DEPTHSTENCIL,
                   &clear_color, 1.0, 0);

   q1 = info.ctx->create_query(info.ctx, PIPE_QUERY_OCCLUSION_COUNTER);
   q2 = info.ctx->create_query(info.ctx, PIPE_QUERY_OCCLUSION_COUNTER);

   /* draw first, large object */
   set_vertices(obj1_vertices, sizeof(obj1_vertices));
   info.ctx->begin_query(info.ctx, q1);
   util_draw_arrays(info.ctx, PIPE_PRIM_QUADS, 0, NUM_VERTS);
   info.ctx->end_query(info.ctx, q1);

   /* draw second, small object behind first object */
   set_vertices(obj2_vertices, sizeof(obj2_vertices));
   info.ctx->begin_query(info.ctx, q2);
   util_draw_arrays(info.ctx, PIPE_PRIM_QUADS, 0, NUM_VERTS);
   info.ctx->end_query(info.ctx, q2);

   info.ctx->get_query_result(info.ctx, q1, TRUE, &res1);
   info.ctx->get_query_result(info.ctx, q2, TRUE, &res2);

   printf("result1 = %lu  result2 = %lu\n", res1, res2);
   if (res1 < expected1_min || res1 > expected1_max)
      printf("  Failure: result1 should be near %d\n", expected1);
   if (res2 < expected2_min || res2 > expected2_max)
      printf("  Failure: result2 should be near %d\n", expected2);

   info.ctx->flush(info.ctx, NULL);

   graw_util_flush_front(&info);

   info.ctx->destroy_query(info.ctx, q1);
   info.ctx->destroy_query(info.ctx, q2);
}


#if 0
static void
resize(int w, int h)
{
   width = w;
   height = h;

   graw_util_viewport(&info, 0, 0, width, height, 30, 1000);
}
#endif


static void
init(void)   
{
   if (!graw_util_create_window(&info, width, height, 1, TRUE))
      exit(1);

   graw_util_default_state(&info, TRUE);

   graw_util_viewport(&info, 0, 0, width, height, -1.0, 1.0);

   set_vertex_shader(&info);
   set_fragment_shader(&info);
}


int
main(int argc, char *argv[])
{
   init();

   printf("The red quad should mostly occlude the blue quad.\n");

   graw_set_display_func(draw);
   /*graw_set_reshape_func(resize);*/
   graw_main_loop();
   return 0;
}
