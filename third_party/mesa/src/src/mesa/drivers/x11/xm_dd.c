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
 * \file xm_dd.h
 * General device driver functions for Xlib driver.
 */

#include "glxheader.h"
#include "main/bufferobj.h"
#include "main/context.h"
#include "main/colormac.h"
#include "main/fbobject.h"
#include "main/macros.h"
#include "main/mipmap.h"
#include "main/image.h"
#include "main/imports.h"
#include "main/mtypes.h"
#include "main/pbo.h"
#include "main/texformat.h"
#include "swrast/swrast.h"
#include "swrast/s_context.h"
#include "swrast_setup/swrast_setup.h"
#include "tnl/tnl.h"
#include "tnl/t_context.h"
#include "drivers/common/meta.h"
#include "xmesaP.h"


static void
finish_or_flush( struct gl_context *ctx )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   if (xmesa) {
      _glthread_LOCK_MUTEX(_xmesa_lock);
      XSync( xmesa->display, False );
      _glthread_UNLOCK_MUTEX(_xmesa_lock);
   }
}


/* Implements glColorMask() */
static void
color_mask(struct gl_context *ctx,
           GLboolean rmask, GLboolean gmask, GLboolean bmask, GLboolean amask)
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaBuffer xmbuf;
   const int xclass = xmesa->xm_visual->visualType;
   (void) amask;

   if (_mesa_is_user_fbo(ctx->DrawBuffer))
      return;

   xmbuf = XMESA_BUFFER(ctx->DrawBuffer);

   if (xclass == GLX_TRUE_COLOR || xclass == GLX_DIRECT_COLOR) {
      unsigned long m;
      if (rmask && gmask && bmask) {
         m = ((unsigned long)~0L);
      }
      else {
         m = 0;
         if (rmask)   m |= GET_REDMASK(xmesa->xm_visual);
         if (gmask)   m |= GET_GREENMASK(xmesa->xm_visual);
         if (bmask)   m |= GET_BLUEMASK(xmesa->xm_visual);
      }
      XMesaSetPlaneMask( xmesa->display, xmbuf->cleargc, m );
   }
}



/**********************************************************************/
/*** glClear implementations                                        ***/
/**********************************************************************/


/**
 * Clear the front or back color buffer, if it's implemented with a pixmap.
 */
static void
clear_pixmap(struct gl_context *ctx, struct xmesa_renderbuffer *xrb,
             GLint x, GLint y, GLint width, GLint height)
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaBuffer xmbuf = XMESA_BUFFER(ctx->DrawBuffer);

   assert(xmbuf);
   assert(xrb->pixmap);
   assert(xmesa);
   assert(xmesa->display);
   assert(xrb->pixmap);
   assert(xmbuf->cleargc);

   XMesaFillRectangle( xmesa->display, xrb->pixmap, xmbuf->cleargc,
                       x, xrb->Base.Base.Height - y - height,
                       width, height );
}


static void
clear_16bit_ximage( struct gl_context *ctx, struct xmesa_renderbuffer *xrb,
                    GLint x, GLint y, GLint width, GLint height)
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   GLuint pixel = (GLuint) xmesa->clearpixel;
   GLint i, j;

   if (xmesa->swapbytes) {
      pixel = ((pixel >> 8) & 0x00ff) | ((pixel << 8) & 0xff00);
   }

   for (j = 0; j < height; j++) {
      GLushort *ptr2 = PIXEL_ADDR2(xrb, x, y + j);
      for (i = 0; i < width; i++) {
         ptr2[i] = pixel;
      }
   }
}


/* Optimized code provided by Nozomi Ytow <noz@xfree86.org> */
static void
clear_24bit_ximage(struct gl_context *ctx, struct xmesa_renderbuffer *xrb,
                   GLint x, GLint y, GLint width, GLint height)
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   const GLubyte r = xmesa->clearcolor[0];
   const GLubyte g = xmesa->clearcolor[1];
   const GLubyte b = xmesa->clearcolor[2];

   if (r == g && g == b) {
      /* same value for all three components (gray) */
      GLint j;
      for (j = 0; j < height; j++) {
         bgr_t *ptr3 = PIXEL_ADDR3(xrb, x, y + j);
         memset(ptr3, r, 3 * width);
      }
   }
   else {
      /* non-gray clear color */
      GLint i, j;
      for (j = 0; j < height; j++) {
         bgr_t *ptr3 = PIXEL_ADDR3(xrb, x, y + j);
         for (i = 0; i < width; i++) {
            ptr3->r = r;
            ptr3->g = g;
            ptr3->b = b;
            ptr3++;
         }
      }
   }
}


