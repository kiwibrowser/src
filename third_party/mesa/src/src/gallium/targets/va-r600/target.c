#include "state_tracker/drm_driver.h"
#include "target-helpers/inline_debug_helper.h"
#include "radeon/drm/radeon_drm_public.h"
#include "r600/r600_public.h"

static struct pipe_screen *create_screen(int fd)
{
   struct radeon_winsys *radeon;
   struct pipe_screen *screen;

   radeon = radeon_drm_winsys_create(fd);
   if (!radeon)
      return NULL;

   screen = r600_screen_create(radeon);
   if (!screen)
      return NULL;

   screen = debug_screen_wrap(screen);

   return screen;
}

DRM_DRIVER_DESCRIPTOR("r600", "radeon", create_screen, NULL)
