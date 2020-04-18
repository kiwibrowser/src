#include "state_tracker/drm_driver.h"
#include "target-helpers/inline_debug_helper.h"
#include "nouveau/drm/nouveau_drm_public.h"

static struct pipe_screen *create_screen(int fd)
{
   struct pipe_screen *screen;

   screen = nouveau_drm_screen_create(fd);
   if (!screen)
      return NULL;

   screen = debug_screen_wrap(screen);

   return screen;
}

DRM_DRIVER_DESCRIPTOR("nouveau", "nouveau", create_screen, NULL)
