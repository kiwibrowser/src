/*
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 *
 * Copyright © 2012 Intel Corporation
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
 *    Jordan Justen <jordan.l.justen@intel.com>
 *
 */

#include "main/imports.h"
#include "main/bufferobj.h"
#include "main/macros.h"

#include "vbo.h"
#include "vbo_context.h"

#define UPDATE_MIN2(a, b) (a) = MIN2((a), (b))
#define UPDATE_MAX2(a, b) (a) = MAX2((a), (b))

/*
 * Notes on primitive restart:
 * The code below is used when the driver does not support primitive
 * restart itself. (ctx->Const.PrimitiveRestartInSoftware == GL_TRUE)
 *
 * We map the index buffer, find the restart indexes, unmap
 * the index buffer then draw the sub-primitives delineated by the restarts.
 *
 * A couple possible optimizations:
 * 1. Save the list of sub-primitive (start, count) values in a list attached
 *    to the index buffer for re-use in subsequent draws.  The list would be
 *    invalidated when the contents of the buffer changed.
 * 2. If drawing triangle strips or quad strips, create a new index buffer
 *    that uses duplicated vertices to render the disjoint strips as one
 *    long strip.  We'd have to be careful to avoid using too much memory
 *    for this.
 *
 * Finally, some apps might perform better if they don't use primitive restart
 * at all rather than this fallback path.  Set MESA_EXTENSION_OVERRIDE to
 * "-GL_NV_primitive_restart" to test that.
 */


struct sub_primitive
{
   GLuint start;
   GLuint count;
   GLuint min_index;
   GLuint max_index;
};


/**
 * Scan the elements array to find restart indexes.  Return an array
 * of struct sub_primitive to indicate how to draw the sub-primitives
 * are delineated by the restart index.
 */
static struct sub_primitive *
find_sub_primitives(const void *elements, unsigned element_size,
                    unsigned start, unsigned end, unsigned restart_index,
                    unsigned *num_sub_prims)
{
   const unsigned max_prims = end - start;
   struct sub_primitive *sub_prims;
   unsigned i, cur_start, cur_count;
   GLuint scan_index;
   unsigned scan_num;

   sub_prims = (struct sub_primitive *)
      malloc(max_prims * sizeof(struct sub_primitive));

   if (!sub_prims) {
      *num_sub_prims = 0;
      return NULL;
   }

   cur_start = start;
   cur_count = 0;
   scan_num = 0;

#define IB_INDEX_READ(TYPE, INDEX) (((const GL##TYPE *) elements)[INDEX])

#define SCAN_ELEMENTS(TYPE) \
   sub_prims[scan_num].min_index = (GL##TYPE) 0xffffffff; \
   sub_prims[scan_num].max_index = 0; \
   for (i = start; i < end; i++) { \
      scan_index = IB_INDEX_READ(TYPE, i); \
      if (scan_index == restart_index) { \
         if (cur_count > 0) { \
            assert(scan_num < max_prims); \
            sub_prims[scan_num].start = cur_start; \
            sub_prims[scan_num].count = cur_count; \
            scan_num++; \
            sub_prims[scan_num].min_index = (GL##TYPE) 0xffffffff; \
            sub_prims[scan_num].max_index = 0; \
         } \
         cur_start = i + 1; \
         cur_count = 0; \
      } \
      else { \
         UPDATE_MIN2(sub_prims[scan_num].min_index, scan_index); \
         UPDATE_MAX2(sub_prims[scan_num].max_index, scan_index); \
         cur_count++; \
      } \
   } \
   if (cur_count > 0) { \
      assert(scan_num < max_prims); \
      sub_prims[scan_num].start = cur_start; \
      sub_prims[scan_num].count = cur_count; \
      scan_num++; \
   }

   switch (element_size) {
   case 1:
      SCAN_ELEMENTS(ubyte);
      break;
   case 2:
      SCAN_ELEMENTS(ushort);
      break;
   case 4:
      SCAN_ELEMENTS(uint);
      break;
   default:
      assert(0 && "bad index_size in find_sub_primitives()");
   }

#undef SCAN_ELEMENTS

   *num_sub_prims = scan_num;

   return sub_prims;
}


/**
 * Handle primitive restart in software.
 *
 * This function breaks up calls into the driver so primitive restart
 * support is not required in the driver.
 */
void
vbo_sw_primitive_restart(struct gl_context *ctx,
                         const struct _mesa_prim *prims,
                         GLuint nr_prims,
                         const struct _mesa_index_buffer *ib)
{
   GLuint prim_num;
   struct sub_primitive *sub_prims;
   struct sub_primitive *sub_prim;
   GLuint num_sub_prims;
   GLuint sub_prim_num;
   GLuint end_index;
   GLuint sub_end_index;
   GLuint restart_index = ctx->Array.RestartIndex;
   struct _mesa_prim temp_prim;
   struct vbo_context *vbo = vbo_context(ctx);
   vbo_draw_func draw_prims_func = vbo->draw_prims;
   GLboolean map_ib = ib->obj->Name && !ib->obj->Pointer;
   void *ptr;

   /* Find the sub-primitives. These are regions in the index buffer which
    * are split based on the primitive restart index value.
    */
   if (map_ib) {
      ctx->Driver.MapBufferRange(ctx, 0, ib->obj->Size, GL_MAP_READ_BIT,
                                 ib->obj);
   }

   ptr = ADD_POINTERS(ib->obj->Pointer, ib->ptr);

   sub_prims = find_sub_primitives(ptr, vbo_sizeof_ib_type(ib->type),
                                   0, ib->count, restart_index,
                                   &num_sub_prims);

   if (map_ib) {
      ctx->Driver.UnmapBuffer(ctx, ib->obj);
   }

   /* Loop over the primitives, and use the located sub-primitives to draw
    * each primitive with a break to implement each primitive restart.
    */
   for (prim_num = 0; prim_num < nr_prims; prim_num++) {
      end_index = prims[prim_num].start + prims[prim_num].count;
      memcpy(&temp_prim, &prims[prim_num], sizeof (temp_prim));
      /* Loop over the sub-primitives drawing sub-ranges of the primitive. */
      for (sub_prim_num = 0; sub_prim_num < num_sub_prims; sub_prim_num++) {
         sub_prim = &sub_prims[sub_prim_num];
         sub_end_index = sub_prim->start + sub_prim->count;
         if (prims[prim_num].start <= sub_prim->start) {
            temp_prim.start = MAX2(prims[prim_num].start, sub_prim->start);
            temp_prim.count = MIN2(sub_end_index, end_index) - temp_prim.start;
            if ((temp_prim.start == sub_prim->start) &&
                (temp_prim.count == sub_prim->count)) {
               draw_prims_func(ctx, &temp_prim, 1, ib,
                               GL_TRUE, sub_prim->min_index, sub_prim->max_index,
                               NULL);
            } else {
               draw_prims_func(ctx, &temp_prim, 1, ib,
                               GL_FALSE, -1, -1,
                               NULL);
            }
         }
         if (sub_end_index >= end_index) {
            break;
         }
      }
   }

   if (sub_prims) {
      free(sub_prims);
   }
}

