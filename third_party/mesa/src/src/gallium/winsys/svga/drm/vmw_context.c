/**********************************************************
 * Copyright 2009 VMware, Inc.  All rights reserved.
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

#include "util/u_debug.h"
#include "util/u_memory.h"
#include "util/u_debug_stack.h"
#include "pipebuffer/pb_buffer.h"
#include "pipebuffer/pb_validate.h"

#include "svga_winsys.h"
#include "vmw_context.h"
#include "vmw_screen.h"
#include "vmw_buffer.h"
#include "vmw_surface.h"
#include "vmw_fence.h"

#define VMW_COMMAND_SIZE (64*1024)
#define VMW_SURFACE_RELOCS (1024)
#define VMW_REGION_RELOCS (512)

#define VMW_MUST_FLUSH_STACK 8

struct vmw_region_relocation
{
   struct SVGAGuestPtr *where;
   struct pb_buffer *buffer;
   /* TODO: put offset info inside where */
   uint32 offset;
};

struct vmw_svga_winsys_context
{
   struct svga_winsys_context base;

   struct vmw_winsys_screen *vws;

#ifdef DEBUG
   boolean must_flush;
   struct debug_stack_frame must_flush_stack[VMW_MUST_FLUSH_STACK];
#endif

   struct {
      uint8_t buffer[VMW_COMMAND_SIZE];
      uint32_t size;
      uint32_t used;
      uint32_t reserved;
   } command;

   struct {
      struct vmw_svga_winsys_surface *handles[VMW_SURFACE_RELOCS];
      uint32_t size;
      uint32_t used;
      uint32_t staged;
      uint32_t reserved;
   } surface;
   
   struct {
      struct vmw_region_relocation relocs[VMW_REGION_RELOCS];
      uint32_t size;
      uint32_t used;
      uint32_t staged;
      uint32_t reserved;
   } region;

   struct pb_validate *validate;

   /**
    * The amount of GMR that is referred by the commands currently batched
    * in the context.
    */
   uint32_t seen_regions;

   /**
    * Whether this context should fail to reserve more commands, not because it
    * ran out of command space, but because a substantial ammount of GMR was
    * referred.
    */
   boolean preemptive_flush;
};


static INLINE struct vmw_svga_winsys_context *
vmw_svga_winsys_context(struct svga_winsys_context *swc)
{
   assert(swc);
   return (struct vmw_svga_winsys_context *)swc;
}


static INLINE unsigned
vmw_translate_to_pb_flags(unsigned flags)
{
   unsigned f = 0;
   if (flags & SVGA_RELOC_READ)
      f |= PB_USAGE_GPU_READ;

   if (flags & SVGA_RELOC_WRITE)
      f |= PB_USAGE_GPU_WRITE;

   return f;
}

static enum pipe_error
vmw_swc_flush(struct svga_winsys_context *swc,
              struct pipe_fence_handle **pfence)
{
   struct vmw_svga_winsys_context *vswc = vmw_svga_winsys_context(swc);
   struct pipe_fence_handle *fence = NULL;
   unsigned i;
   enum pipe_error ret;

   ret = pb_validate_validate(vswc->validate);
   assert(ret == PIPE_OK);
   if(ret == PIPE_OK) {
   
      /* Apply relocations */
      for(i = 0; i < vswc->region.used; ++i) {
         struct vmw_region_relocation *reloc = &vswc->region.relocs[i];
         struct SVGAGuestPtr ptr;

         if(!vmw_gmr_bufmgr_region_ptr(reloc->buffer, &ptr))
            assert(0);

         ptr.offset += reloc->offset;

         *reloc->where = ptr;
      }

      if (vswc->command.used || pfence != NULL)
         vmw_ioctl_command(vswc->vws,
			   vswc->base.cid,
			   0,
                           vswc->command.buffer,
                           vswc->command.used,
                           &fence);

      pb_validate_fence(vswc->validate, fence);
   }

   vswc->command.used = 0;
   vswc->command.reserved = 0;

