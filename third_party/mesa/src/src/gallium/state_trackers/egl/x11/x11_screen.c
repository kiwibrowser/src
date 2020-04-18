/*
 * Mesa 3-D graphics library
 * Version:  7.8
 *
 * Copyright (C) 2009-2010 Chia-I Wu <olv@0xlab.org>
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
 */

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <xf86drm.h>
#include <X11/Xlibint.h>
#include <X11/extensions/XShm.h>

#include "util/u_memory.h"
#include "egllog.h"

#include "x11_screen.h"
#include "dri2.h"
#include "glxinit.h"

struct x11_screen {
   Display *dpy;
   int number;

   /*
    * This is used to fetch GLX visuals/fbconfigs.  It steals code from GLX.
    * It might be better to rewrite the part in Xlib or XCB.
    */
   __GLXdisplayPrivate *glx_dpy;

   int dri_major, dri_minor;
   char *dri_driver;
   char *dri_device;
   int dri_fd;

   x11_drawable_invalidate_buffers dri_invalidate_buffers;
   void *dri_user_data;

   XVisualInfo *visuals;
   int num_visuals;

   /* cached values for x11_drawable_get_depth */
   Drawable last_drawable;
   unsigned int last_depth;
};


/**
 * Create a X11 screen.
 */
struct x11_screen *
x11_screen_create(Display *dpy, int screen)
{
   struct x11_screen *xscr;

   if (screen >= ScreenCount(dpy))
      return NULL;

   xscr = CALLOC_STRUCT(x11_screen);
   if (xscr) {
      xscr->dpy = dpy;
      xscr->number = screen;

      xscr->dri_major = -1;
      xscr->dri_fd = -1;
   }
   return xscr;
}

/**
 * Destroy a X11 screen.
 */
void
x11_screen_destroy(struct x11_screen *xscr)
{
   if (xscr->dri_fd >= 0)
      close(xscr->dri_fd);
   if (xscr->dri_driver)
      Xfree(xscr->dri_driver);
   if (xscr->dri_device)
      Xfree(xscr->dri_device);

#ifdef GLX_DIRECT_RENDERING
   /* xscr->glx_dpy will be destroyed with the X display */
   if (xscr->glx_dpy)
      xscr->glx_dpy->xscr = NULL;
#endif

   if (xscr->visuals)
      XFree(xscr->visuals);
   FREE(xscr);
}

#ifdef GLX_DIRECT_RENDERING

static boolean
x11_screen_init_dri2(struct x11_screen *xscr)
{
   if (xscr->dri_major < 0) {
      int eventBase, errorBase;

      if (!DRI2QueryExtension(xscr->dpy, &eventBase, &errorBase) ||
          !DRI2QueryVersion(xscr->dpy, &xscr->dri_major, &xscr->dri_minor))
         xscr->dri_major = -1;
   }
   return (xscr->dri_major >= 0);
}

static boolean
x11_screen_init_glx(struct x11_screen *xscr)
{
   if (!xscr->glx_dpy)
      xscr->glx_dpy = __glXInitialize(xscr->dpy);
   return (xscr->glx_dpy != NULL);
}

#endif /* GLX_DIRECT_RENDERING */

/**
 * Return true if the screen supports the extension.
 */
boolean
x11_screen_support(struct x11_screen *xscr, enum x11_screen_extension ext)
{
   boolean supported = FALSE;

   switch (ext) {
   case X11_SCREEN_EXTENSION_XSHM:
      supported = XShmQueryExtension(xscr->dpy);
      break;
#ifdef GLX_DIRECT_RENDERING
   case X11_SCREEN_EXTENSION_GLX:
      supported = x11_screen_init_glx(xscr);
      break;
   case X11_SCREEN_EXTENSION_DRI2:
      supported = x11_screen_init_dri2(xscr);
      break;
#endif
   default:
      break;
   }

   return supported;
}

/**
 * Return the X visuals.
 */
const XVisualInfo *
x11_screen_get_visuals(struct x11_screen *xscr, int *num_visuals)
{
   if (!xscr->visuals) {
      XVisualInfo vinfo_template;
      vinfo_template.screen = xscr->number;
      xscr->visuals = XGetVisualInfo(xscr->dpy, VisualScreenMask,
            &vinfo_template, &xscr->num_visuals);
   }

   if (num_visuals)
      *num_visuals = xscr->num_visuals;
   return xscr->visuals;
}

