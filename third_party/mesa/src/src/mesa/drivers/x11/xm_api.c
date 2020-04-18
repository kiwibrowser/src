/*
 * Mesa 3-D graphics library
 * Version:  7.1
 *
 * Copyright (C) 1999-2007  Brian Paul   All Rights Reserved.
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
 * \file xm_api.c
 *
 * All the XMesa* API functions.
 *
 *
 * NOTES:
 *
 * The window coordinate system origin (0,0) is in the lower-left corner
 * of the window.  X11's window coordinate origin is in the upper-left
 * corner of the window.  Therefore, most drawing functions in this
 * file have to flip Y coordinates.
 *
 * Define USE_XSHM in the Makefile with -DUSE_XSHM if you want to compile
 * in support for the MIT Shared Memory extension.  If enabled, when you
 * use an Ximage for the back buffer in double buffered mode, the "swap"
 * operation will be faster.  You must also link with -lXext.
 *
 * Byte swapping:  If the Mesa host and the X display use a different
 * byte order then there's some trickiness to be aware of when using
 * XImages.  The byte ordering used for the XImage is that of the X
 * display, not the Mesa host.
 * The color-to-pixel encoding for True/DirectColor must be done
 * according to the display's visual red_mask, green_mask, and blue_mask.
 * If XPutPixel is used to put a pixel into an XImage then XPutPixel will
 * do byte swapping if needed.  If one wants to directly "poke" the pixel
 * into the XImage's buffer then the pixel must be byte swapped first.  In
 * Mesa, when byte swapping is needed we use the PF_TRUECOLOR pixel format
 * and use XPutPixel everywhere except in the implementation of
 * glClear(GL_COLOR_BUFFER_BIT).  We want this function to be fast so
 * instead of using XPutPixel we "poke" our values after byte-swapping
 * the clear pixel value if needed.
 *
 */

#ifdef __CYGWIN__
#undef WIN32
#undef __WIN32__
#endif

#include "glxheader.h"
#include "xmesaP.h"
#include "main/context.h"
#include "main/extensions.h"
#include "main/framebuffer.h"
#include "main/imports.h"
#include "main/macros.h"
#include "main/renderbuffer.h"
#include "main/teximage.h"
#include "glapi/glthread.h"
#include "swrast/swrast.h"
#include "swrast/s_renderbuffer.h"
#include "swrast_setup/swrast_setup.h"
#include "vbo/vbo.h"
#include "tnl/tnl.h"
#include "tnl/t_context.h"
#include "tnl/t_pipeline.h"
#include "drivers/common/driverfuncs.h"
#include "drivers/common/meta.h"

/**
 * Global X driver lock
 */
_glthread_Mutex _xmesa_lock;



/**********************************************************************/
/*****                     X Utility Functions                    *****/
/**********************************************************************/


/**
 * Return the host's byte order as LSBFirst or MSBFirst ala X.
 */
static int host_byte_order( void )
{
   int i = 1;
   char *cptr = (char *) &i;
   return (*cptr==1) ? LSBFirst : MSBFirst;
}


/**
 * Check if the X Shared Memory extension is available.
 * Return:  0 = not available
 *          1 = shared XImage support available
 *          2 = shared Pixmap support available also
 */
static int check_for_xshm( XMesaDisplay *display )
{
#if defined(USE_XSHM) 
   int ignore;

   if (XQueryExtension( display, "MIT-SHM", &ignore, &ignore, &ignore )) {
      /* Note: we're no longer calling XShmQueryVersion() here.  It seems
       * to be flakey (triggers a spurious X protocol error when we close
       * one display connection and start using a new one.  XShm has been
       * around a long time and hasn't changed so if MIT_SHM is supported
       * we assume we're good to go.
       */
      return 2;
   }
   else {
      return 0;
   }
#else
   /* No  XSHM support */
   return 0;
#endif
}


/**
 * Apply gamma correction to an intensity value in [0..max].  Return the
 * new intensity value.
 */
static GLint
gamma_adjust( GLfloat gamma, GLint value, GLint max )
{
   if (gamma == 1.0) {
      return value;
   }
   else {
      double x = (double) value / (double) max;
      return IROUND_POS((GLfloat) max * pow(x, 1.0F/gamma));
   }
}



/**
 * Return the true number of bits per pixel for XImages.
 * For example, if we request a 24-bit deep visual we may actually need/get
 * 32bpp XImages.  This function returns the appropriate bpp.
 * Input:  dpy - the X display
 *         visinfo - desribes the visual to be used for XImages
 * Return:  true number of bits per pixel for XImages
 */
static int
bits_per_pixel( XMesaVisual xmv )
{
   XMesaDisplay *dpy = xmv->display;
   XMesaVisualInfo visinfo = xmv->visinfo;
   XMesaImage *img;
   int bitsPerPixel;
   /* Create a temporary XImage */
   img = XCreateImage( dpy, visinfo->visual, visinfo->depth,
		       ZPixmap, 0,           /*format, offset*/
		       (char*) MALLOC(8),    /*data*/
		       1, 1,                 /*width, height*/
		       32,                   /*bitmap_pad*/
		       0                     /*bytes_per_line*/
                     );
   assert(img);
   /* grab the bits/pixel value */
   bitsPerPixel = img->bits_per_pixel;
   /* free the XImage */
   free( img->data );
   img->data = NULL;
   XMesaDestroyImage( img );
   return bitsPerPixel;
}



/*
 * Determine if a given X window ID is valid (window exists).
 * Do this by calling XGetWindowAttributes() for the window and
 * checking if we catch an X error.
 * Input:  dpy - the display
 *         win - the window to check for existance
 * Return:  GL_TRUE - window exists
 *          GL_FALSE - window doesn't exist
 */
static GLboolean WindowExistsFlag;

static int window_exists_err_handler( XMesaDisplay* dpy, XErrorEvent* xerr )
{
   (void) dpy;
   if (xerr->error_code == BadWindow) {
      WindowExistsFlag = GL_FALSE;
   }
   return 0;
}

static GLboolean window_exists( XMesaDisplay *dpy, Window win )
{
   XWindowAttributes wa;
   int (*old_handler)( XMesaDisplay*, XErrorEvent* );
   WindowExistsFlag = GL_TRUE;
   old_handler = XSetErrorHandler(window_exists_err_handler);
   XGetWindowAttributes( dpy, win, &wa ); /* dummy request */
   XSetErrorHandler(old_handler);
   return WindowExistsFlag;
}

static Status
get_drawable_size( XMesaDisplay *dpy, Drawable d, GLuint *width, GLuint *height )
{
   Window root;
   Status stat;
   int xpos, ypos;
   unsigned int w, h, bw, depth;
   stat = XGetGeometry(dpy, d, &root, &xpos, &ypos, &w, &h, &bw, &depth);
   *width = w;
   *height = h;
   return stat;
}


/**
 * Return the size of the window (or pixmap) that corresponds to the
 * given XMesaBuffer.
 * \param width  returns width in pixels
 * \param height  returns height in pixels
 */
