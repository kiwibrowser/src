/*
 * Mesa 3-D graphics library
 * Version:  7.1
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
 * Copyright 2008, 2010 George Sapountzis <gsapountzis@gmail.com>
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


#ifndef _SWRAST_PRIV_H
#define _SWRAST_PRIV_H

#include <GL/gl.h>
#include <GL/internal/dri_interface.h>
#include "main/mtypes.h"
#include "dri_util.h"
#include "swrast/s_context.h"


/**
 * Debugging
 */
#define DEBUG_CORE	0
#define DEBUG_SPAN	0

#if DEBUG_CORE
#define TRACE printf("--> %s\n", __FUNCTION__)
#else
#define TRACE
#endif

#if DEBUG_SPAN
#define TRACE_SPAN printf("--> %s\n", __FUNCTION__)
#else
#define TRACE_SPAN
#endif


/**
 * Data types
 */
struct dri_context
{
    /* mesa, base class, must be first */
    struct gl_context Base;

    /* dri */
    __DRIcontext *cPriv;
};

static INLINE struct dri_context *
dri_context(__DRIcontext * driContextPriv)
{
    return (struct dri_context *)driContextPriv->driverPrivate;
}

static INLINE struct dri_context *
swrast_context(struct gl_context *ctx)
{
    return (struct dri_context *) ctx;
}

struct dri_drawable
{
    /* mesa, base class, must be first */
    struct gl_framebuffer Base;

    /* dri */
    __DRIdrawable *dPriv;

    /* scratch row for optimized front-buffer rendering */
    char *row;
};

static INLINE struct dri_drawable *
dri_drawable(__DRIdrawable * driDrawPriv)
{
    return (struct dri_drawable *)driDrawPriv->driverPrivate;
}

static INLINE struct dri_drawable *
swrast_drawable(struct gl_framebuffer *fb)
{
    return (struct dri_drawable *) fb;
}

struct dri_swrast_renderbuffer {
    struct swrast_renderbuffer Base;
    __DRIdrawable *dPriv;

    /* GL_MAP_*_BIT, used for mapping of front buffer. */
    GLbitfield map_mode;
   int map_x, map_y, map_w, map_h;

    /* renderbuffer pitch (in bytes) */
    GLuint pitch;
   /* bits per pixel of storage */
    GLuint bpp;
};

static INLINE struct dri_swrast_renderbuffer *
dri_swrast_renderbuffer(struct gl_renderbuffer *rb)
{
    return (struct dri_swrast_renderbuffer *) rb;
}


/**
 * Pixel formats we support
 */
#define PF_A8R8G8B8   1		/**< 32bpp TrueColor:  8-A, 8-R, 8-G, 8-B bits */
#define PF_R5G6B5     2		/**< 16bpp TrueColor:  5-R, 6-G, 5-B bits */
#define PF_R3G3B2     3		/**<  8bpp TrueColor:  3-R, 3-G, 2-B bits */
#define PF_X8R8G8B8   4		/**< 32bpp TrueColor:  8-R, 8-G, 8-B bits */


#endif /* _SWRAST_PRIV_H_ */
