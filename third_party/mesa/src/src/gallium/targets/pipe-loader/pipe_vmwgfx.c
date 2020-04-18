
#include "target-helpers/inline_debug_helper.h"
#include "state_tracker/drm_driver.h"
#include "svga/drm/svga_drm_public.h"
#include "svga/svga_public.h"

static struct pipe_screen *
create_screen(int fd)
{
   struct svga_winsys_screen *sws;
   struct pipe_screen *screen;

   sws = svga_drm_winsys_screen_create(fd);
   if (!sws)
      return NULL;

   screen = svga_screen_create(sws);
   if (!screen)
      return NULL;

   screen = debug_screen_wrap(screen);

   return screen;
}

PUBLIC
DRM_DRIVER_DESCRIPTOR("vmwgfx", "vmwgfx", create_screen, NULL)
