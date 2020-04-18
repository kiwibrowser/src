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


#include "os/os_thread.h"
#include "pipe/p_state.h"
#include "pipe/p_defines.h"
#include "util/u_inlines.h"
#include "util/u_math.h"
#include "util/u_memory.h"

#include "svga_cmd.h"
#include "svga_context.h"
#include "svga_debug.h"
#include "svga_resource_buffer.h"
#include "svga_resource_buffer_upload.h"
#include "svga_screen.h"
#include "svga_winsys.h"


/**
 * Allocate a winsys_buffer (ie. DMA, aka GMR memory).
 *
 * It will flush and retry in case the first attempt to create a DMA buffer
 * fails, so it should not be called from any function involved in flushing
 * to avoid recursion.
 */
struct svga_winsys_buffer *
svga_winsys_buffer_create( struct svga_context *svga,
                           unsigned alignment,
                           unsigned usage,
                           unsigned size )
{
   struct svga_screen *svgascreen = svga_screen(svga->pipe.screen);
   struct svga_winsys_screen *sws = svgascreen->sws;
   struct svga_winsys_buffer *buf;

   /* Just try */
   buf = sws->buffer_create(sws, alignment, usage, size);
   if (!buf) {
      SVGA_DBG(DEBUG_DMA|DEBUG_PERF, "flushing context to find %d bytes GMR\n",
               size);

      /* Try flushing all pending DMAs */
      svga_context_flush(svga, NULL);
      buf = sws->buffer_create(sws, alignment, usage, size);
   }

   return buf;
}


void
svga_buffer_destroy_hw_storage(struct svga_screen *ss, struct svga_buffer *sbuf)
{
   struct svga_winsys_screen *sws = ss->sws;

   assert(!sbuf->map.count);
   assert(sbuf->hwbuf);
   if (sbuf->hwbuf) {
      sws->buffer_destroy(sws, sbuf->hwbuf);
      sbuf->hwbuf = NULL;
   }
}



/**
 * Allocate DMA'ble storage for the buffer.
 *
 * Called before mapping a buffer.
 */
enum pipe_error
svga_buffer_create_hw_storage(struct svga_screen *ss,
                              struct svga_buffer *sbuf)
{
   assert(!sbuf->user);

   if (!sbuf->hwbuf) {
      struct svga_winsys_screen *sws = ss->sws;
      unsigned alignment = 16;
      unsigned usage = 0;
      unsigned size = sbuf->b.b.width0;

      sbuf->hwbuf = sws->buffer_create(sws, alignment, usage, size);
      if (!sbuf->hwbuf)
         return PIPE_ERROR_OUT_OF_MEMORY;

      assert(!sbuf->dma.pending);
   }

   return PIPE_OK;
}



enum pipe_error
svga_buffer_create_host_surface(struct svga_screen *ss,
                                struct svga_buffer *sbuf)
{
   assert(!sbuf->user);

   if (!sbuf->handle) {
      sbuf->key.flags = 0;

      sbuf->key.format = SVGA3D_BUFFER;
      if (sbuf->b.b.bind & PIPE_BIND_VERTEX_BUFFER)
         sbuf->key.flags |= SVGA3D_SURFACE_HINT_VERTEXBUFFER;
      if (sbuf->b.b.bind & PIPE_BIND_INDEX_BUFFER)
         sbuf->key.flags |= SVGA3D_SURFACE_HINT_INDEXBUFFER;

      sbuf->key.size.width = sbuf->b.b.width0;
      sbuf->key.size.height = 1;
      sbuf->key.size.depth = 1;

      sbuf->key.numFaces = 1;
      sbuf->key.numMipLevels = 1;
      sbuf->key.cachable = 1;

      SVGA_DBG(DEBUG_DMA, "surface_create for buffer sz %d\n", sbuf->b.b.width0);

      sbuf->handle = svga_screen_surface_create(ss, &sbuf->key);
      if (!sbuf->handle)
         return PIPE_ERROR_OUT_OF_MEMORY;

      /* Always set the discard flag on the first time the buffer is written
       * as svga_screen_surface_create might have passed a recycled host
       * buffer.
       */
      sbuf->dma.flags.discard = TRUE;

      SVGA_DBG(DEBUG_DMA, "   --> got sid %p sz %d (buffer)\n", sbuf->handle, sbuf->b.b.width0);
   }

