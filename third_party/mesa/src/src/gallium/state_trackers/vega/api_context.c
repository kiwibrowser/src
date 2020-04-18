/**************************************************************************
 *
 * Copyright 2009 VMware, Inc.  All Rights Reserved.
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

#include "VG/openvg.h"

#include "vg_manager.h"
#include "vg_context.h"
#include "api.h"

#include "pipe/p_context.h"
#include "pipe/p_screen.h"

VGErrorCode vegaGetError(void)
{
   struct vg_context *ctx = vg_current_context();
   VGErrorCode error = VG_NO_CONTEXT_ERROR;

   if (!ctx)
      return error;

   error = ctx->_error;
   ctx->_error = VG_NO_ERROR;

   return error;
}

void vegaFlush(void)
{
   struct vg_context *ctx = vg_current_context();
   struct pipe_context *pipe;

   if (!ctx)
      return;

   pipe = ctx->pipe;
   pipe->flush(pipe, NULL);

   vg_manager_flush_frontbuffer(ctx);
}

void vegaFinish(void)
{
   struct vg_context *ctx = vg_current_context();
   struct pipe_fence_handle *fence = NULL;
   struct pipe_context *pipe;

   if (!ctx)
      return;

   pipe = ctx->pipe;

   pipe->flush(pipe, &fence);
   if (fence) {
      pipe->screen->fence_finish(pipe->screen, fence,
                                 PIPE_TIMEOUT_INFINITE);
      pipe->screen->fence_reference(pipe->screen, &fence, NULL);
   }
}