static void
clear_32bit_ximage(struct gl_context *ctx, struct xmesa_renderbuffer *xrb,
                   GLint x, GLint y, GLint width, GLint height)
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   register GLuint pixel = (GLuint) xmesa->clearpixel;

   if (!xrb->ximage)
      return;

   if (xmesa->swapbytes) {
      pixel = ((pixel >> 24) & 0x000000ff)
            | ((pixel >> 8)  & 0x0000ff00)
            | ((pixel << 8)  & 0x00ff0000)
            | ((pixel << 24) & 0xff000000);
   }

   if (width == xrb->Base.Base.Width && height == xrb->Base.Base.Height) {
      /* clearing whole buffer */
      const GLuint n = xrb->Base.Base.Width * xrb->Base.Base.Height;
      GLuint *ptr4 = (GLuint *) xrb->ximage->data;
      if (pixel == 0) {
         /* common case */
         memset(ptr4, pixel, 4 * n);
      }
      else {
         GLuint i;
         for (i = 0; i < n; i++)
            ptr4[i] = pixel;
      }
   }
   else {
      /* clearing scissored region */
      GLint i, j;
      for (j = 0; j < height; j++) {
         GLuint *ptr4 = PIXEL_ADDR4(xrb, x, y + j);
         for (i = 0; i < width; i++) {
            ptr4[i] = pixel;
         }
      }
   }
}


static void
clear_nbit_ximage(struct gl_context *ctx, struct xmesa_renderbuffer *xrb,
                  GLint x, GLint y, GLint width, GLint height)
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   XMesaImage *img = xrb->ximage;
   GLint i, j;

   /* TODO: optimize this */
   y = YFLIP(xrb, y);
   for (j = 0; j < height; j++) {
      for (i = 0; i < width; i++) {
         XMesaPutPixel(img, x+i, y-j, xmesa->clearpixel);
      }
   }
}



static void
clear_buffers(struct gl_context *ctx, GLbitfield buffers)
{
   if (_mesa_is_winsys_fbo(ctx->DrawBuffer)) {
      /* this is a window system framebuffer */
      const GLuint *colorMask = (GLuint *) &ctx->Color.ColorMask[0];
      const XMesaContext xmesa = XMESA_CONTEXT(ctx);
      XMesaBuffer b = XMESA_BUFFER(ctx->DrawBuffer);
      const GLint x = ctx->DrawBuffer->_Xmin;
      const GLint y = ctx->DrawBuffer->_Ymin;
      const GLint width = ctx->DrawBuffer->_Xmax - x;
      const GLint height = ctx->DrawBuffer->_Ymax - y;

      _mesa_unclamped_float_rgba_to_ubyte(xmesa->clearcolor,
                                          ctx->Color.ClearColor.f);
      xmesa->clearpixel = xmesa_color_to_pixel(ctx,
                                               xmesa->clearcolor[0],
                                               xmesa->clearcolor[1],
                                               xmesa->clearcolor[2],
                                               xmesa->clearcolor[3],
                                               xmesa->xm_visual->undithered_pf);
      XMesaSetForeground(xmesa->display, b->cleargc, xmesa->clearpixel);

      /* we can't handle color or index masking */
      if (*colorMask == 0xffffffff && ctx->Color.IndexMask == 0xffffffff) {
         if (buffers & BUFFER_BIT_FRONT_LEFT) {
            /* clear front color buffer */
            struct gl_renderbuffer *frontRb
               = ctx->DrawBuffer->Attachment[BUFFER_FRONT_LEFT].Renderbuffer;
            if (b->frontxrb == xmesa_renderbuffer(frontRb)) {
               /* renderbuffer is not wrapped - great! */
               b->frontxrb->clearFunc(ctx, b->frontxrb, x, y, width, height);
               buffers &= ~BUFFER_BIT_FRONT_LEFT;
            }
            else {
               /* we can't directly clear an alpha-wrapped color buffer */
            }
         }
         if (buffers & BUFFER_BIT_BACK_LEFT) {
            /* clear back color buffer */
            struct gl_renderbuffer *backRb
               = ctx->DrawBuffer->Attachment[BUFFER_BACK_LEFT].Renderbuffer;
            if (b->backxrb == xmesa_renderbuffer(backRb)) {
               /* renderbuffer is not wrapped - great! */
               b->backxrb->clearFunc(ctx, b->backxrb, x, y, width, height);
               buffers &= ~BUFFER_BIT_BACK_LEFT;
            }
         }
      }
   }
   if (buffers)
      _swrast_Clear(ctx, buffers);
}


