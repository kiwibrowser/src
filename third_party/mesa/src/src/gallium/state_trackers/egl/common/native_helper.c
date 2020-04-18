/*
 * Mesa 3-D graphics library
 * Version:  7.9
 *
 * Copyright (C) 2010 LunarG Inc.
 * Copyright (C) 2011 VMware Inc. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Chia-I Wu <olv@lunarg.com>
 *    Thomas Hellstrom <thellstrom@vmware.com>
 */

#include "util/u_inlines.h"
#include "util/u_memory.h"
#include "pipe/p_screen.h"
#include "pipe/p_context.h"
#include "pipe/p_state.h"

#include "native_helper.h"

/**
 * Number of swap fences and mask
 */

#define EGL_SWAP_FENCES_MAX 4
#define EGL_SWAP_FENCES_MASK 3
#define EGL_SWAP_FENCES_DEFAULT 1

struct resource_surface {
   struct pipe_screen *screen;
   enum pipe_format format;
   uint bind;

   struct pipe_resource *resources[NUM_NATIVE_ATTACHMENTS];
   uint resource_mask;
   uint width, height;

   /**
    * Swap fences.
    */
   struct pipe_fence_handle *swap_fences[EGL_SWAP_FENCES_MAX];
   unsigned int cur_fences;
   unsigned int head;
   unsigned int tail;
   unsigned int desired_fences;
};

struct resource_surface *
resource_surface_create(struct pipe_screen *screen,
                        enum pipe_format format, uint bind)
{
   struct resource_surface *rsurf = CALLOC_STRUCT(resource_surface);
   char *swap_fences = getenv("EGL_THROTTLE_FENCES");

   if (rsurf) {
      rsurf->screen = screen;
      rsurf->format = format;
      rsurf->bind = bind;
      rsurf->desired_fences = (swap_fences) ? atoi(swap_fences) :
	 EGL_SWAP_FENCES_DEFAULT;
      if (rsurf->desired_fences > EGL_SWAP_FENCES_MAX)
	 rsurf->desired_fences = EGL_SWAP_FENCES_MAX;
   }

   return rsurf;
}

static void
resource_surface_free_resources(struct resource_surface *rsurf)
{
   if (rsurf->resource_mask) {
      int i;

      for (i = 0; i < NUM_NATIVE_ATTACHMENTS; i++) {
         if (rsurf->resources[i])
            pipe_resource_reference(&rsurf->resources[i], NULL);
      }
      rsurf->resource_mask = 0x0;
   }
}

void
resource_surface_destroy(struct resource_surface *rsurf)
{
   resource_surface_free_resources(rsurf);
   FREE(rsurf);
}

boolean
resource_surface_set_size(struct resource_surface *rsurf,
                          uint width, uint height)
{
   boolean changed = FALSE;

   if (rsurf->width != width || rsurf->height != height) {
      resource_surface_free_resources(rsurf);
      rsurf->width = width;
      rsurf->height = height;
      changed = TRUE;
   }

   return changed;
}

void
resource_surface_get_size(struct resource_surface *rsurf,
                          uint *width, uint *height)
{
   if (width)
      *width = rsurf->width;
   if (height)
      *height = rsurf->height;
}

boolean
resource_surface_add_resources(struct resource_surface *rsurf,
                               uint resource_mask)
{
   struct pipe_resource templ;
   int i;

   resource_mask &= ~rsurf->resource_mask;
   if (!resource_mask)
      return TRUE;

   if (!rsurf->width || !rsurf->height)
      return FALSE;

   memset(&templ, 0, sizeof(templ));
   templ.target = PIPE_TEXTURE_2D;
   templ.format = rsurf->format;
   templ.bind = rsurf->bind;
   templ.width0 = rsurf->width;
   templ.height0 = rsurf->height;
   templ.depth0 = 1;
   templ.array_size = 1;

   for (i = 0; i < NUM_NATIVE_ATTACHMENTS; i++) {
      if (resource_mask & (1 <<i)) {
         assert(!rsurf->resources[i]);

         rsurf->resources[i] =
            rsurf->screen->resource_create(rsurf->screen, &templ);
         if (rsurf->resources[i])
            rsurf->resource_mask |= 1 << i;
      }
   }

   return ((rsurf->resource_mask & resource_mask) == resource_mask);
}

void
resource_surface_import_resource(struct resource_surface *rsurf,
                                 enum native_attachment which,
                                 struct pipe_resource *pres)
{
	pipe_resource_reference(&rsurf->resources[which], pres);
	rsurf->resource_mask |= 1 << which;
}

