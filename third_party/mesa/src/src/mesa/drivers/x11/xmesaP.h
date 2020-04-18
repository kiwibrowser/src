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


#ifndef XMESAP_H
#define XMESAP_H


#include "xmesa.h"
#include "main/mtypes.h"
#include "swrast/s_context.h"


extern _glthread_Mutex _xmesa_lock;

extern XMesaBuffer XMesaBufferList;

/* for PF_8R8G8B24 pixel format */
typedef struct {
   GLubyte b;
   GLubyte g;
   GLubyte r;
} bgr_t;


struct xmesa_renderbuffer;


/* Function pointer for clearing color buffers */
typedef void (*ClearFunc)( struct gl_context *ctx, struct xmesa_renderbuffer *xrb,
                           GLint x, GLint y, GLint width, GLint height );




/** Framebuffer pixel formats */
enum pixel_format {
   PF_Truecolor,	/**< TrueColor or DirectColor, any depth */
   PF_Dither_True,	/**< TrueColor with dithering */
   PF_8A8R8G8B,		/**< 32-bit TrueColor:  8-A, 8-R, 8-G, 8-B bits */
   PF_8A8B8G8R,		/**< 32-bit TrueColor:  8-A, 8-B, 8-G, 8-R bits */
   PF_8R8G8B,		/**< 32-bit TrueColor:  8-R, 8-G, 8-B bits */
   PF_8R8G8B24,		/**< 24-bit TrueColor:  8-R, 8-G, 8-B bits */
   PF_5R6G5B,		/**< 16-bit TrueColor:  5-R, 6-G, 5-B bits */
   PF_Dither_5R6G5B	/**< 16-bit dithered TrueColor: 5-R, 6-G, 5-B */
};


/**
 * Visual inforation, derived from struct gl_config.
 * Basically corresponds to an XVisualInfo.
 */
struct xmesa_visual {
   struct gl_config mesa_visual;	/* Device independent visual parameters */
   XMesaDisplay *display;	/* The X11 display */
   int screen, visualID;
   int visualType;
   XMesaVisualInfo visinfo;	/* X's visual info (pointer to private copy) */
   XVisualInfo *vishandle;	/* Only used in fakeglx.c */
   GLint BitsPerPixel;		/* True bits per pixel for XImages */

   GLboolean ximage_flag;	/* Use XImage for back buffer (not pixmap)? */

   enum pixel_format dithered_pf;  /* Pixel format when dithering */
   enum pixel_format undithered_pf;/* Pixel format when not dithering */

   GLfloat RedGamma;		/* Gamma values, 1.0 is default */
   GLfloat GreenGamma;
   GLfloat BlueGamma;

   /* For PF_TRUECOLOR */
   GLint rshift, gshift, bshift;/* Pixel color component shifts */
   GLubyte Kernel[16];		/* Dither kernel */
   unsigned long RtoPixel[512];	/* RGB to pixel conversion */
   unsigned long GtoPixel[512];
   unsigned long BtoPixel[512];
   GLubyte PixelToR[256];	/* Pixel to RGB conversion */
   GLubyte PixelToG[256];
   GLubyte PixelToB[256];
};


/**
 * Context info, derived from struct gl_context.
 * Basically corresponds to a GLXContext.
 */
struct xmesa_context {
   struct gl_context mesa;		/* the core library context (containment) */
   XMesaVisual xm_visual;	/* Describes the buffers */
   XMesaBuffer xm_buffer;	/* current span/point/line/triangle buffer */

   XMesaDisplay *display;	/* == xm_visual->display */
   GLboolean swapbytes;		/* Host byte order != display byte order? */
   GLboolean direct;		/* Direct rendering context? */

   enum pixel_format pixelformat;

   GLubyte clearcolor[4];		/* current clearing color */
   unsigned long clearpixel;		/* current clearing pixel value */
};


/**
 * Types of X/GLX drawables we might render into.
 */
typedef enum {
   WINDOW,          /* An X window */
   GLXWINDOW,       /* GLX window */
   PIXMAP,          /* GLX pixmap */
   PBUFFER          /* GLX Pbuffer */
} BufferType;


/** Values for db_mode: */
/*@{*/
#define BACK_PIXMAP	1
#define BACK_XIMAGE	2
/*@}*/


/**
 * An xmesa_renderbuffer represents the back or front color buffer.
 * For the front color buffer:
 *    <drawable> is the X window
 * For the back color buffer:
 *    Either <ximage> or <pixmap> will be used, never both.
 * In any case, <drawable> always equals <pixmap>.
 * For stand-alone Mesa, we could merge <drawable> and <pixmap> into one
 * field.  We don't do that for the server-side GLcore module because
 * pixmaps and drawables are different and we'd need a bunch of casts.
 */
struct xmesa_renderbuffer
{
   struct swrast_renderbuffer Base;  /* Base class */