/* XXX these functions haven't been tested in the Xserver environment */


/**
 * Check if we can do an optimized glDrawPixels into an 8R8G8B visual.
 */
static GLboolean
can_do_DrawPixels_8R8G8B(struct gl_context *ctx, GLenum format, GLenum type)
{
   if (format == GL_BGRA &&
       type == GL_UNSIGNED_BYTE &&
       ctx->DrawBuffer &&
       _mesa_is_winsys_fbo(ctx->DrawBuffer) &&
       ctx->Pixel.ZoomX == 1.0 &&        /* no zooming */
       ctx->Pixel.ZoomY == 1.0 &&
       ctx->_ImageTransferState == 0 /* no color tables, scale/bias, etc */) {
      const SWcontext *swrast = SWRAST_CONTEXT(ctx);

      if (swrast->NewState)
         _swrast_validate_derived( ctx );
      
      if ((swrast->_RasterMask & ~CLIP_BIT) == 0) /* no blend, z-test, etc */ {
         struct gl_renderbuffer *rb = ctx->DrawBuffer->_ColorDrawBuffers[0];
         if (rb) {
            struct xmesa_renderbuffer *xrb = xmesa_renderbuffer(rb);
            if (xrb &&
                xrb->pixmap && /* drawing to pixmap or window */
                _mesa_get_format_bits(xrb->Base.Base.Format, GL_ALPHA_BITS) == 0) {
               return GL_TRUE;
            }
         }
      }
   }
   return GL_FALSE;
}


/**
 * This function implements glDrawPixels() with an XPutImage call when
 * drawing to the front buffer (X Window drawable).
 * The image format must be GL_BGRA to match the PF_8R8G8B pixel format.
 */
