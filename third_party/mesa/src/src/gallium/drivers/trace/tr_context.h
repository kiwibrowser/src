/**************************************************************************
 *
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
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

#ifndef TR_CONTEXT_H_
#define TR_CONTEXT_H_


#include "pipe/p_compiler.h"
#include "util/u_debug.h"
#include "pipe/p_context.h"

#include "tr_screen.h"

#ifdef __cplusplus
extern "C" {
#endif


struct trace_screen;
   
struct trace_context
{
   struct pipe_context base;

   struct pipe_context *pipe;
};


static INLINE struct trace_context *
trace_context(struct pipe_context *pipe)
{
   assert(pipe);
   return (struct trace_context *)pipe;
}


struct pipe_context *
trace_context_create(struct trace_screen *tr_scr,
                     struct pipe_context *pipe);


#ifdef __cplusplus
}
#endif

#endif /* TR_CONTEXT_H_ */
