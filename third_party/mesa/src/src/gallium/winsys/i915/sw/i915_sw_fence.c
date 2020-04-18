
#include "i915_sw_winsys.h"
#include "util/u_memory.h"
#include "util/u_atomic.h"
#include "util/u_inlines.h"

struct i915_sw_fence
{
   struct pipe_reference reference;
};

struct pipe_fence_handle *
i915_sw_fence_create()
{
   struct i915_sw_fence *fence = CALLOC_STRUCT(i915_sw_fence);

   pipe_reference_init(&fence->reference, 1);

   return (struct pipe_fence_handle *)fence;
}

static void
i915_sw_fence_reference(struct i915_winsys *iws,
                        struct pipe_fence_handle **ptr,
                        struct pipe_fence_handle *fence)
{
   struct i915_sw_fence *old = (struct i915_sw_fence *)*ptr;
   struct i915_sw_fence *f = (struct i915_sw_fence *)fence;

   if (pipe_reference(&((struct i915_sw_fence *)(*ptr))->reference, &f->reference)) {
      FREE(old);
   }
   *ptr = fence;
}

static int
i915_sw_fence_signalled(struct i915_winsys *iws,
                        struct pipe_fence_handle *fence)
{
   assert(0);

   return 0;
}

static int
i915_sw_fence_finish(struct i915_winsys *iws,
                     struct pipe_fence_handle *fence)
{
   return 0;
}

void
i915_sw_winsys_init_fence_functions(struct i915_sw_winsys *isws)
{
   isws->base.fence_reference = i915_sw_fence_reference;
   isws->base.fence_signalled = i915_sw_fence_signalled;
   isws->base.fence_finish = i915_sw_fence_finish;
}
