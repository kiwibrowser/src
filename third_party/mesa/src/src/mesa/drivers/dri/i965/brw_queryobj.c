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

/** @file brw_queryobj.c
 *
 * Support for query objects (GL_ARB_occlusion_query, GL_ARB_timer_query,
 * GL_EXT_transform_feedback, and friends).
 *
 * The hardware provides a PIPE_CONTROL command that can report the number of
 * fragments that passed the depth test, or the hardware timer.  They are
 * appropriately synced with the stage of the pipeline for our extensions'
 * needs.
 *
 * To avoid getting samples from another context's rendering in our results,
 * we capture the counts at the start and end of every batchbuffer while the
 * query is active, and sum up the differences.  (We should do so for
 * GL_TIME_ELAPSED as well, but don't).
 */
#include "main/imports.h"

#include "brw_context.h"
#include "brw_defines.h"
#include "brw_state.h"
#include "intel_batchbuffer.h"
#include "intel_reg.h"

static void
write_timestamp(struct intel_context *intel, drm_intel_bo *query_bo, int idx)
{
   if (intel->gen >= 6) {
      /* Emit workaround flushes: */
      if (intel->gen == 6) {
         /* The timestamp write below is a non-zero post-sync op, which on
          * Gen6 necessitates a CS stall.  CS stalls need stall at scoreboard
          * set.  See the comments for intel_emit_post_sync_nonzero_flush().
          */
         BEGIN_BATCH(4);
         OUT_BATCH(_3DSTATE_PIPE_CONTROL | (4 - 2));
         OUT_BATCH(PIPE_CONTROL_CS_STALL | PIPE_CONTROL_STALL_AT_SCOREBOARD);
         OUT_BATCH(0);
         OUT_BATCH(0);
         ADVANCE_BATCH();
      }

      BEGIN_BATCH(5);
      OUT_BATCH(_3DSTATE_PIPE_CONTROL | (5 - 2));
      OUT_BATCH(PIPE_CONTROL_WRITE_TIMESTAMP);
      OUT_RELOC(query_bo,
                I915_GEM_DOMAIN_INSTRUCTION, I915_GEM_DOMAIN_INSTRUCTION,
                PIPE_CONTROL_GLOBAL_GTT_WRITE |
                idx * sizeof(uint64_t));
      OUT_BATCH(0);
      OUT_BATCH(0);
      ADVANCE_BATCH();
   } else {
      BEGIN_BATCH(4);
      OUT_BATCH(_3DSTATE_PIPE_CONTROL | (4 - 2) |
                PIPE_CONTROL_WRITE_TIMESTAMP);
      OUT_RELOC(query_bo,
                I915_GEM_DOMAIN_INSTRUCTION, I915_GEM_DOMAIN_INSTRUCTION,
                PIPE_CONTROL_GLOBAL_GTT_WRITE |
                idx * sizeof(uint64_t));
      OUT_BATCH(0);
      OUT_BATCH(0);
      ADVANCE_BATCH();
   }
}

