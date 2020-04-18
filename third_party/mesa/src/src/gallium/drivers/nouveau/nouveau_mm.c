
#include <inttypes.h>

#include "util/u_inlines.h"
#include "util/u_memory.h"
#include "util/u_double_list.h"

#include "nouveau_winsys.h"
#include "nouveau_screen.h"
#include "nouveau_mm.h"

#define MM_MIN_ORDER 7
#define MM_MAX_ORDER 20

#define MM_NUM_BUCKETS (MM_MAX_ORDER - MM_MIN_ORDER + 1)

#define MM_MIN_SIZE (1 << MM_MIN_ORDER)
#define MM_MAX_SIZE (1 << MM_MAX_ORDER)

struct mm_bucket {
   struct list_head free;
   struct list_head used;
   struct list_head full;
   int num_free;
};

struct nouveau_mman {
   struct nouveau_device *dev;
   struct mm_bucket bucket[MM_NUM_BUCKETS];
   uint32_t domain;
   union nouveau_bo_config config;
   uint64_t allocated;
};

struct mm_slab {
   struct list_head head;
   struct nouveau_bo *bo;
   struct nouveau_mman *cache;
   int order;
   int count;
   int free;
   uint32_t bits[0];
};

static int
mm_slab_alloc(struct mm_slab *slab)
{
   int i, n, b;

   if (slab->free == 0)
      return -1;

   for (i = 0; i < (slab->count + 31) / 32; ++i) {
      b = ffs(slab->bits[i]) - 1;
      if (b >= 0) {
         n = i * 32 + b;
         assert(n < slab->count);
         slab->free--;
         slab->bits[i] &= ~(1 << b);
         return n;
      }
   }
   return -1;
}

static INLINE void
mm_slab_free(struct mm_slab *slab, int i)
{
   assert(i < slab->count);
   slab->bits[i / 32] |= 1 << (i % 32);
   slab->free++;
   assert(slab->free <= slab->count);
}

static INLINE int
mm_get_order(uint32_t size)
{
   int s = __builtin_clz(size) ^ 31;

   if (size > (1 << s))
      s += 1;
   return s;
}

static struct mm_bucket *
mm_bucket_by_order(struct nouveau_mman *cache, int order)
{
   if (order > MM_MAX_ORDER)
      return NULL;
   return &cache->bucket[MAX2(order, MM_MIN_ORDER) - MM_MIN_ORDER];
}

static struct mm_bucket *
mm_bucket_by_size(struct nouveau_mman *cache, unsigned size)
{
   return mm_bucket_by_order(cache, mm_get_order(size));
}

/* size of bo allocation for slab with chunks of (1 << chunk_order) bytes */
static INLINE uint32_t
mm_default_slab_size(unsigned chunk_order)
{
   static const int8_t slab_order[MM_MAX_ORDER - MM_MIN_ORDER + 1] =
   {
      12, 12, 13, 14, 14, 17, 17, 17, 17, 19, 19, 20, 21, 22
   };

   assert(chunk_order <= MM_MAX_ORDER && chunk_order >= MM_MIN_ORDER);

   return 1 << slab_order[chunk_order - MM_MIN_ORDER];
}

static int
mm_slab_new(struct nouveau_mman *cache, int chunk_order)
{
   struct mm_slab *slab;
   int words, ret;
   const uint32_t size = mm_default_slab_size(chunk_order);

   words = ((size >> chunk_order) + 31) / 32;
   assert(words);

   slab = MALLOC(sizeof(struct mm_slab) + words * 4);
   if (!slab)
      return PIPE_ERROR_OUT_OF_MEMORY;

   memset(&slab->bits[0], ~0, words * 4);

   slab->bo = NULL;

   ret = nouveau_bo_new(cache->dev, cache->domain, 0, size, &cache->config,
                        &slab->bo);
   if (ret) {
      FREE(slab);
      return PIPE_ERROR_OUT_OF_MEMORY;
   }

   LIST_INITHEAD(&slab->head);

   slab->cache = cache;
   slab->order = chunk_order;
   slab->count = slab->free = size >> chunk_order;

   LIST_ADD(&slab->head, &mm_bucket_by_order(cache, chunk_order)->free);

   cache->allocated += size;

   if (nouveau_mesa_debug)
      debug_printf("MM: new slab, total memory = %"PRIu64" KiB\n",
                   cache->allocated / 1024);

   return PIPE_OK;
}

