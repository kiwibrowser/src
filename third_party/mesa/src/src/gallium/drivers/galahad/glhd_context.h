/**************************************************************************
 *
 * Copyright 2009 VMware, Inc.
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
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#ifndef GLHD_CONTEXT_H
#define GLHD_CONTEXT_H

#include <stdio.h>

#include "pipe/p_state.h"
#include "pipe/p_context.h"

#include "util/u_debug.h"


struct galahad_context {
   struct pipe_context base;  /**< base class */

   struct pipe_context *pipe;
};


struct pipe_context *
galahad_context_create(struct pipe_screen *screen, struct pipe_context *pipe);


static INLINE struct galahad_context *
galahad_context(struct pipe_context *pipe)
{
   return (struct galahad_context *)pipe;
}

#define glhd_warn(...) \
do { \
    debug_printf("galahad: %s: ", __FUNCTION__); \
    debug_printf(__VA_ARGS__); \
    debug_printf("\n"); \
} while (0)

#define glhd_check(fmt, value, expr) \
   ((value expr) ? (void)0 : debug_printf("galahad: %s:%u: Expected `%s %s`, got %s == " fmt "\n", __FUNCTION__, __LINE__, #value, #expr, #value, value))

#define glhd_error(...) \
    glhd_warn(__VA_ARGS__);

#endif /* GLHD_CONTEXT_H */
