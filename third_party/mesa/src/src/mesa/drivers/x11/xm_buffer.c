/*
 * Mesa 3-D graphics library
 * Version:  6.5.2
 *
 * Copyright (C) 1999-2006  Brian Paul   All Rights Reserved.
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


/**
 * \file xm_buffer.h
 * Framebuffer and renderbuffer-related functions.
 */


#include "glxheader.h"
#include "xmesaP.h"
#include "main/imports.h"
#include "main/formats.h"
#include "main/framebuffer.h"
#include "main/renderbuffer.h"
#include "swrast/s_renderbuffer.h"


#define XMESA_RENDERBUFFER 0x1234


#if defined(USE_XSHM) 
static volatile int mesaXErrorFlag = 0;

/**
 * Catches potential Xlib errors.
 */
static int
mesaHandleXError(XMesaDisplay *dpy, XErrorEvent *event)
{
   (void) dpy;
   (void) event;
   mesaXErrorFlag = 1;
   return 0;
}

/**
 * Allocate a shared memory XImage back buffer for the given XMesaBuffer.
 * Return:  GL_TRUE if success, GL_FALSE if error
 */
static GLboolean
alloc_back_shm_ximage(XMesaBuffer b, GLuint width, GLuint height)
{
   /*
    * We have to do a _lot_ of error checking here to be sure we can
    * really use the XSHM extension.  It seems different servers trigger
    * errors at different points if the extension won't work.  Therefore
    * we have to be very careful...
    */
   GC gc;
   int (*old_handler)(XMesaDisplay *, XErrorEvent *);

   if (width == 0 || height == 0) {
      /* this will be true the first time we're called on 'b' */
      return GL_FALSE;
   }

   b->backxrb->ximage = XShmCreateImage(b->xm_visual->display,
                                        b->xm_visual->visinfo->visual,
                                        b->xm_visual->visinfo->depth,
                                        ZPixmap, NULL, &b->shminfo,
                                        width, height);
   if (b->backxrb->ximage == NULL) {
      _mesa_warning(NULL, "alloc_back_buffer: Shared memory error (XShmCreateImage), disabling.\n");
      b->shm = 0;
      return GL_FALSE;
   }

   b->shminfo.shmid = shmget(IPC_PRIVATE, b->backxrb->ximage->bytes_per_line
			     * b->backxrb->ximage->height, IPC_CREAT|0777);
   if (b->shminfo.shmid < 0) {
      _mesa_warning(NULL, "shmget failed while allocating back buffer.\n");
      XDestroyImage(b->backxrb->ximage);
      b->backxrb->ximage = NULL;
      _mesa_warning(NULL, "alloc_back_buffer: Shared memory error (shmget), disabling.\n");
      b->shm = 0;
      return GL_FALSE;
   }

   b->shminfo.shmaddr = b->backxrb->ximage->data
                      = (char*)shmat(b->shminfo.shmid, 0, 0);
   if (b->shminfo.shmaddr == (char *) -1) {
      _mesa_warning(NULL, "shmat() failed while allocating back buffer.\n");
      XDestroyImage(b->backxrb->ximage);
      shmctl(b->shminfo.shmid, IPC_RMID, 0);
      b->backxrb->ximage = NULL;
      _mesa_warning(NULL, "alloc_back_buffer: Shared memory error (shmat), disabling.\n");
      b->shm = 0;
      return GL_FALSE;
   }

   b->shminfo.readOnly = False;
   mesaXErrorFlag = 0;
   old_handler = XSetErrorHandler(mesaHandleXError);
   /* This may trigger the X protocol error we're ready to catch: */
   XShmAttach(b->xm_visual->display, &b->shminfo);
   XSync(b->xm_visual->display, False);

   if (mesaXErrorFlag) {
      /* we are on a remote display, this error is normal, don't print it */
      XFlush(b->xm_visual->display);
      mesaXErrorFlag = 0;
      XDestroyImage(b->backxrb->ximage);
      shmdt(b->shminfo.shmaddr);
      shmctl(b->shminfo.shmid, IPC_RMID, 0);
      b->backxrb->ximage = NULL;
      b->shm = 0;
      (void) XSetErrorHandler(old_handler);
      return GL_FALSE;
   }

   shmctl(b->shminfo.shmid, IPC_RMID, 0); /* nobody else needs it */

   /* Finally, try an XShmPutImage to be really sure the extension works */
   gc = XCreateGC(b->xm_visual->display, b->frontxrb->drawable, 0, NULL);
   XShmPutImage(b->xm_visual->display, b->frontxrb->drawable, gc,
		 b->backxrb->ximage, 0, 0, 0, 0, 1, 1 /*one pixel*/, False);
   XSync(b->xm_visual->display, False);
   XFreeGC(b->xm_visual->display, gc);
   (void) XSetErrorHandler(old_handler);
   if (mesaXErrorFlag) {
      XFlush(b->xm_visual->display);
      mesaXErrorFlag = 0;
      XDestroyImage(b->backxrb->ximage);
      shmdt(b->shminfo.shmaddr);
      shmctl(b->shminfo.shmid, IPC_RMID, 0);
      b->backxrb->ximage = NULL;
      b->shm = 0;
      return GL_FALSE;
   }

   return GL_TRUE;
}
#else
static GLboolean
alloc_back_shm_ximage(XMesaBuffer b, GLuint width, GLuint height)
{
   /* Can't compile XSHM support */
   return GL_FALSE;
}
#endif