static void
write_depth_count(struct intel_context *intel, drm_intel_bo *query_bo, int idx)
{
   if (intel->gen >= 6) {
      /* Emit Sandybridge workaround flush: */
      if (intel->gen == 6)
         intel_emit_post_sync_nonzero_flush(intel);

      BEGIN_BATCH(5);
      OUT_BATCH(_3DSTATE_PIPE_CONTROL | (5 - 2));
      OUT_BATCH(PIPE_CONTROL_DEPTH_STALL |
                PIPE_CONTROL_WRITE_DEPTH_COUNT);
      OUT_RELOC(query_bo,
                I915_GEM_DOMAIN_INSTRUCTION, I915_GEM_DOMAIN_INSTRUCTION,
                PIPE_CONTROL_GLOBAL_GTT_WRITE |
                (idx * sizeof(uint64_t)));
      OUT_BATCH(0);
      OUT_BATCH(0);
      ADVANCE_BATCH();
   } else {
      BEGIN_BATCH(4);
      OUT_BATCH(_3DSTATE_PIPE_CONTROL | (4 - 2) |
                PIPE_CONTROL_DEPTH_STALL |
                PIPE_CONTROL_WRITE_DEPTH_COUNT);
      /* This object could be mapped cacheable, but we don't have an exposed
       * mechanism to support that.  Since it's going uncached, tell GEM that
       * we're writing to it.  The usual clflush should be all that's required
       * to pick up the results.
       */
      OUT_RELOC(query_bo,
                I915_GEM_DOMAIN_INSTRUCTION, I915_GEM_DOMAIN_INSTRUCTION,
                PIPE_CONTROL_GLOBAL_GTT_WRITE |
                (idx * sizeof(uint64_t)));
      OUT_BATCH(0);
      OUT_BATCH(0);
      ADVANCE_BATCH();
   }
}

/** Waits on the query object's BO and totals the results for this query */
static void
brw_queryobj_get_results(struct gl_context *ctx,
			 struct brw_query_object *query)
{
   struct intel_context *intel = intel_context(ctx);

   int i;
   uint64_t *results;

   if (query->bo == NULL)
      return;

   if (unlikely(INTEL_DEBUG & DEBUG_PERF)) {
      if (drm_intel_bo_busy(query->bo)) {
         perf_debug("Stalling on the GPU waiting for a query object.\n");
      }
   }

   drm_intel_bo_map(query->bo, false);
   results = query->bo->virtual;
   switch (query->Base.Target) {
   case GL_TIME_ELAPSED_EXT:
      if (intel->gen >= 6)
	 query->Base.Result += 80 * (results[1] - results[0]);
      else
	 query->Base.Result += 1000 * ((results[1] >> 32) - (results[0] >> 32));
      break;

   case GL_TIMESTAMP:
      if (intel->gen >= 6) {
         /* Our timer is a clock that increments every 80ns (regardless of
          * other clock scaling in the system).  The timestamp register we can
          * read for glGetTimestamp() masks out the top 32 bits, so we do that
          * here too to let the two counters be compared against each other.
          *
          * If we just multiplied that 32 bits of data by 80, it would roll
          * over at a non-power-of-two, so an application couldn't use
          * GL_QUERY_COUNTER_BITS to handle rollover correctly.  Instead, we
          * report 36 bits and truncate at that (rolling over 5 times as often
          * as the HW counter), and when the 32-bit counter rolls over, it
          * happens to also be at a rollover in the reported value from near
          * (1<<36) to 0.
          *
          * The low 32 bits rolls over in ~343 seconds.  Our 36-bit result
          * rolls over every ~69 seconds.
          */
	 query->Base.Result = 80 * (results[1] & 0xffffffff);
         query->Base.Result &= (1ull << 36) - 1;
      } else {
	 query->Base.Result = 1000 * (results[1] >> 32);
      }

      break;

   case GL_SAMPLES_PASSED_ARB:
      /* Map and count the pixels from the current query BO */
      for (i = query->first_index; i <= query->last_index; i++) {
	 query->Base.Result += results[i * 2 + 1] - results[i * 2];
      }
      break;

   case GL_ANY_SAMPLES_PASSED:
      /* Set true if any of the sub-queries passed. */
      for (i = query->first_index; i <= query->last_index; i++) {
	 if (results[i * 2 + 1] != results[i * 2]) {
            query->Base.Result = GL_TRUE;
            break;
         }
      }
      break;

   case GL_PRIMITIVES_GENERATED:
   case GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN:
      /* We don't actually query the hardware for this value, so query->bo
       * should always be NULL and execution should never reach here.
       */
      assert(!"Unreachable");
      break;

   default:
      assert(!"Unrecognized query target in brw_queryobj_get_results()");
      break;
   }
   drm_intel_bo_unmap(query->bo);

