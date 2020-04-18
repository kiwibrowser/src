/*
 * Mesa 3-D graphics library
 * Version:  7.9
 *
 * Copyright (C) 2010 LunarG Inc.
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
 */

#include "util/u_memory.h"
#include "util/u_string.h"
#include "util/u_inlines.h"
#include "util/u_pointer.h"
#include "util/u_dl.h"
#include "egldriver.h"
#include "eglimage.h"
#include "eglmutex.h"

#include "egl_g3d.h"
#include "egl_g3d_st.h"

struct egl_g3d_st_manager {
   struct st_manager base;
   _EGLDisplay *display;
};

static INLINE struct egl_g3d_st_manager *
egl_g3d_st_manager(struct st_manager *smapi)
{
   return (struct egl_g3d_st_manager *) smapi;
}

static boolean
egl_g3d_st_manager_get_egl_image(struct st_manager *smapi,
                                 void *egl_image,
                                 struct st_egl_image *out)
{
   struct egl_g3d_st_manager *gsmapi = egl_g3d_st_manager(smapi);
   EGLImageKHR handle = (EGLImageKHR) egl_image;
   _EGLImage *img;
   struct egl_g3d_image *gimg;

   /* this is called from state trackers */
   _eglLockMutex(&gsmapi->display->Mutex);

   img = _eglLookupImage(handle, gsmapi->display);
   if (!img) {
      _eglUnlockMutex(&gsmapi->display->Mutex);
      return FALSE;
   }

   gimg = egl_g3d_image(img);

   out->texture = NULL;
   pipe_resource_reference(&out->texture, gimg->texture);
   out->level = gimg->level;
   out->layer = gimg->layer;

   _eglUnlockMutex(&gsmapi->display->Mutex);

   return TRUE;
}

static int
egl_g3d_st_manager_get_param(struct st_manager *smapi,
                             enum st_manager_param param)
{
   return 0;
}

struct st_manager *
egl_g3d_create_st_manager(_EGLDisplay *dpy)
{
   struct egl_g3d_display *gdpy = egl_g3d_display(dpy);
   struct egl_g3d_st_manager *gsmapi;

   gsmapi = CALLOC_STRUCT(egl_g3d_st_manager);
   if (gsmapi) {
      gsmapi->display = dpy;

      gsmapi->base.screen = gdpy->native->screen;
      gsmapi->base.get_egl_image = egl_g3d_st_manager_get_egl_image;
      gsmapi->base.get_param = egl_g3d_st_manager_get_param;
   }

   return &gsmapi->base;;
}

void
egl_g3d_destroy_st_manager(struct st_manager *smapi)
{
   struct egl_g3d_st_manager *gsmapi = egl_g3d_st_manager(smapi);
   FREE(gsmapi);
}

static boolean
egl_g3d_st_framebuffer_flush_front_pbuffer(struct st_framebuffer_iface *stfbi,
                                           enum st_attachment_type statt)
{
   return TRUE;
}

static void
pbuffer_reference_openvg_image(struct egl_g3d_surface *gsurf)
{
   /* TODO */
}

static void
pbuffer_allocate_pbuffer_texture(struct egl_g3d_surface *gsurf)
{
   struct egl_g3d_display *gdpy =
      egl_g3d_display(gsurf->base.Resource.Display);
   struct pipe_screen *screen = gdpy->native->screen;
   struct pipe_resource templ, *ptex;

   memset(&templ, 0, sizeof(templ));
   templ.target = PIPE_TEXTURE_2D;
   templ.last_level = 0;
   templ.width0 = gsurf->base.Width;
   templ.height0 = gsurf->base.Height;
   templ.depth0 = 1;
   templ.array_size = 1;
   templ.format = gsurf->stvis.color_format;
   /* for rendering and binding to texture */
   templ.bind = PIPE_BIND_RENDER_TARGET | PIPE_BIND_SAMPLER_VIEW;

   ptex = screen->resource_create(screen, &templ);
   gsurf->render_texture = ptex;
}

static boolean
egl_g3d_st_framebuffer_validate_pbuffer(struct st_framebuffer_iface *stfbi,
                                        const enum st_attachment_type *statts,
                                        unsigned count,
                                        struct pipe_resource **out)
{
   _EGLSurface *surf = (_EGLSurface *) stfbi->st_manager_private;
   struct egl_g3d_surface *gsurf = egl_g3d_surface(surf);
   unsigned i;

   for (i = 0; i < count; i++) {
      out[i] = NULL;

      if (gsurf->stvis.render_buffer != statts[i])
         continue;

      if (!gsurf->render_texture) {
         switch (gsurf->client_buffer_type) {
         case EGL_NONE:
            pbuffer_allocate_pbuffer_texture(gsurf);
            break;
         case EGL_OPENVG_IMAGE:
            pbuffer_reference_openvg_image(gsurf);
            break;
         default:
            break;
         }

         if (!gsurf->render_texture)
            return FALSE;
      }

      pipe_resource_reference(&out[i], gsurf->render_texture);
   }