void
xmesa_get_window_size(XMesaDisplay *dpy, XMesaBuffer b,
                      GLuint *width, GLuint *height)
{
   Status stat;

   _glthread_LOCK_MUTEX(_xmesa_lock);
   XSync(b->xm_visual->display, 0); /* added for Chromium */
   stat = get_drawable_size(dpy, b->frontxrb->pixmap, width, height);
   _glthread_UNLOCK_MUTEX(_xmesa_lock);

   if (!stat) {
      /* probably querying a window that's recently been destroyed */
      _mesa_warning(NULL, "XGetGeometry failed!\n");
      *width = *height = 1;
   }
}



/**********************************************************************/
/*****                Linked list of XMesaBuffers                 *****/
/**********************************************************************/

XMesaBuffer XMesaBufferList = NULL;


/**
 * Allocate a new XMesaBuffer object which corresponds to the given drawable.
 * Note that XMesaBuffer is derived from struct gl_framebuffer.
 * The new XMesaBuffer will not have any size (Width=Height=0).
 *
 * \param d  the corresponding X drawable (window or pixmap)
 * \param type  either WINDOW, PIXMAP or PBUFFER, describing d
 * \param vis  the buffer's visual
 * \param cmap  the window's colormap, if known.
 * \return new XMesaBuffer or NULL if any problem
 */
static XMesaBuffer
create_xmesa_buffer(XMesaDrawable d, BufferType type,
                    XMesaVisual vis, XMesaColormap cmap)
{
   XMesaBuffer b;

   ASSERT(type == WINDOW || type == PIXMAP || type == PBUFFER);

   b = (XMesaBuffer) CALLOC_STRUCT(xmesa_buffer);
   if (!b)
      return NULL;

   b->display = vis->display;
   b->xm_visual = vis;
   b->type = type;
   b->cmap = cmap;

   _mesa_initialize_window_framebuffer(&b->mesa_buffer, &vis->mesa_visual);
   b->mesa_buffer.Delete = xmesa_delete_framebuffer;

   /*
    * Front renderbuffer
    */
   b->frontxrb = xmesa_new_renderbuffer(NULL, 0, vis, GL_FALSE);
   if (!b->frontxrb) {
      free(b);
      return NULL;
   }
   b->frontxrb->Parent = b;
   b->frontxrb->drawable = d;
   b->frontxrb->pixmap = (XMesaPixmap) d;
   _mesa_add_renderbuffer(&b->mesa_buffer, BUFFER_FRONT_LEFT,
                          &b->frontxrb->Base.Base);

   /*
    * Back renderbuffer
    */
   if (vis->mesa_visual.doubleBufferMode) {
      b->backxrb = xmesa_new_renderbuffer(NULL, 0, vis, GL_TRUE);
      if (!b->backxrb) {
         /* XXX free front xrb too */
         free(b);
         return NULL;
      }
      b->backxrb->Parent = b;
      /* determine back buffer implementation */
      b->db_mode = vis->ximage_flag ? BACK_XIMAGE : BACK_PIXMAP;
      
      _mesa_add_renderbuffer(&b->mesa_buffer, BUFFER_BACK_LEFT,
                             &b->backxrb->Base.Base);
   }

   /*
    * Other renderbuffer (depth, stencil, etc)
    */
   _swrast_add_soft_renderbuffers(&b->mesa_buffer,
                                  GL_FALSE,  /* color */
                                  vis->mesa_visual.haveDepthBuffer,
                                  vis->mesa_visual.haveStencilBuffer,
                                  vis->mesa_visual.haveAccumBuffer,
                                  GL_FALSE,  /* software alpha buffer */
                                  vis->mesa_visual.numAuxBuffers > 0 );

   /* GLX_EXT_texture_from_pixmap */
   b->TextureTarget = 0;
   b->TextureFormat = GLX_TEXTURE_FORMAT_NONE_EXT;
   b->TextureMipmap = 0;

   /* insert buffer into linked list */
   b->Next = XMesaBufferList;
   XMesaBufferList = b;

   return b;
}


/**
 * Find an XMesaBuffer by matching X display and colormap but NOT matching
 * the notThis buffer.
 */
XMesaBuffer
xmesa_find_buffer(XMesaDisplay *dpy, XMesaColormap cmap, XMesaBuffer notThis)
{
   XMesaBuffer b;
   for (b=XMesaBufferList; b; b=b->Next) {
      if (b->display==dpy && b->cmap==cmap && b!=notThis) {
         return b;
      }
   }
   return NULL;
}


/**
 * Remove buffer from linked list, delete if no longer referenced.
 */
static void
xmesa_free_buffer(XMesaBuffer buffer)
{
   XMesaBuffer prev = NULL, b;

   for (b = XMesaBufferList; b; b = b->Next) {
      if (b == buffer) {
         struct gl_framebuffer *fb = &buffer->mesa_buffer;

         /* unlink buffer from list */
         if (prev)
            prev->Next = buffer->Next;
         else
            XMesaBufferList = buffer->Next;

         /* mark as delete pending */
         fb->DeletePending = GL_TRUE;

         /* Since the X window for the XMesaBuffer is going away, we don't
          * want to dereference this pointer in the future.
          */
         b->frontxrb->drawable = 0;

         /* Unreference.  If count = zero we'll really delete the buffer */
         _mesa_reference_framebuffer(&fb, NULL);

         return;
      }
      /* continue search */
      prev = b;
   }
   /* buffer not found in XMesaBufferList */
   _mesa_problem(NULL,"xmesa_free_buffer() - buffer not found\n");
}




/**********************************************************************/
/*****                   Misc Private Functions                   *****/
/**********************************************************************/


/**
 * Setup RGB rendering for a window with a True/DirectColor visual.
 */
