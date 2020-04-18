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

#ifdef NV50_WITH_DRAW_MODULE
#include "draw/draw_context.h"
#endif

#include "nv50_context.h"
#include "nv50_screen.h"
#include "nv50_resource.h"

static void
nv50_flush(struct pipe_context *pipe,
           struct pipe_fence_handle **fence)
{
   struct nouveau_screen *screen = nouveau_screen(pipe->screen);

   if (fence)
      nouveau_fence_ref(screen->fence.current, (struct nouveau_fence **)fence);

   PUSH_KICK(screen->pushbuf);
}

static void
nv50_texture_barrier(struct pipe_context *pipe)
{
   struct nouveau_pushbuf *push = nv50_context(pipe)->base.pushbuf;

   BEGIN_NV04(push, SUBC_3D(NV50_GRAPH_SERIALIZE), 1);
   PUSH_DATA (push, 0);
   BEGIN_NV04(push, NV50_3D(TEX_CACHE_CTL), 1);
   PUSH_DATA (push, 0x20);
}

void
nv50_default_kick_notify(struct nouveau_pushbuf *push)
{
   struct nv50_screen *screen = push->user_priv;

   if (screen) {
      nouveau_fence_next(&screen->base);
      nouveau_fence_update(&screen->base, TRUE);
      if (screen->cur_ctx)
         screen->cur_ctx->state.flushed = TRUE;
   }
}

static void
nv50_context_unreference_resources(struct nv50_context *nv50)
{
   unsigned s, i;

   nouveau_bufctx_del(&nv50->bufctx_3d);
   nouveau_bufctx_del(&nv50->bufctx);

   util_unreference_framebuffer_state(&nv50->framebuffer);

   for (i = 0; i < nv50->num_vtxbufs; ++i)
      pipe_resource_reference(&nv50->vtxbuf[i].buffer, NULL);

   pipe_resource_reference(&nv50->idxbuf.buffer, NULL);

   for (s = 0; s < 3; ++s) {
      for (i = 0; i < nv50->num_textures[s]; ++i)
         pipe_sampler_view_reference(&nv50->textures[s][i], NULL);

      for (i = 0; i < NV50_MAX_PIPE_CONSTBUFS; ++i)
         if (!nv50->constbuf[s][i].user)
            pipe_resource_reference(&nv50->constbuf[s][i].u.buf, NULL);
   }
}

static void
nv50_destroy(struct pipe_context *pipe)
{
   struct nv50_context *nv50 = nv50_context(pipe);

   if (nv50_context_screen(nv50)->cur_ctx == nv50) {
      nv50->base.pushbuf->kick_notify = NULL;
      nv50_context_screen(nv50)->cur_ctx = NULL;
      nouveau_pushbuf_bufctx(nv50->base.pushbuf, NULL);
   }
   /* need to flush before destroying the bufctx */
   nouveau_pushbuf_kick(nv50->base.pushbuf, nv50->base.pushbuf->channel);

   nv50_context_unreference_resources(nv50);

#ifdef NV50_WITH_DRAW_MODULE
   draw_destroy(nv50->draw);
#endif

   nouveau_context_destroy(&nv50->base);
}

struct pipe_context *
nv50_create(struct pipe_screen *pscreen, void *priv)
{
   struct nv50_screen *screen = nv50_screen(pscreen);
   struct nv50_context *nv50;
   struct pipe_context *pipe;
   int ret;
   uint32_t flags;

   nv50 = CALLOC_STRUCT(nv50_context);
   if (!nv50)
      return NULL;
   pipe = &nv50->base.pipe;

   nv50->base.pushbuf = screen->base.pushbuf;

   ret = nouveau_bufctx_new(screen->base.client, NV50_BIND_COUNT,
                            &nv50->bufctx_3d);
   if (!ret)
      ret = nouveau_bufctx_new(screen->base.client, 2, &nv50->bufctx);
   if (ret)
      goto out_err;

   nv50->base.screen    = &screen->base;
   nv50->base.copy_data = nv50_m2mf_copy_linear;
   nv50->base.push_data = nv50_sifc_linear_u8;
   nv50->base.push_cb   = nv50_cb_push;

   nv50->screen = screen;
   pipe->screen = pscreen;
   pipe->priv = priv;

   pipe->destroy = nv50_destroy;

   pipe->draw_vbo = nv50_draw_vbo;
   pipe->clear = nv50_clear;

   pipe->flush = nv50_flush;
   pipe->texture_barrier = nv50_texture_barrier;

   if (!screen->cur_ctx) {
      screen->cur_ctx = nv50;
      nouveau_pushbuf_bufctx(screen->base.pushbuf, nv50->bufctx);
   }

   nv50_init_query_functions(nv50);
   nv50_init_surface_functions(nv50);
   nv50_init_state_functions(nv50);
   nv50_init_resource_functions(pipe);

#ifdef NV50_WITH_DRAW_MODULE
   /* no software fallbacks implemented */
   nv50->draw = draw_create(pipe);
   assert(nv50->draw);
   draw_set_rasterize_stage(nv50->draw, nv50_draw_render_stage(nv50));
#endif

   nouveau_context_init_vdec(&nv50->base);

   flags = NOUVEAU_BO_VRAM | NOUVEAU_BO_RD;

   BCTX_REFN_bo(nv50->bufctx_3d, SCREEN, flags, screen->code);
   BCTX_REFN_bo(nv50->bufctx_3d, SCREEN, flags, screen->uniforms);
   BCTX_REFN_bo(nv50->bufctx_3d, SCREEN, flags, screen->txc);
   BCTX_REFN_bo(nv50->bufctx_3d, SCREEN, flags, screen->stack_bo);

   flags = NOUVEAU_BO_GART | NOUVEAU_BO_WR;

   BCTX_REFN_bo(nv50->bufctx_3d, SCREEN, flags, screen->fence.bo);
   BCTX_REFN_bo(nv50->bufctx, FENCE, flags, screen->fence.bo);

   nv50->base.scratch.bo_size = 2 << 20;

   return pipe;

out_err:
   if (nv50) {
      if (nv50->bufctx_3d)
         nouveau_bufctx_del(&nv50->bufctx_3d);
      if (nv50->bufctx)
         nouveau_bufctx_del(&nv50->bufctx);
      FREE(nv50);
   }
   return NULL;
}

void
nv50_bufctx_fence(struct nouveau_bufctx *bufctx, boolean on_flush)
{
   struct nouveau_list *list = on_flush ? &bufctx->current : &bufctx->pending;
   struct nouveau_list *it;

   for (it = list->next; it != list; it = it->next) {
      struct nouveau_bufref *ref = (struct nouveau_bufref *)it;
      struct nv04_resource *res = ref->priv;
      if (res)
         nv50_resource_validate(res, (unsigned)ref->priv_data);
   }
}
