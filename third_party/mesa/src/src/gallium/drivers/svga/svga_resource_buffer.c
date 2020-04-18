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

#include "svga_cmd.h"

#include "pipe/p_state.h"
#include "pipe/p_defines.h"
#include "util/u_inlines.h"
#include "os/os_thread.h"
#include "util/u_math.h"
#include "util/u_memory.h"

#include "svga_context.h"
#include "svga_screen.h"
#include "svga_resource_buffer.h"
#include "svga_resource_buffer_upload.h"
#include "svga_winsys.h"
#include "svga_debug.h"


/**
 * Vertex and index buffers need hardware backing.  Constant buffers
 * do not.  No other types of buffers currently supported.
 */
static INLINE boolean
svga_buffer_needs_hw_storage(unsigned usage)
{
   return usage & (PIPE_BIND_VERTEX_BUFFER | PIPE_BIND_INDEX_BUFFER);
}


/**
 * Create a buffer transfer.
 *
 * Unlike texture DMAs (which are written immediately to the command buffer and
 * therefore inherently serialized with other context operations), for buffers
 * we try to coalesce multiple range mappings (i.e, multiple calls to this
 * function) into a single DMA command, for better efficiency in command
 * processing.  This means we need to exercise extra care here to ensure that
 * the end result is exactly the same as if one DMA was used for every mapped
 * range.
 */
static struct pipe_transfer *
svga_buffer_get_transfer(struct pipe_context *pipe,
                         struct pipe_resource *resource,
                         unsigned level,
                         unsigned usage,
                         const struct pipe_box *box)
{
   struct svga_context *svga = svga_context(pipe);
   struct svga_screen *ss = svga_screen(pipe->screen);
   struct svga_buffer *sbuf = svga_buffer(resource);
   struct pipe_transfer *transfer;

   transfer = CALLOC_STRUCT(pipe_transfer);
   if (transfer == NULL) {
      return NULL;
   }

   transfer->resource = resource;
   transfer->level = level;
   transfer->usage = usage;
   transfer->box = *box;

   if (usage & PIPE_TRANSFER_WRITE) {
      if (usage & PIPE_TRANSFER_DISCARD_WHOLE_RESOURCE) {
         /*
          * Flush any pending primitives, finish writing any pending DMA
          * commands, and tell the host to discard the buffer contents on
          * the next DMA operation.
          */

         svga_hwtnl_flush_buffer(svga, resource);

         if (sbuf->dma.pending) {
            svga_buffer_upload_flush(svga, sbuf);

            /*
             * Instead of flushing the context command buffer, simply discard
             * the current hwbuf, and start a new one.
             */

            svga_buffer_destroy_hw_storage(ss, sbuf);
         }

         sbuf->map.num_ranges = 0;
         sbuf->dma.flags.discard = TRUE;
      }

      if (usage & PIPE_TRANSFER_UNSYNCHRONIZED) {
         if (!sbuf->map.num_ranges) {
            /*
             * No pending ranges to upload so far, so we can tell the host to
             * not synchronize on the next DMA command.
             */

            sbuf->dma.flags.unsynchronized = TRUE;
         }
      } else {
         /*
          * Synchronizing, so flush any pending primitives, finish writing any
          * pending DMA command, and ensure the next DMA will be done in order.
          */

         svga_hwtnl_flush_buffer(svga, resource);

         if (sbuf->dma.pending) {
            svga_buffer_upload_flush(svga, sbuf);

            if (sbuf->hwbuf) {
               /*
                * We have a pending DMA upload from a hardware buffer, therefore
                * we need to ensure that the host finishes processing that DMA
                * command before the state tracker can start overwriting the
                * hardware buffer.
                *
                * XXX: This could be avoided by tying the hardware buffer to
                * the transfer (just as done with textures), which would allow
                * overlapping DMAs commands to be queued on the same context
                * buffer. However, due to the likelihood of software vertex
                * processing, it is more convenient to hold on to the hardware
                * buffer, allowing to quickly access the contents from the CPU
                * without having to do a DMA download from the host.
                */

               if (usage & PIPE_TRANSFER_DONTBLOCK) {
                  /*
                   * Flushing the command buffer here will most likely cause
                   * the map of the hwbuf below to block, so preemptively
                   * return NULL here if DONTBLOCK is set to prevent unnecessary
                   * command buffer flushes.
                   */

                  FREE(transfer);
                  return NULL;
               }

               svga_context_flush(svga, NULL);
            }
         }

         sbuf->dma.flags.unsynchronized = FALSE;
      }
   }

   if (!sbuf->swbuf && !sbuf->hwbuf) {
      if (svga_buffer_create_hw_storage(ss, sbuf) != PIPE_OK) {
         /*
          * We can't create a hardware buffer big enough, so create a malloc
          * buffer instead.
          */
         if (0) {
            debug_printf("%s: failed to allocate %u KB of DMA, "
                         "splitting DMA transfers\n",
                         __FUNCTION__,
                         (sbuf->b.b.width0 + 1023)/1024);
         }

         sbuf->swbuf = align_malloc(sbuf->b.b.width0, 16);
         if (!sbuf->swbuf) {
            FREE(transfer);
            return NULL;
         }
      }
   }

   return transfer;
}