static void
setup_truecolor(XMesaVisual v, XMesaBuffer buffer, XMesaColormap cmap)
{
   unsigned long rmask, gmask, bmask;
   (void) buffer;
   (void) cmap;

   /* Compute red multiplier (mask) and bit shift */
   v->rshift = 0;
   rmask = GET_REDMASK(v);
   while ((rmask & 1)==0) {
      v->rshift++;
      rmask = rmask >> 1;
   }

   /* Compute green multiplier (mask) and bit shift */
   v->gshift = 0;
   gmask = GET_GREENMASK(v);
   while ((gmask & 1)==0) {
      v->gshift++;
      gmask = gmask >> 1;
   }

   /* Compute blue multiplier (mask) and bit shift */
   v->bshift = 0;
   bmask = GET_BLUEMASK(v);
   while ((bmask & 1)==0) {
      v->bshift++;
      bmask = bmask >> 1;
   }

   /*
    * Compute component-to-pixel lookup tables and dithering kernel
    */
   {
      static GLubyte kernel[16] = {
          0*16,  8*16,  2*16, 10*16,
         12*16,  4*16, 14*16,  6*16,
          3*16, 11*16,  1*16,  9*16,
         15*16,  7*16, 13*16,  5*16,
      };
      GLint rBits = _mesa_bitcount(rmask);
      GLint gBits = _mesa_bitcount(gmask);
      GLint bBits = _mesa_bitcount(bmask);
      GLint maxBits;
      GLuint i;

      /* convert pixel components in [0,_mask] to RGB values in [0,255] */
      for (i=0; i<=rmask; i++)
         v->PixelToR[i] = (unsigned char) ((i * 255) / rmask);
      for (i=0; i<=gmask; i++)
         v->PixelToG[i] = (unsigned char) ((i * 255) / gmask);
      for (i=0; i<=bmask; i++)
         v->PixelToB[i] = (unsigned char) ((i * 255) / bmask);

      /* convert RGB values from [0,255] to pixel components */

      for (i=0;i<256;i++) {
         GLint r = gamma_adjust(v->RedGamma,   i, 255);
         GLint g = gamma_adjust(v->GreenGamma, i, 255);
         GLint b = gamma_adjust(v->BlueGamma,  i, 255);
         v->RtoPixel[i] = (r >> (8-rBits)) << v->rshift;
         v->GtoPixel[i] = (g >> (8-gBits)) << v->gshift;
         v->BtoPixel[i] = (b >> (8-bBits)) << v->bshift;
      }
      /* overflow protection */
      for (i=256;i<512;i++) {
         v->RtoPixel[i] = v->RtoPixel[255];
         v->GtoPixel[i] = v->GtoPixel[255];
         v->BtoPixel[i] = v->BtoPixel[255];
      }

      /* setup dithering kernel */
      maxBits = rBits;
      if (gBits > maxBits)  maxBits = gBits;
      if (bBits > maxBits)  maxBits = bBits;
      for (i=0;i<16;i++) {
         v->Kernel[i] = kernel[i] >> maxBits;
      }

      v->undithered_pf = PF_Truecolor;
      v->dithered_pf = (GET_VISUAL_DEPTH(v)<24) ? PF_Dither_True : PF_Truecolor;
   }

   /*
    * Now check for TrueColor visuals which we can optimize.
    */
   if (   GET_REDMASK(v)  ==0x0000ff
       && GET_GREENMASK(v)==0x00ff00
       && GET_BLUEMASK(v) ==0xff0000
       && CHECK_BYTE_ORDER(v)
       && v->BitsPerPixel==32
       && v->RedGamma==1.0 && v->GreenGamma==1.0 && v->BlueGamma==1.0) {
      /* common 32 bpp config used on SGI, Sun */
      v->undithered_pf = v->dithered_pf = PF_8A8B8G8R; /* ABGR */
   }
   else if (GET_REDMASK(v)  == 0xff0000
         && GET_GREENMASK(v)== 0x00ff00
         && GET_BLUEMASK(v) == 0x0000ff
         && CHECK_BYTE_ORDER(v)
         && v->RedGamma == 1.0 && v->GreenGamma == 1.0 && v->BlueGamma == 1.0){
      if (v->BitsPerPixel==32) {
         /* if 32 bpp, and visual indicates 8 bpp alpha channel */
         if (GET_VISUAL_DEPTH(v) == 32 && v->mesa_visual.alphaBits == 8)
            v->undithered_pf = v->dithered_pf = PF_8A8R8G8B; /* ARGB */
         else
            v->undithered_pf = v->dithered_pf = PF_8R8G8B; /* xRGB */
      }
      else if (v->BitsPerPixel == 24) {
         v->undithered_pf = v->dithered_pf = PF_8R8G8B24; /* RGB */
      }
   }
   else if (GET_REDMASK(v)  ==0xf800
       &&   GET_GREENMASK(v)==0x07e0
       &&   GET_BLUEMASK(v) ==0x001f
       && CHECK_BYTE_ORDER(v)
       && v->BitsPerPixel==16
       && v->RedGamma==1.0 && v->GreenGamma==1.0 && v->BlueGamma==1.0) {
      /* 5-6-5 RGB */
      v->undithered_pf = PF_5R6G5B;
      v->dithered_pf = PF_Dither_5R6G5B;
   }
}


/**
 * When a context is bound for the first time, we can finally finish
 * initializing the context's visual and buffer information.
 * \param v  the XMesaVisual to initialize
 * \param b  the XMesaBuffer to initialize (may be NULL)
 * \param rgb_flag  TRUE = RGBA mode, FALSE = color index mode
 * \param window  the window/pixmap we're rendering into
 * \param cmap  the colormap associated with the window/pixmap
 * \return GL_TRUE=success, GL_FALSE=failure
 */
static GLboolean
initialize_visual_and_buffer(XMesaVisual v, XMesaBuffer b,
                             XMesaDrawable window,
                             XMesaColormap cmap)
{
   const int xclass = v->visualType;


   ASSERT(!b || b->xm_visual == v);

   /* Save true bits/pixel */
   v->BitsPerPixel = bits_per_pixel(v);
   assert(v->BitsPerPixel > 0);

   /* RGB WINDOW:
    * We support RGB rendering into almost any kind of visual.
    */
   if (xclass == GLX_TRUE_COLOR || xclass == GLX_DIRECT_COLOR) {
      setup_truecolor( v, b, cmap );
   }
   else {
      _mesa_warning(NULL, "XMesa: RGB mode rendering not supported in given visual.\n");
      return GL_FALSE;
   }
   v->mesa_visual.indexBits = 0;

   if (_mesa_getenv("MESA_NO_DITHER")) {
      v->dithered_pf = v->undithered_pf;
   }


   /*
    * If MESA_INFO env var is set print out some debugging info
    * which can help Brian figure out what's going on when a user
    * reports bugs.
    */
   if (_mesa_getenv("MESA_INFO")) {
      printf("X/Mesa visual = %p\n", (void *) v);
      printf("X/Mesa dithered pf = %u\n", v->dithered_pf);
      printf("X/Mesa undithered pf = %u\n", v->undithered_pf);
      printf("X/Mesa level = %d\n", v->mesa_visual.level);
      printf("X/Mesa depth = %d\n", GET_VISUAL_DEPTH(v));
      printf("X/Mesa bits per pixel = %d\n", v->BitsPerPixel);
   }

   if (b && window) {
      /* Do window-specific initializations */

      /* these should have been set in create_xmesa_buffer */
      ASSERT(b->frontxrb->drawable == window);
      ASSERT(b->frontxrb->pixmap == (XMesaPixmap) window);

      /* Setup for single/double buffering */
      if (v->mesa_visual.doubleBufferMode) {
         /* Double buffered */
         b->shm = check_for_xshm( v->display );
      }

      /* X11 graphics contexts */
      b->gc = XCreateGC( v->display, window, 0, NULL );
      XMesaSetFunction( v->display, b->gc, GXcopy );

      /* cleargc - for glClear() */
      b->cleargc = XCreateGC( v->display, window, 0, NULL );
      XMesaSetFunction( v->display, b->cleargc, GXcopy );

      /*
       * Don't generate Graphics Expose/NoExpose events in swapbuffers().
       * Patch contributed by Michael Pichler May 15, 1995.
       */
      {
         XGCValues gcvalues;
         gcvalues.graphics_exposures = False;
         b->swapgc = XCreateGC(v->display, window,
                               GCGraphicsExposures, &gcvalues);
      }
      XMesaSetFunction( v->display, b->swapgc, GXcopy );
   }

   return GL_TRUE;
}



/*
 * Convert an RGBA color to a pixel value.
 */
