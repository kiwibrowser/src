
#include "util/u_inlines.h"
#include "util/u_memory.h"
#include "util/u_math.h"

#include "nouveau_screen.h"
#include "nouveau_context.h"
#include "nouveau_winsys.h"
#include "nouveau_fence.h"
#include "nouveau_buffer.h"
#include "nouveau_mm.h"

struct nouveau_transfer {
   struct pipe_transfer base;
};

static INLINE struct nouveau_transfer *
nouveau_transfer(struct pipe_transfer *transfer)
{
   return (struct nouveau_transfer *)transfer;
}

static INLINE boolean
nouveau_buffer_allocate(struct nouveau_screen *screen,
                        struct nv04_resource *buf, unsigned domain)
{
   uint32_t size = buf->base.width0;

   if (buf->base.bind & PIPE_BIND_CONSTANT_BUFFER)
      size = align(size, 0x100);

   if (domain == NOUVEAU_BO_VRAM) {
      buf->mm = nouveau_mm_allocate(screen->mm_VRAM, size,
                                    &buf->bo, &buf->offset);
      if (!buf->bo)
         return nouveau_buffer_allocate(screen, buf, NOUVEAU_BO_GART);
   } else
   if (domain == NOUVEAU_BO_GART) {
      buf->mm = nouveau_mm_allocate(screen->mm_GART, size,
                                    &buf->bo, &buf->offset);
      if (!buf->bo)
         return FALSE;
   }
   if (domain != NOUVEAU_BO_GART) {
      if (!buf->data) {
         buf->data = MALLOC(buf->base.width0);
         if (!buf->data)
            return FALSE;
      }
   }
   buf->domain = domain;
   if (buf->bo)
      buf->address = buf->bo->offset + buf->offset;

   return TRUE;
}

static INLINE void
release_allocation(struct nouveau_mm_allocation **mm,
                   struct nouveau_fence *fence)
{
   nouveau_fence_work(fence, nouveau_mm_free_work, *mm);
   (*mm) = NULL;
}

INLINE void
nouveau_buffer_release_gpu_storage(struct nv04_resource *buf)
{
   nouveau_bo_ref(NULL, &buf->bo);

   if (buf->mm)
      release_allocation(&buf->mm, buf->fence);

   buf->domain = 0;
}

static INLINE boolean
nouveau_buffer_reallocate(struct nouveau_screen *screen,
                          struct nv04_resource *buf, unsigned domain)
{
   nouveau_buffer_release_gpu_storage(buf);

   return nouveau_buffer_allocate(screen, buf, domain);
}

static void
nouveau_buffer_destroy(struct pipe_screen *pscreen,
                       struct pipe_resource *presource)
{
   struct nv04_resource *res = nv04_resource(presource);

   nouveau_buffer_release_gpu_storage(res);

   if (res->data && !(res->status & NOUVEAU_BUFFER_STATUS_USER_MEMORY))
      FREE(res->data);

   nouveau_fence_ref(NULL, &res->fence);
   nouveau_fence_ref(NULL, &res->fence_wr);

   FREE(res);
}

/* Maybe just migrate to GART right away if we actually need to do this. */
boolean
nouveau_buffer_download(struct nouveau_context *nv, struct nv04_resource *buf,
                        unsigned start, unsigned size)
{
   struct nouveau_mm_allocation *mm;
   struct nouveau_bo *bounce = NULL;
   uint32_t offset;

   assert(buf->domain == NOUVEAU_BO_VRAM);

   mm = nouveau_mm_allocate(nv->screen->mm_GART, size, &bounce, &offset);
   if (!bounce)
      return FALSE;

   nv->copy_data(nv, bounce, offset, NOUVEAU_BO_GART,
                 buf->bo, buf->offset + start, NOUVEAU_BO_VRAM, size);

   if (nouveau_bo_map(bounce, NOUVEAU_BO_RD, nv->screen->client))
      return FALSE;
   memcpy(buf->data + start, (uint8_t *)bounce->map + offset, size);

   buf->status &= ~NOUVEAU_BUFFER_STATUS_GPU_WRITING;

   nouveau_bo_ref(NULL, &bounce);
   if (mm)
      nouveau_mm_free(mm);
   return TRUE;
}

