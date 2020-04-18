#include "state_tracker/drm_driver.h"
#include "target-helpers/inline_debug_helper.h"
#include "radeon/drm/radeon_drm_public.h"
#include "radeonsi/radeonsi_public.h"

static struct pipe_screen *create_screen(int fd)
{
   struct radeon_winsys *radeon;
   struct pipe_screen *screen;

   radeon = radeon_drm_winsys_create(fd);
   if (!radeon)
      return NULL;

   screen = radeonsi_screen_create(radeon);
   if (!screen)
      return NULL;

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

DRM_DRIVER_DESCRIPTOR("radeonsi", "radeon", create_screen, drm_configuration)