unsigned long
xmesa_color_to_pixel(struct gl_context *ctx,
                     GLubyte r, GLubyte g, GLubyte b, GLubyte a,
                     GLuint pixelFormat)
{
   XMesaContext xmesa = XMESA_CONTEXT(ctx);
   switch (pixelFormat) {
      case PF_Truecolor:
         {
            unsigned long p;
            PACK_TRUECOLOR( p, r, g, b );
            return p;
         }
      case PF_8A8B8G8R:
         return PACK_8A8B8G8R( r, g, b, a );
      case PF_8A8R8G8B:
         return PACK_8A8R8G8B( r, g, b, a );
      case PF_8R8G8B:
         /* fall through */
      case PF_8R8G8B24:
         return PACK_8R8G8B( r, g, b );
      case PF_5R6G5B:
         return PACK_5R6G5B( r, g, b );
      case PF_Dither_True:
         /* fall through */
      case PF_Dither_5R6G5B:
         {
            unsigned long p;
            PACK_TRUEDITHER(p, 1, 0, r, g, b);
            return p;
         }
      default:
         _mesa_problem(ctx, "Bad pixel format in xmesa_color_to_pixel");
   }
   return 0;
}


#define NUM_VISUAL_TYPES   6

/**
 * Convert an X visual type to a GLX visual type.
 * 
 * \param visualType X visual type (i.e., \c TrueColor, \c StaticGray, etc.)
 *        to be converted.
 * \return If \c visualType is a valid X visual type, a GLX visual type will
 *         be returned.  Otherwise \c GLX_NONE will be returned.
 * 
 * \note
 * This code was lifted directly from lib/GL/glx/glcontextmodes.c in the
 * DRI CVS tree.
 */
static GLint
xmesa_convert_from_x_visual_type( int visualType )
{
    static const int glx_visual_types[ NUM_VISUAL_TYPES ] = {
	GLX_STATIC_GRAY,  GLX_GRAY_SCALE,
	GLX_STATIC_COLOR, GLX_PSEUDO_COLOR,
	GLX_TRUE_COLOR,   GLX_DIRECT_COLOR
    };

    return ( (unsigned) visualType < NUM_VISUAL_TYPES )
	? glx_visual_types[ visualType ] : GLX_NONE;
}


/**********************************************************************/
/*****                       Public Functions                     *****/
/**********************************************************************/


/*
 * Create a new X/Mesa visual.
 * Input:  display - X11 display
 *         visinfo - an XVisualInfo pointer
 *         rgb_flag - GL_TRUE = RGB mode,
 *                    GL_FALSE = color index mode
 *         alpha_flag - alpha buffer requested?
 *         db_flag - GL_TRUE = double-buffered,
 *                   GL_FALSE = single buffered
 *         stereo_flag - stereo visual?
 *         ximage_flag - GL_TRUE = use an XImage for back buffer,
 *                       GL_FALSE = use an off-screen pixmap for back buffer
 *         depth_size - requested bits/depth values, or zero
 *         stencil_size - requested bits/stencil values, or zero
 *         accum_red_size - requested bits/red accum values, or zero
 *         accum_green_size - requested bits/green accum values, or zero
 *         accum_blue_size - requested bits/blue accum values, or zero
 *         accum_alpha_size - requested bits/alpha accum values, or zero
 *         num_samples - number of samples/pixel if multisampling, or zero
 *         level - visual level, usually 0
 *         visualCaveat - ala the GLX extension, usually GLX_NONE
 * Return;  a new XMesaVisual or 0 if error.
 */
PUBLIC
XMesaVisual XMesaCreateVisual( XMesaDisplay *display,
                               XMesaVisualInfo visinfo,
                               GLboolean rgb_flag,
                               GLboolean alpha_flag,
                               GLboolean db_flag,
                               GLboolean stereo_flag,
                               GLboolean ximage_flag,
                               GLint depth_size,
                               GLint stencil_size,
                               GLint accum_red_size,
                               GLint accum_green_size,
                               GLint accum_blue_size,
                               GLint accum_alpha_size,
                               GLint num_samples,
                               GLint level,
                               GLint visualCaveat )
{
   char *gamma;
   XMesaVisual v;
   GLint red_bits, green_bits, blue_bits, alpha_bits;

   /* For debugging only */
   if (_mesa_getenv("MESA_XSYNC")) {
      /* This makes debugging X easier.
       * In your debugger, set a breakpoint on _XError to stop when an
       * X protocol error is generated.
       */
      XSynchronize( display, 1 );
   }

   /* Color-index rendering not supported. */
   if (!rgb_flag)
      return NULL;

   v = (XMesaVisual) CALLOC_STRUCT(xmesa_visual);
   if (!v) {
      return NULL;
   }

   v->display = display;

   /* Save a copy of the XVisualInfo struct because the user may Xfree()
    * the struct but we may need some of the information contained in it
    * at a later time.
    */
   v->visinfo = (XVisualInfo *) MALLOC(sizeof(*visinfo));
   if(!v->visinfo) {
      free(v);
      return NULL;
   }
   memcpy(v->visinfo, visinfo, sizeof(*visinfo));

   /* check for MESA_GAMMA environment variable */
   gamma = _mesa_getenv("MESA_GAMMA");
   if (gamma) {
      v->RedGamma = v->GreenGamma = v->BlueGamma = 0.0;
      sscanf( gamma, "%f %f %f", &v->RedGamma, &v->GreenGamma, &v->BlueGamma );
      if (v->RedGamma<=0.0)    v->RedGamma = 1.0;
      if (v->GreenGamma<=0.0)  v->GreenGamma = v->RedGamma;
      if (v->BlueGamma<=0.0)   v->BlueGamma = v->RedGamma;
   }
   else {
      v->RedGamma = v->GreenGamma = v->BlueGamma = 1.0;
   }

   v->ximage_flag = ximage_flag;

   v->mesa_visual.redMask = visinfo->red_mask;
   v->mesa_visual.greenMask = visinfo->green_mask;
   v->mesa_visual.blueMask = visinfo->blue_mask;
   v->visualID = visinfo->visualid;
   v->screen = visinfo->screen;

#if !(defined(__cplusplus) || defined(c_plusplus))
   v->visualType = xmesa_convert_from_x_visual_type(visinfo->class);
#else
   v->visualType = xmesa_convert_from_x_visual_type(visinfo->c_class);
#endif

   v->mesa_visual.visualRating = visualCaveat;

   if (alpha_flag)
      v->mesa_visual.alphaBits = 8;

   (void) initialize_visual_and_buffer( v, NULL, 0, 0 );

   {
      const int xclass = v->visualType;
      if (xclass == GLX_TRUE_COLOR || xclass == GLX_DIRECT_COLOR) {
         red_bits   = _mesa_bitcount(GET_REDMASK(v));
         green_bits = _mesa_bitcount(GET_GREENMASK(v));
         blue_bits  = _mesa_bitcount(GET_BLUEMASK(v));
      }
      else {
         /* this is an approximation */
         int depth;
         depth = GET_VISUAL_DEPTH(v);
         red_bits = depth / 3;
         depth -= red_bits;
         green_bits = depth / 2;
         depth -= green_bits;
         blue_bits = depth;
         alpha_bits = 0;
         assert( red_bits + green_bits + blue_bits == GET_VISUAL_DEPTH(v) );
      }
      alpha_bits = v->mesa_visual.alphaBits;
   }

   if (!_mesa_initialize_visual(&v->mesa_visual,
                                db_flag, stereo_flag,
                                red_bits, green_bits,
                                blue_bits, alpha_bits,
                                depth_size,
                                stencil_size,
                                accum_red_size, accum_green_size,
                                accum_blue_size, accum_alpha_size,
                                0)) {
      FREE(v);
      return NULL;
   }

   /* XXX minor hack */
   v->mesa_visual.level = level;
   return v;
}


