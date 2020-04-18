
#include "target-helpers/inline_wrapper_sw_helper.h"
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

   screen = sw_screen_wrap(screen);

   screen = debug_screen_wrap(screen);

   return screen;
}

static const struct drm_conf_ret throttle_ret = {
   .type = DRM_CONF_INT,
   .val.val_int = 2,
};

static const struct drm_conf_ret *drm_configuration(enum drm_conf conf)
{
   switch (conf) {
   case DRM_CONF_THROTTLE:
      return &throttle_ret;
   default:
      break;
   }
   return NULL;
}

DRM_DRIVER_DESCRIPTOR("vmwgfx", "vmwgfx", create_screen, drm_configuration)