static void
xmesa_DrawPixels_8R8G8B( struct gl_context *ctx,
                         GLint x, GLint y, GLsizei width, GLsizei height,
                         GLenum format, GLenum type,
                         const struct gl_pixelstore_attrib *unpack,
                         const GLvoid *pixels )
{
   if (can_do_DrawPixels_8R8G8B(ctx, format, type)) {
      const SWcontext *swrast = SWRAST_CONTEXT( ctx );
      struct gl_pixelstore_attrib clippedUnpack = *unpack;
      int dstX = x;
      int dstY = y;
      int w = width;
      int h = height;

      if (swrast->NewState)
         _swrast_validate_derived( ctx );

      if (_mesa_is_bufferobj(unpack->BufferObj)) {
         /* unpack from PBO */
         GLubyte *buf;
         if (!_mesa_validate_pbo_access(2, unpack, width, height, 1,
                                        format, type, INT_MAX, pixels)) {
            _mesa_error(ctx, GL_INVALID_OPERATION,
                        "glDrawPixels(invalid PBO access)");
            return;
         }
         buf = (GLubyte *) ctx->Driver.MapBufferRange(ctx, 0,
						      unpack->BufferObj->Size,
						      GL_MAP_READ_BIT,
						      unpack->BufferObj);
         if (!buf) {
            /* buffer is already mapped - that's an error */
            _mesa_error(ctx, GL_INVALID_OPERATION,
                        "glDrawPixels(PBO is mapped)");
            return;
         }
         pixels = ADD_POINTERS(buf, pixels);
      }

      if (_mesa_clip_drawpixels(ctx, &dstX, &dstY, &w, &h, &clippedUnpack)) {
         const XMesaContext xmesa = XMESA_CONTEXT(ctx);
         XMesaDisplay *dpy = xmesa->xm_visual->display;
         XMesaBuffer xmbuf = XMESA_BUFFER(ctx->DrawBuffer);
         const XMesaGC gc = xmbuf->cleargc;  /* effected by glColorMask */
         struct xmesa_renderbuffer *xrb
            = xmesa_renderbuffer(ctx->DrawBuffer->_ColorDrawBuffers[0]);
         const int srcX = clippedUnpack.SkipPixels;
         const int srcY = clippedUnpack.SkipRows;
         const int rowLength = clippedUnpack.RowLength;
         XMesaImage ximage;

         ASSERT(xmesa->xm_visual->dithered_pf == PF_8R8G8B);
         ASSERT(xmesa->xm_visual->undithered_pf == PF_8R8G8B);
         ASSERT(dpy);
         ASSERT(gc);

         /* This is a little tricky since all coordinates up to now have
          * been in the OpenGL bottom-to-top orientation.  X is top-to-bottom
          * so we have to carefully compute the Y coordinates/addresses here.
          */
         memset(&ximage, 0, sizeof(XMesaImage));
         ximage.width = width;
         ximage.height = height;
         ximage.format = ZPixmap;
         ximage.data = (char *) pixels
            + ((srcY + h - 1) * rowLength + srcX) * 4;
         ximage.byte_order = LSBFirst;
         ximage.bitmap_unit = 32;
         ximage.bitmap_bit_order = LSBFirst;
         ximage.bitmap_pad = 32;
         ximage.depth = 32;
         ximage.bits_per_pixel = 32;
         ximage.bytes_per_line = -rowLength * 4; /* negative to flip image */
         /* it seems we don't need to set the ximage.red/green/blue_mask fields */
         /* flip Y axis for dest position */
         dstY = YFLIP(xrb, dstY) - h + 1;
         XPutImage(dpy, xrb->pixmap, gc, &ximage, 0, 0, dstX, dstY, w, h);
      }

      if (_mesa_is_bufferobj(unpack->BufferObj)) {
         ctx->Driver.UnmapBuffer(ctx, unpack->BufferObj);
      }
   }
   else {
      /* software fallback */
      _swrast_DrawPixels(ctx, x, y, width, height,
                         format, type, unpack, pixels);
   }
}



/**
 * Check if we can do an optimized glDrawPixels into an 5R6G5B visual.
 */
static GLboolean
can_do_DrawPixels_5R6G5B(struct gl_context *ctx, GLenum format, GLenum type)
{
   if (format == GL_RGB &&
       type == GL_UNSIGNED_SHORT_5_6_5 &&
       !ctx->Color.DitherFlag &&  /* no dithering */
       ctx->DrawBuffer &&
       _mesa_is_winsys_fbo(ctx->DrawBuffer) &&
       ctx->Pixel.ZoomX == 1.0 &&        /* no zooming */
       ctx->Pixel.ZoomY == 1.0 &&
       ctx->_ImageTransferState == 0 /* no color tables, scale/bias, etc */) {
      const SWcontext *swrast = SWRAST_CONTEXT(ctx);

      if (swrast->NewState)
         _swrast_validate_derived( ctx );
      
      if ((swrast->_RasterMask & ~CLIP_BIT) == 0) /* no blend, z-test, etc */ {
         struct gl_renderbuffer *rb = ctx->DrawBuffer->_ColorDrawBuffers[0];
         if (rb) {
            struct xmesa_renderbuffer *xrb = xmesa_renderbuffer(rb);
            if (xrb &&
                xrb->pixmap && /* drawing to pixmap or window */
                _mesa_get_format_bits(xrb->Base.Base.Format, GL_ALPHA_BITS) == 0) {
               return GL_TRUE;
            }
         }
      }
   }
   return GL_FALSE;
}


/**
 * This function implements glDrawPixels() with an XPutImage call when
 * drawing to the front buffer (X Window drawable).  The image format
 * must be GL_RGB and image type must be GL_UNSIGNED_SHORT_5_6_5 to
 * match the PF_5R6G5B pixel format.
 */