PUBLIC
void XMesaDestroyVisual( XMesaVisual v )
{
   free(v->visinfo);
   free(v);
}



/**
 * Create a new XMesaContext.
 * \param v  the XMesaVisual
 * \param share_list  another XMesaContext with which to share display
 *                    lists or NULL if no sharing is wanted.
 * \return an XMesaContext or NULL if error.
 */
PUBLIC
XMesaContext XMesaCreateContext( XMesaVisual v, XMesaContext share_list )
{
   static GLboolean firstTime = GL_TRUE;
   XMesaContext c;
   struct gl_context *mesaCtx;
   struct dd_function_table functions;
   TNLcontext *tnl;

   if (firstTime) {
      _glthread_INIT_MUTEX(_xmesa_lock);
      firstTime = GL_FALSE;
   }

   /* Note: the XMesaContext contains a Mesa struct gl_context struct (inheritance) */
   c = (XMesaContext) CALLOC_STRUCT(xmesa_context);
   if (!c)
      return NULL;

   mesaCtx = &(c->mesa);

   /* initialize with default driver functions, then plug in XMesa funcs */
   _mesa_init_driver_functions(&functions);
   xmesa_init_driver_functions(v, &functions);
   if (!_mesa_initialize_context(mesaCtx, API_OPENGL, &v->mesa_visual,
                      share_list ? &(share_list->mesa) : (struct gl_context *) NULL,
                      &functions, (void *) c)) {
      free(c);
      return NULL;
   }

   /* Enable this to exercise fixed function -> shader translation
    * with software rendering.
    */
   if (0) {
      mesaCtx->VertexProgram._MaintainTnlProgram = GL_TRUE;
      mesaCtx->FragmentProgram._MaintainTexEnvProgram = GL_TRUE;
   }

   _mesa_enable_sw_extensions(mesaCtx);
   _mesa_enable_1_3_extensions(mesaCtx);
   _mesa_enable_1_4_extensions(mesaCtx);
   _mesa_enable_1_5_extensions(mesaCtx);
   _mesa_enable_2_0_extensions(mesaCtx);
   _mesa_enable_2_1_extensions(mesaCtx);
    if (mesaCtx->Mesa_DXTn) {
       _mesa_enable_extension(mesaCtx, "GL_EXT_texture_compression_s3tc");
       _mesa_enable_extension(mesaCtx, "GL_S3_s3tc");
    }
    _mesa_enable_extension(mesaCtx, "GL_3DFX_texture_compression_FXT1");
#if ENABLE_EXT_timer_query
    _mesa_enable_extension(mesaCtx, "GL_EXT_timer_query");
#endif


   /* finish up xmesa context initializations */
   c->swapbytes = CHECK_BYTE_ORDER(v) ? GL_FALSE : GL_TRUE;
   c->xm_visual = v;
   c->xm_buffer = NULL;   /* set later by XMesaMakeCurrent */
   c->display = v->display;
   c->pixelformat = v->dithered_pf;      /* Dithering is enabled by default */

   /* Initialize the software rasterizer and helper modules.
    */
   if (!_swrast_CreateContext( mesaCtx ) ||
       !_vbo_CreateContext( mesaCtx ) ||
       !_tnl_CreateContext( mesaCtx ) ||
       !_swsetup_CreateContext( mesaCtx )) {
      _mesa_free_context_data(&c->mesa);
      free(c);
      return NULL;
   }

   /* tnl setup */
   tnl = TNL_CONTEXT(mesaCtx);
   tnl->Driver.RunPipeline = _tnl_run_pipeline;
   /* swrast setup */
   xmesa_register_swrast_functions( mesaCtx );
   _swsetup_Wakeup(mesaCtx);

   _mesa_meta_init(mesaCtx);

   return c;
}



PUBLIC
void XMesaDestroyContext( XMesaContext c )
{
   struct gl_context *mesaCtx = &c->mesa;

   _mesa_meta_free( mesaCtx );

   _swsetup_DestroyContext( mesaCtx );
   _swrast_DestroyContext( mesaCtx );
   _tnl_DestroyContext( mesaCtx );
   _vbo_DestroyContext( mesaCtx );
   _mesa_free_context_data( mesaCtx );
   free( c );
}



/**
 * Private function for creating an XMesaBuffer which corresponds to an
 * X window or pixmap.
 * \param v  the window's XMesaVisual
 * \param w  the window we're wrapping
 * \return  new XMesaBuffer or NULL if error
 */
PUBLIC XMesaBuffer
XMesaCreateWindowBuffer(XMesaVisual v, XMesaWindow w)
{
   XWindowAttributes attr;
   XMesaBuffer b;
   XMesaColormap cmap;
   int depth;

   assert(v);
   assert(w);

   /* Check that window depth matches visual depth */
   XGetWindowAttributes( v->display, w, &attr );
   depth = attr.depth;
   if (GET_VISUAL_DEPTH(v) != depth) {
      _mesa_warning(NULL, "XMesaCreateWindowBuffer: depth mismatch between visual (%d) and window (%d)!\n",
                    GET_VISUAL_DEPTH(v), depth);
      return NULL;
   }

   /* Find colormap */
   if (attr.colormap) {
      cmap = attr.colormap;
   }
   else {
      _mesa_warning(NULL, "Window %u has no colormap!\n", (unsigned int) w);
      /* this is weird, a window w/out a colormap!? */
      /* OK, let's just allocate a new one and hope for the best */
      cmap = XCreateColormap(v->display, w, attr.visual, AllocNone);
   }

   b = create_xmesa_buffer((XMesaDrawable) w, WINDOW, v, cmap);
   if (!b)
      return NULL;

   if (!initialize_visual_and_buffer( v, b, (XMesaDrawable) w, cmap )) {
      xmesa_free_buffer(b);
      return NULL;
   }

   return b;
}



/**
 * Create a new XMesaBuffer from an X pixmap.
 *
 * \param v    the XMesaVisual
 * \param p    the pixmap
 * \param cmap the colormap, may be 0 if using a \c GLX_TRUE_COLOR or
 *             \c GLX_DIRECT_COLOR visual for the pixmap
 * \returns new XMesaBuffer or NULL if error
 */
PUBLIC XMesaBuffer
XMesaCreatePixmapBuffer(XMesaVisual v, XMesaPixmap p, XMesaColormap cmap)
{
   XMesaBuffer b;

   assert(v);

   b = create_xmesa_buffer((XMesaDrawable) p, PIXMAP, v, cmap);
   if (!b)
      return NULL;

   if (!initialize_visual_and_buffer(v, b, (XMesaDrawable) p, cmap)) {
      xmesa_free_buffer(b);
      return NULL;
   }

   return b;
}