static boolean
nouveau_buffer_upload(struct nouveau_context *nv, struct nv04_resource *buf,
                      unsigned start, unsigned size)
{
   struct nouveau_mm_allocation *mm;
   struct nouveau_bo *bounce = NULL;
   uint32_t offset;

   if (size <= 192 && (nv->push_data || nv->push_cb)) {
      if (buf->base.bind & PIPE_BIND_CONSTANT_BUFFER)
         nv->push_cb(nv, buf->bo, buf->domain, buf->offset, buf->base.width0,
                     start, size / 4, (const uint32_t *)(buf->data + start));
      else
         nv->push_data(nv, buf->bo, buf->offset + start, buf->domain,
                       size, buf->data + start);
      return TRUE;
   }

   mm = nouveau_mm_allocate(nv->screen->mm_GART, size, &bounce, &offset);
   if (!bounce)
      return FALSE;

   nouveau_bo_map(bounce, 0, nv->screen->client);
   memcpy((uint8_t *)bounce->map + offset, buf->data + start, size);

   nv->copy_data(nv, buf->bo, buf->offset + start, NOUVEAU_BO_VRAM,
                 bounce, offset, NOUVEAU_BO_GART, size);

   nouveau_bo_ref(NULL, &bounce);
   if (mm)
      release_allocation(&mm, nv->screen->fence.current);

   if (start == 0 && size == buf->base.width0)
      buf->status &= ~NOUVEAU_BUFFER_STATUS_GPU_WRITING;
   return TRUE;
}

static struct pipe_transfer *
nouveau_buffer_transfer_get(struct pipe_context *pipe,
                            struct pipe_resource *resource,
                            unsigned level, unsigned usage,
                            const struct pipe_box *box)
{
   struct nv04_resource *buf = nv04_resource(resource);
   struct nouveau_context *nv = nouveau_context(pipe);
   struct nouveau_transfer *xfr = CALLOC_STRUCT(nouveau_transfer);
   if (!xfr)
      return NULL;

   xfr->base.resource = resource;
   xfr->base.box.x = box->x;
   xfr->base.box.width = box->width;
   xfr->base.usage = usage;

   if (buf->domain == NOUVEAU_BO_VRAM) {
      if (usage & PIPE_TRANSFER_READ) {
         if (buf->status & NOUVEAU_BUFFER_STATUS_GPU_WRITING)
            nouveau_buffer_download(nv, buf, 0, buf->base.width0);
      }
   }

   return &xfr->base;
}

static void
nouveau_buffer_transfer_destroy(struct pipe_context *pipe,
                                struct pipe_transfer *transfer)
{
   struct nv04_resource *buf = nv04_resource(transfer->resource);
   struct nouveau_transfer *xfr = nouveau_transfer(transfer);
   struct nouveau_context *nv = nouveau_context(pipe);

   if (xfr->base.usage & PIPE_TRANSFER_WRITE) {
      if (buf->domain == NOUVEAU_BO_VRAM) {
         nouveau_buffer_upload(nv, buf, transfer->box.x, transfer->box.width);
      }

      if (buf->domain != 0 && (buf->base.bind & (PIPE_BIND_VERTEX_BUFFER |
                                                 PIPE_BIND_INDEX_BUFFER)))
         nouveau_context(pipe)->vbo_dirty = TRUE;
   }

   FREE(xfr);
}

