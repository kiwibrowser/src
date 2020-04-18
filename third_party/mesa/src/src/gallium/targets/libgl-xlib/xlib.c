/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Bismarck, ND., USA
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR 
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE 
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * 
 **************************************************************************/

/*
 * Authors:
 *   Keith Whitwell
 */
#include "pipe/p_compiler.h"
#include "util/u_debug.h"
#include "state_tracker/xlib_sw_winsys.h"
#include "xm_public.h"

#include "state_tracker/st_gl_api.h"
#include "target-helpers/inline_sw_helper.h"
#include "target-helpers/inline_debug_helper.h"



/* Helper function to build a subset of a driver stack consisting of
 * one of the software rasterizers (llvmpipe, softpipe) and the
 * xlib winsys.
 */
static struct pipe_screen *
swrast_xlib_create_screen( Display *display )
{
   struct sw_winsys *winsys;
   struct pipe_screen *screen = NULL;

   /* Create the underlying winsys, which performs presents to Xlib
    * drawables:
    */
   winsys = xlib_create_sw_winsys( display );
   if (winsys == NULL)
      return NULL;

   /* Create a software rasterizer on top of that winsys:
    */
   screen = sw_screen_create( winsys );
   if (screen == NULL)
      goto fail;

   /* Inject any wrapping layers we want to here:
    */
   return debug_screen_wrap( screen );

fail:
   if (winsys)
      winsys->destroy( winsys );

   return NULL;
}

static struct xm_driver xlib_driver = 
{
   .create_pipe_screen = swrast_xlib_create_screen,
   .create_st_api = st_gl_api_create,
};


/* Build the rendering stack.
 */
static void _init( void ) __attribute__((constructor));
static void _init( void )
{
   /* Initialize the xlib libgl code, pass in the winsys:
    */
   xmesa_set_driver( &xlib_driver );
}


/***********************************************************************
 *
 * Butt-ugly hack to convince the linker not to throw away public GL
 * symbols (they are all referenced from getprocaddress, I guess).
 */
extern void (*linker_foo(const unsigned char *procName))();
extern void (*glXGetProcAddress(const unsigned char *procName))();

extern void (*linker_foo(const unsigned char *procName))()
{
   return glXGetProcAddress(procName);
}


/**
 * When GLX_INDIRECT_RENDERING is defined, some symbols are missing in
 * libglapi.a.  We need to define them here.
 */
#ifdef GLX_INDIRECT_RENDERING

#define GL_GLEXT_PROTOTYPES
#include "GL/gl.h"
#include "glapi/glapi.h"
#include "glapi/glapitable.h"

#if defined(USE_MGL_NAMESPACE)
#define NAME(func)  mgl##func
#else
#define NAME(func)  gl##func
#endif

#define DISPATCH(FUNC, ARGS, MESSAGE)		\
   GET_DISPATCH()->FUNC ARGS

#define RETURN_DISPATCH(FUNC, ARGS, MESSAGE) 	\
   return GET_DISPATCH()->FUNC ARGS

/* skip normal ones */
#define _GLAPI_SKIP_NORMAL_ENTRY_POINTS
#include "glapi/glapitemp.h"

#endif /* GLX_INDIRECT_RENDERING */