/**
 * For GLX_EXT_texture_from_pixmap
 */
XMesaBuffer
XMesaCreatePixmapTextureBuffer(XMesaVisual v, XMesaPixmap p,
                               XMesaColormap cmap,
                               int format, int target, int mipmap)
{
   GET_CURRENT_CONTEXT(ctx);
   XMesaBuffer b;
   GLuint width, height;

   assert(v);

   b = create_xmesa_buffer((XMesaDrawable) p, PIXMAP, v, cmap);
   if (!b)
      return NULL;

   /* get pixmap size, update framebuffer/renderbuffer dims */
   xmesa_get_window_size(v->display, b, &width, &height);
   _mesa_resize_framebuffer(NULL, &(b->mesa_buffer), width, height);

   if (target == 0) {
      /* examine dims */
      if (ctx->Extensions.ARB_texture_non_power_of_two) {
         target = GLX_TEXTURE_2D_EXT;
      }
      else if (   _mesa_bitcount(width)  == 1
               && _mesa_bitcount(height) == 1) {
         /* power of two size */
         if (height == 1) {
            target = GLX_TEXTURE_1D_EXT;
         }
         else {
            target = GLX_TEXTURE_2D_EXT;
         }
      }
      else if (ctx->Extensions.NV_texture_rectangle) {
         target = GLX_TEXTURE_RECTANGLE_EXT;
      }
      else {
         /* non power of two textures not supported */
         XMesaDestroyBuffer(b);
         return 0;
      }
   }

   b->TextureTarget = target;
   b->TextureFormat = format;
   b->TextureMipmap = mipmap;

   if (!initialize_visual_and_buffer(v, b, (XMesaDrawable) p, cmap)) {
      xmesa_free_buffer(b);
      return NULL;
   }

   return b;
}



XMesaBuffer
XMesaCreatePBuffer(XMesaVisual v, XMesaColormap cmap,
                   unsigned int width, unsigned int height)
{
   XMesaWindow root;
   XMesaDrawable drawable;  /* X Pixmap Drawable */
   XMesaBuffer b;

   /* allocate pixmap for front buffer */
   root = RootWindow( v->display, v->visinfo->screen );
   drawable = XCreatePixmap(v->display, root, width, height,
                            v->visinfo->depth);
   if (!drawable)
      return NULL;

   b = create_xmesa_buffer(drawable, PBUFFER, v, cmap);
   if (!b)
      return NULL;

   if (!initialize_visual_and_buffer(v, b, drawable, cmap)) {
      xmesa_free_buffer(b);
      return NULL;
   }

   return b;
}



/*
 * Deallocate an XMesaBuffer structure and all related info.
 */
PUBLIC void
XMesaDestroyBuffer(XMesaBuffer b)
{
   xmesa_free_buffer(b);
}


/**
 * Query the current window size and update the corresponding struct gl_framebuffer
 * and all attached renderbuffers.
 * Called when:
 *  1. the first time a buffer is bound to a context.
 *  2. from glViewport to poll for window size changes
 *  3. from the XMesaResizeBuffers() API function.
 * Note: it's possible (and legal) for xmctx to be NULL.  That can happen
 * when resizing a buffer when no rendering context is bound.
 */
void
xmesa_check_and_update_buffer_size(XMesaContext xmctx, XMesaBuffer drawBuffer)
{
   GLuint width, height;
   xmesa_get_window_size(drawBuffer->display, drawBuffer, &width, &height);
   if (drawBuffer->mesa_buffer.Width != width ||
       drawBuffer->mesa_buffer.Height != height) {
      struct gl_context *ctx = xmctx ? &xmctx->mesa : NULL;
      _mesa_resize_framebuffer(ctx, &(drawBuffer->mesa_buffer), width, height);
   }
   drawBuffer->mesa_buffer.Initialized = GL_TRUE; /* XXX TEMPORARY? */
}


/*
 * Bind buffer b to context c and make c the current rendering context.
 */
GLboolean XMesaMakeCurrent( XMesaContext c, XMesaBuffer b )
{
   return XMesaMakeCurrent2( c, b, b );
}


/*
 * Bind buffer b to context c and make c the current rendering context.
 */
PUBLIC
GLboolean XMesaMakeCurrent2( XMesaContext c, XMesaBuffer drawBuffer,
                             XMesaBuffer readBuffer )
{
   if (c) {
      if (!drawBuffer || !readBuffer)
         return GL_FALSE;  /* must specify buffers! */

      if (&(c->mesa) == _mesa_get_current_context()
          && c->mesa.DrawBuffer == &drawBuffer->mesa_buffer
          && c->mesa.ReadBuffer == &readBuffer->mesa_buffer
          && XMESA_BUFFER(c->mesa.DrawBuffer)->wasCurrent) {
         /* same context and buffer, do nothing */
         return GL_TRUE;
      }

      c->xm_buffer = drawBuffer;

      /* Call this periodically to detect when the user has begun using
       * GL rendering from multiple threads.
       */
      _glapi_check_multithread();

      xmesa_check_and_update_buffer_size(c, drawBuffer);
      if (readBuffer != drawBuffer)
         xmesa_check_and_update_buffer_size(c, readBuffer);

      _mesa_make_current(&(c->mesa),
                         &drawBuffer->mesa_buffer,
                         &readBuffer->mesa_buffer);

      /*
       * Must recompute and set these pixel values because colormap
       * can be different for different windows.
       */
      c->clearpixel = xmesa_color_to_pixel( &c->mesa,
					    c->clearcolor[0],
					    c->clearcolor[1],
					    c->clearcolor[2],
					    c->clearcolor[3],
					    c->xm_visual->undithered_pf);
      XMesaSetForeground(c->display, drawBuffer->cleargc, c->clearpixel);

      /* Solution to Stephane Rehel's problem with glXReleaseBuffersMESA(): */
      drawBuffer->wasCurrent = GL_TRUE;
   }
   else {
      /* Detach */
      _mesa_make_current( NULL, NULL, NULL );
   }
   return GL_TRUE;
}


/*
 * Unbind the context c from its buffer.
 */
GLboolean XMesaUnbindContext( XMesaContext c )
{
   /* A no-op for XFree86 integration purposes */
   return GL_TRUE;
}


XMesaContext XMesaGetCurrentContext( void )
{
   GET_CURRENT_CONTEXT(ctx);
   if (ctx) {
      XMesaContext xmesa = XMESA_CONTEXT(ctx);
      return xmesa;
   }
   else {
      return 0;
   }
}


XMesaBuffer XMesaGetCurrentBuffer( void )
{
   GET_CURRENT_CONTEXT(ctx);
   if (ctx) {
      XMesaBuffer xmbuf = XMESA_BUFFER(ctx->DrawBuffer);
      return xmbuf;
   }
   else {
      return 0;
   }
}


/* New in Mesa 3.1 */
XMesaBuffer XMesaGetCurrentReadBuffer( void )
{
   GET_CURRENT_CONTEXT(ctx);
   if (ctx) {
      return XMESA_BUFFER(ctx->ReadBuffer);
   }
   else {
      return 0;
   }
}



