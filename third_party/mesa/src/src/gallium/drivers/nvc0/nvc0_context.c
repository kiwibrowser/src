/*
 * Copyright 2010 Christoph Bumiller
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

#include "pipe/p_defines.h"
#include "util/u_framebuffer.h"

#ifdef NVC0_WITH_DRAW_MODULE
#include "draw/draw_context.h"
#endif

#include "nvc0_context.h"
#include "nvc0_screen.h"
#include "nvc0_resource.h"

static void
nvc0_flush(struct pipe_context *pipe,
           struct pipe_fence_handle **fence)
{
   struct nvc0_context *nvc0 = nvc0_context(pipe);
   struct nouveau_screen *screen = &nvc0->screen->base;

   if (fence)
      nouveau_fence_ref(screen->fence.current, (struct nouveau_fence **)fence);

   PUSH_KICK(nvc0->base.pushbuf); /* fencing handled in kick_notify */
}

static void
nvc0_texture_barrier(struct pipe_context *pipe)
{
   struct nouveau_pushbuf *push = nvc0_context(pipe)->base.pushbuf;

   IMMED_NVC0(push, NVC0_3D(SERIALIZE), 0);
   IMMED_NVC0(push, NVC0_3D(TEX_CACHE_CTL), 0);
}

static void
nvc0_context_unreference_resources(struct nvc0_context *nvc0)
{
   unsigned s, i;

   nouveau_bufctx_del(&nvc0->bufctx_3d);
   nouveau_bufctx_del(&nvc0->bufctx);

   util_unreference_framebuffer_state(&nvc0->framebuffer);

   for (i = 0; i < nvc0->num_vtxbufs; ++i)
      pipe_resource_reference(&nvc0->vtxbuf[i].buffer, NULL);

   pipe_resource_reference(&nvc0->idxbuf.buffer, NULL);

   for (s = 0; s < 5; ++s) {
      for (i = 0; i < nvc0->num_textures[s]; ++i)
         pipe_sampler_view_reference(&nvc0->textures[s][i], NULL);

      for (i = 0; i < NVC0_MAX_PIPE_CONSTBUFS; ++i)
         if (!nvc0->constbuf[s][i].user)
            pipe_resource_reference(&nvc0->constbuf[s][i].u.buf, NULL);
   }

   for (i = 0; i < nvc0->num_tfbbufs; ++i)
      pipe_so_target_reference(&nvc0->tfbbuf[i], NULL);
}

static void
nvc0_destroy(struct pipe_context *pipe)
{
   struct nvc0_context *nvc0 = nvc0_context(pipe);

   if (nvc0->screen->cur_ctx == nvc0) {
      nvc0->base.pushbuf->kick_notify = NULL;
      nvc0->screen->cur_ctx = NULL;
      nouveau_pushbuf_bufctx(nvc0->base.pushbuf, NULL);
   }
   nouveau_pushbuf_kick(nvc0->base.pushbuf, nvc0->base.pushbuf->channel);

   nvc0_context_unreference_resources(nvc0);

#ifdef NVC0_WITH_DRAW_MODULE
   draw_destroy(nvc0->draw);
#endif

   nouveau_context_destroy(&nvc0->base);
}

void
nvc0_default_kick_notify(struct nouveau_pushbuf *push)
{
   struct nvc0_screen *screen = push->user_priv;

   if (screen) {
      nouveau_fence_next(&screen->base);
      nouveau_fence_update(&screen->base, TRUE);
      if (screen->cur_ctx)
         screen->cur_ctx->state.flushed = TRUE;
   }
}

struct pipe_context *
nvc0_create(struct pipe_screen *pscreen, void *priv)
{
   struct nvc0_screen *screen = nvc0_screen(pscreen);
   struct nvc0_context *nvc0;
   struct pipe_context *pipe;
   int ret;
   uint32_t flags;

   nvc0 = CALLOC_STRUCT(nvc0_context);
   if (!nvc0)
      return NULL;
   pipe = &nvc0->base.pipe;

   nvc0->base.pushbuf = screen->base.pushbuf;

   ret = nouveau_bufctx_new(screen->base.client, NVC0_BIND_COUNT,
                            &nvc0->bufctx_3d);
   if (!ret)
      nouveau_bufctx_new(screen->base.client, 2, &nvc0->bufctx);
   if (ret)
      goto out_err;

   nvc0->screen = screen;
   nvc0->base.screen = &screen->base;

   pipe->screen = pscreen;
   pipe->priv = priv;

   pipe->destroy = nvc0_destroy;

   pipe->draw_vbo = nvc0_draw_vbo;
   pipe->clear = nvc0_clear;

   pipe->flush = nvc0_flush;
   pipe->texture_barrier = nvc0_texture_barrier;

   if (!screen->cur_ctx) {
      screen->cur_ctx = nvc0;
      nouveau_pushbuf_bufctx(screen->base.pushbuf, nvc0->bufctx);
   }
   screen->base.pushbuf->kick_notify = nvc0_default_kick_notify;

   nvc0_init_query_functions(nvc0);
   nvc0_init_surface_functions(nvc0);
   nvc0_init_state_functions(nvc0);
   nvc0_init_transfer_functions(nvc0);
   nvc0_init_resource_functions(pipe);

#ifdef NVC0_WITH_DRAW_MODULE
   /* no software fallbacks implemented */
   nvc0->draw = draw_create(pipe);
   assert(nvc0->draw);
   draw_set_rasterize_stage(nvc0->draw, nvc0_draw_render_stage(nvc0));
#endif

   nouveau_context_init_vdec(&nvc0->base);

   /* shader builtin library is per-screen, but we need a context for m2mf */
   nvc0_program_library_upload(nvc0);

   /* add permanently resident buffers to bufctxts */

   flags = NOUVEAU_BO_VRAM | NOUVEAU_BO_RD;

   BCTX_REFN_bo(nvc0->bufctx_3d, SCREEN, flags, screen->text);
   BCTX_REFN_bo(nvc0->bufctx_3d, SCREEN, flags, screen->uniform_bo);
   BCTX_REFN_bo(nvc0->bufctx_3d, SCREEN, flags, screen->txc);
   BCTX_REFN_bo(nvc0->bufctx_3d, SCREEN, flags, screen->poly_cache);

   flags = NOUVEAU_BO_GART | NOUVEAU_BO_WR;

   BCTX_REFN_bo(nvc0->bufctx_3d, SCREEN, flags, screen->fence.bo);
   BCTX_REFN_bo(nvc0->bufctx, FENCE, flags, screen->fence.bo);

   nvc0->base.scratch.bo_size = 2 << 20;

   return pipe;

out_err:
   if (nvc0) {
      if (nvc0->bufctx_3d)
         nouveau_bufctx_del(&nvc0->bufctx_3d);
      if (nvc0->bufctx)
         nouveau_bufctx_del(&nvc0->bufctx);
      FREE(nvc0);
   }
   return NULL;
}

void
nvc0_bufctx_fence(struct nvc0_context *nvc0, struct nouveau_bufctx *bufctx,
                  boolean on_flush)
{
   struct nouveau_list *list = on_flush ? &bufctx->current : &bufctx->pending;
   struct nouveau_list *it;

   for (it = list->next; it != list; it = it->next) {
      struct nouveau_bufref *ref = (struct nouveau_bufref *)it;
      struct nv04_resource *res = ref->priv;
      if (res)
         nvc0_resource_validate(res, (unsigned)ref->priv_data);
   }
}
