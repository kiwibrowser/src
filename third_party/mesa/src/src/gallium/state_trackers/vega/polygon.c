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

#include "polygon.h"

#include "matrix.h" /*for floatsEqual*/
#include "vg_context.h"
#include "vg_state.h"
#include "renderer.h"
#include "util_array.h"
#include "VG/openvg.h"

#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_state.h"
#include "util/u_inlines.h"
#include "pipe/p_screen.h"

#include "util/u_draw_quad.h"
#include "util/u_math.h"

#include <string.h>
#include <stdlib.h>

#define DEBUG_POLYGON 0

#define COMPONENTS 2

struct polygon
{
   VGfloat *data;
   VGint    size;

   VGint    num_verts;

   VGboolean dirty;
   void *user_vbuf;
   struct pipe_screen *screen;
};

static float *ptr_to_vertex(float *data, int idx)
{
   return data + (idx * COMPONENTS);
}

#if 0
static void polygon_print(struct polygon *poly)
{
   int i;
   float *vert;
   debug_printf("Polygon %p, size = %d\n", poly, poly->num_verts);
   for (i = 0; i < poly->num_verts; ++i) {
      vert = ptr_to_vertex(poly->data, i);
      debug_printf("%f, %f,  ", vert[0], vert[1]);
   }
   debug_printf("\nend\n");
}
#endif


struct polygon * polygon_create(int size)
{
   struct polygon *poly = (struct polygon*)malloc(sizeof(struct polygon));

   poly->data = malloc(sizeof(float) * COMPONENTS * size);
   poly->size = size;
   poly->num_verts = 0;
   poly->dirty = VG_TRUE;
   poly->user_vbuf = NULL;

   return poly;
}

struct polygon * polygon_create_from_data(float *data, int size)
{
   struct polygon *poly = polygon_create(size);

   memcpy(poly->data, data, sizeof(float) * COMPONENTS * size);
   poly->num_verts = size;
   poly->dirty = VG_TRUE;
   poly->user_vbuf = NULL;

   return poly;
}

void polygon_destroy(struct polygon *poly)
{
   free(poly->data);
   free(poly);
}

void polygon_resize(struct polygon *poly, int new_size)
{
   float *data = (float*)malloc(sizeof(float) * COMPONENTS * new_size);
   int size = MIN2(sizeof(float) * COMPONENTS * new_size,
                   sizeof(float) * COMPONENTS * poly->size);
   memcpy(data, poly->data, size);
   free(poly->data);
   poly->data = data;
   poly->size = new_size;
   poly->dirty = VG_TRUE;
}

int polygon_size(struct polygon *poly)
{
   return poly->size;
}

int polygon_vertex_count(struct polygon *poly)
{
   return poly->num_verts;
}

float * polygon_data(struct polygon *poly)
{
   return poly->data;
}

void polygon_vertex_append(struct polygon *p,
                           float x, float y)
{
   float *vert;
#if DEBUG_POLYGON
   debug_printf("Append vertex [%f, %f]\n", x, y);
#endif
   if (p->num_verts >= p->size) {
      polygon_resize(p, p->size * 2);
   }

   vert = ptr_to_vertex(p->data, p->num_verts);
   vert[0] = x;
   vert[1] = y;
   ++p->num_verts;
   p->dirty = VG_TRUE;
}

void polygon_set_vertex(struct polygon *p, int idx,
                        float x, float y)
{
   float *vert;
   if (idx >= p->num_verts) {
      /*fixme: error reporting*/
      abort();
      return;
   }

   vert = ptr_to_vertex(p->data, idx);
   vert[0] = x;
   vert[1] = y;
   p->dirty = VG_TRUE;
}

void polygon_vertex(struct polygon *p, int idx,
                    float *vertex)
{
   float *vert;
   if (idx >= p->num_verts) {
      /*fixme: error reporting*/
      abort();
      return;
   }

   vert = ptr_to_vertex(p->data, idx);
   vertex[0] = vert[0];
   vertex[1] = vert[1];
}

void polygon_bounding_rect(struct polygon *p,
                           float *rect)
{
   int i;
   float minx, miny, maxx, maxy;
   float *vert = ptr_to_vertex(p->data, 0);
   minx = vert[0];
   maxx = vert[0];
   miny = vert[1];
   maxy = vert[1];

   for (i = 1; i < p->num_verts; ++i) {
      vert = ptr_to_vertex(p->data, i);
      minx = MIN2(vert[0], minx);
      miny = MIN2(vert[1], miny);

      maxx = MAX2(vert[0], maxx);
      maxy = MAX2(vert[1], maxy);
   }

   rect[0] = minx;
   rect[1] = miny;
   rect[2] = maxx - minx;
   rect[3] = maxy - miny;
}

int polygon_contains_point(struct polygon *p,
                           float x, float y)
{
   return 0;
}