GLboolean XMesaSetFXmode( GLint mode )
{
   (void) mode;
   return GL_FALSE;
}



/*
 * Copy the back buffer to the front buffer.  If there's no back buffer
 * this is a no-op.
 */
PUBLIC
void XMesaSwapBuffers( XMesaBuffer b )
{
   GET_CURRENT_CONTEXT(ctx);

   if (!b->backxrb) {
      /* single buffered */
      return;
   }

   /* If we're swapping the buffer associated with the current context
    * we have to flush any pending rendering commands first.
    */
   if (ctx && ctx->DrawBuffer == &(b->mesa_buffer))
      _mesa_notifySwapBuffers(ctx);

   if (b->db_mode) {
      if (b->backxrb->ximage) {
	 /* Copy Ximage (back buf) from client memory to server window */
#if defined(USE_XSHM) 
	 if (b->shm) {
            /*_glthread_LOCK_MUTEX(_xmesa_lock);*/
	    XShmPutImage( b->xm_visual->display, b->frontxrb->drawable,
			  b->swapgc,
			  b->backxrb->ximage, 0, 0,
			  0, 0, b->mesa_buffer.Width, b->mesa_buffer.Height,
                          False );
            /*_glthread_UNLOCK_MUTEX(_xmesa_lock);*/
	 }
	 else
#endif
         {
            /*_glthread_LOCK_MUTEX(_xmesa_lock);*/
            XMesaPutImage( b->xm_visual->display, b->frontxrb->drawable,
			   b->swapgc,
			   b->backxrb->ximage, 0, 0,
			   0, 0, b->mesa_buffer.Width, b->mesa_buffer.Height );
            /*_glthread_UNLOCK_MUTEX(_xmesa_lock);*/
         }
      }
      else if (b->backxrb->pixmap) {
	 /* Copy pixmap (back buf) to window (front buf) on server */
         /*_glthread_LOCK_MUTEX(_xmesa_lock);*/
	 XMesaCopyArea( b->xm_visual->display,
			b->backxrb->pixmap,   /* source drawable */
			b->frontxrb->drawable,  /* dest. drawable */
			b->swapgc,
			0, 0, b->mesa_buffer.Width, b->mesa_buffer.Height,
			0, 0                 /* dest region */
		      );
         /*_glthread_UNLOCK_MUTEX(_xmesa_lock);*/
      }
   }
   XSync( b->xm_visual->display, False );
}



/*
 * Copy sub-region of back buffer to front buffer
 */
void XMesaCopySubBuffer( XMesaBuffer b, int x, int y, int width, int height )
{
   GET_CURRENT_CONTEXT(ctx);

   /* If we're swapping the buffer associated with the current context
    * we have to flush any pending rendering commands first.
    */
   if (ctx && ctx->DrawBuffer == &(b->mesa_buffer))
      _mesa_notifySwapBuffers(ctx);

   if (!b->backxrb) {
      /* single buffered */
      return; 
   }

   if (b->db_mode) {
      int yTop = b->mesa_buffer.Height - y - height;
      if (b->backxrb->ximage) {
         /* Copy Ximage from host's memory to server's window */
#if defined(USE_XSHM) 
         if (b->shm) {
            /* XXX assuming width and height aren't too large! */
            XShmPutImage( b->xm_visual->display, b->frontxrb->drawable,
                          b->swapgc,
                          b->backxrb->ximage, x, yTop,
                          x, yTop, width, height, False );
            /* wait for finished event??? */
         }
         else
#endif
         {
            /* XXX assuming width and height aren't too large! */
            XMesaPutImage( b->xm_visual->display, b->frontxrb->drawable,
			   b->swapgc,
			   b->backxrb->ximage, x, yTop,
			   x, yTop, width, height );
         }
      }
      else {
         /* Copy pixmap to window on server */
         XMesaCopyArea( b->xm_visual->display,
			b->backxrb->pixmap,           /* source drawable */
			b->frontxrb->drawable,        /* dest. drawable */
			b->swapgc,
			x, yTop, width, height,  /* source region */
			x, yTop                  /* dest region */
                      );
      }
   }
}


/*
 * Return a pointer to the XMesa backbuffer Pixmap or XImage.  This function
 * is a way to get "under the hood" of X/Mesa so one can manipulate the
 * back buffer directly.
 * Output:  pixmap - pointer to back buffer's Pixmap, or 0
 *          ximage - pointer to back buffer's XImage, or NULL
 * Return:  GL_TRUE = context is double buffered
 *          GL_FALSE = context is single buffered
 */
GLboolean XMesaGetBackBuffer( XMesaBuffer b,
                              XMesaPixmap *pixmap,
                              XMesaImage **ximage )
{
   if (b->db_mode) {
      if (pixmap)
         *pixmap = b->backxrb->pixmap;
      if (ximage)
         *ximage = b->backxrb->ximage;
      return GL_TRUE;
   }
   else {
      *pixmap = 0;
      *ximage = NULL;
      return GL_FALSE;
   }
}


/*
 * Return the depth buffer associated with an XMesaBuffer.
 * Input:  b - the XMesa buffer handle
 * Output:  width, height - size of buffer in pixels
 *          bytesPerValue - bytes per depth value (2 or 4)
 *          buffer - pointer to depth buffer values
 * Return:  GL_TRUE or GL_FALSE to indicate success or failure.
 */
GLboolean XMesaGetDepthBuffer( XMesaBuffer b, GLint *width, GLint *height,
                               GLint *bytesPerValue, void **buffer )
{
   struct gl_renderbuffer *rb
      = b->mesa_buffer.Attachment[BUFFER_DEPTH].Renderbuffer;
   struct xmesa_renderbuffer *xrb = xmesa_renderbuffer(rb);

   if (!xrb || !xrb->Base.Buffer) {
      *width = 0;
      *height = 0;
      *bytesPerValue = 0;
      *buffer = 0;
      return GL_FALSE;
   }
   else {
      *width = b->mesa_buffer.Width;
      *height = b->mesa_buffer.Height;
      *bytesPerValue = b->mesa_buffer.Visual.depthBits <= 16
         ? sizeof(GLushort) : sizeof(GLuint);
      *buffer = (void *) xrb->Base.Buffer;
      return GL_TRUE;
   }
}


void XMesaFlush( XMesaContext c )
{
   if (c && c->xm_visual) {
      XSync( c->xm_visual->display, False );
   }
}



const char *XMesaGetString( XMesaContext c, int name )
{
   (void) c;
   if (name==XMESA_VERSION) {
      return "5.0";
   }
   else if (name==XMESA_EXTENSIONS) {
      return "";
   }
   else {
      return NULL;
   }
}



XMesaBuffer XMesaFindBuffer( XMesaDisplay *dpy, XMesaDrawable d )
{
   XMesaBuffer b;
   for (b=XMesaBufferList; b; b=b->Next) {
      if (b->frontxrb->drawable == d && b->display == dpy) {
         return b;
      }
   }
   return NULL;
}


/**
 * Free/destroy all XMesaBuffers associated with given display.
 */
void xmesa_destroy_buffers_on_display(XMesaDisplay *dpy)
{
   XMesaBuffer b, next;
   for (b = XMesaBufferList; b; b = next) {
      next = b->Next;
      if (b->display == dpy) {
         xmesa_free_buffer(b);
      }
   }
}


