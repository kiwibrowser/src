/*
 * Copyright 2012 Red Hat Inc.
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
 *
 * Authors: Ben Skeggs
 *
 */

#include "draw/draw_context.h"

#include "nouveau/nv_object.xml.h"
#include "nv30-40_3d.xml.h"

#include "nouveau/nouveau_fence.h"
#include "nv30_context.h"
#include "nv30_transfer.h"
#include "nv30_state.h"

static void
nv30_context_kick_notify(struct nouveau_pushbuf *push)
{
   struct nouveau_screen *screen;
   struct nv30_context *nv30;

   if (!push->user_priv)
      return;
   nv30 = container_of(push->user_priv, nv30, bufctx);
   screen = &nv30->screen->base;

   nouveau_fence_next(screen);
   nouveau_fence_update(screen, TRUE);

   if (push->bufctx) {
      struct nouveau_bufref *bref;
      LIST_FOR_EACH_ENTRY(bref, &push->bufctx->current, thead) {
         struct nv04_resource *res = bref->priv;
         if (res && res->mm) {
            nouveau_fence_ref(screen->fence.current, &res->fence);

            if (bref->flags & NOUVEAU_BO_RD)
               res->status |= NOUVEAU_BUFFER_STATUS_GPU_READING;

            if (bref->flags & NOUVEAU_BO_WR) {
               nouveau_fence_ref(screen->fence.current, &res->fence_wr);
               res->status |= NOUVEAU_BUFFER_STATUS_GPU_WRITING;
            }
         }
      }
   }
}

static void
nv30_context_flush(struct pipe_context *pipe, struct pipe_fence_handle **fence)
{
   struct nv30_context *nv30 = nv30_context(pipe);
   struct nouveau_pushbuf *push = nv30->base.pushbuf;

   if (fence)
      nouveau_fence_ref(nv30->screen->base.fence.current,
                        (struct nouveau_fence **)fence);

   PUSH_KICK(push);
}

static void
nv30_context_destroy(struct pipe_context *pipe)
{
   struct nv30_context *nv30 = nv30_context(pipe);

   if (nv30->draw)
      draw_destroy(nv30->draw);

   nouveau_bufctx_del(&nv30->bufctx);

   if (nv30->screen->cur_ctx == nv30)
      nv30->screen->cur_ctx = NULL;

   nouveau_context_destroy(&nv30->base);
}

#define FAIL_CONTEXT_INIT(str, err)                   \
   do {                                               \
      NOUVEAU_ERR(str, err);                          \
      nv30_context_destroy(pipe);                     \
      return NULL;                                    \
   } while(0)

struct pipe_context *
nv30_context_create(struct pipe_screen *pscreen, void *priv)
{
   struct nv30_screen *screen = nv30_screen(pscreen);
   struct nv30_context *nv30 = CALLOC_STRUCT(nv30_context);
   struct nouveau_pushbuf *push;
   struct pipe_context *pipe;
   int ret;

   if (!nv30)
      return NULL;

   nv30->screen = screen;
   nv30->base.screen = &screen->base;
   nv30->base.copy_data = nv30_transfer_copy_data;

   pipe = &nv30->base.pipe;
   pipe->screen = pscreen;
   pipe->priv = priv;
   pipe->destroy = nv30_context_destroy;
   pipe->flush = nv30_context_flush;

   /*XXX: *cough* per-context client */
   nv30->base.client = screen->base.client;

   /*XXX: *cough* per-context pushbufs */
   push = screen->base.pushbuf;
   nv30->base.pushbuf = push;
   nv30->base.pushbuf->user_priv = push->user_priv; /* hack at validate time */
   nv30->base.pushbuf->rsvd_kick = 16; /* hack in screen before first space */
   nv30->base.pushbuf->kick_notify = nv30_context_kick_notify;

   ret = nouveau_bufctx_new(nv30->base.client, 64, &nv30->bufctx);
   if (ret) {
      nv30_context_destroy(pipe);
      return NULL;
   }

   /*XXX: make configurable with performance vs quality, these defaults
    *     match the binary driver's defaults
    */
   if (screen->eng3d->oclass < NV40_3D_CLASS)
      nv30->config.filter = 0x00000004;
   else
      nv30->config.filter = 0x00002dc4;

   nv30->config.aniso = NV40_3D_TEX_WRAP_ANISO_MIP_FILTER_OPTIMIZATION_OFF;

   if (debug_get_bool_option("NV30_SWTNL", FALSE))
      nv30->draw_flags |= NV30_NEW_SWTNL;

   /*XXX: nvfx... */
   nv30->is_nv4x = (screen->eng3d->oclass >= NV40_3D_CLASS) ? ~0 : 0;
   nv30->use_nv4x = (screen->eng3d->oclass >= NV40_3D_CLASS) ? ~0 : 0;
   nv30->render_mode = HW;

   nv30->sample_mask = 0xffff;
   nv30_vbo_init(pipe);
   nv30_query_init(pipe);
   nv30_state_init(pipe);
   nv30_resource_init(pipe);
   nv30_clear_init(pipe);
   nv30_fragprog_init(pipe);
   nv30_vertprog_init(pipe);
   nv30_texture_init(pipe);
   nv30_fragtex_init(pipe);
   nv40_verttex_init(pipe);
   nv30_draw_init(pipe);

   return pipe;
}