/**
 * Map a range of a buffer.
 */
static void *
svga_buffer_transfer_map( struct pipe_context *pipe,
                          struct pipe_transfer *transfer )
{
   struct svga_buffer *sbuf = svga_buffer(transfer->resource);

   uint8_t *map;

   if (sbuf->swbuf) {
      /* User/malloc buffer */
      map = sbuf->swbuf;
   }
   else if (sbuf->hwbuf) {
      struct svga_screen *ss = svga_screen(pipe->screen);
      struct svga_winsys_screen *sws = ss->sws;

      map = sws->buffer_map(sws, sbuf->hwbuf, transfer->usage);
   }
   else {
      map = NULL;
   }

   if (map) {
      ++sbuf->map.count;
      map += transfer->box.x;
   }
   
   return map;
}


static void
svga_buffer_transfer_flush_region( struct pipe_context *pipe,
                                   struct pipe_transfer *transfer,
                                   const struct pipe_box *box)
{
   struct svga_screen *ss = svga_screen(pipe->screen);
   struct svga_buffer *sbuf = svga_buffer(transfer->resource);

   unsigned offset = transfer->box.x + box->x;
   unsigned length = box->width;

   assert(transfer->usage & PIPE_TRANSFER_WRITE);
   assert(transfer->usage & PIPE_TRANSFER_FLUSH_EXPLICIT);

   pipe_mutex_lock(ss->swc_mutex);
   svga_buffer_add_range(sbuf, offset, offset + length);
   pipe_mutex_unlock(ss->swc_mutex);
}


static void
svga_buffer_transfer_unmap( struct pipe_context *pipe,
                            struct pipe_transfer *transfer )
{
   struct svga_screen *ss = svga_screen(pipe->screen);
   struct svga_winsys_screen *sws = ss->sws;
   struct svga_buffer *sbuf = svga_buffer(transfer->resource);
   
   pipe_mutex_lock(ss->swc_mutex);
   
   assert(sbuf->map.count);
   if (sbuf->map.count) {
      --sbuf->map.count;
   }

   if (sbuf->hwbuf) {
      sws->buffer_unmap(sws, sbuf->hwbuf);
   }

   if (transfer->usage & PIPE_TRANSFER_WRITE) {
      if (!(transfer->usage & PIPE_TRANSFER_FLUSH_EXPLICIT)) {
         /*
          * Mapped range not flushed explicitly, so flush the whole buffer,
          * and tell the host to discard the contents when processing the DMA
          * command.
          */

         SVGA_DBG(DEBUG_DMA, "flushing the whole buffer\n");
   
         sbuf->dma.flags.discard = TRUE;

         svga_buffer_add_range(sbuf, 0, sbuf->b.b.width0);
      }
   }

