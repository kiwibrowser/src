
#include "i915_sw_winsys.h"
#include "i915_sw_public.h"
#include "util/u_memory.h"


/*
 * Helper functions
 */


static void
i915_sw_get_device_id(unsigned int *device_id)
{
   /* just pick a i945 hw id */
   *device_id = 0x27A2;
}

static void
i915_sw_destroy(struct i915_winsys *iws)
{
   struct i915_sw_winsys *isws = i915_sw_winsys(iws);
   FREE(isws);
}


/*
 * Exported functions
 */


struct i915_winsys *
i915_sw_winsys_create()
{
   struct i915_sw_winsys *isws;
   unsigned int deviceID;

   isws = CALLOC_STRUCT(i915_sw_winsys);
   if (!isws)
      return NULL;

   i915_sw_get_device_id(&deviceID);

   i915_sw_winsys_init_batchbuffer_functions(isws);
   i915_sw_winsys_init_buffer_functions(isws);
   i915_sw_winsys_init_fence_functions(isws);

   isws->base.destroy = i915_sw_destroy;

   isws->base.pci_id = deviceID;
   isws->max_batch_size = 16 * 4096;

   isws->dump_cmd = debug_get_bool_option("I915_DUMP_CMD", FALSE);

   return &isws->base;
}
