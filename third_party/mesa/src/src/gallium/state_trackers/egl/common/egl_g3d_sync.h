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

#ifndef _EGL_G3D_SYNC_H_
#define _EGL_G3D_SYNC_H_

#include "egl_g3d.h"

_EGLSync *
egl_g3d_create_sync(_EGLDriver *drv, _EGLDisplay *dpy,
                    EGLenum type, const EGLint *attrib_list);

EGLBoolean
egl_g3d_destroy_sync(_EGLDriver *drv, _EGLDisplay *dpy, _EGLSync *sync);

EGLint
egl_g3d_client_wait_sync(_EGLDriver *drv, _EGLDisplay *dpy, _EGLSync *sync,
                         EGLint flags, EGLTimeKHR timeout);

EGLBoolean
egl_g3d_signal_sync(_EGLDriver *drv, _EGLDisplay *dpy, _EGLSync *sync,
                    EGLenum mode);

#endif /* _EGL_G3D_SYNC_H_ */
