/* Display a cleared blue window.  This demo has no dependencies on
 * any utility code, just the graw interface and gallium.
 */

#include <stdio.h>
#include "state_tracker/graw.h"
#include "pipe/p_screen.h"
#include "pipe/p_context.h"
#include "pipe/p_state.h"
#include "pipe/p_defines.h"

enum pipe_format formats[] = {
   PIPE_FORMAT_R8G8B8A8_UNORM,
   PIPE_FORMAT_B8G8R8A8_UNORM,
   PIPE_FORMAT_NONE
};

static const int WIDTH = 300;
static const int HEIGHT = 300;

struct pipe_screen *screen;
struct pipe_context *ctx;
struct pipe_surface *surf;
struct pipe_resource *tex;
static void *window = NULL;

static void draw( void )
{
   union pipe_color_union clear_color = { {1, 0, 1, 1} };

   ctx->clear(ctx, PIPE_CLEAR_COLOR, &clear_color, 0, 0);
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