   return PIPE_OK;
}


void
svga_buffer_destroy_host_surface(struct svga_screen *ss,
                                 struct svga_buffer *sbuf)
{
   if (sbuf->handle) {
      SVGA_DBG(DEBUG_DMA, " ungrab sid %p sz %d\n", sbuf->handle, sbuf->b.b.width0);
      svga_screen_surface_destroy(ss, &sbuf->key, &sbuf->handle);
   }
}


/**
 * Variant of SVGA3D_BufferDMA which leaves the copy box temporarily in blank.
 */
static enum pipe_error
svga_buffer_upload_command(struct svga_context *svga,
                           struct svga_buffer *sbuf)
{
   struct svga_winsys_context *swc = svga->swc;
   struct svga_winsys_buffer *guest = sbuf->hwbuf;
   struct svga_winsys_surface *host = sbuf->handle;
   SVGA3dTransferType transfer = SVGA3D_WRITE_HOST_VRAM;
   SVGA3dCmdSurfaceDMA *cmd;
   uint32 numBoxes = sbuf->map.num_ranges;
   SVGA3dCopyBox *boxes;
   SVGA3dCmdSurfaceDMASuffix *pSuffix;
   unsigned region_flags;
   unsigned surface_flags;
   struct pipe_resource *dummy;

   if (transfer == SVGA3D_WRITE_HOST_VRAM) {
      region_flags = SVGA_RELOC_READ;
      surface_flags = SVGA_RELOC_WRITE;
   }
   else if (transfer == SVGA3D_READ_HOST_VRAM) {
      region_flags = SVGA_RELOC_WRITE;
      surface_flags = SVGA_RELOC_READ;
   }
   else {
      assert(0);
      return PIPE_ERROR_BAD_INPUT;
   }

   assert(numBoxes);

   cmd = SVGA3D_FIFOReserve(swc,
                            SVGA_3D_CMD_SURFACE_DMA,
                            sizeof *cmd + numBoxes * sizeof *boxes + sizeof *pSuffix,
                            2);
   if (!cmd)
      return PIPE_ERROR_OUT_OF_MEMORY;

   swc->region_relocation(swc, &cmd->guest.ptr, guest, 0, region_flags);
   cmd->guest.pitch = 0;

   swc->surface_relocation(swc, &cmd->host.sid, host, surface_flags);
   cmd->host.face = 0;
   cmd->host.mipmap = 0;

   cmd->transfer = transfer;

   sbuf->dma.boxes = (SVGA3dCopyBox *)&cmd[1];
   sbuf->dma.svga = svga;

   /* Increment reference count */
   dummy = NULL;
   pipe_resource_reference(&dummy, &sbuf->b.b);

   pSuffix = (SVGA3dCmdSurfaceDMASuffix *)((uint8_t*)cmd + sizeof *cmd + numBoxes * sizeof *boxes);
   pSuffix->suffixSize = sizeof *pSuffix;
   pSuffix->maximumOffset = sbuf->b.b.width0;
   pSuffix->flags = sbuf->dma.flags;

   SVGA_FIFOCommitAll(swc);

   sbuf->dma.flags.discard = FALSE;

   return PIPE_OK;
}


/**
 * Patch up the upload DMA command reserved by svga_buffer_upload_command
 * with the final ranges.
 */
void
svga_buffer_upload_flush(struct svga_context *svga,
                         struct svga_buffer *sbuf)
{
   SVGA3dCopyBox *boxes;
   unsigned i;
   struct pipe_resource *dummy;

   if (!sbuf->dma.pending) {
      return;
   }

   assert(sbuf->handle);
   assert(sbuf->hwbuf);
   assert(sbuf->map.num_ranges);
   assert(sbuf->dma.svga == svga);
   assert(sbuf->dma.boxes);

