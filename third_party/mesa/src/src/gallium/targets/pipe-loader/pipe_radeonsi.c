#include "state_tracker/drm_driver.h"
#include "target-helpers/inline_debug_helper.h"
#include "radeon/drm/radeon_drm_public.h"
#include "radeonsi/radeonsi_public.h"

static struct pipe_screen *
create_screen(int fd)
{
   struct radeon_winsys *rw;
   struct pipe_screen *screen;

   rw = radeon_drm_winsys_create(fd);
   if (!rw)
      return NULL;

   screen = radeonsi_screen_create(rw);
   if (!screen)
      return NULL;

   screen = debug_screen_wrap(screen);

   return screen;
}

PUBLIC
DRM_DRIVER_DESCRIPTOR("radeonsi", "radeon", create_screen, NULL)