   return TRUE;
}

static boolean
egl_g3d_st_framebuffer_flush_front(struct st_framebuffer_iface *stfbi,
                                   enum st_attachment_type statt)
{
   _EGLSurface *surf = (_EGLSurface *) stfbi->st_manager_private;
   struct egl_g3d_surface *gsurf = egl_g3d_surface(surf);
   struct native_present_control ctrl;

   memset(&ctrl, 0, sizeof(ctrl));
   ctrl.natt = NATIVE_ATTACHMENT_FRONT_LEFT;

   return gsurf->native->present(gsurf->native, &ctrl);
}

static boolean 
egl_g3d_st_framebuffer_validate(struct st_framebuffer_iface *stfbi,
                                const enum st_attachment_type *statts,
                                unsigned count,
                                struct pipe_resource **out)
{
   _EGLSurface *surf = (_EGLSurface *) stfbi->st_manager_private;
   struct egl_g3d_surface *gsurf = egl_g3d_surface(surf);
   struct pipe_resource *textures[NUM_NATIVE_ATTACHMENTS];
   uint attachment_mask = 0;
   unsigned i;

   for (i = 0; i < count; i++) {
      int natt;

      switch (statts[i]) {
      case ST_ATTACHMENT_FRONT_LEFT:
         natt = NATIVE_ATTACHMENT_FRONT_LEFT;
         break;
      case ST_ATTACHMENT_BACK_LEFT:
         natt = NATIVE_ATTACHMENT_BACK_LEFT;
         break;
      case ST_ATTACHMENT_FRONT_RIGHT:
         natt = NATIVE_ATTACHMENT_FRONT_RIGHT;
         break;
      case ST_ATTACHMENT_BACK_RIGHT:
         natt = NATIVE_ATTACHMENT_BACK_RIGHT;
         break;
      default:
         natt = -1;
         break;
      }

      if (natt >= 0)
         attachment_mask |= 1 << natt;
   }

   if (!gsurf->native->validate(gsurf->native, attachment_mask,
         &gsurf->sequence_number, textures, &gsurf->base.Width,
         &gsurf->base.Height))
      return FALSE;

   for (i = 0; i < count; i++) {
      struct pipe_resource *tex;
      int natt;

      switch (statts[i]) {
      case ST_ATTACHMENT_FRONT_LEFT:
         natt = NATIVE_ATTACHMENT_FRONT_LEFT;
         break;
      case ST_ATTACHMENT_BACK_LEFT:
         natt = NATIVE_ATTACHMENT_BACK_LEFT;
         break;
      case ST_ATTACHMENT_FRONT_RIGHT:
         natt = NATIVE_ATTACHMENT_FRONT_RIGHT;
         break;
      case ST_ATTACHMENT_BACK_RIGHT:
         natt = NATIVE_ATTACHMENT_BACK_RIGHT;
         break;
      default:
         natt = -1;
         break;
      }

      if (natt >= 0) {
         tex = textures[natt];

         if (statts[i] == stfbi->visual->render_buffer)
            pipe_resource_reference(&gsurf->render_texture, tex);

         if (attachment_mask & (1 << natt)) {
            /* transfer the ownership to the caller */
            out[i] = tex;
            attachment_mask &= ~(1 << natt);
         }
         else {
            /* the attachment is listed more than once */
            pipe_resource_reference(&out[i], tex);
         }
      }
   }

   return TRUE;
}

struct st_framebuffer_iface *
egl_g3d_create_st_framebuffer(_EGLSurface *surf)
{
   struct egl_g3d_surface *gsurf = egl_g3d_surface(surf);
   struct st_framebuffer_iface *stfbi;

   stfbi = CALLOC_STRUCT(st_framebuffer_iface);
   if (!stfbi)
      return NULL;

   stfbi->visual = &gsurf->stvis;
   p_atomic_set(&stfbi->stamp, 1);

   if (gsurf->base.Type != EGL_PBUFFER_BIT) {
      stfbi->flush_front = egl_g3d_st_framebuffer_flush_front;
      stfbi->validate = egl_g3d_st_framebuffer_validate;
   }
   else {
      stfbi->flush_front = egl_g3d_st_framebuffer_flush_front_pbuffer;
      stfbi->validate = egl_g3d_st_framebuffer_validate_pbuffer;
   }
   stfbi->st_manager_private = (void *) &gsurf->base;

   return stfbi;
}

void
egl_g3d_destroy_st_framebuffer(struct st_framebuffer_iface *stfbi)
{
   FREE(stfbi);
}