   /*
    * Patch the DMA command with the final copy box.
    */

   SVGA_DBG(DEBUG_DMA, "dma to sid %p\n", sbuf->handle);

   boxes = sbuf->dma.boxes;
   for (i = 0; i < sbuf->map.num_ranges; ++i) {
      SVGA_DBG(DEBUG_DMA, "  bytes %u - %u\n",
               sbuf->map.ranges[i].start, sbuf->map.ranges[i].end);

      boxes[i].x = sbuf->map.ranges[i].start;
      boxes[i].y = 0;
      boxes[i].z = 0;
      boxes[i].w = sbuf->map.ranges[i].end - sbuf->map.ranges[i].start;
      boxes[i].h = 1;
      boxes[i].d = 1;
      boxes[i].srcx = sbuf->map.ranges[i].start;
      boxes[i].srcy = 0;
      boxes[i].srcz = 0;
   }

   sbuf->map.num_ranges = 0;

   assert(sbuf->head.prev && sbuf->head.next);
   LIST_DEL(&sbuf->head);
#ifdef DEBUG
   sbuf->head.next = sbuf->head.prev = NULL;
#endif
   sbuf->dma.pending = FALSE;
   sbuf->dma.flags.discard = FALSE;
   sbuf->dma.flags.unsynchronized = FALSE;

   sbuf->dma.svga = NULL;
   sbuf->dma.boxes = NULL;

   /* Decrement reference count (and potentially destroy) */
   dummy = &sbuf->b.b;
   pipe_resource_reference(&dummy, NULL);
}


/**
 * Note a dirty range.
 *
 * This function only notes the range down. It doesn't actually emit a DMA
 * upload command. That only happens when a context tries to refer to this
 * buffer, and the DMA upload command is added to that context's command
 * buffer.
 *
 * We try to lump as many contiguous DMA transfers together as possible.
 */
void
svga_buffer_add_range(struct svga_buffer *sbuf,
                      unsigned start,
                      unsigned end)
{
   unsigned i;
   unsigned nearest_range;
   unsigned nearest_dist;

   assert(end > start);

   if (sbuf->map.num_ranges < SVGA_BUFFER_MAX_RANGES) {
      nearest_range = sbuf->map.num_ranges;
      nearest_dist = ~0;
   } else {
      nearest_range = SVGA_BUFFER_MAX_RANGES - 1;
      nearest_dist = 0;
   }

   /*
    * Try to grow one of the ranges.
    */

   for (i = 0; i < sbuf->map.num_ranges; ++i) {
      int left_dist;
      int right_dist;
      int dist;

      left_dist = start - sbuf->map.ranges[i].end;
      right_dist = sbuf->map.ranges[i].start - end;
      dist = MAX2(left_dist, right_dist);

      if (dist <= 0) {
         /*
          * Ranges are contiguous or overlapping -- extend this one and return.
          *
          * Note that it is not this function's task to prevent overlapping
          * ranges, as the GMR was already given so it is too late to do
          * anything.  If the ranges overlap here it must surely be because
          * PIPE_TRANSFER_UNSYNCHRONIZED was set.
          */

         sbuf->map.ranges[i].start = MIN2(sbuf->map.ranges[i].start, start);
         sbuf->map.ranges[i].end   = MAX2(sbuf->map.ranges[i].end,   end);
         return;
      }
      else {
         /*
          * Discontiguous ranges -- keep track of the nearest range.
          */

         if (dist < nearest_dist) {
            nearest_range = i;
            nearest_dist = dist;
         }
      }
   }

   /*
    * We cannot add a new range to an existing DMA command, so patch-up the
    * pending DMA upload and start clean.
    */

   svga_buffer_upload_flush(sbuf->dma.svga, sbuf);

   assert(!sbuf->dma.pending);
   assert(!sbuf->dma.svga);
   assert(!sbuf->dma.boxes);

   if (sbuf->map.num_ranges < SVGA_BUFFER_MAX_RANGES) {
      /*
       * Add a new range.
       */

      sbuf->map.ranges[sbuf->map.num_ranges].start = start;
      sbuf->map.ranges[sbuf->map.num_ranges].end = end;
      ++sbuf->map.num_ranges;
   } else {
      /*
       * Everything else failed, so just extend the nearest range.
       *
       * It is OK to do this because we always keep a local copy of the
       * host buffer data, for SW TNL, and the host never modifies the buffer.
       */

      assert(nearest_range < SVGA_BUFFER_MAX_RANGES);
      assert(nearest_range < sbuf->map.num_ranges);
      sbuf->map.ranges[nearest_range].start = MIN2(sbuf->map.ranges[nearest_range].start, start);
      sbuf->map.ranges[nearest_range].end   = MAX2(sbuf->map.ranges[nearest_range].end,   end);
   }
}



