/*
 * Copyright 2008 Ben Skeggs
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "draw/draw_pipe.h"

#include "nvc0_context.h"

struct nvc0_render_stage {
   struct draw_stage stage;
   struct nvc0_context *nvc0;
};

static INLINE struct nvc0_render_stage *
nvc0_render_stage(struct draw_stage *stage)
{
   return (struct nvc0_render_stage *)stage;
}

static void
nvc0_render_point(struct draw_stage *stage, struct prim_header *prim)
{
   NOUVEAU_ERR("\n");
}

static void
nvc0_render_line(struct draw_stage *stage, struct prim_header *prim)
{
   NOUVEAU_ERR("\n");
}

static void
nvc0_render_tri(struct draw_stage *stage, struct prim_header *prim)
{
   NOUVEAU_ERR("\n");
}

static void
nvc0_render_flush(struct draw_stage *stage, unsigned flags)
{
}

static void
nvc0_render_reset_stipple_counter(struct draw_stage *stage)
{
   NOUVEAU_ERR("\n");
}

static void
nvc0_render_destroy(struct draw_stage *stage)
{
   FREE(stage);
}

struct draw_stage *
nvc0_draw_render_stage(struct nvc0_context *nvc0)
{
   struct nvc0_render_stage *rs = CALLOC_STRUCT(nvc0_render_stage);

   rs->nvc0 = nvc0;
   rs->stage.draw = nvc0->draw;
   rs->stage.destroy = nvc0_render_destroy;
   rs->stage.point = nvc0_render_point;
   rs->stage.line = nvc0_render_line;
   rs->stage.tri = nvc0_render_tri;
   rs->stage.flush = nvc0_render_flush;
   rs->stage.reset_stipple_counter = nvc0_render_reset_stipple_counter;

   return &rs->stage;
}
