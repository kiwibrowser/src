/**********************************************************
 * Copyright 2008-2009 VMware, Inc.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 **********************************************************/

#include "util/u_math.h"
#include "util/u_memory.h"
#include "util/u_hash.h"

#include "svga_debug.h"
#include "svga_format.h"
#include "svga_winsys.h"
#include "svga_screen.h"
#include "svga_screen_cache.h"


#define SVGA_SURFACE_CACHE_ENABLED 1


/**
 * Return the size of the surface described by the key (in bytes).
 */
static unsigned
surface_size(const struct svga_host_surface_cache_key *key)
{
   unsigned bw, bh, bpb, total_size, i;

   assert(key->numMipLevels > 0);
   assert(key->numFaces > 0);

   if (key->format == SVGA3D_BUFFER) {
      /* Special case: we don't want to count vertex/index buffers
       * against the cache size limit, so view them as zero-sized.
       */
      return 0;
   }

   svga_format_size(key->format, &bw, &bh, &bpb);

   total_size = 0;

   for (i = 0; i < key->numMipLevels; i++) {
      unsigned w = u_minify(key->size.width, i);
      unsigned h = u_minify(key->size.height, i);
      unsigned d = u_minify(key->size.depth, i);
      unsigned img_size = ((w + bw - 1) / bw) * ((h + bh - 1) / bh) * d * bpb;
      total_size += img_size;
   }

   total_size *= key->numFaces;

   return total_size;
}


/**
 * Compute the bucket for this key.
 */
static INLINE unsigned
svga_screen_cache_bucket(const struct svga_host_surface_cache_key *key)
{
   return util_hash_crc32(key, sizeof *key) % SVGA_HOST_SURFACE_CACHE_BUCKETS;
}


/**
 * Search the cache for a surface that matches the key.  If a match is
 * found, remove it from the cache and return the surface pointer.
 * Return NULL otherwise.
 */
static INLINE struct svga_winsys_surface *
svga_screen_cache_lookup(struct svga_screen *svgascreen,
                         const struct svga_host_surface_cache_key *key)
{
   struct svga_host_surface_cache *cache = &svgascreen->cache;
   struct svga_winsys_screen *sws = svgascreen->sws;
   struct svga_host_surface_cache_entry *entry;
   struct svga_winsys_surface *handle = NULL;
   struct list_head *curr, *next;
   unsigned bucket;
   unsigned tries = 0;

   assert(key->cachable);

   bucket = svga_screen_cache_bucket(key);

   pipe_mutex_lock(cache->mutex);

   curr = cache->bucket[bucket].next;
   next = curr->next;
   while (curr != &cache->bucket[bucket]) {
      ++tries;

      entry = LIST_ENTRY(struct svga_host_surface_cache_entry, curr, bucket_head);

      assert(entry->handle);

      if (memcmp(&entry->key, key, sizeof *key) == 0 &&
         sws->fence_signalled(sws, entry->fence, 0) == 0) {
         unsigned surf_size;

         assert(sws->surface_is_flushed(sws, entry->handle));

         handle = entry->handle; /* Reference is transfered here. */
         entry->handle = NULL;

         LIST_DEL(&entry->bucket_head);

         LIST_DEL(&entry->head);

         LIST_ADD(&entry->head, &cache->empty);

         /* update the cache size */
         surf_size = surface_size(&entry->key);
         assert(surf_size <= cache->total_size);
         if (surf_size > cache->total_size)
            cache->total_size = 0; /* should never happen, but be safe */
         else
            cache->total_size -= surf_size;

         break;
      }

      curr = next;
      next = curr->next;
   }

   pipe_mutex_unlock(cache->mutex);

   if (SVGA_DEBUG & DEBUG_DMA)
      debug_printf("%s: cache %s after %u tries (bucket %d)\n", __FUNCTION__,
                   handle ? "hit" : "miss", tries, bucket);

   return handle;
}


/**
 * Free the least recently used entries in the surface cache until the
 * cache size is <= the target size OR there are no unused entries left
 * to discard.  We don't do any flushing to try to free up additional
 * surfaces.
 */
static void
svga_screen_cache_shrink(struct svga_screen *svgascreen,
                         unsigned target_size)
{
   struct svga_host_surface_cache *cache = &svgascreen->cache;
   struct svga_winsys_screen *sws = svgascreen->sws;
   struct svga_host_surface_cache_entry *entry = NULL, *next_entry;

   /* Walk over the list of unused buffers in reverse order: from oldest
    * to newest.
    */
   LIST_FOR_EACH_ENTRY_SAFE_REV(entry, next_entry, &cache->unused, head) {
      if (entry->key.format != SVGA3D_BUFFER) {
         /* we don't want to discard vertex/index buffers */

         cache->total_size -= surface_size(&entry->key);

         assert(entry->handle);
         sws->surface_reference(sws, &entry->handle, NULL);

         LIST_DEL(&entry->bucket_head);
         LIST_DEL(&entry->head);
         LIST_ADD(&entry->head, &cache->empty);

         if (cache->total_size <= target_size) {
            /* all done */
            break;
         }
      }
   }
}


