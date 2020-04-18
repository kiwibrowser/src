
/**************************************************************************
 * 
 * Copyright 2006 Tungsten Graphics, Inc., Cedar Park, Texas.
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

#ifndef INTEL_BUFFERS_H
#define INTEL_BUFFERS_H

#include "dri_util.h"
#include "drm.h"
#include "intel_context.h"

struct intel_context;
struct intel_framebuffer;

extern struct intel_region *intel_readbuf_region(struct intel_context *intel);

extern struct intel_region *intel_drawbuf_region(struct intel_context *intel);

extern void intel_check_front_buffer_rendering(struct intel_context *intel);

static inline void
intel_draw_buffer(struct gl_context * ctx)
{
   struct intel_context *intel = intel_context(ctx);

   intel->vtbl.update_draw_buffer(intel);
}

extern void intelInitBufferFuncs(struct dd_function_table *functions);
#ifdef I915
void intelCalcViewport(struct gl_context * ctx);
#endif

#endif /* INTEL_BUFFERS_H */
