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


#ifndef EGLMODE_INCLUDED
#define EGLMODE_INCLUDED

#include "egltypedefs.h"


#ifdef EGL_MESA_screen_surface


#define EGL_NO_MODE_MESA 0


/**
 * Data structure which corresponds to an EGLModeMESA.
 */
struct _egl_mode
{
   EGLModeMESA Handle;     /* the public/opaque handle which names this mode */
   EGLint Width, Height;   /* size in pixels */
   EGLint RefreshRate;     /* rate * 1000.0 */
   EGLint Optimal;
   EGLint Interlaced;
   const char *Name;

   /* Other possible attributes */
   /* interlaced */
   /* external sync */
};


extern _EGLMode *
_eglLookupMode(EGLModeMESA mode, _EGLDisplay *dpy);


extern EGLBoolean
_eglChooseModeMESA(_EGLDriver *drv, _EGLDisplay *dpy, _EGLScreen *scrn,
                   const EGLint *attrib_list, EGLModeMESA *modes,
                   EGLint modes_size, EGLint *num_modes);


extern EGLBoolean
_eglGetModesMESA(_EGLDriver *drv, _EGLDisplay *dpy, _EGLScreen *scrn,
                 EGLModeMESA *modes, EGLint modes_size, EGLint *num_modes);


extern EGLBoolean
_eglGetModeAttribMESA(_EGLDriver *drv, _EGLDisplay *dpy, _EGLMode *m,
                      EGLint attribute, EGLint *value);


extern const char *
_eglQueryModeStringMESA(_EGLDriver *drv, _EGLDisplay *dpy, _EGLMode *m);


#endif /* EGL_MESA_screen_surface */


#endif /* EGLMODE_INCLUDED */