static void
xmesa_DrawPixels_5R6G5B( struct gl_context *ctx,
                         GLint x, GLint y, GLsizei width, GLsizei height,
                         GLenum format, GLenum type,
                         const struct gl_pixelstore_attrib *unpack,
                         const GLvoid *pixels )
{
   if (can_do_DrawPixels_5R6G5B(ctx, format, type)) {
      const SWcontext *swrast = SWRAST_CONTEXT( ctx );
      struct gl_pixelstore_attrib clippedUnpack = *unpack;
      int dstX = x;
      int dstY = y;
      int w = width;
      int h = height;

      if (swrast->NewState)
         _swrast_validate_derived( ctx );
      
      if (_mesa_is_bufferobj(unpack->BufferObj)) {
         /* unpack from PBO */
         GLubyte *buf;
         if (!_mesa_validate_pbo_access(2, unpack, width, height, 1,
                                        format, type, INT_MAX, pixels)) {
            _mesa_error(ctx, GL_INVALID_OPERATION,
                        "glDrawPixels(invalid PBO access)");
            return;
         }
         buf = (GLubyte *) ctx->Driver.MapBufferRange(ctx, 0,
						      unpack->BufferObj->Size,
						      GL_MAP_READ_BIT,
						      unpack->BufferObj);
         if (!buf) {
            /* buffer is already mapped - that's an error */
            _mesa_error(ctx, GL_INVALID_OPERATION,
                        "glDrawPixels(PBO is mapped)");
            return;
         }
         pixels = ADD_POINTERS(buf, pixels);
      }

      if (_mesa_clip_drawpixels(ctx, &dstX, &dstY, &w, &h, &clippedUnpack)) {
         const XMesaContext xmesa = XMESA_CONTEXT(ctx);
         XMesaDisplay *dpy = xmesa->xm_visual->display;
         XMesaBuffer xmbuf = XMESA_BUFFER(ctx->DrawBuffer);
         const XMesaGC gc = xmbuf->cleargc;  /* effected by glColorMask */
         struct xmesa_renderbuffer *xrb
            = xmesa_renderbuffer(ctx->DrawBuffer->_ColorDrawBuffers[0]);
         const int srcX = clippedUnpack.SkipPixels;
         const int srcY = clippedUnpack.SkipRows;
         const int rowLength = clippedUnpack.RowLength;
         XMesaImage ximage;

         ASSERT(xmesa->xm_visual->undithered_pf == PF_5R6G5B);
         ASSERT(dpy);
         ASSERT(gc);

         /* This is a little tricky since all coordinates up to now have
          * been in the OpenGL bottom-to-top orientation.  X is top-to-bottom
          * so we have to carefully compute the Y coordinates/addresses here.
          */
         memset(&ximage, 0, sizeof(XMesaImage));
         ximage.width = width;
         ximage.height = height;
         ximage.format = ZPixmap;
         ximage.data = (char *) pixels
            + ((srcY + h - 1) * rowLength + srcX) * 2;
         ximage.byte_order = LSBFirst;
         ximage.bitmap_unit = 16;
         ximage.bitmap_bit_order = LSBFirst;
         ximage.bitmap_pad = 16;
         ximage.depth = 16;
         ximage.bits_per_pixel = 16;
         ximage.bytes_per_line = -rowLength * 2; /* negative to flip image */
         /* it seems we don't need to set the ximage.red/green/blue_mask fields */
         /* flip Y axis for dest position */
         dstY = YFLIP(xrb, dstY) - h + 1;
         XPutImage(dpy, xrb->pixmap, gc, &ximage, 0, 0, dstX, dstY, w, h);
      }

      if (unpack->BufferObj->Name) {
         ctx->Driver.UnmapBuffer(ctx, unpack->BufferObj);
      }
   }
   else {
      /* software fallback */
      _swrast_DrawPixels(ctx, x, y, width, height,
                         format, type, unpack, pixels);
   }
}


/**
 * Determine if we can do an optimized glCopyPixels.
 */