   drm_intel_bo_unreference(query->bo);
   query->bo = NULL;
}

static struct gl_query_object *
brw_new_query_object(struct gl_context *ctx, GLuint id)
{
   struct brw_query_object *query;

   query = calloc(1, sizeof(struct brw_query_object));

   query->Base.Id = id;
   query->Base.Result = 0;
   query->Base.Active = false;
   query->Base.Ready = true;

   return &query->Base;
}

static void
brw_delete_query(struct gl_context *ctx, struct gl_query_object *q)
{
   struct brw_query_object *query = (struct brw_query_object *)q;

   drm_intel_bo_unreference(query->bo);
   free(query);
}

static void
brw_begin_query(struct gl_context *ctx, struct gl_query_object *q)
{
   struct brw_context *brw = brw_context(ctx);
   struct intel_context *intel = intel_context(ctx);
   struct brw_query_object *query = (struct brw_query_object *)q;

   switch (query->Base.Target) {
   case GL_TIME_ELAPSED_EXT:
      drm_intel_bo_unreference(query->bo);
      query->bo = drm_intel_bo_alloc(intel->bufmgr, "timer query", 4096, 4096);
      write_timestamp(intel, query->bo, 0);
      break;

   case GL_ANY_SAMPLES_PASSED:
   case GL_SAMPLES_PASSED_ARB:
      /* Reset our driver's tracking of query state. */
      drm_intel_bo_unreference(query->bo);
      query->bo = NULL;
      query->first_index = -1;
      query->last_index = -1;

      brw->query.obj = query;
      intel->stats_wm++;
      break;

   case GL_PRIMITIVES_GENERATED:
      /* We don't actually query the hardware for this value; we keep track of
       * it a software counter.  So just reset the counter.
       */
      brw->sol.primitives_generated = 0;
      brw->sol.counting_primitives_generated = true;
      break;

   case GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN:
      /* We don't actually query the hardware for this value; we keep track of
       * it a software counter.  So just reset the counter.
       */
      brw->sol.primitives_written = 0;
      brw->sol.counting_primitives_written = true;
      break;

   default:
      assert(!"Unrecognized query target in brw_begin_query()");
      break;
   }
}

/**
 * Begin the ARB_occlusion_query query on a query object.
 */
static void
brw_end_query(struct gl_context *ctx, struct gl_query_object *q)
{
   struct brw_context *brw = brw_context(ctx);
   struct intel_context *intel = intel_context(ctx);
   struct brw_query_object *query = (struct brw_query_object *)q;

   switch (query->Base.Target) {
   case GL_TIMESTAMP:
      drm_intel_bo_unreference(query->bo);
      query->bo = drm_intel_bo_alloc(intel->bufmgr, "timer query",
				     4096, 4096);
      /* FALLTHROUGH */

   case GL_TIME_ELAPSED_EXT:
      write_timestamp(intel, query->bo, 1);
      intel_batchbuffer_flush(intel);
      break;

   case GL_ANY_SAMPLES_PASSED:
   case GL_SAMPLES_PASSED_ARB:
      /* Flush the batchbuffer in case it has writes to our query BO.
       * Have later queries write to a new query BO so that further rendering
       * doesn't delay the collection of our results.
       */
      if (query->bo) {
	 brw_emit_query_end(brw);
	 intel_batchbuffer_flush(intel);

	 drm_intel_bo_unreference(brw->query.bo);
	 brw->query.bo = NULL;
      }

      brw->query.obj = NULL;

      intel->stats_wm--;
      break;

   case GL_PRIMITIVES_GENERATED:
      /* We don't actually query the hardware for this value; we keep track of
       * it in a software counter.  So just read the counter and store it in
       * the query object.
       */
      query->Base.Result = brw->sol.primitives_generated;
      brw->sol.counting_primitives_generated = false;

      /* And set brw->query.obj to NULL so that this query won't try to wait
       * for any rendering to complete.
       */
      query->bo = NULL;
      break;

   case GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN:
      /* We don't actually query the hardware for this value; we keep track of
       * it in a software counter.  So just read the counter and store it in
       * the query object.
       */
      query->Base.Result = brw->sol.primitives_written;
      brw->sol.counting_primitives_written = false;

      /* And set brw->query.obj to NULL so that this query won't try to wait
       * for any rendering to complete.
       */
      query->bo = NULL;
      break;

   default:
      assert(!"Unrecognized query target in brw_end_query()");
      break;
   }
}