static INLINE boolean
nouveau_buffer_sync(struct nv04_resource *buf, unsigned rw)
{
   if (rw == PIPE_TRANSFER_READ) {
      if (!buf->fence_wr)
         return TRUE;
      if (!nouveau_fence_wait(buf->fence_wr))
         return FALSE;
   } else {
      if (!buf->fence)
         return TRUE;
      if (!nouveau_fence_wait(buf->fence))
         return FALSE;

      nouveau_fence_ref(NULL, &buf->fence);
   }
   nouveau_fence_ref(NULL, &buf->fence_wr);

   return TRUE;
}

static INLINE boolean
nouveau_buffer_busy(struct nv04_resource *buf, unsigned rw)
{
   if (rw == PIPE_TRANSFER_READ)
      return (buf->fence_wr && !nouveau_fence_signalled(buf->fence_wr));
   else
      return (buf->fence && !nouveau_fence_signalled(buf->fence));
}

static void *
nouveau_buffer_transfer_map(struct pipe_context *pipe,
                            struct pipe_transfer *transfer)
{
   struct nouveau_context *nv = nouveau_context(pipe);
   struct nouveau_transfer *xfr = nouveau_transfer(transfer);
   struct nv04_resource *buf = nv04_resource(transfer->resource);
   struct nouveau_bo *bo = buf->bo;
   uint8_t *map;
   int ret;
   uint32_t offset = xfr->base.box.x;
   uint32_t flags = 0;

   if (buf->domain != NOUVEAU_BO_GART)
      return buf->data + offset;

   if (!buf->mm)
      flags = nouveau_screen_transfer_flags(xfr->base.usage);

   offset += buf->offset;

   ret = nouveau_bo_map(buf->bo, flags, nv->screen->client);
   if (ret)
      return NULL;
   map = (uint8_t *)bo->map + offset;

   if (buf->mm) {
      if (xfr->base.usage & PIPE_TRANSFER_DONTBLOCK) {
         if (nouveau_buffer_busy(buf, xfr->base.usage & PIPE_TRANSFER_READ_WRITE))
            return NULL;
      } else
      if (!(xfr->base.usage & PIPE_TRANSFER_UNSYNCHRONIZED)) {
         nouveau_buffer_sync(buf, xfr->base.usage & PIPE_TRANSFER_READ_WRITE);
      }
   }
   return map;
}



static void
nouveau_buffer_transfer_flush_region(struct pipe_context *pipe,
                                     struct pipe_transfer *transfer,
                                     const struct pipe_box *box)
{
#if 0
   struct nv04_resource *res = nv04_resource(transfer->resource);
   struct nouveau_bo *bo = res->bo;
   unsigned offset = res->offset + transfer->box.x + box->x;

   /* not using non-snoop system memory yet, no need for cflush */
   if (1)
      return;

   /* XXX: maybe need to upload for VRAM buffers here */
#endif
}

static void
nouveau_buffer_transfer_unmap(struct pipe_context *pipe,
                              struct pipe_transfer *transfer)
{
}


void *
nouveau_resource_map_offset(struct nouveau_context *nv,
                            struct nv04_resource *res, uint32_t offset,
                            uint32_t flags)
{
   if ((res->domain == NOUVEAU_BO_VRAM) &&
       (res->status & NOUVEAU_BUFFER_STATUS_GPU_WRITING))
      nouveau_buffer_download(nv, res, 0, res->base.width0);

   if ((res->domain != NOUVEAU_BO_GART) ||
       (res->status & NOUVEAU_BUFFER_STATUS_USER_MEMORY))
      return res->data + offset;

   if (res->mm) {
      unsigned rw;
      rw = (flags & NOUVEAU_BO_WR) ? PIPE_TRANSFER_WRITE : PIPE_TRANSFER_READ;
      nouveau_buffer_sync(res, rw);
      if (nouveau_bo_map(res->bo, 0, NULL))
         return NULL;
   } else {
      if (nouveau_bo_map(res->bo, flags, nv->screen->client))
         return NULL;
   }
   return (uint8_t *)res->bo->map + res->offset + offset;
}