void polygon_append_polygon(struct polygon *dst,
                            struct polygon *src)
{
   if (dst->num_verts + src->num_verts >= dst->size) {
      polygon_resize(dst, dst->num_verts + src->num_verts * 1.5);
   }
   memcpy(ptr_to_vertex(dst->data, dst->num_verts),
          src->data, src->num_verts * COMPONENTS * sizeof(VGfloat));
   dst->num_verts += src->num_verts;
}

VGboolean polygon_is_closed(struct polygon *p)
{
   VGfloat start[2], end[2];

   polygon_vertex(p, 0, start);
   polygon_vertex(p, p->num_verts - 1, end);

   return floatsEqual(start[0], end[0]) && floatsEqual(start[1], end[1]);
}

static void polygon_prepare_buffer(struct vg_context *ctx,
                                   struct polygon *poly)
{
   struct pipe_context *pipe;

   /*polygon_print(poly);*/

   pipe = ctx->pipe;

   if (poly->user_vbuf == NULL || poly->dirty) {
      poly->screen = pipe->screen;
      poly->user_vbuf = poly->data;
      poly->dirty = VG_FALSE;
   }
}

void polygon_fill(struct polygon *poly, struct vg_context *ctx)
{
   struct pipe_vertex_element velement;
   struct pipe_vertex_buffer vbuffer;
   VGfloat bounds[4];
   VGfloat min_x, min_y, max_x, max_y;

   assert(poly);
   polygon_bounding_rect(poly, bounds);
   min_x = bounds[0];
   min_y = bounds[1];
   max_x = bounds[0] + bounds[2];
   max_y = bounds[1] + bounds[3];

#if DEBUG_POLYGON
   debug_printf("Poly bounds are [%f, %f], [%f, %f]\n",
                min_x, min_y, max_x, max_y);
#endif

   polygon_prepare_buffer(ctx, poly);

   /* tell renderer about the vertex attributes */
   memset(&velement, 0, sizeof(velement));
   velement.src_offset = 0;
   velement.instance_divisor = 0;
   velement.vertex_buffer_index = 0;
   velement.src_format = PIPE_FORMAT_R32G32_FLOAT;

   /* tell renderer about the vertex buffer */
   memset(&vbuffer, 0, sizeof(vbuffer));
   vbuffer.user_buffer = poly->user_vbuf;
   vbuffer.stride = COMPONENTS * sizeof(float);  /* vertex size */

   renderer_polygon_stencil_begin(ctx->renderer,
         &velement, ctx->state.vg.fill_rule, VG_FALSE);
   renderer_polygon_stencil(ctx->renderer, &vbuffer,
         PIPE_PRIM_TRIANGLE_FAN, 0, (VGuint) poly->num_verts);
   renderer_polygon_stencil_end(ctx->renderer);

   renderer_polygon_fill_begin(ctx->renderer, VG_FALSE);
   renderer_polygon_fill(ctx->renderer, min_x, min_y, max_x, max_y);
   renderer_polygon_fill_end(ctx->renderer);
}

void polygon_array_fill(struct polygon_array *polyarray, struct vg_context *ctx)
{
   struct array *polys = polyarray->array;
   VGfloat min_x = polyarray->min_x;
   VGfloat min_y = polyarray->min_y;
   VGfloat max_x = polyarray->max_x;
   VGfloat max_y = polyarray->max_y;
   struct pipe_vertex_element velement;
   struct pipe_vertex_buffer vbuffer;
   VGint i;


#if DEBUG_POLYGON
   debug_printf("%s: Poly bounds are [%f, %f], [%f, %f]\n",
                __FUNCTION__,
                min_x, min_y, max_x, max_y);
#endif

   /* tell renderer about the vertex attributes */
   memset(&velement, 0, sizeof(velement));
   velement.src_offset = 0;
   velement.instance_divisor = 0;
   velement.vertex_buffer_index = 0;
   velement.src_format = PIPE_FORMAT_R32G32_FLOAT;

   /* tell renderer about the vertex buffer */
   memset(&vbuffer, 0, sizeof(vbuffer));
   vbuffer.stride = COMPONENTS * sizeof(float);  /* vertex size */

   /* prepare the stencil buffer */
   renderer_polygon_stencil_begin(ctx->renderer,
         &velement, ctx->state.vg.fill_rule, VG_FALSE);
   for (i = 0; i < polys->num_elements; ++i) {
      struct polygon *poly = (((struct polygon**)polys->data)[i]);

      polygon_prepare_buffer(ctx, poly);
      vbuffer.user_buffer = poly->user_vbuf;

      renderer_polygon_stencil(ctx->renderer, &vbuffer,
            PIPE_PRIM_TRIANGLE_FAN, 0, (VGuint) poly->num_verts);
   }
   renderer_polygon_stencil_end(ctx->renderer);

   /* fill it */
   renderer_polygon_fill_begin(ctx->renderer, VG_FALSE);
   renderer_polygon_fill(ctx->renderer, min_x, min_y, max_x, max_y);
   renderer_polygon_fill_end(ctx->renderer);
}