static GLboolean
can_do_CopyPixels(struct gl_context *ctx, GLenum type)
{
   if (type == GL_COLOR &&
       ctx->_ImageTransferState == 0 &&  /* no color tables, scale/bias, etc */
       ctx->Pixel.ZoomX == 1.0 &&        /* no zooming */
       ctx->Pixel.ZoomY == 1.0 &&
       ctx->Color.DrawBuffer[0] == GL_FRONT &&  /* copy to front buf */
       ctx->Pixel.ReadBuffer == GL_FRONT &&    /* copy from front buf */
       ctx->ReadBuffer->_ColorReadBuffer &&
       ctx->DrawBuffer->_ColorDrawBuffers[0]) {
      const SWcontext *swrast = SWRAST_CONTEXT( ctx );

      if (swrast->NewState)
         _swrast_validate_derived( ctx );

      if ((swrast->_RasterMask & ~CLIP_BIT) == 0x0 &&
          ctx->ReadBuffer &&
          ctx->ReadBuffer->_ColorReadBuffer &&
          ctx->DrawBuffer &&
          ctx->DrawBuffer->_ColorDrawBuffers[0]) {
         struct xmesa_renderbuffer *srcXrb
            = xmesa_renderbuffer(ctx->ReadBuffer->_ColorReadBuffer);
         struct xmesa_renderbuffer *dstXrb
            = xmesa_renderbuffer(ctx->DrawBuffer->_ColorDrawBuffers[0]);
         if (srcXrb->pixmap && dstXrb->pixmap) {
            return GL_TRUE;
         }
      }
   }
   return GL_FALSE;
}


/**
 * Implement glCopyPixels for the front color buffer (or back buffer Pixmap)
 * for the color buffer.  Don't support zooming, pixel transfer, etc.
 * We do support copying from one window to another, ala glXMakeCurrentRead.
 */
static void
xmesa_CopyPixels( struct gl_context *ctx,
                  GLint srcx, GLint srcy, GLsizei width, GLsizei height,
                  GLint destx, GLint desty, GLenum type )
{
   if (can_do_CopyPixels(ctx, type)) {
      const XMesaContext xmesa = XMESA_CONTEXT(ctx);
      XMesaDisplay *dpy = xmesa->xm_visual->display;
      XMesaBuffer xmbuf = XMESA_BUFFER(ctx->DrawBuffer);
      const XMesaGC gc = xmbuf->cleargc;  /* effected by glColorMask */
      struct xmesa_renderbuffer *srcXrb
         = xmesa_renderbuffer(ctx->ReadBuffer->_ColorReadBuffer);
      struct xmesa_renderbuffer *dstXrb
         = xmesa_renderbuffer(ctx->DrawBuffer->_ColorDrawBuffers[0]);

      ASSERT(dpy);
      ASSERT(gc);

      /* Note: we don't do any special clipping work here.  We could,
       * but X will do it for us.
       */
      srcy = YFLIP(srcXrb, srcy) - height + 1;
      desty = YFLIP(dstXrb, desty) - height + 1;
      XCopyArea(dpy, srcXrb->pixmap, dstXrb->pixmap, gc,
                srcx, srcy, width, height, destx, desty);
   }
   else {
      _swrast_CopyPixels(ctx, srcx, srcy, width, height, destx, desty, type );
   }
}




/*
 * Every driver should implement a GetString function in order to
 * return a meaningful GL_RENDERER string.
 */
static const GLubyte *
get_string( struct gl_context *ctx, GLenum name )
{
   (void) ctx;
   switch (name) {
      case GL_RENDERER:
         return (const GLubyte *) "Mesa X11";
      case GL_VENDOR:
         return NULL;
      default:
         return NULL;
   }
}


/*
 * We implement the glEnable function only because we care about
 * dither enable/disable.
 */
static void
enable( struct gl_context *ctx, GLenum pname, GLboolean state )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);

   switch (pname) {
      case GL_DITHER:
         if (state)
            xmesa->pixelformat = xmesa->xm_visual->dithered_pf;
         else
            xmesa->pixelformat = xmesa->xm_visual->undithered_pf;
         break;
      default:
         ;  /* silence compiler warning */
   }
}


/**
 * Called when the driver should update its state, based on the new_state
 * flags.
 */
