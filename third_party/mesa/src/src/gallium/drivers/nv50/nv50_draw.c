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

#include "nv50_context.h"

struct nv50_render_stage {
   struct draw_stage stage;
   struct nv50_context *nv50;
};

static INLINE struct nv50_render_stage *
nv50_render_stage(struct draw_stage *stage)
{
   return (struct nv50_render_stage *)stage;
}

static void
nv50_render_point(struct draw_stage *stage, struct prim_header *prim)
{
   NOUVEAU_ERR("\n");
}

static void
nv50_render_line(struct draw_stage *stage, struct prim_header *prim)
{
   NOUVEAU_ERR("\n");
}

static void
nv50_render_tri(struct draw_stage *stage, struct prim_header *prim)
{
   NOUVEAU_ERR("\n");
}

static void
nv50_render_flush(struct draw_stage *stage, unsigned flags)
{
}

static void
nv50_render_reset_stipple_counter(struct draw_stage *stage)
{
   NOUVEAU_ERR("\n");
}

static void
nv50_render_destroy(struct draw_stage *stage)
{
   FREE(stage);
}

struct draw_stage *
nv50_draw_render_stage(struct nv50_context *nv50)
{
   struct nv50_render_stage *rs = CALLOC_STRUCT(nv50_render_stage);

   rs->nv50 = nv50;
   rs->stage.draw = nv50->draw;
   rs->stage.destroy = nv50_render_destroy;
   rs->stage.point = nv50_render_point;
   rs->stage.line = nv50_render_line;
   rs->stage.tri = nv50_render_tri;
   rs->stage.flush = nv50_render_flush;
   rs->stage.reset_stipple_counter = nv50_render_reset_stipple_counter;

   return &rs->stage;
}