const struct u_resource_vtbl nouveau_buffer_vtbl =
{
   u_default_resource_get_handle,     /* get_handle */
   nouveau_buffer_destroy,               /* resource_destroy */
   nouveau_buffer_transfer_get,          /* get_transfer */
   nouveau_buffer_transfer_destroy,      /* transfer_destroy */
   nouveau_buffer_transfer_map,          /* transfer_map */
   nouveau_buffer_transfer_flush_region, /* transfer_flush_region */
   nouveau_buffer_transfer_unmap,        /* transfer_unmap */
   u_default_transfer_inline_write    /* transfer_inline_write */
};

struct pipe_resource *
nouveau_buffer_create(struct pipe_screen *pscreen,
                      const struct pipe_resource *templ)
{
   struct nouveau_screen *screen = nouveau_screen(pscreen);
   struct nv04_resource *buffer;
   boolean ret;

   buffer = CALLOC_STRUCT(nv04_resource);
   if (!buffer)
      return NULL;

   buffer->base = *templ;
   buffer->vtbl = &nouveau_buffer_vtbl;
   pipe_reference_init(&buffer->base.reference, 1);
   buffer->base.screen = pscreen;

   if (buffer->base.bind &
       (screen->vidmem_bindings & screen->sysmem_bindings)) {
      switch (buffer->base.usage) {
      case PIPE_USAGE_DEFAULT:
      case PIPE_USAGE_IMMUTABLE:
      case PIPE_USAGE_STATIC:
         buffer->domain = NOUVEAU_BO_VRAM;
         break;
      case PIPE_USAGE_DYNAMIC:
      case PIPE_USAGE_STAGING:
      case PIPE_USAGE_STREAM:
         buffer->domain = NOUVEAU_BO_GART;
         break;
      default:
         assert(0);
         break;
      }
   } else {
      if (buffer->base.bind & screen->vidmem_bindings)
         buffer->domain = NOUVEAU_BO_VRAM;
      else
      if (buffer->base.bind & screen->sysmem_bindings)
         buffer->domain = NOUVEAU_BO_GART;
   }
   ret = nouveau_buffer_allocate(screen, buffer, buffer->domain);

   if (ret == FALSE)
      goto fail;

   return &buffer->base;

fail:
   FREE(buffer);
   return NULL;
}


struct pipe_resource *
nouveau_user_buffer_create(struct pipe_screen *pscreen, void *ptr,
                           unsigned bytes, unsigned bind)
{
   struct nv04_resource *buffer;

   buffer = CALLOC_STRUCT(nv04_resource);
   if (!buffer)
      return NULL;

   pipe_reference_init(&buffer->base.reference, 1);
   buffer->vtbl = &nouveau_buffer_vtbl;
   buffer->base.screen = pscreen;
   buffer->base.format = PIPE_FORMAT_R8_UNORM;
   buffer->base.usage = PIPE_USAGE_IMMUTABLE;
   buffer->base.bind = bind;
   buffer->base.width0 = bytes;
   buffer->base.height0 = 1;
   buffer->base.depth0 = 1;

   buffer->data = ptr;
   buffer->status = NOUVEAU_BUFFER_STATUS_USER_MEMORY;

   return &buffer->base;
}

/* Like download, but for GART buffers. Merge ? */
static INLINE boolean
nouveau_buffer_data_fetch(struct nouveau_context *nv, struct nv04_resource *buf,
                          struct nouveau_bo *bo, unsigned offset, unsigned size)
{
   if (!buf->data) {
      buf->data = MALLOC(size);
      if (!buf->data)
         return FALSE;
   }
   if (nouveau_bo_map(bo, NOUVEAU_BO_RD, nv->screen->client))
      return FALSE;
   memcpy(buf->data, (uint8_t *)bo->map + offset, size);

   return TRUE;
}