   for(i = 0; i < vswc->surface.used + vswc->surface.staged; ++i) {
      struct vmw_svga_winsys_surface *vsurf =
	 vswc->surface.handles[i];
      p_atomic_dec(&vsurf->validated);
      vmw_svga_winsys_surface_reference(&vswc->surface.handles[i], NULL);
   }

   vswc->surface.used = 0;
   vswc->surface.reserved = 0;

   for(i = 0; i < vswc->region.used + vswc->region.staged; ++i) {
      pb_reference(&vswc->region.relocs[i].buffer, NULL);
   }

   vswc->region.used = 0;
   vswc->region.reserved = 0;

#ifdef DEBUG
   vswc->must_flush = FALSE;
#endif
   vswc->preemptive_flush = FALSE;
   vswc->seen_regions = 0;

   if(pfence)
      vmw_fence_reference(vswc->vws, pfence, fence);

   vmw_fence_reference(vswc->vws, &fence, NULL);

   return ret;
}


static void *
vmw_swc_reserve(struct svga_winsys_context *swc,
                uint32_t nr_bytes, uint32_t nr_relocs )
{
   struct vmw_svga_winsys_context *vswc = vmw_svga_winsys_context(swc);

#ifdef DEBUG
   /* Check if somebody forgot to check the previous failure */
   if(vswc->must_flush) {
      debug_printf("Forgot to flush:\n");
      debug_backtrace_dump(vswc->must_flush_stack, VMW_MUST_FLUSH_STACK);
      assert(!vswc->must_flush);
   }
#endif

   assert(nr_bytes <= vswc->command.size);
   if(nr_bytes > vswc->command.size)
      return NULL;

   if(vswc->preemptive_flush ||
      vswc->command.used + nr_bytes > vswc->command.size ||
      vswc->surface.used + nr_relocs > vswc->surface.size ||
      vswc->region.used + nr_relocs > vswc->region.size) {
#ifdef DEBUG
      vswc->must_flush = TRUE;
      debug_backtrace_capture(vswc->must_flush_stack, 1,
                              VMW_MUST_FLUSH_STACK);
#endif
      return NULL;
   }

   assert(vswc->command.used + nr_bytes <= vswc->command.size);
   assert(vswc->surface.used + nr_relocs <= vswc->surface.size);
   assert(vswc->region.used + nr_relocs <= vswc->region.size);
   
   vswc->command.reserved = nr_bytes;
   vswc->surface.reserved = nr_relocs;
   vswc->surface.staged = 0;
   vswc->region.reserved = nr_relocs;
   vswc->region.staged = 0;
   
   return vswc->command.buffer + vswc->command.used;
}


static void
vmw_swc_surface_relocation(struct svga_winsys_context *swc,
                           uint32 *where,
                           struct svga_winsys_surface *surface,
                           unsigned flags)
{
   struct vmw_svga_winsys_context *vswc = vmw_svga_winsys_context(swc);
   struct vmw_svga_winsys_surface *vsurf;

   if(!surface) {
      *where = SVGA3D_INVALID_ID;
      return;
   }

   assert(vswc->surface.staged < vswc->surface.reserved);

   vsurf = vmw_svga_winsys_surface(surface);

   *where = vsurf->sid;

   vmw_svga_winsys_surface_reference(&vswc->surface.handles[vswc->surface.used + vswc->surface.staged], vsurf);
   p_atomic_inc(&vsurf->validated);
   ++vswc->surface.staged;
}