/**
 * Transfers a handle reference.
 */
static INLINE void
svga_screen_cache_add(struct svga_screen *svgascreen,
                      const struct svga_host_surface_cache_key *key,
                      struct svga_winsys_surface **p_handle)
{
   struct svga_host_surface_cache *cache = &svgascreen->cache;
   struct svga_winsys_screen *sws = svgascreen->sws;
   struct svga_host_surface_cache_entry *entry = NULL;
   struct svga_winsys_surface *handle = *p_handle;
   unsigned surf_size;
   
   assert(key->cachable);

   assert(handle);
   if (!handle)
      return;
   
   surf_size = surface_size(key);

   *p_handle = NULL;
   pipe_mutex_lock(cache->mutex);
   
   if (surf_size >= SVGA_HOST_SURFACE_CACHE_BYTES) {
      /* this surface is too large to cache, just free it */
      sws->surface_reference(sws, &handle, NULL);
      pipe_mutex_unlock(cache->mutex);
      return;
   }

   if (cache->total_size + surf_size > SVGA_HOST_SURFACE_CACHE_BYTES) {
      /* Adding this surface would exceed the cache size.
       * Try to discard least recently used entries until we hit the
       * new target cache size.
       */
      unsigned target_size = SVGA_HOST_SURFACE_CACHE_BYTES - surf_size;

      svga_screen_cache_shrink(svgascreen, target_size);

      if (cache->total_size > target_size) {
         /* we weren't able to shrink the cache as much as we wanted so
          * just discard this surface.
          */
         sws->surface_reference(sws, &handle, NULL);
         pipe_mutex_unlock(cache->mutex);
         return;
      }
   }

   if (!LIST_IS_EMPTY(&cache->empty)) {
      /* use the first empty entry */
      entry = LIST_ENTRY(struct svga_host_surface_cache_entry,
                         cache->empty.next, head);

      LIST_DEL(&entry->head);
   }
   else if (!LIST_IS_EMPTY(&cache->unused)) {
      /* free the last used buffer and reuse its entry */
      entry = LIST_ENTRY(struct svga_host_surface_cache_entry,
                         cache->unused.prev, head);
      SVGA_DBG(DEBUG_CACHE|DEBUG_DMA,
               "unref sid %p (make space)\n", entry->handle);

      cache->total_size -= surface_size(&entry->key);

      sws->surface_reference(sws, &entry->handle, NULL);

      LIST_DEL(&entry->bucket_head);

      LIST_DEL(&entry->head);
   }

   if (entry) {
      entry->handle = handle;
      memcpy(&entry->key, key, sizeof entry->key);

      SVGA_DBG(DEBUG_CACHE|DEBUG_DMA,
               "cache sid %p\n", entry->handle);
      LIST_ADD(&entry->head, &cache->validated);

      cache->total_size += surf_size;
   }
   else {
      /* Couldn't cache the buffer -- this really shouldn't happen */
      SVGA_DBG(DEBUG_CACHE|DEBUG_DMA,
               "unref sid %p (couldn't find space)\n", handle);
      sws->surface_reference(sws, &handle, NULL);
   }

   pipe_mutex_unlock(cache->mutex);
}


/**
 * Called during the screen flush to move all buffers not in a validate list
 * into the unused list.
 */
void
svga_screen_cache_flush(struct svga_screen *svgascreen,
                        struct pipe_fence_handle *fence)
{
   struct svga_host_surface_cache *cache = &svgascreen->cache;
   struct svga_winsys_screen *sws = svgascreen->sws;
   struct svga_host_surface_cache_entry *entry;
   struct list_head *curr, *next;
   unsigned bucket;

   pipe_mutex_lock(cache->mutex);

   curr = cache->validated.next;
   next = curr->next;
   while (curr != &cache->validated) {
      entry = LIST_ENTRY(struct svga_host_surface_cache_entry, curr, head);

      assert(entry->handle);

      if (sws->surface_is_flushed(sws, entry->handle)) {
         LIST_DEL(&entry->head);

         svgascreen->sws->fence_reference(svgascreen->sws, &entry->fence, fence);

         LIST_ADD(&entry->head, &cache->unused);

         bucket = svga_screen_cache_bucket(&entry->key);
         LIST_ADD(&entry->bucket_head, &cache->bucket[bucket]);
      }

      curr = next;
      next = curr->next;
   }

   pipe_mutex_unlock(cache->mutex);
}


/**
 * Free all the surfaces in the cache.
 * Called when destroying the svga screen object.
 */