/**
 * Setup an off-screen pixmap or Ximage to use as the back buffer.
 * Input:  b - the X/Mesa buffer
 */
static void
alloc_back_buffer(XMesaBuffer b, GLuint width, GLuint height)
{
   if (b->db_mode == BACK_XIMAGE) {
      /* Deallocate the old backxrb->ximage, if any */
      if (b->backxrb->ximage) {
#if defined(USE_XSHM) 
	 if (b->shm) {
	    XShmDetach(b->xm_visual->display, &b->shminfo);
	    XDestroyImage(b->backxrb->ximage);
	    shmdt(b->shminfo.shmaddr);
	 }
	 else
#endif
	   XMesaDestroyImage(b->backxrb->ximage);
	 b->backxrb->ximage = NULL;
      }

      if (width == 0 || height == 0)
         return;

      /* Allocate new back buffer */
      if (b->shm == 0 || !alloc_back_shm_ximage(b, width, height)) {
	 /* Allocate a regular XImage for the back buffer. */
	 b->backxrb->ximage = XCreateImage(b->xm_visual->display,
                                      b->xm_visual->visinfo->visual,
                                      GET_VISUAL_DEPTH(b->xm_visual),
				      ZPixmap, 0,   /* format, offset */
				      NULL,
                                      width, height,
				      8, 0);  /* pad, bytes_per_line */
	 if (!b->backxrb->ximage) {
	    _mesa_warning(NULL, "alloc_back_buffer: XCreateImage failed.\n");
            return;
	 }
         b->backxrb->ximage->data = (char *) MALLOC(b->backxrb->ximage->height
                                        * b->backxrb->ximage->bytes_per_line);
         if (!b->backxrb->ximage->data) {
            _mesa_warning(NULL, "alloc_back_buffer: MALLOC failed.\n");
            XMesaDestroyImage(b->backxrb->ximage);
            b->backxrb->ximage = NULL;
         }
      }
      b->backxrb->pixmap = None;
   }
   else if (b->db_mode == BACK_PIXMAP) {
      /* Free the old back pixmap */
      if (b->backxrb->pixmap) {
         XMesaFreePixmap(b->xm_visual->display, b->backxrb->pixmap);
         b->backxrb->pixmap = 0;
      }

      if (width > 0 && height > 0) {
         /* Allocate new back pixmap */
         b->backxrb->pixmap = XMesaCreatePixmap(b->xm_visual->display,
                                                b->frontxrb->drawable,
                                                width, height,
                                                GET_VISUAL_DEPTH(b->xm_visual));
      }

      b->backxrb->ximage = NULL;
      b->backxrb->drawable = b->backxrb->pixmap;
   }
}


static void
xmesa_delete_renderbuffer(struct gl_context *ctx, struct gl_renderbuffer *rb)
{
   /* XXX Note: the ximage or Pixmap attached to this renderbuffer
    * should probably get freed here, but that's currently done in
    * XMesaDestroyBuffer().
    */
   free(rb);
}


/**
 * Reallocate renderbuffer storage for front color buffer.
 * Called via gl_renderbuffer::AllocStorage()
 */
static GLboolean
xmesa_alloc_front_storage(struct gl_context *ctx, struct gl_renderbuffer *rb,
                          GLenum internalFormat, GLuint width, GLuint height)
{
   struct xmesa_renderbuffer *xrb = xmesa_renderbuffer(rb);

   /* just clear these to be sure we don't accidentally use them */
   xrb->origin2 = NULL;
   xrb->origin3 = NULL;
   xrb->origin4 = NULL;

   /* for the FLIP macro: */
   xrb->bottom = height - 1;

   rb->Width = width;
   rb->Height = height;
   rb->InternalFormat = internalFormat;

   return GL_TRUE;
}


/**
 * Reallocate renderbuffer storage for back color buffer.
 * Called via gl_renderbuffer::AllocStorage()
 */
