/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
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

/*
 * This file implements the st_draw_vbo() function which is called from
 * Mesa's VBO module.  All point/line/triangle rendering is done through
 * this function whether the user called glBegin/End, glDrawArrays,
 * glDrawElements, glEvalMesh, or glCalList, etc.
 *
 * Authors:
 *   Keith Whitwell <keith@tungstengraphics.com>
 */


#include "main/imports.h"
#include "main/image.h"
#include "main/bufferobj.h"
#include "main/macros.h"
#include "main/mfeatures.h"

#include "vbo/vbo.h"

#include "st_context.h"
#include "st_atom.h"
#include "st_cb_bufferobjects.h"
#include "st_cb_xformfb.h"
#include "st_draw.h"
#include "st_program.h"

#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "util/u_inlines.h"
#include "util/u_format.h"
#include "util/u_prim.h"
#include "util/u_draw_quad.h"
#include "util/u_upload_mgr.h"
#include "draw/draw_context.h"
#include "cso_cache/cso_context.h"

#include "../glsl/ir_uniform.h"


/**
 * This is very similar to vbo_all_varyings_in_vbos() but we are
 * only interested in per-vertex data.  See bug 38626.
 */
static GLboolean
all_varyings_in_vbos(const struct gl_client_array *arrays[])
{
   GLuint i;
   
   for (i = 0; i < VERT_ATTRIB_MAX; i++)
      if (arrays[i]->StrideB &&
          !arrays[i]->InstanceDivisor &&
          !_mesa_is_bufferobj(arrays[i]->BufferObj))
	 return GL_FALSE;

   return GL_TRUE;
}


/**
 * Basically, translate Mesa's index buffer information into
 * a pipe_index_buffer object.
 * \return TRUE or FALSE for success/failure
 */
static boolean
setup_index_buffer(struct st_context *st,
                   const struct _mesa_index_buffer *ib,
                   struct pipe_index_buffer *ibuffer)
{
   struct gl_buffer_object *bufobj = ib->obj;

   ibuffer->index_size = vbo_sizeof_ib_type(ib->type);

   /* get/create the index buffer object */
   if (_mesa_is_bufferobj(bufobj)) {
      /* indices are in a real VBO */
      ibuffer->buffer = st_buffer_object(bufobj)->buffer;
      ibuffer->offset = pointer_to_offset(ib->ptr);
   }
   else if (st->indexbuf_uploader) {
      if (u_upload_data(st->indexbuf_uploader, 0,
                        ib->count * ibuffer->index_size, ib->ptr,
                        &ibuffer->offset, &ibuffer->buffer) != PIPE_OK) {
         /* out of memory */
         return FALSE;
      }
      u_upload_unmap(st->indexbuf_uploader);
   }
   else {
      /* indices are in user space memory */
      ibuffer->user_buffer = ib->ptr;
   }

   cso_set_index_buffer(st->cso_context, ibuffer);
   return TRUE;
}


/**
 * Prior to drawing, check that any uniforms referenced by the
 * current shader have been set.  If a uniform has not been set,
 * issue a warning.
 */
static void
check_uniforms(struct gl_context *ctx)
{
   struct gl_shader_program *shProg[3] = {
      ctx->Shader.CurrentVertexProgram,
      ctx->Shader.CurrentGeometryProgram,
      ctx->Shader.CurrentFragmentProgram,
   };
   unsigned j;

   for (j = 0; j < 3; j++) {
      unsigned i;

      if (shProg[j] == NULL || !shProg[j]->LinkStatus)
	 continue;

      for (i = 0; i < shProg[j]->NumUserUniformStorage; i++) {
         const struct gl_uniform_storage *u = &shProg[j]->UniformStorage[i];
         if (!u->initialized) {
            _mesa_warning(ctx,
                          "Using shader with uninitialized uniform: %s",
                          u->name);
         }
      }
   }
}


/**
 * Translate OpenGL primtive type (GL_POINTS, GL_TRIANGLE_STRIP, etc) to
 * the corresponding Gallium type.
 */
static unsigned
translate_prim(const struct gl_context *ctx, unsigned prim)
{
   /* GL prims should match Gallium prims, spot-check a few */
   STATIC_ASSERT(GL_POINTS == PIPE_PRIM_POINTS);
   STATIC_ASSERT(GL_QUADS == PIPE_PRIM_QUADS);
   STATIC_ASSERT(GL_TRIANGLE_STRIP_ADJACENCY == PIPE_PRIM_TRIANGLE_STRIP_ADJACENCY);

   /* Avoid quadstrips if it's easy to do so:
    * Note: it's important to do the correct trimming if we change the
    * prim type!  We do that wherever this function is called.
    */
   if (prim == GL_QUAD_STRIP &&
       ctx->Light.ShadeModel != GL_FLAT &&
       ctx->Polygon.FrontMode == GL_FILL &&
       ctx->Polygon.BackMode == GL_FILL)
      prim = GL_TRIANGLE_STRIP;

   return prim;
}