void
xmesa_update_state( struct gl_context *ctx, GLbitfield new_state )
{
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);

   /* Propagate statechange information to swrast and swrast_setup
    * modules.  The X11 driver has no internal GL-dependent state.
    */
   _swrast_InvalidateState( ctx, new_state );
   _tnl_InvalidateState( ctx, new_state );
   _vbo_InvalidateState( ctx, new_state );
   _swsetup_InvalidateState( ctx, new_state );

   if (_mesa_is_user_fbo(ctx->DrawBuffer))
      return;

   /*
    * GL_DITHER, GL_READ/DRAW_BUFFER, buffer binding state, etc. effect
    * renderbuffer span/clear funcs.
    * Check _NEW_COLOR to detect dither enable/disable.
    */
   if (new_state & (_NEW_COLOR | _NEW_BUFFERS)) {
      XMesaBuffer xmbuf = XMESA_BUFFER(ctx->DrawBuffer);
      struct xmesa_renderbuffer *front_xrb, *back_xrb;

      front_xrb = xmbuf->frontxrb;
      if (front_xrb) {
         front_xrb->clearFunc = clear_pixmap;
      }

      back_xrb = xmbuf->backxrb;
      if (back_xrb) {
         if (xmbuf->backxrb->pixmap) {
            back_xrb->clearFunc = clear_pixmap;
         }
         else {
            switch (xmesa->xm_visual->BitsPerPixel) {
            case 16:
               back_xrb->clearFunc = clear_16bit_ximage;
               break;
            case 24:
               back_xrb->clearFunc = clear_24bit_ximage;
               break;
            case 32:
               back_xrb->clearFunc = clear_32bit_ximage;
               break;
            default:
               back_xrb->clearFunc = clear_nbit_ximage;
               break;
            }
         }
      }
   }
}


/**
 * Called by glViewport.
 * This is a good time for us to poll the current X window size and adjust
 * our renderbuffers to match the current window size.
 * Remember, we have no opportunity to respond to conventional
 * X Resize/StructureNotify events since the X driver has no event loop.
 * Thus, we poll.
 * Note that this trick isn't fool-proof.  If the application never calls
 * glViewport, our notion of the current window size may be incorrect.
 * That problem led to the GLX_MESA_resize_buffers extension.
 */
static void
xmesa_viewport(struct gl_context *ctx, GLint x, GLint y, GLsizei w, GLsizei h)
{
   XMesaContext xmctx = XMESA_CONTEXT(ctx);
   XMesaBuffer xmdrawbuf = XMESA_BUFFER(ctx->WinSysDrawBuffer);
   XMesaBuffer xmreadbuf = XMESA_BUFFER(ctx->WinSysReadBuffer);
   xmesa_check_and_update_buffer_size(xmctx, xmdrawbuf);
   xmesa_check_and_update_buffer_size(xmctx, xmreadbuf);
   (void) x;
   (void) y;
   (void) w;
   (void) h;
}


#if ENABLE_EXT_timer_query

/*
 * The GL_EXT_timer_query extension is not enabled for the XServer
 * indirect renderer.  Not sure about how/if wrapping of gettimeofday()
 * is done, etc.
 */

struct xmesa_query_object
{
   struct gl_query_object Base;
   struct timeval StartTime;
};


static struct gl_query_object *
xmesa_new_query_object(struct gl_context *ctx, GLuint id)
{
   struct xmesa_query_object *q = CALLOC_STRUCT(xmesa_query_object);
   if (q) {
      q->Base.Id = id;
      q->Base.Ready = GL_TRUE;
   }
   return &q->Base;
}


static void
xmesa_begin_query(struct gl_context *ctx, struct gl_query_object *q)
{
   if (q->Target == GL_TIME_ELAPSED_EXT) {
      struct xmesa_query_object *xq = (struct xmesa_query_object *) q;
      (void) gettimeofday(&xq->StartTime, NULL);
   }
}


/**
 * Return the difference between the two given times in microseconds.
 */
#ifdef __VMS
#define suseconds_t unsigned int
#endif
static GLuint64EXT
time_diff(const struct timeval *t0, const struct timeval *t1)
{
   GLuint64EXT seconds0 = t0->tv_sec & 0xff;  /* 0 .. 255 seconds */
   GLuint64EXT seconds1 = t1->tv_sec & 0xff;  /* 0 .. 255 seconds */
   GLuint64EXT nanosec0 = (seconds0 * 1000000 + t0->tv_usec) * 1000;
   GLuint64EXT nanosec1 = (seconds1 * 1000000 + t1->tv_usec) * 1000;
   return nanosec1 - nanosec0;
}


