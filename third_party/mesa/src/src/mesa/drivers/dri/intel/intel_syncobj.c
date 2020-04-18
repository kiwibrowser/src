/*
 * Copyright © 2008 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Authors:
 *    Eric Anholt <eric@anholt.net>
 *
 */

/** @file intel_syncobj.c
 *
 * Support for ARB_sync
 *
 * ARB_sync is implemented by flushing the current batchbuffer and keeping a
 * reference on it.  We can then check for completion or wait for completion
 * using the normal buffer object mechanisms.  This does mean that if an
 * application is using many sync objects, it will emit small batchbuffers
 * which may end up being a significant overhead.  In other tests of removing
 * gratuitous batchbuffer syncs in Mesa, it hasn't appeared to be a significant
 * performance bottleneck, though.
 */

#include "main/simple_list.h"
#include "main/imports.h"

#include "intel_context.h"
#include "intel_batchbuffer.h"
#include "intel_reg.h"

static struct gl_sync_object *
intel_new_sync_object(struct gl_context *ctx, GLuint id)
{
   struct intel_sync_object *sync;

   sync = calloc(1, sizeof(struct intel_sync_object));

   return &sync->Base;
}

static void
intel_delete_sync_object(struct gl_context *ctx, struct gl_sync_object *s)
{
   struct intel_sync_object *sync = (struct intel_sync_object *)s;

   drm_intel_bo_unreference(sync->bo);
   free(sync);
}

static void
intel_fence_sync(struct gl_context *ctx, struct gl_sync_object *s,
	       GLenum condition, GLbitfield flags)
{
   struct intel_context *intel = intel_context(ctx);
   struct intel_sync_object *sync = (struct intel_sync_object *)s;

   assert(condition == GL_SYNC_GPU_COMMANDS_COMPLETE);
   intel_batchbuffer_emit_mi_flush(intel);

   sync->bo = intel->batch.bo;
   drm_intel_bo_reference(sync->bo);

   intel_flush(ctx);
}

/* We ignore the user-supplied timeout.  This is weaselly -- we're allowed to
 * round to an implementation-dependent accuracy, and right now our
 * implementation "rounds" to the wait-forever value.
 *
 * The fix would be a new kernel function to do the GTT transition with a
 * timeout.
 */
static void intel_client_wait_sync(struct gl_context *ctx, struct gl_sync_object *s,
				 GLbitfield flags, GLuint64 timeout)
{
   struct intel_sync_object *sync = (struct intel_sync_object *)s;

   if (sync->bo) {
      drm_intel_bo_wait_rendering(sync->bo);
      s->StatusFlag = 1;
      drm_intel_bo_unreference(sync->bo);
      sync->bo = NULL;
   }
}

/* We have nothing to do for WaitSync.  Our GL command stream is sequential,
 * so given that the sync object has already flushed the batchbuffer,
 * any batchbuffers coming after this waitsync will naturally not occur until
 * the previous one is done.
 */
static void intel_server_wait_sync(struct gl_context *ctx, struct gl_sync_object *s,
				 GLbitfield flags, GLuint64 timeout)
{
}

static void intel_check_sync(struct gl_context *ctx, struct gl_sync_object *s)
{
   struct intel_sync_object *sync = (struct intel_sync_object *)s;

   if (sync->bo && !drm_intel_bo_busy(sync->bo)) {
      drm_intel_bo_unreference(sync->bo);
      sync->bo = NULL;
      s->StatusFlag = 1;
   }
}

void intel_init_syncobj_functions(struct dd_function_table *functions)
{
   functions->NewSyncObject = intel_new_sync_object;
   functions->DeleteSyncObject = intel_delete_sync_object;
   functions->FenceSync = intel_fence_sync;
   functions->CheckSync = intel_check_sync;
   functions->ClientWaitSync = intel_client_wait_sync;
   functions->ServerWaitSync = intel_server_wait_sync;
}