/**
 * Copy the contents of the malloc buffer to a hardware buffer.
 */
static enum pipe_error
svga_buffer_update_hw(struct svga_screen *ss, struct svga_buffer *sbuf)
{
   assert(!sbuf->user);
   if (!sbuf->hwbuf) {
      enum pipe_error ret;
      void *map;

      assert(sbuf->swbuf);
      if (!sbuf->swbuf)
         return PIPE_ERROR;

      ret = svga_buffer_create_hw_storage(ss, sbuf);
      if (ret != PIPE_OK)
         return ret;

      pipe_mutex_lock(ss->swc_mutex);
      map = ss->sws->buffer_map(ss->sws, sbuf->hwbuf, PIPE_TRANSFER_WRITE);
      assert(map);
      if (!map) {
	 pipe_mutex_unlock(ss->swc_mutex);
         svga_buffer_destroy_hw_storage(ss, sbuf);
         return PIPE_ERROR;
      }

      memcpy(map, sbuf->swbuf, sbuf->b.b.width0);
      ss->sws->buffer_unmap(ss->sws, sbuf->hwbuf);

      /* This user/malloc buffer is now indistinguishable from a gpu buffer */
      assert(!sbuf->map.count);
      if (!sbuf->map.count) {
         if (sbuf->user)
            sbuf->user = FALSE;
         else
            align_free(sbuf->swbuf);
         sbuf->swbuf = NULL;
      }

      pipe_mutex_unlock(ss->swc_mutex);
   }

   return PIPE_OK;
}


/**
 * Upload the buffer to the host in a piecewise fashion.
 *
 * Used when the buffer is too big to fit in the GMR aperture.
 */
static enum pipe_error
svga_buffer_upload_piecewise(struct svga_screen *ss,
                             struct svga_context *svga,
                             struct svga_buffer *sbuf)
{
   struct svga_winsys_screen *sws = ss->sws;
   const unsigned alignment = sizeof(void *);
   const unsigned usage = 0;
   unsigned i;

   assert(sbuf->map.num_ranges);
   assert(!sbuf->dma.pending);

   SVGA_DBG(DEBUG_DMA, "dma to sid %p\n", sbuf->handle);

   for (i = 0; i < sbuf->map.num_ranges; ++i) {
      struct svga_buffer_range *range = &sbuf->map.ranges[i];
      unsigned offset = range->start;
      unsigned size = range->end - range->start;

      while (offset < range->end) {
         struct svga_winsys_buffer *hwbuf;
         uint8_t *map;
         enum pipe_error ret;

         if (offset + size > range->end)
            size = range->end - offset;

         hwbuf = sws->buffer_create(sws, alignment, usage, size);
         while (!hwbuf) {
            size /= 2;
            if (!size)
               return PIPE_ERROR_OUT_OF_MEMORY;
            hwbuf = sws->buffer_create(sws, alignment, usage, size);
         }

         SVGA_DBG(DEBUG_DMA, "  bytes %u - %u\n",
                  offset, offset + size);

         map = sws->buffer_map(sws, hwbuf,
                               PIPE_TRANSFER_WRITE |
                               PIPE_TRANSFER_DISCARD_RANGE);
         assert(map);
         if (map) {
            memcpy(map, sbuf->swbuf, size);
            sws->buffer_unmap(sws, hwbuf);
         }

         ret = SVGA3D_BufferDMA(svga->swc,
                                hwbuf, sbuf->handle,
                                SVGA3D_WRITE_HOST_VRAM,
                                size, 0, offset, sbuf->dma.flags);
         if (ret != PIPE_OK) {
            svga_context_flush(svga, NULL);
            ret =  SVGA3D_BufferDMA(svga->swc,
                                    hwbuf, sbuf->handle,
                                    SVGA3D_WRITE_HOST_VRAM,
                                    size, 0, offset, sbuf->dma.flags);
            assert(ret == PIPE_OK);
         }

         sbuf->dma.flags.discard = FALSE;

         sws->buffer_destroy(sws, hwbuf);

         offset += size;
      }
   }

