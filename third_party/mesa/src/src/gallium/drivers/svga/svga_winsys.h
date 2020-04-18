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

/**
 * @file
 * VMware SVGA specific winsys interface.
 * 
 * @author Jose Fonseca <jfonseca@vmware.com>
 * 
 * Documentation taken from the VMware SVGA DDK.
 */

#ifndef SVGA_WINSYS_H_
#define SVGA_WINSYS_H_


#include "svga_types.h"
#include "svga_reg.h"
#include "svga3d_reg.h"

#include "pipe/p_compiler.h"
#include "pipe/p_defines.h"


struct svga_winsys_screen;
struct svga_winsys_buffer;
struct pipe_screen;
struct pipe_context;
struct pipe_fence_handle;
struct pipe_resource;
struct svga_region;
struct winsys_handle;


#define SVGA_BUFFER_USAGE_PINNED  (1 << 0)
#define SVGA_BUFFER_USAGE_WRAPPED (1 << 1)


#define SVGA_RELOC_WRITE 0x1
#define SVGA_RELOC_READ  0x2

#define SVGA_FENCE_FLAG_EXEC      (1 << 0)
#define SVGA_FENCE_FLAG_QUERY     (1 << 1)

/** Opaque surface handle */
struct svga_winsys_surface;


/**
 * SVGA per-context winsys interface.
 */
struct svga_winsys_context
{
   void
   (*destroy)(struct svga_winsys_context *swc);

   void *       
   (*reserve)(struct svga_winsys_context *swc, 
	      uint32_t nr_bytes, uint32_t nr_relocs );
   
   /**
    * Emit a relocation for a host surface.
    * 
    * @param flags bitmask of SVGA_RELOC_* flags
    * 
    * NOTE: Order of this call does matter. It should be the same order
    * as relocations appear in the command buffer.
    */
   void
   (*surface_relocation)(struct svga_winsys_context *swc, 
	                 uint32 *sid, 
	                 struct svga_winsys_surface *surface,
	                 unsigned flags);
   
   /**
    * Emit a relocation for a guest memory region.
    * 
    * @param flags bitmask of SVGA_RELOC_* flags
    * 
    * NOTE: Order of this call does matter. It should be the same order
    * as relocations appear in the command buffer.
    */
   void
   (*region_relocation)(struct svga_winsys_context *swc, 
	                struct SVGAGuestPtr *ptr, 
	                struct svga_winsys_buffer *buffer,
	                uint32 offset,
                        unsigned flags);

   void
   (*commit)(struct svga_winsys_context *swc);
   
   enum pipe_error
   (*flush)(struct svga_winsys_context *swc, 
	    struct pipe_fence_handle **pfence);

   /** 
    * Context ID used to fill in the commands
    * 
    * Context IDs are arbitrary small non-negative integers,
    * global to the entire SVGA device.
    */
   uint32 cid;
};


/**
 * SVGA per-screen winsys interface.
 */
struct svga_winsys_screen
{
   void
   (*destroy)(struct svga_winsys_screen *sws);
   
   SVGA3dHardwareVersion
   (*get_hw_version)(struct svga_winsys_screen *sws);

   boolean
   (*get_cap)(struct svga_winsys_screen *sws,
              SVGA3dDevCapIndex index,
              SVGA3dDevCapResult *result);
   
   /**
    * Create a new context.
    *
    * Context objects encapsulate all render state, and shader
    * objects are per-context.
    *
    * Surfaces are not per-context. The same surface can be shared
    * between multiple contexts, and surface operations can occur
    * without a context.
    */
   struct svga_winsys_context *
   (*context_create)(struct svga_winsys_screen *sws);
   
   
   /**
    * This creates a "surface" object in the SVGA3D device,
    * and returns the surface ID (sid). Surfaces are generic
    * containers for host VRAM objects like textures, vertex
    * buffers, and depth/stencil buffers.
    *
    * Surfaces are hierarchial:
    *
    * - Surface may have multiple faces (for cube maps)
    *
    * - Each face has a list of mipmap levels
    *
    * - Each mipmap image may have multiple volume
    *   slices, if the image is three dimensional.
    *
    * - Each slice is a 2D array of 'blocks'
    *
    * - Each block may be one or more pixels.
    *   (Usually 1, more for DXT or YUV formats.)
    *
    * Surfaces are generic host VRAM objects. The SVGA3D device
    * may optimize surfaces according to the format they were
    * created with, but this format does not limit the ways in
    * which the surface may be used. For example, a depth surface
    * can be used as a texture, or a floating point image may
    * be used as a vertex buffer. Some surface usages may be
    * lower performance, due to software emulation, but any
    * usage should work with any surface.
    */
   struct svga_winsys_surface *
   (*surface_create)(struct svga_winsys_screen *sws,
                     SVGA3dSurfaceFlags flags,
                     SVGA3dSurfaceFormat format,
                     SVGA3dSize size,
                     uint32 numFaces,
                     uint32 numMipLevels);

