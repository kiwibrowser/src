#include <stdio.h>
#include <sys/ioctl.h>

#include "i915_drm.h"

#include "state_tracker/drm_driver.h"

#include "i915_drm_winsys.h"
#include "i915_drm_public.h"
#include "util/u_memory.h"


/*
 * Helper functions
 */


static void
i915_drm_get_device_id(int fd, unsigned int *device_id)
{
   int ret;
   struct drm_i915_getparam gp;

   gp.param = I915_PARAM_CHIPSET_ID;
   gp.value = (int *)device_id;

   ret = ioctl(fd, DRM_IOCTL_I915_GETPARAM, &gp, sizeof(gp));
   assert(ret == 0);
}

static void
i915_drm_winsys_destroy(struct i915_winsys *iws)
{
   struct i915_drm_winsys *idws = i915_drm_winsys(iws);

   drm_intel_bufmgr_destroy(idws->gem_manager);

   FREE(idws);
}

struct i915_winsys *
i915_drm_winsys_create(int drmFD)
{
   struct i915_drm_winsys *idws;
   unsigned int deviceID;

   idws = CALLOC_STRUCT(i915_drm_winsys);
   if (!idws)
      return NULL;

   i915_drm_get_device_id(drmFD, &deviceID);

   i915_drm_winsys_init_batchbuffer_functions(idws);
   i915_drm_winsys_init_buffer_functions(idws);
   i915_drm_winsys_init_fence_functions(idws);

   idws->fd = drmFD;
   idws->base.pci_id = deviceID;
   idws->max_batch_size = 16 * 4096;

   idws->base.destroy = i915_drm_winsys_destroy;

   idws->gem_manager = drm_intel_bufmgr_gem_init(idws->fd, idws->max_batch_size);
   drm_intel_bufmgr_gem_enable_reuse(idws->gem_manager);
   drm_intel_bufmgr_gem_enable_fenced_relocs(idws->gem_manager);

   idws->dump_cmd = debug_get_bool_option("I915_DUMP_CMD", FALSE);
   idws->dump_raw_file = debug_get_option("I915_DUMP_RAW_FILE", NULL);
   idws->send_cmd = !debug_get_bool_option("I915_NO_HW", FALSE);

   return &idws->base;
}
