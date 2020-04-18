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

#ifndef _XM_ST_H_
#define _XM_ST_H_

#include "pipe/p_compiler.h"
#include "state_tracker/st_api.h"

#include "xm_api.h"

struct st_framebuffer_iface *
xmesa_create_st_framebuffer(XMesaDisplay xmdpy, XMesaBuffer b);

void
xmesa_destroy_st_framebuffer(struct st_framebuffer_iface *stfbi);

void
xmesa_swap_st_framebuffer(struct st_framebuffer_iface *stfbi);

void
xmesa_copy_st_framebuffer(struct st_framebuffer_iface *stfbi,
                          enum st_attachment_type src,
                          enum st_attachment_type dst,
                          int x, int y, int w, int h);

struct pipe_resource*
xmesa_get_attachment(struct st_framebuffer_iface *stfbi,
                     enum st_attachment_type st_attachment);

struct pipe_context*
xmesa_get_context(struct st_framebuffer_iface* stfbi);

boolean
xmesa_st_framebuffer_validate_textures(struct st_framebuffer_iface *stfbi,
                                       unsigned width, unsigned height,
                                       unsigned mask);
#endif /* _XM_ST_H_ */