/**
 * Return the depth of a drawable.
 *
 * Unlike other drawable functions, the drawable needs not be a DRI2 drawable.
 */
uint
x11_drawable_get_depth(struct x11_screen *xscr, Drawable drawable)
{
   unsigned int depth;

   if (drawable != xscr->last_drawable) {
      Window root;
      int x, y;
      unsigned int w, h, border;
      Status ok;

      ok = XGetGeometry(xscr->dpy, drawable, &root,
            &x, &y, &w, &h, &border, &depth);
      if (!ok)
         depth = 0;

      xscr->last_drawable = drawable;
      xscr->last_depth = depth;
   }
   else {
      depth = xscr->last_depth;
   }

   return depth;
}

#ifdef GLX_DIRECT_RENDERING

/**
 * Return the GLX fbconfigs.
 */
const __GLcontextModes *
x11_screen_get_glx_configs(struct x11_screen *xscr)
{
   return (x11_screen_init_glx(xscr))
      ? xscr->glx_dpy->screenConfigs[xscr->number]->configs
      : NULL;
}

/**
 * Probe the screen for the DRI2 driver name.
 */
const char *
x11_screen_probe_dri2(struct x11_screen *xscr, int *major, int *minor)
{
   if (!x11_screen_init_dri2(xscr))
      return NULL;

   /* get the driver name and the device name */
   if (!xscr->dri_driver) {
      if (!DRI2Connect(xscr->dpy, RootWindow(xscr->dpy, xscr->number),
               &xscr->dri_driver, &xscr->dri_device))
         xscr->dri_driver = xscr->dri_device = NULL;
   }
   if (major)
      *major = xscr->dri_major;
   if (minor)
      *minor = xscr->dri_minor;

   return xscr->dri_driver;
}

/**
 * Enable DRI2 and returns the file descriptor of the DRM device.  The file
 * descriptor will be closed automatically when the screen is destoryed.
 */
int
x11_screen_enable_dri2(struct x11_screen *xscr,
                       x11_drawable_invalidate_buffers invalidate_buffers,
                       void *user_data)
{
   if (xscr->dri_fd < 0) {
      int fd;
      drm_magic_t magic;

      /* get the driver name and the device name first */
      if (!x11_screen_probe_dri2(xscr, NULL, NULL))
         return -1;

#ifdef O_CLOEXEC
      fd = open(xscr->dri_device, O_RDWR | O_CLOEXEC);
      if (fd == -1 && errno == EINVAL)
#endif
      {
         fd = open(xscr->dri_device, O_RDWR);
         if (fd != -1)
            fcntl(fd, F_SETFD, fcntl(fd, F_GETFD) | FD_CLOEXEC);
      }
      if (fd < 0) {
         _eglLog(_EGL_WARNING, "failed to open %s", xscr->dri_device);
         return -1;
      }

      memset(&magic, 0, sizeof(magic));
      if (drmGetMagic(fd, &magic)) {
         _eglLog(_EGL_WARNING, "failed to get magic");
         close(fd);
         return -1;
      }

      if (!DRI2Authenticate(xscr->dpy,
               RootWindow(xscr->dpy, xscr->number), magic)) {
         _eglLog(_EGL_WARNING, "failed to authenticate magic");
         close(fd);
         return -1;
      }

      if (!x11_screen_init_glx(xscr)) {
         _eglLog(_EGL_WARNING, "failed to initialize GLX");
         close(fd);
         return -1;
      }
      if (xscr->glx_dpy->xscr) {
         _eglLog(_EGL_WARNING,
               "display is already managed by another x11 screen");
         close(fd);
         return -1;
      }

      xscr->glx_dpy->xscr = xscr;
      xscr->dri_invalidate_buffers = invalidate_buffers;
      xscr->dri_user_data = user_data;

      xscr->dri_fd = fd;
   }

   return xscr->dri_fd;
}

char *
x11_screen_get_device_name(struct x11_screen *xscr)
{
   return xscr->dri_device;
}

int
x11_screen_authenticate(struct x11_screen *xscr, uint32_t id)
{
   boolean authenticated;

   authenticated = DRI2Authenticate(xscr->dpy,
         RootWindow(xscr->dpy, xscr->number), id);
   
   return authenticated ? 0 : -1;
}