/* @return token to identify slab or NULL if we just allocated a new bo */
struct nouveau_mm_allocation *
nouveau_mm_allocate(struct nouveau_mman *cache,
                    uint32_t size, struct nouveau_bo **bo, uint32_t *offset)
{
   struct mm_bucket *bucket;
   struct mm_slab *slab;
   struct nouveau_mm_allocation *alloc;
   int ret;

   bucket = mm_bucket_by_size(cache, size);
   if (!bucket) {
      ret = nouveau_bo_new(cache->dev, cache->domain, 0, size, &cache->config,
                           bo);
      if (ret)
         debug_printf("bo_new(%x, %x): %i\n",
                      size, cache->config.nv50.memtype, ret);

      *offset = 0;
      return NULL;
   }

   if (!LIST_IS_EMPTY(&bucket->used)) {
      slab = LIST_ENTRY(struct mm_slab, bucket->used.next, head);
   } else {
      if (LIST_IS_EMPTY(&bucket->free)) {
         mm_slab_new(cache, MAX2(mm_get_order(size), MM_MIN_ORDER));
      }
      slab = LIST_ENTRY(struct mm_slab, bucket->free.next, head);

      LIST_DEL(&slab->head);
      LIST_ADD(&slab->head, &bucket->used);
   }

   *offset = mm_slab_alloc(slab) << slab->order;

   alloc = MALLOC_STRUCT(nouveau_mm_allocation);
   if (!alloc)
      return NULL;

   nouveau_bo_ref(slab->bo, bo);

   if (slab->free == 0) {
      LIST_DEL(&slab->head);
      LIST_ADD(&slab->head, &bucket->full);
   }

   alloc->next = NULL;
   alloc->offset = *offset;
   alloc->priv = (void *)slab;

   return alloc;
}

void
nouveau_mm_free(struct nouveau_mm_allocation *alloc)
{
   struct mm_slab *slab = (struct mm_slab *)alloc->priv;
   struct mm_bucket *bucket = mm_bucket_by_order(slab->cache, slab->order);

   mm_slab_free(slab, alloc->offset >> slab->order);

   if (slab->free == slab->count) {
      LIST_DEL(&slab->head);
      LIST_ADDTAIL(&slab->head, &bucket->free);
   } else
   if (slab->free == 1) {
      LIST_DEL(&slab->head);
      LIST_ADDTAIL(&slab->head, &bucket->used);
   }

   FREE(alloc);
}

void
nouveau_mm_free_work(void *data)
{
   nouveau_mm_free(data);
}

struct nouveau_mman *
nouveau_mm_create(struct nouveau_device *dev, uint32_t domain,
                  union nouveau_bo_config *config)
{
   struct nouveau_mman *cache = MALLOC_STRUCT(nouveau_mman);
   int i;

   if (!cache)
      return NULL;

   cache->dev = dev;
   cache->domain = domain;
   cache->config = *config;
   cache->allocated = 0;

   for (i = 0; i < MM_NUM_BUCKETS; ++i) {
      LIST_INITHEAD(&cache->bucket[i].free);
      LIST_INITHEAD(&cache->bucket[i].used);
      LIST_INITHEAD(&cache->bucket[i].full);
   }

   return cache;
}

static INLINE void
nouveau_mm_free_slabs(struct list_head *head)
{
   struct mm_slab *slab, *next;

   LIST_FOR_EACH_ENTRY_SAFE(slab, next, head, head) {
      LIST_DEL(&slab->head);
      nouveau_bo_ref(NULL, &slab->bo);
      FREE(slab);
   }
}

void
nouveau_mm_destroy(struct nouveau_mman *cache)
{
   int i;

   if (!cache)
      return;

   for (i = 0; i < MM_NUM_BUCKETS; ++i) {
      if (!LIST_IS_EMPTY(&cache->bucket[i].used) ||
          !LIST_IS_EMPTY(&cache->bucket[i].full))
         debug_printf("WARNING: destroying GPU memory cache "
                      "with some buffers still in use\n");

      nouveau_mm_free_slabs(&cache->bucket[i].free);
      nouveau_mm_free_slabs(&cache->bucket[i].used);
      nouveau_mm_free_slabs(&cache->bucket[i].full);
   }

   FREE(cache);
}