static void brw_wait_query(struct gl_context *ctx, struct gl_query_object *q)
{
   struct brw_query_object *query = (struct brw_query_object *)q;

   brw_queryobj_get_results(ctx, query);
   query->Base.Ready = true;
}

static void brw_check_query(struct gl_context *ctx, struct gl_query_object *q)
{
   struct brw_query_object *query = (struct brw_query_object *)q;

   if (query->bo == NULL || !drm_intel_bo_busy(query->bo)) {
      brw_queryobj_get_results(ctx, query);
      query->Base.Ready = true;
   }
}

/** Called to set up the query BO and account for its aperture space */
void
brw_prepare_query_begin(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;

   /* Skip if we're not doing any queries. */
   if (!brw->query.obj)
      return;

   /* Get a new query BO if we're going to need it. */
   if (brw->query.bo == NULL ||
       brw->query.index * 2 + 1 >= 4096 / sizeof(uint64_t)) {
      drm_intel_bo_unreference(brw->query.bo);
      brw->query.bo = NULL;

      brw->query.bo = drm_intel_bo_alloc(intel->bufmgr, "query", 4096, 1);

      /* clear target buffer */
      drm_intel_bo_map(brw->query.bo, true);
      memset((char *)brw->query.bo->virtual, 0, 4096);
      drm_intel_bo_unmap(brw->query.bo);

      brw->query.index = 0;
   }
}

/** Called just before primitive drawing to get a beginning PS_DEPTH_COUNT. */
void
brw_emit_query_begin(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;
   struct gl_context *ctx = &intel->ctx;
   struct brw_query_object *query = brw->query.obj;

   /* Skip if we're not doing any queries, or we've emitted the start. */
   if (!query || brw->query.active)
      return;

   write_depth_count(intel, brw->query.bo, brw->query.index * 2);

   if (query->bo != brw->query.bo) {
      if (query->bo != NULL)
	 brw_queryobj_get_results(ctx, query);
      drm_intel_bo_reference(brw->query.bo);
      query->bo = brw->query.bo;
      query->first_index = brw->query.index;
   }
   query->last_index = brw->query.index;
   brw->query.active = true;
}

/** Called at batchbuffer flush to get an ending PS_DEPTH_COUNT */
void
brw_emit_query_end(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;

   if (!brw->query.active)
      return;

   write_depth_count(intel, brw->query.bo, brw->query.index * 2 + 1);

   brw->query.active = false;
   brw->query.index++;
}

static uint64_t
brw_get_timestamp(struct gl_context *ctx)
{
   struct intel_context *intel = intel_context(ctx);
   uint64_t result = 0;

   drm_intel_reg_read(intel->bufmgr, TIMESTAMP, &result);

   /* See logic in brw_queryobj_get_results() */
   result = result >> 32;
   result *= 80;
   result &= (1ull << 36) - 1;

   return result;
}

void brw_init_queryobj_functions(struct dd_function_table *functions)
{
   functions->NewQueryObject = brw_new_query_object;
   functions->DeleteQuery = brw_delete_query;
   functions->BeginQuery = brw_begin_query;
   functions->EndQuery = brw_end_query;
   functions->CheckQuery = brw_check_query;
   functions->WaitQuery = brw_wait_query;
   functions->GetTimestamp = brw_get_timestamp;
}