/*
 * Look for XMesaBuffers whose X window has been destroyed.
 * Deallocate any such XMesaBuffers.
 */
void XMesaGarbageCollect( XMesaDisplay* dpy )
{
   XMesaBuffer b, next;
   for (b=XMesaBufferList; b; b=next) {
      next = b->Next;
      if (b->display && b->display == dpy && b->frontxrb->drawable && b->type == WINDOW) {
         XSync(b->display, False);
         if (!window_exists( b->display, b->frontxrb->drawable )) {
            /* found a dead window, free the ancillary info */
            XMesaDestroyBuffer( b );
         }
      }
   }
}


unsigned long XMesaDitherColor( XMesaContext xmesa, GLint x, GLint y,
                                GLfloat red, GLfloat green,
                                GLfloat blue, GLfloat alpha )
{
   GLint r = (GLint) (red   * 255.0F);
   GLint g = (GLint) (green * 255.0F);
   GLint b = (GLint) (blue  * 255.0F);
   GLint a = (GLint) (alpha * 255.0F);

   switch (xmesa->pixelformat) {
      case PF_Truecolor:
         {
            unsigned long p;
            PACK_TRUECOLOR( p, r, g, b );
            return p;
         }
      case PF_8A8B8G8R:
         return PACK_8A8B8G8R( r, g, b, a );
      case PF_8A8R8G8B:
         return PACK_8A8R8G8B( r, g, b, a );
      case PF_8R8G8B:
         return PACK_8R8G8B( r, g, b );
      case PF_5R6G5B:
         return PACK_5R6G5B( r, g, b );
      case PF_Dither_5R6G5B:
         /* fall through */
      case PF_Dither_True:
         {
            unsigned long p;
            PACK_TRUEDITHER(p, x, y, r, g, b);
            return p;
         }
      default:
         _mesa_problem(NULL, "Bad pixel format in XMesaDitherColor");
   }
   return 0;
}


/*
 * This is typically called when the window size changes and we need
 * to reallocate the buffer's back/depth/stencil/accum buffers.
 */
PUBLIC void
XMesaResizeBuffers( XMesaBuffer b )
{
   GET_CURRENT_CONTEXT(ctx);
   XMesaContext xmctx = XMESA_CONTEXT(ctx);
   if (!xmctx)
      return;
   xmesa_check_and_update_buffer_size(xmctx, b);
}


static GLint
xbuffer_to_renderbuffer(int buffer)
{
   assert(MAX_AUX_BUFFERS <= 4);

   switch (buffer) {
   case GLX_FRONT_LEFT_EXT:
      return BUFFER_FRONT_LEFT;
   case GLX_FRONT_RIGHT_EXT:
      return BUFFER_FRONT_RIGHT;
   case GLX_BACK_LEFT_EXT:
      return BUFFER_BACK_LEFT;
   case GLX_BACK_RIGHT_EXT:
      return BUFFER_BACK_RIGHT;
   case GLX_AUX0_EXT:
      return BUFFER_AUX0;
   case GLX_AUX1_EXT:
   case GLX_AUX2_EXT:
   case GLX_AUX3_EXT:
   case GLX_AUX4_EXT:
   case GLX_AUX5_EXT:
   case GLX_AUX6_EXT:
   case GLX_AUX7_EXT:
   case GLX_AUX8_EXT:
   case GLX_AUX9_EXT:
   default:
      /* BadValue error */
      return -1;
   }
}


PUBLIC void
XMesaBindTexImage(XMesaDisplay *dpy, XMesaBuffer drawable, int buffer,
                  const int *attrib_list)
{
#if 0
   GET_CURRENT_CONTEXT(ctx);
   const GLuint unit = ctx->Texture.CurrentUnit;
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];
   struct gl_texture_object *texObj;
#endif
   struct gl_renderbuffer *rb;
   struct xmesa_renderbuffer *xrb;
   GLint b;
   XMesaImage *img = NULL;
   GLboolean freeImg = GL_FALSE;

   b = xbuffer_to_renderbuffer(buffer);
   if (b < 0)
      return;

   if (drawable->TextureFormat == GLX_TEXTURE_FORMAT_NONE_EXT)
      return; /* BadMatch error */

   rb = drawable->mesa_buffer.Attachment[b].Renderbuffer;
   if (!rb) {
      /* invalid buffer */
      return;
   }
   xrb = xmesa_renderbuffer(rb);

#if 0
   switch (drawable->TextureTarget) {
   case GLX_TEXTURE_1D_EXT:
      texObj = texUnit->CurrentTex[TEXTURE_1D_INDEX];
      break;
   case GLX_TEXTURE_2D_EXT:
      texObj = texUnit->CurrentTex[TEXTURE_2D_INDEX];
      break;
   case GLX_TEXTURE_RECTANGLE_EXT:
      texObj = texUnit->CurrentTex[TEXTURE_RECT_INDEX];
      break;
   default:
      return; /* BadMatch error */
   }
#endif

   /*
    * The following is a quick and simple way to implement
    * BindTexImage.  The better way is to write some new FetchTexel()
    * functions which would extract texels from XImages.  We'd still
    * need to use GetImage when texturing from a Pixmap (front buffer)
    * but texturing from a back buffer (XImage) would avoid an image
    * copy.
    */

   /* get XImage */
   if (xrb->pixmap) {
      img = XMesaGetImage(dpy, xrb->pixmap, 0, 0, rb->Width, rb->Height, ~0L,
			  ZPixmap);
      freeImg = GL_TRUE;
   }
   else if (xrb->ximage) {
      img = xrb->ximage;
   }

   /* store the XImage as a new texture image */
   if (img) {
      GLenum format, type, intFormat;
      if (img->bits_per_pixel == 32) {
         format = GL_BGRA;
         type = GL_UNSIGNED_BYTE;
         intFormat = GL_RGBA;
      }
      else if (img->bits_per_pixel == 24) {
         format = GL_BGR;
         type = GL_UNSIGNED_BYTE;
         intFormat = GL_RGB;
      }
      else if (img->bits_per_pixel == 16) {
         format = GL_BGR;
         type = GL_UNSIGNED_SHORT_5_6_5;
         intFormat = GL_RGB;
      }
      else {
         _mesa_problem(NULL, "Unexpected XImage format in XMesaBindTexImage");
         return;
      }
      if (drawable->TextureFormat == GLX_TEXTURE_FORMAT_RGBA_EXT) {
         intFormat = GL_RGBA;
      }
      else if (drawable->TextureFormat == GLX_TEXTURE_FORMAT_RGB_EXT) {
         intFormat = GL_RGB;
      }

      _mesa_TexImage2D(GL_TEXTURE_2D, 0, intFormat, rb->Width, rb->Height, 0,
                       format, type, img->data);

      if (freeImg) {
	 XMesaDestroyImage(img);
      }
   }
}



PUBLIC void
XMesaReleaseTexImage(XMesaDisplay *dpy, XMesaBuffer drawable, int buffer)
{
   const GLint b = xbuffer_to_renderbuffer(buffer);
   if (b < 0)
      return;

   /* no-op for now */
}