   XMesaBuffer Parent;  /**< The XMesaBuffer this renderbuffer belongs to */
   XMesaDrawable drawable;	/* Usually the X window ID */
   XMesaPixmap pixmap;	/* Back color buffer */
   XMesaImage *ximage;	/* The back buffer, if not using a Pixmap */

   GLushort *origin2;	/* used for PIXEL_ADDR2 macro */
   GLint width2;
   GLubyte *origin3;	/* used for PIXEL_ADDR3 macro */
   GLint width3;
   GLuint *origin4;	/* used for PIXEL_ADDR4 macro */
   GLint width4;

   GLint bottom;	/* used for FLIP macro, equals height - 1 */

   ClearFunc clearFunc;

   GLuint map_x, map_y, map_w, map_h;
   GLbitfield map_mode;
   XMesaImage *map_ximage;
};


/**
 * Framebuffer information, derived from.
 * Basically corresponds to a GLXDrawable.
 */
struct xmesa_buffer {
   struct gl_framebuffer mesa_buffer;	/* depth, stencil, accum, etc buffers */
				/* This MUST BE FIRST! */
   GLboolean wasCurrent;	/* was ever the current buffer? */
   XMesaVisual xm_visual;	/* the X/Mesa visual */

   XMesaDisplay *display;
   BufferType type;             /* window, pixmap, pbuffer or glxwindow */

   GLboolean largestPbuffer;    /**< for pbuffers */
   GLboolean preservedContents; /**< for pbuffers */

   struct xmesa_renderbuffer *frontxrb; /* front color renderbuffer */
   struct xmesa_renderbuffer *backxrb;  /* back color renderbuffer */

   XMesaColormap cmap;		/* the X colormap */

   unsigned long selectedEvents;/* for pbuffers only */

   GLint db_mode;		/* 0 = single buffered */
				/* BACK_PIXMAP = use Pixmap for back buffer */
				/* BACK_XIMAGE = use XImage for back buffer */
   GLuint shm;			/* X Shared Memory extension status:	*/
				/*    0 = not available			*/
				/*    1 = XImage support available	*/
				/*    2 = Pixmap support available too	*/
#if defined(USE_XSHM) 
   XShmSegmentInfo shminfo;
#endif

   //   XMesaImage *rowimage;	/* Used for optimized span writing */
   XMesaPixmap stipple_pixmap;	/* For polygon stippling */
   XMesaGC stipple_gc;		/* For polygon stippling */

   XMesaGC gc;			/* scratch GC for span, line, tri drawing */
   XMesaGC cleargc;		/* GC for clearing the color buffer */
   XMesaGC swapgc;		/* GC for swapping the color buffers */

   /* The following are here instead of in the XMesaVisual
    * because they depend on the window's colormap.
    */

   /* For PF_DITHER, PF_LOOKUP, PF_GRAYSCALE */
   unsigned long color_table[576];	/* RGB -> pixel value */

   /* For PF_DITHER, PF_LOOKUP, PF_GRAYSCALE */
   GLubyte pixel_to_r[65536];		/* pixel value -> red */
   GLubyte pixel_to_g[65536];		/* pixel value -> green */
   GLubyte pixel_to_b[65536];		/* pixel value -> blue */

   /* Used to do XAllocColor/XFreeColors accounting: */
   int num_alloced;
   unsigned long alloced_colors[256];

   /* GLX_EXT_texture_from_pixmap */
   GLint TextureTarget; /** GLX_TEXTURE_1D_EXT, for example */
   GLint TextureFormat; /** GLX_TEXTURE_FORMAT_RGB_EXT, for example */
   GLint TextureMipmap; /** 0 or 1 */

   struct xmesa_buffer *Next;	/* Linked list pointer: */
};


/**
 * If pixelformat==PF_TRUECOLOR:
 */
#define PACK_TRUECOLOR( PIXEL, R, G, B )	\
   PIXEL = xmesa->xm_visual->RtoPixel[R]	\
         | xmesa->xm_visual->GtoPixel[G]	\
         | xmesa->xm_visual->BtoPixel[B];	\


/**
 * If pixelformat==PF_TRUEDITHER:
 */
#define PACK_TRUEDITHER( PIXEL, X, Y, R, G, B )			\
{								\
   int d = xmesa->xm_visual->Kernel[((X)&3) | (((Y)&3)<<2)];	\
   PIXEL = xmesa->xm_visual->RtoPixel[(R)+d]			\
         | xmesa->xm_visual->GtoPixel[(G)+d]			\
         | xmesa->xm_visual->BtoPixel[(B)+d];			\
}



/**
 * If pixelformat==PF_8A8B8G8R:
 */
#define PACK_8A8B8G8R( R, G, B, A )	\
	( ((A) << 24) | ((B) << 16) | ((G) << 8) | (R) )