   sbuf->map.num_ranges = 0;

   return PIPE_OK;
}




/* Get (or create/upload) the winsys surface handle so that we can
 * refer to this buffer in fifo commands.
 */
struct svga_winsys_surface *
svga_buffer_handle(struct svga_context *svga,
                   struct pipe_resource *buf)
{
   struct pipe_screen *screen = svga->pipe.screen;
   struct svga_screen *ss = svga_screen(screen);
   struct svga_buffer *sbuf;
   enum pipe_error ret;

   if (!buf)
      return NULL;

   sbuf = svga_buffer(buf);

   assert(!sbuf->map.count);
   assert(!sbuf->user);

   if (!sbuf->handle) {
      ret = svga_buffer_create_host_surface(ss, sbuf);
      if (ret != PIPE_OK)
	 return NULL;
   }

   assert(sbuf->handle);

   if (sbuf->map.num_ranges) {
      if (!sbuf->dma.pending) {
         /*
          * No pending DMA upload yet, so insert a DMA upload command now.
          */

         /*
          * Migrate the data from swbuf -> hwbuf if necessary.
          */
         ret = svga_buffer_update_hw(ss, sbuf);
         if (ret == PIPE_OK) {
            /*
             * Queue a dma command.
             */

            ret = svga_buffer_upload_command(svga, sbuf);
            if (ret == PIPE_ERROR_OUT_OF_MEMORY) {
               svga_context_flush(svga, NULL);
               ret = svga_buffer_upload_command(svga, sbuf);
               assert(ret == PIPE_OK);
            }
            if (ret == PIPE_OK) {
               sbuf->dma.pending = TRUE;
               assert(!sbuf->head.prev && !sbuf->head.next);
               LIST_ADDTAIL(&sbuf->head, &svga->dirty_buffers);
            }
         }
         else if (ret == PIPE_ERROR_OUT_OF_MEMORY) {
            /*
             * The buffer is too big to fit in the GMR aperture, so break it in
             * smaller pieces.
             */
            ret = svga_buffer_upload_piecewise(ss, svga, sbuf);
         }

         if (ret != PIPE_OK) {
            /*
             * Something unexpected happened above. There is very little that
             * we can do other than proceeding while ignoring the dirty ranges.
             */
            assert(0);
            sbuf->map.num_ranges = 0;
         }
      }
      else {
         /*
          * There a pending dma already. Make sure it is from this context.
          */
         assert(sbuf->dma.svga == svga);
      }
   }

   assert(!sbuf->map.num_ranges || sbuf->dma.pending);

   return sbuf->handle;
}



void
svga_context_flush_buffers(struct svga_context *svga)
{
   struct list_head *curr, *next;
   struct svga_buffer *sbuf;

   curr = svga->dirty_buffers.next;
   next = curr->next;
   while(curr != &svga->dirty_buffers) {
      sbuf = LIST_ENTRY(struct svga_buffer, curr, head);

      assert(p_atomic_read(&sbuf->b.b.reference.count) != 0);
      assert(sbuf->dma.pending);

      svga_buffer_upload_flush(svga, sbuf);

      curr = next;
      next = curr->next;
   }
}