static GLboolean
xmesa_alloc_back_storage(struct gl_context *ctx, struct gl_renderbuffer *rb,
                         GLenum internalFormat, GLuint width, GLuint height)
{
   struct xmesa_renderbuffer *xrb = xmesa_renderbuffer(rb);

   /* reallocate the back buffer XImage or Pixmap */
   assert(xrb->Parent);
   alloc_back_buffer(xrb->Parent, width, height);

   /* same as front buffer */
   /* XXX why is this here? */
   (void) xmesa_alloc_front_storage(ctx, rb, internalFormat, width, height);

   /* plus... */
   if (xrb->ximage) {
      /* Needed by PIXELADDR2 macro */
      xrb->width2 = xrb->ximage->bytes_per_line / 2;
      xrb->origin2 = (GLushort *) xrb->ximage->data + xrb->width2 * (height - 1);

      /* Needed by PIXELADDR3 macro */
      xrb->width3 = xrb->ximage->bytes_per_line;
      xrb->origin3 = (GLubyte *) xrb->ximage->data + xrb->width3 * (height - 1);

      /* Needed by PIXELADDR4 macro */
      xrb->width4 = xrb->ximage->width;
      xrb->origin4 = (GLuint *) xrb->ximage->data + xrb->width4 * (height - 1);
   }
   else {
      /* out of memory or buffer size is 0 x 0 */
      xrb->width2 = xrb->width3 = xrb->width4 = 0;
      xrb->origin2 = NULL;
      xrb->origin3 = NULL;
      xrb->origin4 = NULL;
   }

   return GL_TRUE;
}


/**
 * Used for allocating front/back renderbuffers for an X window.
 */
struct xmesa_renderbuffer *
xmesa_new_renderbuffer(struct gl_context *ctx, GLuint name,
                       const struct xmesa_visual *xmvis,
                       GLboolean backBuffer)
{
   struct xmesa_renderbuffer *xrb = CALLOC_STRUCT(xmesa_renderbuffer);
   if (xrb) {
      GLuint name = 0;
      _mesa_init_renderbuffer(&xrb->Base.Base, name);

      xrb->Base.Base.Delete = xmesa_delete_renderbuffer;
      if (backBuffer)
         xrb->Base.Base.AllocStorage = xmesa_alloc_back_storage;
      else
         xrb->Base.Base.AllocStorage = xmesa_alloc_front_storage;

      xrb->Base.Base.InternalFormat = GL_RGBA;
      xrb->Base.Base._BaseFormat = GL_RGBA;
      xrb->Base.Base.ClassID = XMESA_RENDERBUFFER;

      switch (xmvis->undithered_pf) {
      case PF_8R8G8B:
         /* This will really only happen for pixmaps.  We'll access the
          * pixmap via a temporary XImage which will be 32bpp.
          */
         xrb->Base.Base.Format = MESA_FORMAT_XRGB8888;
         break;
      case PF_8A8R8G8B:
         xrb->Base.Base.Format = MESA_FORMAT_ARGB8888;
         break;
      case PF_8A8B8G8R:
         xrb->Base.Base.Format = MESA_FORMAT_RGBA8888_REV;
         break;
      case PF_5R6G5B:
         xrb->Base.Base.Format = MESA_FORMAT_RGB565;
         break;
      default:
         _mesa_warning(ctx, "Bad pixel format in xmesa_new_renderbuffer");
         xrb->Base.Base.Format = MESA_FORMAT_ARGB8888;
         break;
      }

      /* only need to set Red/Green/EtcBits fields for user-created RBs */
   }
   return xrb;
}


/**
 * Called via gl_framebuffer::Delete() method when this buffer
 * is _really_ being deleted.
 */
void
xmesa_delete_framebuffer(struct gl_framebuffer *fb)
{
   XMesaBuffer b = XMESA_BUFFER(fb);

   if (b->num_alloced > 0) {
      /* If no other buffer uses this X colormap then free the colors. */
      if (!xmesa_find_buffer(b->display, b->cmap, b)) {
         XFreeColors(b->display, b->cmap,
                     b->alloced_colors, b->num_alloced, 0);
      }
   }

   if (b->gc)
      XMesaFreeGC(b->display, b->gc);
   if (b->cleargc)
      XMesaFreeGC(b->display, b->cleargc);
   if (b->swapgc)
      XMesaFreeGC(b->display, b->swapgc);

   if (fb->Visual.doubleBufferMode) {
      /* free back ximage/pixmap/shmregion */
      if (b->backxrb->ximage) {
#if defined(USE_XSHM) 
         if (b->shm) {
            XShmDetach( b->display, &b->shminfo );
            XDestroyImage( b->backxrb->ximage );
            shmdt( b->shminfo.shmaddr );
         }
         else
#endif
            XMesaDestroyImage( b->backxrb->ximage );
         b->backxrb->ximage = NULL;
      }
      if (b->backxrb->pixmap) {
         XMesaFreePixmap( b->display, b->backxrb->pixmap );
      }
   }

   _mesa_free_framebuffer_data(fb);
   free(fb);
}


