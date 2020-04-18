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

#ifndef _X11_SCREEN_H_
#define _X11_SCREEN_H_

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/dri2tokens.h>
#include "GL/gl.h" /* for GL types needed by __GLcontextModes */
#include "glcore.h"  /* for __GLcontextModes */
#include "pipe/p_compiler.h"
#include "common/native.h"

enum x11_screen_extension {
   X11_SCREEN_EXTENSION_XSHM,
   X11_SCREEN_EXTENSION_GLX,
   X11_SCREEN_EXTENSION_DRI2,
};

/* the same as DRI2Buffer */
struct x11_drawable_buffer {
   unsigned int attachment;
   unsigned int name;
   unsigned int pitch;
   unsigned int cpp;
   unsigned int flags;
};

struct x11_screen;

typedef void (*x11_drawable_invalidate_buffers)(struct x11_screen *xscr,
                                                Drawable drawable,
                                                void *user_data);

struct x11_screen *
x11_screen_create(Display *dpy, int screen);

void
x11_screen_destroy(struct x11_screen *xscr);

boolean
x11_screen_support(struct x11_screen *xscr, enum x11_screen_extension ext);

const XVisualInfo *
x11_screen_get_visuals(struct x11_screen *xscr, int *num_visuals);

uint
x11_drawable_get_depth(struct x11_screen *xscr, Drawable drawable);

#ifdef GLX_DIRECT_RENDERING

/* GLX */
const __GLcontextModes *
x11_screen_get_glx_configs(struct x11_screen *xscr);

const __GLcontextModes *
x11_screen_get_glx_visuals(struct x11_screen *xscr);

__GLcontextModes *
x11_context_modes_create(unsigned count);

void
x11_context_modes_destroy(__GLcontextModes *modes);

unsigned
x11_context_modes_count(const __GLcontextModes *modes);

/* DRI2 */
const char *
x11_screen_probe_dri2(struct x11_screen *xscr, int *major, int *minor);

int
x11_screen_enable_dri2(struct x11_screen *xscr,
                       x11_drawable_invalidate_buffers invalidate_buffers,
                       void *user_data);

char *
x11_screen_get_device_name(struct x11_screen *xscr);

int
x11_screen_authenticate(struct x11_screen *xscr, uint32_t id);

void
x11_drawable_enable_dri2(struct x11_screen *xscr,
                         Drawable drawable, boolean on);

void
x11_drawable_copy_buffers_region(struct x11_screen *xscr, Drawable drawable,
                                 int num_rects, const int *rects,
                                 int src_buf, int dst_buf);

/**
 * Copy between buffers of the DRI2 drawable.
 */
static INLINE void
x11_drawable_copy_buffers(struct x11_screen *xscr, Drawable drawable,
                          int x, int y, int width, int height,
                          int src_buf, int dst_buf)
{
    int rect[4] = { x, y, width, height };
    x11_drawable_copy_buffers_region(xscr, drawable, 1, rect, src_buf, dst_buf);
}

struct x11_drawable_buffer *
x11_drawable_get_buffers(struct x11_screen *xscr, Drawable drawable,
                         int *width, int *height, unsigned int *attachments,
                         boolean with_format, int num_ins, int *num_outs);

#endif /* GLX_DIRECT_RENDERING */

#endif /* _X11_SCREEN_H_ */