static void
xmesa_end_query(struct gl_context *ctx, struct gl_query_object *q)
{
   if (q->Target == GL_TIME_ELAPSED_EXT) {
      struct xmesa_query_object *xq = (struct xmesa_query_object *) q;
      struct timeval endTime;
      (void) gettimeofday(&endTime, NULL);
      /* result is in nanoseconds! */
      q->Result = time_diff(&xq->StartTime, &endTime);
   }
   q->Ready = GL_TRUE;
}

#endif /* ENABLE_timer_query */


/**
 * Initialize the device driver function table with the functions
 * we implement in this driver.
 */
void
xmesa_init_driver_functions( XMesaVisual xmvisual,
                             struct dd_function_table *driver )
{
   driver->GetString = get_string;
   driver->UpdateState = xmesa_update_state;
   driver->GetBufferSize = NULL; /* OBSOLETE */
   driver->Flush = finish_or_flush;
   driver->Finish = finish_or_flush;
   driver->ColorMask = color_mask;
   driver->Enable = enable;
   driver->Viewport = xmesa_viewport;
   if (TEST_META_FUNCS) {
      driver->Clear = _mesa_meta_Clear;
      driver->CopyPixels = _mesa_meta_CopyPixels;
      driver->BlitFramebuffer = _mesa_meta_BlitFramebuffer;
      driver->DrawPixels = _mesa_meta_DrawPixels;
      driver->Bitmap = _mesa_meta_Bitmap;
   }
   else {
      driver->Clear = clear_buffers;
      driver->CopyPixels = xmesa_CopyPixels;
      if (xmvisual->undithered_pf == PF_8R8G8B &&
          xmvisual->dithered_pf == PF_8R8G8B &&
          xmvisual->BitsPerPixel == 32) {
         driver->DrawPixels = xmesa_DrawPixels_8R8G8B;
      }
      else if (xmvisual->undithered_pf == PF_5R6G5B) {
         driver->DrawPixels = xmesa_DrawPixels_5R6G5B;
      }
   }

   driver->MapRenderbuffer = xmesa_MapRenderbuffer;
   driver->UnmapRenderbuffer = xmesa_UnmapRenderbuffer;

   driver->GenerateMipmap = _mesa_generate_mipmap;

#if ENABLE_EXT_timer_query
   driver->NewQueryObject = xmesa_new_query_object;
   driver->BeginQuery = xmesa_begin_query;
   driver->EndQuery = xmesa_end_query;
#endif
}


#define XMESA_NEW_POINT  (_NEW_POINT | \
                          _NEW_RENDERMODE | \
                          _SWRAST_NEW_RASTERMASK)

#define XMESA_NEW_LINE   (_NEW_LINE | \
                          _NEW_TEXTURE | \
                          _NEW_LIGHT | \
                          _NEW_DEPTH | \
                          _NEW_RENDERMODE | \
                          _SWRAST_NEW_RASTERMASK)

#define XMESA_NEW_TRIANGLE (_NEW_POLYGON | \
                            _NEW_TEXTURE | \
                            _NEW_LIGHT | \
                            _NEW_DEPTH | \
                            _NEW_RENDERMODE | \
                            _SWRAST_NEW_RASTERMASK)


/**
 * Extend the software rasterizer with our line/point/triangle
 * functions.
 * Called during context creation only.
 */
void xmesa_register_swrast_functions( struct gl_context *ctx )
{
   SWcontext *swrast = SWRAST_CONTEXT( ctx );

   swrast->choose_point = xmesa_choose_point;
   swrast->choose_line = xmesa_choose_line;
   swrast->choose_triangle = xmesa_choose_triangle;

   /* XXX these lines have no net effect.  Remove??? */
   swrast->InvalidatePointMask |= XMESA_NEW_POINT;
   swrast->InvalidateLineMask |= XMESA_NEW_LINE;
   swrast->InvalidateTriangleMask |= XMESA_NEW_TRIANGLE;
}
