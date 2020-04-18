
#include "target-helpers/inline_debug_helper.h"
#include "state_tracker/drm_driver.h"
#include "i915/drm/i915_drm_public.h"
#include "i915/i915_public.h"

static struct pipe_screen *
create_screen(int fd)
{
   struct i915_winsys *iws;
   struct pipe_screen *screen;

   iws = i915_drm_winsys_create(fd);
   if (!iws)
      return NULL;

   screen = i915_screen_create(iws);
   if (!screen)
      return NULL;

   screen = debug_screen_wrap(screen);

   return screen;
}

PUBLIC
DRM_DRIVER_DESCRIPTOR("i915", "i915", create_screen, NULL)