void
resource_surface_get_resources(struct resource_surface *rsurf,
                               struct pipe_resource **resources,
                               uint resource_mask)
{
   int i;

   for (i = 0; i < NUM_NATIVE_ATTACHMENTS; i++) {
      if (resource_mask & (1 << i)) {
         resources[i] = NULL;
         pipe_resource_reference(&resources[i], rsurf->resources[i]);
      }
   }
}

struct pipe_resource *
resource_surface_get_single_resource(struct resource_surface *rsurf,
                                     enum native_attachment which)
{
   struct pipe_resource *pres = NULL;
   pipe_resource_reference(&pres, rsurf->resources[which]);
   return pres;
}

static INLINE void
pointer_swap(const void **p1, const void **p2)
{
   const void *tmp = *p1;
   *p1 = *p2;
   *p2 = tmp;
}

void
resource_surface_swap_buffers(struct resource_surface *rsurf,
                              enum native_attachment buf1,
                              enum native_attachment buf2,
                              boolean only_if_exist)
{
   const uint buf1_bit = 1 << buf1;
   const uint buf2_bit = 1 << buf2;
   uint mask;

   if (only_if_exist && !(rsurf->resources[buf1] && rsurf->resources[buf2]))
      return;

   pointer_swap((const void **) &rsurf->resources[buf1],
                (const void **) &rsurf->resources[buf2]);

   /* swap mask bits */
   mask = rsurf->resource_mask & ~(buf1_bit | buf2_bit);
   if (rsurf->resource_mask & buf1_bit)
      mask |= buf2_bit;
   if (rsurf->resource_mask & buf2_bit)
      mask |= buf1_bit;

   rsurf->resource_mask = mask;
}

boolean
resource_surface_present(struct resource_surface *rsurf,
                         enum native_attachment which,
                         void *winsys_drawable_handle)
{
   struct pipe_resource *pres = rsurf->resources[which];

   if (!pres)
      return TRUE;

   rsurf->screen->flush_frontbuffer(rsurf->screen,
         pres, 0, 0, winsys_drawable_handle);

   return TRUE;
}

/**
 * Schedule a copy swap from the back to the front buffer using the
 * native display's copy context.
 */
boolean
resource_surface_copy_swap(struct resource_surface *rsurf,
			   struct native_display *ndpy)
{
   struct pipe_resource *ftex;
   struct pipe_resource *btex;
   struct pipe_context *pipe;
   struct pipe_box src_box;
   boolean ret = FALSE;

   pipe = ndpy_get_copy_context(ndpy);
   if (!pipe)
      return FALSE;

   ftex = resource_surface_get_single_resource(rsurf,
					       NATIVE_ATTACHMENT_FRONT_LEFT);
   if (!ftex)
      goto out_no_ftex;
   btex = resource_surface_get_single_resource(rsurf,
					       NATIVE_ATTACHMENT_BACK_LEFT);
   if (!btex)
      goto out_no_btex;

   u_box_origin_2d(ftex->width0, ftex->height0, &src_box);
   pipe->resource_copy_region(pipe, ftex, 0, 0, 0, 0,
			      btex, 0, &src_box);
   ret = TRUE;

 out_no_btex:
   pipe_resource_reference(&btex, NULL);
 out_no_ftex:
   pipe_resource_reference(&ftex, NULL);

   return ret;
}

static struct pipe_fence_handle *
swap_fences_pop_front(struct resource_surface *rsurf)
{
   struct pipe_screen *screen = rsurf->screen;
   struct pipe_fence_handle *fence = NULL;

   if (rsurf->desired_fences == 0)
      return NULL;

   if (rsurf->cur_fences >= rsurf->desired_fences) {
      screen->fence_reference(screen, &fence, rsurf->swap_fences[rsurf->tail]);
      screen->fence_reference(screen, &rsurf->swap_fences[rsurf->tail++], NULL);
      rsurf->tail &= EGL_SWAP_FENCES_MASK;
      --rsurf->cur_fences;
   }
   return fence;
}

static void
swap_fences_push_back(struct resource_surface *rsurf,
		      struct pipe_fence_handle *fence)
{
   struct pipe_screen *screen = rsurf->screen;

   if (!fence || rsurf->desired_fences == 0)
      return;

   while(rsurf->cur_fences == rsurf->desired_fences)
      swap_fences_pop_front(rsurf);

   rsurf->cur_fences++;
   screen->fence_reference(screen, &rsurf->swap_fences[rsurf->head++],
			   fence);
   rsurf->head &= EGL_SWAP_FENCES_MASK;
}

