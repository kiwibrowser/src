/* Test texture swizzles */

#include <stdio.h>

#include "graw_util.h"


static struct graw_info info;

static struct pipe_resource *texture = NULL;
static struct pipe_sampler_view *sv = NULL;
static void *sampler = NULL;

static const int WIDTH = 300;
static const int HEIGHT = 300;

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


static void set_vertices(void)
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

static void set_vertex_shader(void)
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

static void set_fragment_shader(void)
{
   void *handle;
   const char *text =
      "FRAG\n"
      "DCL IN[0], GENERIC[0], PERSPECTIVE\n"
      "DCL OUT[0], COLOR\n"
      "DCL SAMP[0]\n"
      "  0: TXP OUT[0], IN[0], SAMP[0], 2D\n"
      "  2: END\n";

   handle = graw_parse_fragment_shader(info.ctx, text);
   info.ctx->bind_fs_state(info.ctx, handle);
}


static void draw(void)
{
   union pipe_color_union clear_color;

   clear_color.f[0] = 0.5;
   clear_color.f[1] = 0.5;
   clear_color.f[2] = 0.5;
   clear_color.f[3] = 1.0;

   info.ctx->clear(info.ctx, PIPE_CLEAR_COLOR, &clear_color, 0, 0);
   util_draw_arrays(info.ctx, PIPE_PRIM_QUADS, 0, 4);
   info.ctx->flush(info.ctx, NULL);

   graw_util_flush_front(&info);
}



static void
init_tex(const unsigned swizzle[4])
{ 
#define SIZE 256
   struct pipe_sampler_view sv_template;
   ubyte tex2d[SIZE][SIZE][4];
   int s, t;

   for (s = 0; s < SIZE; s++) {
      for (t = 0; t < SIZE; t++) {
         tex2d[t][s][0] = 0;  /*B*/
         tex2d[t][s][1] = t;  /*G*/
         tex2d[t][s][2] = s;  /*R*/
         tex2d[t][s][3] = 1;  /*A*/
      }
   }

   texture = graw_util_create_tex2d(&info, SIZE, SIZE, 
                                    PIPE_FORMAT_B8G8R8A8_UNORM, tex2d);

   memset(&sv_template, 0, sizeof sv_template);
   sv_template.format = texture->format;
   sv_template.texture = texture;
   sv_template.swizzle_r = swizzle[0];
   sv_template.swizzle_g = swizzle[1];
   sv_template.swizzle_b = swizzle[2];
   sv_template.swizzle_a = swizzle[3];
   sv = info.ctx->create_sampler_view(info.ctx, texture, &sv_template);
   if (sv == NULL)
      exit(5);

   info.ctx->set_fragment_sampler_views(info.ctx, 1, &sv);

   sampler = graw_util_create_simple_sampler(&info,
                                             PIPE_TEX_WRAP_REPEAT,
                                             PIPE_TEX_FILTER_NEAREST);

   info.ctx->bind_fragment_sampler_states(info.ctx, 1, &sampler);
#undef SIZE
}


static void
init(const unsigned swizzle[4])
{
   if (!graw_util_create_window(&info, WIDTH, HEIGHT, 1, FALSE))
      exit(1);

   graw_util_default_state(&info, FALSE);
   
   graw_util_viewport(&info, 0, 0, WIDTH, HEIGHT, 30, 10000);

   init_tex(swizzle);

   set_vertices();
   set_vertex_shader();
   set_fragment_shader();
}


static unsigned
char_to_swizzle(char c)
{
   switch (c) {
   case 'r':
      return PIPE_SWIZZLE_RED;
   case 'g':
      return PIPE_SWIZZLE_GREEN;
   case 'b':
      return PIPE_SWIZZLE_BLUE;
   case 'a':
      return PIPE_SWIZZLE_ALPHA;
   case '0':
      return PIPE_SWIZZLE_ZERO;
   case '1':
      return PIPE_SWIZZLE_ONE;
   default:
      return PIPE_SWIZZLE_RED;
   }
}


int main(int argc, char *argv[])
{
   const char swizzle_names[] = "rgba01";
   uint swizzle[4];
   int i;

   swizzle[0] = PIPE_SWIZZLE_RED;
   swizzle[1] = PIPE_SWIZZLE_GREEN;
   swizzle[2] = PIPE_SWIZZLE_BLUE;
   swizzle[3] = PIPE_SWIZZLE_ALPHA;

   for (i = 1; i < argc; i++) {
      swizzle[i-1] = char_to_swizzle(argv[i][0]);
   }

   printf("Example:\n");
   printf("  tex-swizzle r 0 g 1\n");
   printf("Current swizzle = ");
   for (i = 0; i < 4; i++) {
      printf("%c", swizzle_names[swizzle[i]]);
   }
   printf("\n");

   init(swizzle);

   graw_set_display_func(draw);
   graw_main_loop();
   return 0;
}