/* Migrate a linear buffer (vertex, index, constants) USER -> GART -> VRAM. */
boolean
nouveau_buffer_migrate(struct nouveau_context *nv,
                       struct nv04_resource *buf, const unsigned new_domain)
{
   struct nouveau_screen *screen = nv->screen;
   struct nouveau_bo *bo;
   const unsigned old_domain = buf->domain;
   unsigned size = buf->base.width0;
   unsigned offset;
   int ret;

   assert(new_domain != old_domain);

   if (new_domain == NOUVEAU_BO_GART && old_domain == 0) {
      if (!nouveau_buffer_allocate(screen, buf, new_domain))
         return FALSE;
      ret = nouveau_bo_map(buf->bo, 0, nv->screen->client);
      if (ret)
         return ret;
      memcpy((uint8_t *)buf->bo->map + buf->offset, buf->data, size);
      FREE(buf->data);
   } else
   if (old_domain != 0 && new_domain != 0) {
      struct nouveau_mm_allocation *mm = buf->mm;

      if (new_domain == NOUVEAU_BO_VRAM) {
         /* keep a system memory copy of our data in case we hit a fallback */
         if (!nouveau_buffer_data_fetch(nv, buf, buf->bo, buf->offset, size))
            return FALSE;
         if (nouveau_mesa_debug)
            debug_printf("migrating %u KiB to VRAM\n", size / 1024);
      }

      offset = buf->offset;
      bo = buf->bo;
      buf->bo = NULL;
      buf->mm = NULL;
      nouveau_buffer_allocate(screen, buf, new_domain);

      nv->copy_data(nv, buf->bo, buf->offset, new_domain,
                    bo, offset, old_domain, buf->base.width0);

      nouveau_bo_ref(NULL, &bo);
      if (mm)
         release_allocation(&mm, screen->fence.current);
   } else
   if (new_domain == NOUVEAU_BO_VRAM && old_domain == 0) {
      if (!nouveau_buffer_allocate(screen, buf, NOUVEAU_BO_VRAM))
         return FALSE;
      if (!nouveau_buffer_upload(nv, buf, 0, buf->base.width0))
         return FALSE;
   } else
      return FALSE;

   assert(buf->domain == new_domain);
   return TRUE;
}

/* Migrate data from glVertexAttribPointer(non-VBO) user buffers to GART.
 * We'd like to only allocate @size bytes here, but then we'd have to rebase
 * the vertex indices ...
 */
boolean
nouveau_user_buffer_upload(struct nouveau_context *nv,
                           struct nv04_resource *buf,
                           unsigned base, unsigned size)
{
   struct nouveau_screen *screen = nouveau_screen(buf->base.screen);
   int ret;

   assert(buf->status & NOUVEAU_BUFFER_STATUS_USER_MEMORY);

   buf->base.width0 = base + size;
   if (!nouveau_buffer_reallocate(screen, buf, NOUVEAU_BO_GART))
      return FALSE;

   ret = nouveau_bo_map(buf->bo, 0, nv->screen->client);
   if (ret)
      return FALSE;
   memcpy((uint8_t *)buf->bo->map + buf->offset + base, buf->data + base, size);

   return TRUE;
}


/* Scratch data allocation. */

static INLINE int
nouveau_scratch_bo_alloc(struct nouveau_context *nv, struct nouveau_bo **pbo,
                         unsigned size)
{
   return nouveau_bo_new(nv->screen->device, NOUVEAU_BO_GART | NOUVEAU_BO_MAP,
                         4096, size, NULL, pbo);
}

void
nouveau_scratch_runout_release(struct nouveau_context *nv)
{
   if (!nv->scratch.nr_runout)
      return;
   do {
      --nv->scratch.nr_runout;
      nouveau_bo_ref(NULL, &nv->scratch.runout[nv->scratch.nr_runout]);
   } while (nv->scratch.nr_runout);

   FREE(nv->scratch.runout);
   nv->scratch.end = 0;
   nv->scratch.runout = NULL;
}