/**
 * This function gets plugged into the VBO module and is called when
 * we have something to render.
 * Basically, translate the information into the format expected by gallium.
 */
void
st_draw_vbo(struct gl_context *ctx,
            const struct _mesa_prim *prims,
            GLuint nr_prims,
            const struct _mesa_index_buffer *ib,
	    GLboolean index_bounds_valid,
            GLuint min_index,
            GLuint max_index,
            struct gl_transform_feedback_object *tfb_vertcount)
{
   struct st_context *st = st_context(ctx);
   struct pipe_index_buffer ibuffer = {0};
   struct pipe_draw_info info;
   const struct gl_client_array **arrays = ctx->Array._DrawArrays;
   unsigned i;

   /* Mesa core state should have been validated already */
   assert(ctx->NewState == 0x0);

   /* Validate state. */
   if (st->dirty.st || ctx->NewDriverState) {
      st_validate_state(st);

      if (st->vertex_array_out_of_memory)
         return;

#if 0
      if (MESA_VERBOSE & VERBOSE_GLSL) {
         check_uniforms(ctx);
      }
#else
      (void) check_uniforms;
#endif
   }

   util_draw_init_info(&info);
   if (ib) {
      /* Get index bounds for user buffers. */
      if (!index_bounds_valid)
         if (!all_varyings_in_vbos(arrays))
            vbo_get_minmax_indices(ctx, prims, ib, &min_index, &max_index,
                                   nr_prims);

      if (!setup_index_buffer(st, ib, &ibuffer)) {
         /* out of memory */
         return;
      }

      info.indexed = TRUE;
      if (min_index != ~0 && max_index != ~0) {
         info.min_index = min_index;
         info.max_index = max_index;
      }

      /* The VBO module handles restart for the non-indexed GLDrawArrays
       * so we only set these fields for indexed drawing:
       */
      info.primitive_restart = ctx->Array.PrimitiveRestart;
      info.restart_index = ctx->Array.RestartIndex;
   }
   else {
      /* Transform feedback drawing is always non-indexed. */
      /* Set info.count_from_stream_output. */
      if (tfb_vertcount) {
         st_transform_feedback_draw_init(tfb_vertcount, &info);
      }
   }

   /* do actual drawing */
   for (i = 0; i < nr_prims; i++) {
      info.mode = translate_prim( ctx, prims[i].mode );
      info.start = prims[i].start;
      info.count = prims[i].count;
      info.start_instance = prims[i].base_instance;
      info.instance_count = prims[i].num_instances;
      info.index_bias = prims[i].basevertex;
      if (!ib) {
         info.min_index = info.start;
         info.max_index = info.start + info.count - 1;
      }

      if (info.count_from_stream_output) {
         cso_draw_vbo(st->cso_context, &info);
      }
      else if (info.primitive_restart) {
         /* don't trim, restarts might be inside index list */
         cso_draw_vbo(st->cso_context, &info);
      }
      else if (u_trim_pipe_prim(info.mode, &info.count))
         cso_draw_vbo(st->cso_context, &info);
   }

   if (ib && st->indexbuf_uploader && !_mesa_is_bufferobj(ib->obj)) {
      pipe_resource_reference(&ibuffer.buffer, NULL);
   }
}


void
st_init_draw(struct st_context *st)
{
   struct gl_context *ctx = st->ctx;

   vbo_set_draw_func(ctx, st_draw_vbo);

#if FEATURE_feedback || FEATURE_rastpos
   st->draw = draw_create(st->pipe); /* for selection/feedback */

   /* Disable draw options that might convert points/lines to tris, etc.
    * as that would foul-up feedback/selection mode.
    */
   draw_wide_line_threshold(st->draw, 1000.0f);
   draw_wide_point_threshold(st->draw, 1000.0f);
   draw_enable_line_stipple(st->draw, FALSE);
   draw_enable_point_sprites(st->draw, FALSE);
#endif
}


void
st_destroy_draw(struct st_context *st)
{
#if FEATURE_feedback || FEATURE_rastpos
   draw_destroy(st->draw);
#endif
}
