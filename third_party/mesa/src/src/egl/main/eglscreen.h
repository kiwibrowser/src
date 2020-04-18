/**************************************************************************
 *
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
 * Copyright 2009-2010 Chia-I Wu <olvaffe@gmail.com>
 * Copyright 2010 LunarG, Inc.
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
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/


#ifndef EGLSCREEN_INCLUDED
#define EGLSCREEN_INCLUDED


#include "egltypedefs.h"


#ifdef EGL_MESA_screen_surface


#define _EGL_SCREEN_MAX_MODES 16


/**
 * Per-screen information.
 * Note that an EGL screen doesn't have a size.  A screen may be set to
 * one of several display modes (width/height/scanrate).  The screen
 * then displays a drawing surface.  The drawing surface must be at least
 * as large as the display mode's resolution.  If it's larger, the
 * OriginX and OriginY fields control what part of the surface is visible
 * on the screen.
 */
struct _egl_screen
{
   _EGLDisplay *Display;

   EGLScreenMESA Handle; /* The public/opaque handle which names this object */

   _EGLMode *CurrentMode;
   _EGLSurface *CurrentSurface;

   EGLint OriginX, OriginY; /**< Origin of scan-out region w.r.t. surface */
   EGLint StepX, StepY;     /**< Screen position/origin granularity */

   EGLint NumModes;
   _EGLMode *Modes;  /**< array [NumModes] */
};


PUBLIC void
_eglInitScreen(_EGLScreen *screen, _EGLDisplay *dpy, EGLint num_modes);


PUBLIC EGLScreenMESA
_eglLinkScreen(_EGLScreen *screen);


extern _EGLScreen *
_eglLookupScreen(EGLScreenMESA screen, _EGLDisplay *dpy);


/**
 * Return the handle of a linked screen.
 */
static INLINE EGLScreenMESA
_eglGetScreenHandle(_EGLScreen *screen)
{
   return (screen) ? screen->Handle : (EGLScreenMESA) 0;
}


extern EGLBoolean
_eglGetScreensMESA(_EGLDriver *drv, _EGLDisplay *dpy, EGLScreenMESA *screens, EGLint max_screens, EGLint *num_screens);


extern EGLBoolean
_eglScreenPositionMESA(_EGLDriver *drv, _EGLDisplay *dpy, _EGLScreen *scrn, EGLint x, EGLint y);


extern EGLBoolean
_eglQueryScreenSurfaceMESA(_EGLDriver *drv, _EGLDisplay *dpy,
                           _EGLScreen *scrn, _EGLSurface **surface);


extern EGLBoolean
_eglQueryScreenModeMESA(_EGLDriver *drv, _EGLDisplay *dpy, _EGLScreen *scrn, _EGLMode **m);


extern EGLBoolean
_eglQueryScreenMESA(_EGLDriver *drv, _EGLDisplay *dpy, _EGLScreen *scrn, EGLint attribute, EGLint *value);


#endif /* EGL_MESA_screen_surface */


#endif /* EGLSCREEN_INCLUDED */