   pipe_mutex_unlock(ss->swc_mutex);
}


/**
 * Destroy transfer
 */
static void
svga_buffer_transfer_destroy(struct pipe_context *pipe,
                             struct pipe_transfer *transfer)
{
   FREE(transfer);
}


static void
svga_buffer_destroy( struct pipe_screen *screen,
		     struct pipe_resource *buf )
{
   struct svga_screen *ss = svga_screen(screen); 
   struct svga_buffer *sbuf = svga_buffer( buf );

   assert(!p_atomic_read(&buf->reference.count));
   
   assert(!sbuf->dma.pending);

   if(sbuf->handle)
      svga_buffer_destroy_host_surface(ss, sbuf);
   
   if(sbuf->uploaded.buffer)
      pipe_resource_reference(&sbuf->uploaded.buffer, NULL);

   if(sbuf->hwbuf)
      svga_buffer_destroy_hw_storage(ss, sbuf);
   
   if(sbuf->swbuf && !sbuf->user)
      align_free(sbuf->swbuf);
   
   FREE(sbuf);
}


struct u_resource_vtbl svga_buffer_vtbl = 
{
   u_default_resource_get_handle,      /* get_handle */
   svga_buffer_destroy,		     /* resource_destroy */
   svga_buffer_get_transfer,	     /* get_transfer */
   svga_buffer_transfer_destroy,     /* transfer_destroy */
   svga_buffer_transfer_map,	     /* transfer_map */
   svga_buffer_transfer_flush_region,  /* transfer_flush_region */
   svga_buffer_transfer_unmap,	     /* transfer_unmap */
   u_default_transfer_inline_write   /* transfer_inline_write */
};



struct pipe_resource *
svga_buffer_create(struct pipe_screen *screen,
		   const struct pipe_resource *template)
{
   struct svga_screen *ss = svga_screen(screen);
   struct svga_buffer *sbuf;
   
   sbuf = CALLOC_STRUCT(svga_buffer);
   if(!sbuf)
      goto error1;
   
   sbuf->b.b = *template;
   sbuf->b.vtbl = &svga_buffer_vtbl;
   pipe_reference_init(&sbuf->b.b.reference, 1);
   sbuf->b.b.screen = screen;

   if(svga_buffer_needs_hw_storage(template->bind)) {
      if(svga_buffer_create_host_surface(ss, sbuf) != PIPE_OK)
         goto error2;
   }
   else {
      sbuf->swbuf = align_malloc(template->width0, 64);
      if(!sbuf->swbuf)
         goto error2;
   }
      
   debug_reference(&sbuf->b.b.reference,
                   (debug_reference_descriptor)debug_describe_resource, 0);

   return &sbuf->b.b; 

error2:
   FREE(sbuf);
error1:
   return NULL;
}

struct pipe_resource *
svga_user_buffer_create(struct pipe_screen *screen,
                        void *ptr,
                        unsigned bytes,
			unsigned bind)
{
   struct svga_buffer *sbuf;
   
   sbuf = CALLOC_STRUCT(svga_buffer);
   if(!sbuf)
      goto no_sbuf;
      
   pipe_reference_init(&sbuf->b.b.reference, 1);
   sbuf->b.vtbl = &svga_buffer_vtbl;
   sbuf->b.b.screen = screen;
   sbuf->b.b.format = PIPE_FORMAT_R8_UNORM; /* ?? */
   sbuf->b.b.usage = PIPE_USAGE_IMMUTABLE;
   sbuf->b.b.bind = bind;
   sbuf->b.b.width0 = bytes;
   sbuf->b.b.height0 = 1;
   sbuf->b.b.depth0 = 1;
   sbuf->b.b.array_size = 1;

   sbuf->swbuf = ptr;
   sbuf->user = TRUE;

   debug_reference(&sbuf->b.b.reference,
                   (debug_reference_descriptor)debug_describe_resource, 0);
   
   return &sbuf->b.b; 

no_sbuf:
   return NULL;
}



