
#ifndef I915_SW_WINSYS_H
#define I915_SW_WINSYS_H

#include "i915/i915_winsys.h"


/*
 * Winsys
 */


struct i915_sw_winsys
{
   struct i915_winsys base;

   boolean dump_cmd;

   size_t max_batch_size;
};

static INLINE struct i915_sw_winsys *
i915_sw_winsys(struct i915_winsys *iws)
{
   return (struct i915_sw_winsys *)iws;
}

struct pipe_fence_handle * i915_sw_fence_create(void);

void i915_sw_winsys_init_batchbuffer_functions(struct i915_sw_winsys *idws);
void i915_sw_winsys_init_buffer_functions(struct i915_sw_winsys *idws);
void i915_sw_winsys_init_fence_functions(struct i915_sw_winsys *idws);


/*
 * Buffer
 */


struct i915_sw_buffer {
   unsigned magic;

   void *ptr;
   unsigned map_count;
   enum i915_winsys_buffer_type type;
   enum i915_winsys_buffer_tile tiling;
   unsigned stride;
};

static INLINE struct i915_sw_buffer *
i915_sw_buffer(struct i915_winsys_buffer *buffer)
{
   return (struct i915_sw_buffer *)buffer;
}

#endif
