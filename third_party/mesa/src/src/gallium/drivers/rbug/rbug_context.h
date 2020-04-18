/**************************************************************************
 *
 * Copyright 2010 VMware, Inc.
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

#ifndef RBUG_CONTEXT_H
#define RBUG_CONTEXT_H

#include "pipe/p_state.h"
#include "pipe/p_context.h"

#include "rbug_screen.h"


struct rbug_context {
   struct pipe_context base;  /**< base class */

   struct pipe_context *pipe;

   struct rbug_list list;

   /* call locking */
   pipe_mutex call_mutex;

   /* current state */
   struct {
      struct rbug_shader *shader[PIPE_SHADER_TYPES];

      struct rbug_sampler_view *views[PIPE_SHADER_TYPES][PIPE_MAX_SAMPLERS];
      struct rbug_resource *texs[PIPE_SHADER_TYPES][PIPE_MAX_SAMPLERS];
      unsigned num_views[PIPE_SHADER_TYPES];

      unsigned nr_cbufs;
      struct rbug_resource *cbufs[PIPE_MAX_COLOR_BUFS];
      struct rbug_resource *zsbuf;
   } curr;

   /* draw locking */
   pipe_mutex draw_mutex;
   pipe_condvar draw_cond;
   unsigned draw_num_rules;
   int draw_blocker;
   int draw_blocked;

   struct {
      struct rbug_shader *shader[PIPE_SHADER_TYPES];

      struct rbug_resource *texture;
      struct rbug_resource *surf;

      int blocker;
   } draw_rule;

   /* list of state objects */
   pipe_mutex list_mutex;
   unsigned num_shaders;
   struct rbug_list shaders;
};

static INLINE struct rbug_context *
rbug_context(struct pipe_context *pipe)
{
   return (struct rbug_context *)pipe;
}


/**********************************************************
 * rbug_context.c
 */

struct pipe_context *
rbug_context_create(struct pipe_screen *screen, struct pipe_context *pipe);


/**********************************************************
 * rbug_core.c
 */

void rbug_notify_draw_blocked(struct rbug_context *rb_context);


#endif /* RBUG_CONTEXT_H */