void
svga_screen_cache_cleanup(struct svga_screen *svgascreen)
{
   struct svga_host_surface_cache *cache = &svgascreen->cache;
   struct svga_winsys_screen *sws = svgascreen->sws;
   unsigned i;

   for (i = 0; i < SVGA_HOST_SURFACE_CACHE_SIZE; ++i) {
      if (cache->entries[i].handle) {
	 SVGA_DBG(DEBUG_CACHE|DEBUG_DMA,
                  "unref sid %p (shutdown)\n", cache->entries[i].handle);
	 sws->surface_reference(sws, &cache->entries[i].handle, NULL);

         cache->total_size -= surface_size(&cache->entries[i].key);
      }

      if (cache->entries[i].fence)
         svgascreen->sws->fence_reference(svgascreen->sws,
                                          &cache->entries[i].fence, NULL);
   }

   pipe_mutex_destroy(cache->mutex);
}


enum pipe_error
svga_screen_cache_init(struct svga_screen *svgascreen)
{
   struct svga_host_surface_cache *cache = &svgascreen->cache;
   unsigned i;

   assert(cache->total_size == 0);

   pipe_mutex_init(cache->mutex);

   for (i = 0; i < SVGA_HOST_SURFACE_CACHE_BUCKETS; ++i)
      LIST_INITHEAD(&cache->bucket[i]);

   LIST_INITHEAD(&cache->unused);

   LIST_INITHEAD(&cache->validated);

   LIST_INITHEAD(&cache->empty);
   for (i = 0; i < SVGA_HOST_SURFACE_CACHE_SIZE; ++i)
      LIST_ADDTAIL(&cache->entries[i].head, &cache->empty);

   return PIPE_OK;
}


/**
 * Allocate a new host-side surface.  If the surface is marked as cachable,
 * first try re-using a surface in the cache of freed surfaces.  Otherwise,
 * allocate a new surface.
 */
struct svga_winsys_surface *
svga_screen_surface_create(struct svga_screen *svgascreen,
                           struct svga_host_surface_cache_key *key)
{
   struct svga_winsys_screen *sws = svgascreen->sws;
   struct svga_winsys_surface *handle = NULL;
   boolean cachable = SVGA_SURFACE_CACHE_ENABLED && key->cachable;

   SVGA_DBG(DEBUG_CACHE|DEBUG_DMA,
            "%s sz %dx%dx%d mips %d faces %d cachable %d\n",
            __FUNCTION__,
            key->size.width,
            key->size.height,
            key->size.depth,
            key->numMipLevels,
            key->numFaces,
            key->cachable);

   if (cachable) {
      if (key->format == SVGA3D_BUFFER) {
         /* For buffers, round the buffer size up to the nearest power
          * of two to increase the probability of cache hits.  Keep
          * texture surface dimensions unchanged.
          */
         uint32_t size = 1;
         while (size < key->size.width)
            size <<= 1;
         key->size.width = size;
	 /* Since we're reusing buffers we're effectively transforming all
	  * of them into dynamic buffers.
	  *
	  * It would be nice to not cache long lived static buffers. But there
	  * is no way to detect the long lived from short lived ones yet. A
	  * good heuristic would be buffer size.
	  */
	 key->flags &= ~SVGA3D_SURFACE_HINT_STATIC;
	 key->flags |= SVGA3D_SURFACE_HINT_DYNAMIC;
      }

      handle = svga_screen_cache_lookup(svgascreen, key);
      if (handle) {
         if (key->format == SVGA3D_BUFFER)
            SVGA_DBG(DEBUG_CACHE|DEBUG_DMA,
                     "reuse sid %p sz %d (buffer)\n", handle,
                     key->size.width);
         else
            SVGA_DBG(DEBUG_CACHE|DEBUG_DMA,
                     "reuse sid %p sz %dx%dx%d mips %d faces %d\n", handle,
                     key->size.width,
                     key->size.height,
                     key->size.depth,
                     key->numMipLevels,
                     key->numFaces);
      }
   }

   if (!handle) {
      handle = sws->surface_create(sws,
                                   key->flags,
                                   key->format,
                                   key->size,
                                   key->numFaces,
                                   key->numMipLevels);
      if (handle)
         SVGA_DBG(DEBUG_CACHE|DEBUG_DMA,
                  "  CREATE sid %p sz %dx%dx%d\n",
                  handle,
                  key->size.width,
                  key->size.height,
                  key->size.depth);
   }

   return handle;
}


/**
 * Release a surface.  We don't actually free the surface- we put
 * it into the cache of freed surfaces (if it's cachable).
 */
void
svga_screen_surface_destroy(struct svga_screen *svgascreen,
                            const struct svga_host_surface_cache_key *key,
                            struct svga_winsys_surface **p_handle)
{
   struct svga_winsys_screen *sws = svgascreen->sws;

   /* We only set the cachable flag for surfaces of which we are the
    * exclusive owner.  So just hold onto our existing reference in
    * that case.
    */
   if (SVGA_SURFACE_CACHE_ENABLED && key->cachable) {
      svga_screen_cache_add(svgascreen, key, p_handle);
   }
   else {
      SVGA_DBG(DEBUG_DMA,
               "unref sid %p (uncachable)\n", *p_handle);
      sws->surface_reference(sws, p_handle, NULL);
   }
}