/**
 * Called via ctx->Driver.MapRenderbuffer()
 */
void
xmesa_MapRenderbuffer(struct gl_context *ctx,
                      struct gl_renderbuffer *rb,
                      GLuint x, GLuint y, GLuint w, GLuint h,
                      GLbitfield mode,
                      GLubyte **mapOut, GLint *rowStrideOut)
{
   struct xmesa_renderbuffer *xrb = xmesa_renderbuffer(rb);

   if (xrb->Base.Base.ClassID == XMESA_RENDERBUFFER) {
      XImage *ximage = xrb->ximage;

      assert(!xrb->map_mode); /* only a single mapping allowed */

      xrb->map_mode = mode;
      xrb->map_x = x;
      xrb->map_y = y;
      xrb->map_w = w;
      xrb->map_h = h;

      if (ximage) {
         int y2 = rb->Height - y - 1;

         *mapOut = (GLubyte *) ximage->data
            + y2 * ximage->bytes_per_line
            + x * ximage->bits_per_pixel / 8;
      }
      else {
         /* this must be a pixmap/window renderbuffer */
         int (*old_handler)(XMesaDisplay *, XErrorEvent *);
         int y2 = rb->Height - y - h;

         assert(xrb->pixmap);

         /* Install error handler for XGetImage() in case the the window
          * isn't mapped.  If we fail we'll create a temporary XImage.
          */
         mesaXErrorFlag = 0;
         old_handler = XSetErrorHandler(mesaHandleXError);

         /* read pixel data out of the pixmap/window into an XImage */
         ximage = XGetImage(xrb->Parent->display,
                            xrb->pixmap, x, y2, w, h,
                            AllPlanes, ZPixmap);

         XSetErrorHandler(old_handler);

         if (mesaXErrorFlag) {
            /* create new, temporary XImage */
            int bytes_per_line =
               _mesa_format_row_stride(xrb->Base.Base.Format,
                                       xrb->Base.Base.Width);
            char *image = (char *) malloc(bytes_per_line *
                                          xrb->Base.Base.Height);
            ximage = XCreateImage(xrb->Parent->display,
                                  xrb->Parent->xm_visual->visinfo->visual,
                                  xrb->Parent->xm_visual->visinfo->depth,
                                  ZPixmap, /* format */
                                  0, /* offset */
                                  image, /* data */
                                  xrb->Base.Base.Width,
                                  xrb->Base.Base.Height,
                                  8, /* pad */
                                  bytes_per_line);
         }

         if (!ximage) {
            *mapOut = NULL;
            *rowStrideOut = 0;
            return;
         }

         xrb->map_ximage = ximage;

         /* the first row of the OpenGL image is last row of the XImage */
         *mapOut = (GLubyte *) ximage->data
            + (h - 1) * ximage->bytes_per_line;
      }

      /* We return a negative stride here since XImage data is upside down
       * with respect to OpenGL images.
       */
      *rowStrideOut = -ximage->bytes_per_line;
      return;
   }

   /* otherwise, this is an ordinary malloc-based renderbuffer */
   _swrast_map_soft_renderbuffer(ctx, rb, x, y, w, h, mode,
                                 mapOut, rowStrideOut);
}


/**
 * Called via ctx->Driver.UnmapRenderbuffer()
 */
void
xmesa_UnmapRenderbuffer(struct gl_context *ctx, struct gl_renderbuffer *rb)
{
   struct xmesa_renderbuffer *xrb = xmesa_renderbuffer(rb);

   if (xrb->Base.Base.ClassID == XMESA_RENDERBUFFER) {
      XImage *ximage = xrb->ximage;

      if (!ximage) {
         /* this must be a pixmap/window renderbuffer */
         assert(xrb->pixmap);
         assert(xrb->map_ximage);
         if (xrb->map_ximage) {
            if (xrb->map_mode & GL_MAP_WRITE_BIT) {
               /* put modified ximage data back into the pixmap/window */
               int y2 = rb->Height - xrb->map_y - xrb->map_h;
               GC gc = XCreateGC(xrb->Parent->display, xrb->pixmap, 0, NULL);

               XPutImage(xrb->Parent->display,
                         xrb->pixmap,              /* dest */
                         gc,
                         xrb->map_ximage,          /* source */
                         0, 0,                     /* src x, y */
                         xrb->map_x, y2,           /* dest x, y */
                         xrb->map_w, xrb->map_h);  /* size */

               XFreeGC(xrb->Parent->display, gc);
            }
            XMesaDestroyImage(xrb->map_ximage);
            xrb->map_ximage = NULL;
         }
      }

      xrb->map_mode = 0x0;

      return;
   }

   /* otherwise, this is an ordinary malloc-based renderbuffer */
   _swrast_unmap_soft_renderbuffer(ctx, rb);
}


