
#include "i915_sw_winsys.h"
#include "i915/i915_batchbuffer.h"
#include "i915/i915_debug.h"
#include "util/u_memory.h"

#define BATCH_RESERVED 16

#define INTEL_DEFAULT_RELOCS 100
#define INTEL_MAX_RELOCS 400

#define INTEL_BATCH_NO_CLIPRECTS 0x1
#define INTEL_BATCH_CLIPRECTS    0x2

#define INTEL_ALWAYS_FLUSH

struct i915_sw_batchbuffer
{
   struct i915_winsys_batchbuffer base;

   size_t actual_size;
};

static INLINE struct i915_sw_batchbuffer *
i915_sw_batchbuffer(struct i915_winsys_batchbuffer *batch)
{
   return (struct i915_sw_batchbuffer *)batch;
}

static void
i915_sw_batchbuffer_reset(struct i915_sw_batchbuffer *batch)
{
   memset(batch->base.map, 0, batch->actual_size);
   batch->base.ptr = batch->base.map;
   batch->base.size = batch->actual_size - BATCH_RESERVED;
   batch->base.relocs = 0;
}

static struct i915_winsys_batchbuffer *
i915_sw_batchbuffer_create(struct i915_winsys *iws)
{
   struct i915_sw_winsys *isws = i915_sw_winsys(iws);
   struct i915_sw_batchbuffer *batch = CALLOC_STRUCT(i915_sw_batchbuffer);

   batch->actual_size = isws->max_batch_size;

   batch->base.map = MALLOC(batch->actual_size);
   batch->base.ptr = NULL;
   batch->base.size = 0;

   batch->base.relocs = 0;

   batch->base.iws = iws;

   i915_sw_batchbuffer_reset(batch);

   return &batch->base;
}

static boolean
i915_sw_batchbuffer_validate_buffers(struct i915_winsys_batchbuffer *batch,
				     struct i915_winsys_buffer **buffer,
				     int num_of_buffers)
{
   return TRUE;
}

static int
i915_sw_batchbuffer_reloc(struct i915_winsys_batchbuffer *ibatch,
                          struct i915_winsys_buffer *buffer,
                          enum i915_winsys_buffer_usage usage,
                          unsigned pre_add, boolean fenced)
{
   struct i915_sw_batchbuffer *batch = i915_sw_batchbuffer(ibatch);
   int ret = 0;

   if (usage == I915_USAGE_SAMPLER) {

   } else if (usage == I915_USAGE_RENDER) {

   } else if (usage == I915_USAGE_2D_TARGET) {

   } else if (usage == I915_USAGE_2D_SOURCE) {

   } else if (usage == I915_USAGE_VERTEX) {

   } else {
      assert(0);
      return -1;
   }

   ((uint32_t*)batch->base.ptr)[0] = 0;
   batch->base.ptr += 4;

   if (!ret)
      batch->base.relocs++;

   return ret;
}

static void
i915_sw_batchbuffer_flush(struct i915_winsys_batchbuffer *ibatch,
                          struct pipe_fence_handle **fence)
{
   struct i915_sw_batchbuffer *batch = i915_sw_batchbuffer(ibatch);
   unsigned used = 0;

   assert(i915_winsys_batchbuffer_space(ibatch) >= 0);

   used = batch->base.ptr - batch->base.map;
   assert((used & 3) == 0);

#ifdef INTEL_ALWAYS_FLUSH
   /* MI_FLUSH | FLUSH_MAP_CACHE */
   i915_winsys_batchbuffer_dword_unchecked(ibatch, (0x4<<23)|(1<<0));
   used += 4;
#endif

   if ((used & 4) == 0) {
      /* MI_NOOP */
      i915_winsys_batchbuffer_dword_unchecked(ibatch, 0);
   }
   /* MI_BATCH_BUFFER_END */
   i915_winsys_batchbuffer_dword_unchecked(ibatch, (0xA<<23));

   used = batch->base.ptr - batch->base.map;
   assert((used & 4) == 0);

   if (i915_sw_winsys(ibatch->iws)->dump_cmd) {
      i915_dump_batchbuffer(ibatch);
   }

   if (fence) {
      ibatch->iws->fence_reference(ibatch->iws, fence, NULL);

      (*fence) = i915_sw_fence_create();
   }

   i915_sw_batchbuffer_reset(batch);
}

static void
i915_sw_batchbuffer_destroy(struct i915_winsys_batchbuffer *ibatch)
{
   struct i915_sw_batchbuffer *batch = i915_sw_batchbuffer(ibatch);

   FREE(batch->base.map);
   FREE(batch);
}

void i915_sw_winsys_init_batchbuffer_functions(struct i915_sw_winsys *isws)
{
   isws->base.batchbuffer_create = i915_sw_batchbuffer_create;
   isws->base.validate_buffers = i915_sw_batchbuffer_validate_buffers;
   isws->base.batchbuffer_reloc = i915_sw_batchbuffer_reloc;
   isws->base.batchbuffer_flush = i915_sw_batchbuffer_flush;
   isws->base.batchbuffer_destroy = i915_sw_batchbuffer_destroy;
}