boolean
resource_surface_throttle(struct resource_surface *rsurf)
{
   struct pipe_screen *screen = rsurf->screen;
   struct pipe_fence_handle *fence = swap_fences_pop_front(rsurf);

   if (fence) {
      (void) screen->fence_finish(screen, fence, PIPE_TIMEOUT_INFINITE);
      screen->fence_reference(screen, &fence, NULL);
      return TRUE;
   }

   return FALSE;
}

boolean
resource_surface_flush(struct resource_surface *rsurf,
		       struct native_display *ndpy)
{
   struct pipe_fence_handle *fence = NULL;
   struct pipe_screen *screen = rsurf->screen;
   struct pipe_context *pipe= ndpy_get_copy_context(ndpy);

   if (!pipe)
      return FALSE;

   pipe->flush(pipe, &fence);
   if (fence == NULL)
      return FALSE;

   swap_fences_push_back(rsurf, fence);
   screen->fence_reference(screen, &fence, NULL);

   return TRUE;
}

void
resource_surface_wait(struct resource_surface *rsurf)
{
   while (resource_surface_throttle(rsurf));
}

boolean
native_display_copy_to_pixmap(struct native_display *ndpy,
                              EGLNativePixmapType pix,
                              struct pipe_resource *src)
{
   struct pipe_context *pipe;
   struct native_surface *nsurf;
   struct pipe_resource *dst;
   struct pipe_resource *tmp[NUM_NATIVE_ATTACHMENTS];
   const enum native_attachment natt = NATIVE_ATTACHMENT_FRONT_LEFT;

   pipe = ndpy_get_copy_context(ndpy);
   if (!pipe)
      return FALSE;

   nsurf = ndpy->create_pixmap_surface(ndpy, pix, NULL);
   if (!nsurf)
      return FALSE;

   /* get the texutre */
   tmp[natt] = NULL;
   nsurf->validate(nsurf, 1 << natt, NULL, tmp, NULL, NULL);
   dst = tmp[natt];

   if (dst && dst->format == src->format) {
      struct native_present_control ctrl;
      struct pipe_box src_box;

      u_box_origin_2d(src->width0, src->height0, &src_box);
      pipe->resource_copy_region(pipe, dst, 0, 0, 0, 0, src, 0, &src_box);
      pipe->flush(pipe, NULL);

      memset(&ctrl, 0, sizeof(ctrl));
      ctrl.natt = natt;
      nsurf->present(nsurf, &ctrl);
   }

   if (dst)
      pipe_resource_reference(&dst, NULL);

   nsurf->destroy(nsurf);

   return TRUE;
}

#include "state_tracker/drm_driver.h"
struct pipe_resource *
drm_display_import_native_buffer(struct native_display *ndpy,
                                 struct native_buffer *nbuf)
{
   struct pipe_screen *screen = ndpy->screen;
   struct pipe_resource *res = NULL;

   switch (nbuf->type) {
   case NATIVE_BUFFER_DRM:
      {
         struct winsys_handle wsh;

         memset(&wsh, 0, sizeof(wsh));
         wsh.handle = nbuf->u.drm.name;
         wsh.stride = nbuf->u.drm.stride;

         res = screen->resource_from_handle(screen, &nbuf->u.drm.templ, &wsh);
      }
      break;
   default:
      break;
   }

   return res;
}

boolean
drm_display_export_native_buffer(struct native_display *ndpy,
                                 struct pipe_resource *res,
                                 struct native_buffer *nbuf)
{
   struct pipe_screen *screen = ndpy->screen;
   boolean ret = FALSE;

   switch (nbuf->type) {
   case NATIVE_BUFFER_DRM:
      {
         struct winsys_handle wsh;

         if ((nbuf->u.drm.templ.bind & res->bind) != nbuf->u.drm.templ.bind)
            break;

         memset(&wsh, 0, sizeof(wsh));
         wsh.type = DRM_API_HANDLE_TYPE_KMS;
         if (!screen->resource_get_handle(screen, res, &wsh))
            break;

         nbuf->u.drm.handle = wsh.handle;
         nbuf->u.drm.stride = wsh.stride;

         /* get the name of the GEM object */
         if (nbuf->u.drm.templ.bind & PIPE_BIND_SHARED) {
            memset(&wsh, 0, sizeof(wsh));
            wsh.type = DRM_API_HANDLE_TYPE_SHARED;
            if (!screen->resource_get_handle(screen, res, &wsh))
               break;

            nbuf->u.drm.name = wsh.handle;
         }

         nbuf->u.drm.templ = *res;
         ret = TRUE;
      }
      break;
   default:
      break;
   }

   return ret;
}