static void
vmw_swc_region_relocation(struct svga_winsys_context *swc,
                          struct SVGAGuestPtr *where,
                          struct svga_winsys_buffer *buffer,
                          uint32 offset,
                          unsigned flags)
{
   struct vmw_svga_winsys_context *vswc = vmw_svga_winsys_context(swc);
   struct vmw_region_relocation *reloc;
   unsigned translated_flags;
   enum pipe_error ret;
   
   assert(vswc->region.staged < vswc->region.reserved);

   reloc = &vswc->region.relocs[vswc->region.used + vswc->region.staged];
   reloc->where = where;
   pb_reference(&reloc->buffer, vmw_pb_buffer(buffer));
   reloc->offset = offset;

   ++vswc->region.staged;

   translated_flags = vmw_translate_to_pb_flags(flags);
   ret = pb_validate_add_buffer(vswc->validate, reloc->buffer, translated_flags);
   /* TODO: Update pipebuffer to reserve buffers and not fail here */
   assert(ret == PIPE_OK);
   (void)ret;

   /*
    * Flush preemptively the FIFO commands to keep the GMR working set within
    * the GMR pool size.
    *
    * This is necessary for applications like SPECviewperf that generate huge
    * amounts of immediate vertex data, so that we don't pile up too much of
    * that vertex data neither in the guest nor in the host.
    *
    * Note that in the current implementation if a region is referred twice in
    * a command stream, it will be accounted twice. We could detect repeated
    * regions and count only once, but there is no incentive to do that, since
    * regions are typically short-lived; always referred in a single command;
    * and at the worst we just flush the commands a bit sooner, which for the
    * SVGA virtual device it's not a performance issue since flushing commands
    * to the FIFO won't cause flushing in the host.
    */
   vswc->seen_regions += reloc->buffer->size;
   if(vswc->seen_regions >= VMW_GMR_POOL_SIZE/3)
      vswc->preemptive_flush = TRUE;
}


static void
vmw_swc_commit(struct svga_winsys_context *swc)
{
   struct vmw_svga_winsys_context *vswc = vmw_svga_winsys_context(swc);

   assert(vswc->command.reserved);
   assert(vswc->command.used + vswc->command.reserved <= vswc->command.size);
   vswc->command.used += vswc->command.reserved;
   vswc->command.reserved = 0;

   assert(vswc->surface.staged <= vswc->surface.reserved);
   assert(vswc->surface.used + vswc->surface.staged <= vswc->surface.size);
   vswc->surface.used += vswc->surface.staged;
   vswc->surface.staged = 0;
   vswc->surface.reserved = 0;

   assert(vswc->region.staged <= vswc->region.reserved);
   assert(vswc->region.used + vswc->region.staged <= vswc->region.size);
   vswc->region.used += vswc->region.staged;
   vswc->region.staged = 0;
   vswc->region.reserved = 0;
}


static void
vmw_swc_destroy(struct svga_winsys_context *swc)
{
   struct vmw_svga_winsys_context *vswc = vmw_svga_winsys_context(swc);
   unsigned i;

   for(i = 0; i < vswc->region.used; ++i) {
      pb_reference(&vswc->region.relocs[i].buffer, NULL);
   }

   for(i = 0; i < vswc->surface.used; ++i) {
      p_atomic_dec(&vswc->surface.handles[i]->validated);
      vmw_svga_winsys_surface_reference(&vswc->surface.handles[i], NULL);
   }
   pb_validate_destroy(vswc->validate);
   vmw_ioctl_context_destroy(vswc->vws, swc->cid);
   FREE(vswc);
}


struct svga_winsys_context *
vmw_svga_winsys_context_create(struct svga_winsys_screen *sws)
{
   struct vmw_winsys_screen *vws = vmw_winsys_screen(sws);
   struct vmw_svga_winsys_context *vswc;

   vswc = CALLOC_STRUCT(vmw_svga_winsys_context);
   if(!vswc)
      return NULL;

   vswc->base.destroy = vmw_swc_destroy;
   vswc->base.reserve = vmw_swc_reserve;
   vswc->base.surface_relocation = vmw_swc_surface_relocation;
   vswc->base.region_relocation = vmw_swc_region_relocation;
   vswc->base.commit = vmw_swc_commit;
   vswc->base.flush = vmw_swc_flush;

   vswc->base.cid = vmw_ioctl_context_create(vws);

   vswc->vws = vws;

   vswc->command.size = VMW_COMMAND_SIZE;
   vswc->surface.size = VMW_SURFACE_RELOCS;
   vswc->region.size = VMW_REGION_RELOCS;

   vswc->validate = pb_validate_create();
   if(!vswc->validate) {
      FREE(vswc);
      return NULL;
   }

   return &vswc->base;
}