/* Allocate an extra bo if we can't fit everything we need simultaneously.
 * (Could happen for very large user arrays.)
 */
static INLINE boolean
nouveau_scratch_runout(struct nouveau_context *nv, unsigned size)
{
   int ret;
   const unsigned n = nv->scratch.nr_runout++;

   nv->scratch.runout = REALLOC(nv->scratch.runout,
                                (n + 0) * sizeof(*nv->scratch.runout),
                                (n + 1) * sizeof(*nv->scratch.runout));
   nv->scratch.runout[n] = NULL;

   ret = nouveau_scratch_bo_alloc(nv, &nv->scratch.runout[n], size);
   if (!ret) {
      ret = nouveau_bo_map(nv->scratch.runout[n], 0, NULL);
      if (ret)
         nouveau_bo_ref(NULL, &nv->scratch.runout[--nv->scratch.nr_runout]);
   }
   if (!ret) {
      nv->scratch.current = nv->scratch.runout[n];
      nv->scratch.offset = 0;
      nv->scratch.end = size;
      nv->scratch.map = nv->scratch.current->map;
   }
   return !ret;
}

/* Continue to next scratch buffer, if available (no wrapping, large enough).
 * Allocate it if it has not yet been created.
 */
static INLINE boolean
nouveau_scratch_next(struct nouveau_context *nv, unsigned size)
{
   struct nouveau_bo *bo;
   int ret;
   const unsigned i = (nv->scratch.id + 1) % NOUVEAU_MAX_SCRATCH_BUFS;

   if ((size > nv->scratch.bo_size) || (i == nv->scratch.wrap))
      return FALSE;
   nv->scratch.id = i;

   bo = nv->scratch.bo[i];
   if (!bo) {
      ret = nouveau_scratch_bo_alloc(nv, &bo, nv->scratch.bo_size);
      if (ret)
         return FALSE;
      nv->scratch.bo[i] = bo;
   }
   nv->scratch.current = bo;
   nv->scratch.offset = 0;
   nv->scratch.end = nv->scratch.bo_size;

   ret = nouveau_bo_map(bo, NOUVEAU_BO_WR, nv->screen->client);
   if (!ret)
      nv->scratch.map = bo->map;
   return !ret;
}

static boolean
nouveau_scratch_more(struct nouveau_context *nv, unsigned min_size)
{
   boolean ret;

   ret = nouveau_scratch_next(nv, min_size);
   if (!ret)
      ret = nouveau_scratch_runout(nv, min_size);
   return ret;
}


/* Copy data to a scratch buffer and return address & bo the data resides in. */
uint64_t
nouveau_scratch_data(struct nouveau_context *nv,
                     const void *data, unsigned base, unsigned size,
                     struct nouveau_bo **bo)
{
   unsigned bgn = MAX2(base, nv->scratch.offset);
   unsigned end = bgn + size;

   if (end >= nv->scratch.end) {
      end = base + size;
      if (!nouveau_scratch_more(nv, end))
         return 0;
      bgn = base;
   }
   nv->scratch.offset = align(end, 4);

   memcpy(nv->scratch.map + bgn, (const uint8_t *)data + base, size);

   *bo = nv->scratch.current;
   return (*bo)->offset + (bgn - base);
}

void *
nouveau_scratch_get(struct nouveau_context *nv,
                    unsigned size, uint64_t *gpu_addr, struct nouveau_bo **pbo)
{
   unsigned bgn = nv->scratch.offset;
   unsigned end = nv->scratch.offset + size;

   if (end >= nv->scratch.end) {
      end = size;
      if (!nouveau_scratch_more(nv, end))
         return NULL;
      bgn = 0;
   }
   nv->scratch.offset = align(end, 4);

   *pbo = nv->scratch.current;
   *gpu_addr = nv->scratch.current->offset + bgn;
   return nv->scratch.map + bgn;
}