/**
 * Like PACK_8A8B8G8R() but don't use alpha.  This is usually an acceptable
 * shortcut.
 */
#define PACK_8B8G8R( R, G, B )   ( ((B) << 16) | ((G) << 8) | (R) )



/**
 * If pixelformat==PF_8R8G8B:
 */
#define PACK_8R8G8B( R, G, B)	 ( ((R) << 16) | ((G) << 8) | (B) )


/**
 * If pixelformat==PF_5R6G5B:
 */
#define PACK_5R6G5B( R, G, B)	 ( (((R) & 0xf8) << 8) | (((G) & 0xfc) << 3) | ((B) >> 3) )


/**
 * If pixelformat==PF_8A8R8G8B:
 */
#define PACK_8A8R8G8B( R, G, B, A )	\
	( ((A) << 24) | ((R) << 16) | ((G) << 8) | (B) )




/**
 * Converts a GL window Y coord to an X window Y coord:
 */
#define YFLIP(XRB, Y)  ((XRB)->bottom - (Y))


/**
 * Return the address of a 2, 3 or 4-byte pixel in the buffer's XImage:
 * X==0 is left, Y==0 is bottom.
 */
#define PIXEL_ADDR2(XRB, X, Y)  \
   ( (XRB)->origin2 - (Y) * (XRB)->width2 + (X) )

#define PIXEL_ADDR3(XRB, X, Y)  \
   ( (bgr_t *) ( (XRB)->origin3 - (Y) * (XRB)->width3 + 3 * (X) ))

#define PIXEL_ADDR4(XRB, X, Y)  \
   ( (XRB)->origin4 - (Y) * (XRB)->width4 + (X) )



/*
 * External functions:
 */

extern struct xmesa_renderbuffer *
xmesa_new_renderbuffer(struct gl_context *ctx, GLuint name,
                       const struct xmesa_visual *xmvis,
                       GLboolean backBuffer);

extern void
xmesa_delete_framebuffer(struct gl_framebuffer *fb);

extern XMesaBuffer
xmesa_find_buffer(XMesaDisplay *dpy, XMesaColormap cmap, XMesaBuffer notThis);

extern unsigned long
xmesa_color_to_pixel( struct gl_context *ctx,
                      GLubyte r, GLubyte g, GLubyte b, GLubyte a,
                      GLuint pixelFormat );

extern void
xmesa_get_window_size(XMesaDisplay *dpy, XMesaBuffer b,
                      GLuint *width, GLuint *height);

extern void
xmesa_check_and_update_buffer_size(XMesaContext xmctx, XMesaBuffer drawBuffer);

extern void
xmesa_init_driver_functions( XMesaVisual xmvisual,
                             struct dd_function_table *driver );

extern void
xmesa_update_state( struct gl_context *ctx, GLbitfield new_state );


extern void
xmesa_MapRenderbuffer(struct gl_context *ctx,
                      struct gl_renderbuffer *rb,
                      GLuint x, GLuint y, GLuint w, GLuint h,
                      GLbitfield mode,
                      GLubyte **mapOut, GLint *rowStrideOut);

extern void
xmesa_UnmapRenderbuffer(struct gl_context *ctx, struct gl_renderbuffer *rb);

extern void
xmesa_destroy_buffers_on_display(XMesaDisplay *dpy);


/**
 * Using a function instead of an ordinary cast is safer.
 */
static INLINE struct xmesa_renderbuffer *
xmesa_renderbuffer(struct gl_renderbuffer *rb)
{
   return (struct xmesa_renderbuffer *) rb;
}


/**
 * Return pointer to XMesaContext corresponding to a Mesa struct gl_context.
 * Since we're using structure containment, it's just a cast!.
 */
static INLINE XMesaContext
XMESA_CONTEXT(struct gl_context *ctx)
{
   return (XMesaContext) ctx;
}


/**
 * Return pointer to XMesaBuffer corresponding to a Mesa struct gl_framebuffer.
 * Since we're using structure containment, it's just a cast!.
 */
static INLINE XMesaBuffer
XMESA_BUFFER(struct gl_framebuffer *b)
{
   return (XMesaBuffer) b;
}


/* Plugged into the software rasterizer.  Try to use internal
 * swrast-style point, line and triangle functions.
 */
extern void xmesa_choose_point( struct gl_context *ctx );
extern void xmesa_choose_line( struct gl_context *ctx );
extern void xmesa_choose_triangle( struct gl_context *ctx );


extern void xmesa_register_swrast_functions( struct gl_context *ctx );



#if   defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#define ENABLE_EXT_timer_query 1 /* should have 64-bit GLuint64EXT */
#else
#define ENABLE_EXT_timer_query 0 /* may not have 64-bit GLuint64EXT */
#endif


#define TEST_META_FUNCS 0


#endif