   /**
    * Creates a surface from a winsys handle.
    * Used to implement pipe_screen::resource_from_handle.
    */
   struct svga_winsys_surface *
   (*surface_from_handle)(struct svga_winsys_screen *sws,
                          struct winsys_handle *whandle,
                          SVGA3dSurfaceFormat *format);

   /**
    * Get a winsys_handle from a surface.
    * Used to implement pipe_screen::resource_get_handle.
    */
   boolean
   (*surface_get_handle)(struct svga_winsys_screen *sws,
                         struct svga_winsys_surface *surface,
                         unsigned stride,
                         struct winsys_handle *whandle);

   /**
    * Whether this surface is sitting in a validate list
    */
   boolean
   (*surface_is_flushed)(struct svga_winsys_screen *sws,
                         struct svga_winsys_surface *surface);

   /**
    * Reference a SVGA3D surface object. This allows sharing of a
    * surface between different objects.
    */
   void 
   (*surface_reference)(struct svga_winsys_screen *sws,
			struct svga_winsys_surface **pdst,
			struct svga_winsys_surface *src);

   /**
    * Buffer management. Buffer attributes are mostly fixed over its lifetime.
    *
    * @param usage bitmask of SVGA_BUFFER_USAGE_* flags.
    *
    * alignment indicates the client's alignment requirements, eg for
    * SSE instructions.
    */
   struct svga_winsys_buffer *
   (*buffer_create)( struct svga_winsys_screen *sws, 
	             unsigned alignment, 
	             unsigned usage,
	             unsigned size );

   /** 
    * Map the entire data store of a buffer object into the client's address.
    * usage is a bitmask of PIPE_TRANSFER_*
    */
   void *
   (*buffer_map)( struct svga_winsys_screen *sws, 
	          struct svga_winsys_buffer *buf,
		  unsigned usage );
   
   void 
   (*buffer_unmap)( struct svga_winsys_screen *sws, 
                    struct svga_winsys_buffer *buf );

   void 
   (*buffer_destroy)( struct svga_winsys_screen *sws,
	              struct svga_winsys_buffer *buf );


   /**
    * Reference a fence object.
    */
   void
   (*fence_reference)( struct svga_winsys_screen *sws,
                       struct pipe_fence_handle **pdst,
                       struct pipe_fence_handle *src );

   /**
    * Checks whether the fence has been signalled.
    * \param flags  driver-specific meaning
    * \return zero on success.
    */
   int (*fence_signalled)( struct svga_winsys_screen *sws,
                           struct pipe_fence_handle *fence,
                           unsigned flag );

   /**
    * Wait for the fence to finish.
    * \param flags  driver-specific meaning
    * \return zero on success.
    */
   int (*fence_finish)( struct svga_winsys_screen *sws,
                        struct pipe_fence_handle *fence,
                        unsigned flag );

};


struct svga_winsys_screen *
svga_winsys_screen(struct pipe_screen *screen);

struct svga_winsys_context *
svga_winsys_context(struct pipe_context *context);

struct pipe_resource *
svga_screen_buffer_wrap_surface(struct pipe_screen *screen,
				enum SVGA3dSurfaceFormat format,
				struct svga_winsys_surface *srf);

struct svga_winsys_surface *
svga_screen_buffer_get_winsys_surface(struct pipe_resource *buffer);

#endif /* SVGA_WINSYS_H_ */