/**
 * Create/Destroy the DRI drawable.
 */
void
x11_drawable_enable_dri2(struct x11_screen *xscr,
                         Drawable drawable, boolean on)
{
   if (on)
      DRI2CreateDrawable(xscr->dpy, drawable);
   else
      DRI2DestroyDrawable(xscr->dpy, drawable);
}

/**
 * Copy between buffers of the DRI2 drawable.
 */
void
x11_drawable_copy_buffers_region(struct x11_screen *xscr, Drawable drawable,
                                 int num_rects, const int *rects,
                                 int src_buf, int dst_buf)
{
   XserverRegion region;
   XRectangle *rectangles = CALLOC(num_rects, sizeof(XRectangle));

   for (int i = 0; i < num_rects; i++) {
      rectangles[i].x = rects[i * 4 + 0];
      rectangles[i].y = rects[i * 4 + 1];
      rectangles[i].width = rects[i * 4 + 2];
      rectangles[i].height = rects[i * 4 + 3];
   }

   region = XFixesCreateRegion(xscr->dpy, rectangles, num_rects);
   DRI2CopyRegion(xscr->dpy, drawable, region, dst_buf, src_buf);
   XFixesDestroyRegion(xscr->dpy, region);
   FREE(rectangles);
}

/**
 * Get the buffers of the DRI2 drawable.  The returned array should be freed.
 */
struct x11_drawable_buffer *
x11_drawable_get_buffers(struct x11_screen *xscr, Drawable drawable,
                         int *width, int *height, unsigned int *attachments,
                         boolean with_format, int num_ins, int *num_outs)
{
   DRI2Buffer *dri2bufs;

   if (with_format)
      dri2bufs = DRI2GetBuffersWithFormat(xscr->dpy, drawable, width, height,
            attachments, num_ins, num_outs);
   else
      dri2bufs = DRI2GetBuffers(xscr->dpy, drawable, width, height,
            attachments, num_ins, num_outs);

   return (struct x11_drawable_buffer *) dri2bufs;
}

/**
 * Create a mode list of the given size.
 */
__GLcontextModes *
x11_context_modes_create(unsigned count)
{
   const size_t size = sizeof(__GLcontextModes);
   __GLcontextModes *base = NULL;
   __GLcontextModes **next;
   unsigned i;

   next = &base;
   for (i = 0; i < count; i++) {
      *next = (__GLcontextModes *) CALLOC(1, size);
      if (*next == NULL) {
         x11_context_modes_destroy(base);
         base = NULL;
         break;
      }
      next = &((*next)->next);
   }

   return base;
}

/**
 * Destroy a mode list.
 */
void
x11_context_modes_destroy(__GLcontextModes *modes)
{
   while (modes != NULL) {
      __GLcontextModes *next = modes->next;
      FREE(modes);
      modes = next;
   }
}

/**
 * Return the number of the modes in the mode list.
 */
unsigned
x11_context_modes_count(const __GLcontextModes *modes)
{
   const __GLcontextModes *mode;
   int count = 0;
   for (mode = modes; mode; mode = mode->next)
      count++;
   return count;
}

extern void
dri2InvalidateBuffers(Display *dpy, XID drawable);

/**
 * This is called from src/glx/dri2.c.
 */
void
dri2InvalidateBuffers(Display *dpy, XID drawable)
{
   __GLXdisplayPrivate *priv = __glXInitialize(dpy);
   struct x11_screen *xscr = NULL;

   if (priv && priv->xscr)
      xscr = priv->xscr;
   if (!xscr || !xscr->dri_invalidate_buffers)
      return;

   xscr->dri_invalidate_buffers(xscr, drawable, xscr->dri_user_data);
}

extern unsigned
dri2GetSwapEventType(Display *dpy, XID drawable);

extern void *
dri2GetGlxDrawableFromXDrawableId(Display *dpy, XID id);

extern void *
GetGLXDrawable(Display *dpy, XID drawable);

/**
 * This is also called from src/glx/dri2.c.
 */
unsigned dri2GetSwapEventType(Display *dpy, XID drawable)
{
   return 0;
}

void *
dri2GetGlxDrawableFromXDrawableId(Display *dpy, XID id)
{
   return NULL;
}

void *
GetGLXDrawable(Display *dpy, XID drawable)
{
   return NULL;
}

#endif /* GLX_DIRECT_RENDERING */
