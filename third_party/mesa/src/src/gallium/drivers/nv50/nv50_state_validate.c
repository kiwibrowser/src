
#include "nv50_context.h"
#include "os/os_time.h"

static void
nv50_validate_fb(struct nv50_context *nv50)
{
   struct nouveau_pushbuf *push = nv50->base.pushbuf;
   struct pipe_framebuffer_state *fb = &nv50->framebuffer;
   unsigned i;
   unsigned ms_mode = NV50_3D_MULTISAMPLE_MODE_MS1;

   nouveau_bufctx_reset(nv50->bufctx_3d, NV50_BIND_FB);

   BEGIN_NV04(push, NV50_3D(RT_CONTROL), 1);
   PUSH_DATA (push, (076543210 << 4) | fb->nr_cbufs);
   BEGIN_NV04(push, NV50_3D(SCREEN_SCISSOR_HORIZ), 2);
   PUSH_DATA (push, fb->width << 16);
   PUSH_DATA (push, fb->height << 16);

   for (i = 0; i < fb->nr_cbufs; ++i) {
      struct nv50_miptree *mt = nv50_miptree(fb->cbufs[i]->texture);
      struct nv50_surface *sf = nv50_surface(fb->cbufs[i]);
      struct nouveau_bo *bo = mt->base.bo;

      BEGIN_NV04(push, NV50_3D(RT_ADDRESS_HIGH(i)), 5);
      PUSH_DATAh(push, bo->offset + sf->offset);
      PUSH_DATA (push, bo->offset + sf->offset);
      PUSH_DATA (push, nv50_format_table[sf->base.format].rt);
      if (likely(nouveau_bo_memtype(bo))) {
         PUSH_DATA (push, mt->level[sf->base.u.tex.level].tile_mode);
         PUSH_DATA (push, mt->layer_stride >> 2);
         BEGIN_NV04(push, NV50_3D(RT_HORIZ(i)), 2);
         PUSH_DATA (push, sf->width);
         PUSH_DATA (push, sf->height);
         BEGIN_NV04(push, NV50_3D(RT_ARRAY_MODE), 1);
         PUSH_DATA (push, sf->depth);
      } else {
         PUSH_DATA (push, 0);
         PUSH_DATA (push, 0);
         BEGIN_NV04(push, NV50_3D(RT_HORIZ(i)), 2);
         PUSH_DATA (push, NV50_3D_RT_HORIZ_LINEAR | mt->level[0].pitch);
         PUSH_DATA (push, sf->height);
         BEGIN_NV04(push, NV50_3D(RT_ARRAY_MODE), 1);
         PUSH_DATA (push, 0);

         assert(!fb->zsbuf);
         assert(!mt->ms_mode);
      }

      ms_mode = mt->ms_mode;

      if (mt->base.status & NOUVEAU_BUFFER_STATUS_GPU_READING)
         nv50->state.rt_serialize = TRUE;
      mt->base.status |= NOUVEAU_BUFFER_STATUS_GPU_WRITING;
      mt->base.status &= NOUVEAU_BUFFER_STATUS_GPU_READING;

      /* only register for writing, otherwise we'd always serialize here */
      BCTX_REFN(nv50->bufctx_3d, FB, &mt->base, WR);
   }

   if (fb->zsbuf) {
      struct nv50_miptree *mt = nv50_miptree(fb->zsbuf->texture);
      struct nv50_surface *sf = nv50_surface(fb->zsbuf);
      struct nouveau_bo *bo = mt->base.bo;
      int unk = mt->base.base.target == PIPE_TEXTURE_2D;

      BEGIN_NV04(push, NV50_3D(ZETA_ADDRESS_HIGH), 5);
      PUSH_DATAh(push, bo->offset + sf->offset);
      PUSH_DATA (push, bo->offset + sf->offset);
      PUSH_DATA (push, nv50_format_table[fb->zsbuf->format].rt);
      PUSH_DATA (push, mt->level[sf->base.u.tex.level].tile_mode);
      PUSH_DATA (push, mt->layer_stride >> 2);
      BEGIN_NV04(push, NV50_3D(ZETA_ENABLE), 1);
      PUSH_DATA (push, 1);
      BEGIN_NV04(push, NV50_3D(ZETA_HORIZ), 3);
      PUSH_DATA (push, sf->width);
      PUSH_DATA (push, sf->height);
      PUSH_DATA (push, (unk << 16) | sf->depth);

      ms_mode = mt->ms_mode;

      if (mt->base.status & NOUVEAU_BUFFER_STATUS_GPU_READING)
         nv50->state.rt_serialize = TRUE;
      mt->base.status |= NOUVEAU_BUFFER_STATUS_GPU_WRITING;
      mt->base.status &= NOUVEAU_BUFFER_STATUS_GPU_READING;

      BCTX_REFN(nv50->bufctx_3d, FB, &mt->base, WR);
   } else {
      BEGIN_NV04(push, NV50_3D(ZETA_ENABLE), 1);
      PUSH_DATA (push, 0);
   }

   BEGIN_NV04(push, NV50_3D(MULTISAMPLE_MODE), 1);
   PUSH_DATA (push, ms_mode);

   BEGIN_NV04(push, NV50_3D(VIEWPORT_HORIZ(0)), 2);
   PUSH_DATA (push, fb->width << 16);
   PUSH_DATA (push, fb->height << 16);
}

static void
nv50_validate_blend_colour(struct nv50_context *nv50)
{
   struct nouveau_pushbuf *push = nv50->base.pushbuf;

   BEGIN_NV04(push, NV50_3D(BLEND_COLOR(0)), 4);
   PUSH_DATAf(push, nv50->blend_colour.color[0]);
   PUSH_DATAf(push, nv50->blend_colour.color[1]);
   PUSH_DATAf(push, nv50->blend_colour.color[2]);
   PUSH_DATAf(push, nv50->blend_colour.color[3]);
}

static void
nv50_validate_stencil_ref(struct nv50_context *nv50)
{
   struct nouveau_pushbuf *push = nv50->base.pushbuf;

   BEGIN_NV04(push, NV50_3D(STENCIL_FRONT_FUNC_REF), 1);
   PUSH_DATA (push, nv50->stencil_ref.ref_value[0]);
   BEGIN_NV04(push, NV50_3D(STENCIL_BACK_FUNC_REF), 1);
   PUSH_DATA (push, nv50->stencil_ref.ref_value[1]);
}

static void
nv50_validate_stipple(struct nv50_context *nv50)
{
   struct nouveau_pushbuf *push = nv50->base.pushbuf;
   unsigned i;

   BEGIN_NV04(push, NV50_3D(POLYGON_STIPPLE_PATTERN(0)), 32);
   for (i = 0; i < 32; ++i)
      PUSH_DATA(push, util_bswap32(nv50->stipple.stipple[i]));
}

static void
nv50_validate_scissor(struct nv50_context *nv50)
{
   struct nouveau_pushbuf *push = nv50->base.pushbuf;
   struct pipe_scissor_state *s = &nv50->scissor;
#ifdef NV50_SCISSORS_CLIPPING
   struct pipe_viewport_state *vp = &nv50->viewport;
   int minx, maxx, miny, maxy;

   if (!(nv50->dirty &
         (NV50_NEW_SCISSOR | NV50_NEW_VIEWPORT | NV50_NEW_FRAMEBUFFER)) &&
       nv50->state.scissor == nv50->rast->pipe.scissor)
      return;
   nv50->state.scissor = nv50->rast->pipe.scissor;

   if (nv50->state.scissor) {
      minx = s->minx;
      maxx = s->maxx;
      miny = s->miny;
      maxy = s->maxy;
   } else {
      minx = 0;
      maxx = nv50->framebuffer.width;
      miny = 0;
      maxy = nv50->framebuffer.height;
   }

   minx = MAX2(minx, (int)(vp->translate[0] - fabsf(vp->scale[0])));
   maxx = MIN2(maxx, (int)(vp->translate[0] + fabsf(vp->scale[0])));
   miny = MAX2(miny, (int)(vp->translate[1] - fabsf(vp->scale[1])));
   maxy = MIN2(maxy, (int)(vp->translate[1] + fabsf(vp->scale[1])));

   BEGIN_NV04(push, NV50_3D(SCISSOR_HORIZ(0)), 2);
   PUSH_DATA (push, (maxx << 16) | minx);
   PUSH_DATA (push, (maxy << 16) | miny);
#else
   BEGIN_NV04(push, NV50_3D(SCISSOR_HORIZ(0)), 2);
   PUSH_DATA (push, (s->maxx << 16) | s->minx);
   PUSH_DATA (push, (s->maxy << 16) | s->miny);
#endif
}

static void
nv50_validate_viewport(struct nv50_context *nv50)
{
   struct nouveau_pushbuf *push = nv50->base.pushbuf;
   float zmin, zmax;

   BEGIN_NV04(push, NV50_3D(VIEWPORT_TRANSLATE_X(0)), 3);
   PUSH_DATAf(push, nv50->viewport.translate[0]);
   PUSH_DATAf(push, nv50->viewport.translate[1]);
   PUSH_DATAf(push, nv50->viewport.translate[2]);
   BEGIN_NV04(push, NV50_3D(VIEWPORT_SCALE_X(0)), 3);
   PUSH_DATAf(push, nv50->viewport.scale[0]);
   PUSH_DATAf(push, nv50->viewport.scale[1]);
   PUSH_DATAf(push, nv50->viewport.scale[2]);

   zmin = nv50->viewport.translate[2] - fabsf(nv50->viewport.scale[2]);
   zmax = nv50->viewport.translate[2] + fabsf(nv50->viewport.scale[2]);

#ifdef NV50_SCISSORS_CLIPPING
   BEGIN_NV04(push, NV50_3D(DEPTH_RANGE_NEAR(0)), 2);
   PUSH_DATAf(push, zmin);
   PUSH_DATAf(push, zmax);
#endif
}

static INLINE void
nv50_check_program_ucps(struct nv50_context *nv50,
                        struct nv50_program *vp, uint8_t mask)
{
   const unsigned n = util_logbase2(mask) + 1;

   if (vp->vp.clpd_nr >= n)
      return;
   nv50_program_destroy(nv50, vp);

   vp->vp.clpd_nr = n;
   if (likely(vp == nv50->vertprog)) {
      nv50->dirty |= NV50_NEW_VERTPROG;
      nv50_vertprog_validate(nv50);
   } else {
      nv50->dirty |= NV50_NEW_GMTYPROG;
      nv50_gmtyprog_validate(nv50);
   }
   nv50_fp_linkage_validate(nv50);
}

static void
nv50_validate_clip(struct nv50_context *nv50)
{
   struct nouveau_pushbuf *push = nv50->base.pushbuf;
   struct nv50_program *vp;
   uint8_t clip_enable;

   if (nv50->dirty & NV50_NEW_CLIP) {
      BEGIN_NV04(push, NV50_3D(CB_ADDR), 1);
      PUSH_DATA (push, (0 << 8) | NV50_CB_AUX);
      BEGIN_NI04(push, NV50_3D(CB_DATA(0)), PIPE_MAX_CLIP_PLANES * 4);
      PUSH_DATAp(push, &nv50->clip.ucp[0][0], PIPE_MAX_CLIP_PLANES * 4);
   }

   vp = nv50->gmtyprog;
   if (likely(!vp))
      vp = nv50->vertprog;

   clip_enable = nv50->rast->pipe.clip_plane_enable;

   BEGIN_NV04(push, NV50_3D(CLIP_DISTANCE_ENABLE), 1);
   PUSH_DATA (push, clip_enable);

   if (clip_enable)
      nv50_check_program_ucps(nv50, vp, clip_enable);
}

static void
nv50_validate_blend(struct nv50_context *nv50)
{
   struct nouveau_pushbuf *push = nv50->base.pushbuf;

   PUSH_SPACE(push, nv50->blend->size);
   PUSH_DATAp(push, nv50->blend->state, nv50->blend->size);
}

static void
nv50_validate_zsa(struct nv50_context *nv50)
{
   struct nouveau_pushbuf *push = nv50->base.pushbuf;

   PUSH_SPACE(push, nv50->zsa->size);
   PUSH_DATAp(push, nv50->zsa->state, nv50->zsa->size);
}

static void
nv50_validate_rasterizer(struct nv50_context *nv50)
{
   struct nouveau_pushbuf *push = nv50->base.pushbuf;

   PUSH_SPACE(push, nv50->rast->size);
   PUSH_DATAp(push, nv50->rast->state, nv50->rast->size);
}

static void
nv50_validate_sample_mask(struct nv50_context *nv50)
{
   struct nouveau_pushbuf *push = nv50->base.pushbuf;

   unsigned mask[4] =
   {
      nv50->sample_mask & 0xffff,
      nv50->sample_mask & 0xffff,
      nv50->sample_mask & 0xffff,
      nv50->sample_mask & 0xffff
   };

   BEGIN_NV04(push, NV50_3D(MSAA_MASK(0)), 4);
   PUSH_DATA (push, mask[0]);
   PUSH_DATA (push, mask[1]);
   PUSH_DATA (push, mask[2]);
   PUSH_DATA (push, mask[3]);
}

static void
nv50_switch_pipe_context(struct nv50_context *ctx_to)
{
   struct nv50_context *ctx_from = ctx_to->screen->cur_ctx;

   if (ctx_from)
      ctx_to->state = ctx_from->state;

   ctx_to->dirty = ~0;

   if (!ctx_to->vertex)
      ctx_to->dirty &= ~(NV50_NEW_VERTEX | NV50_NEW_ARRAYS);

   if (!ctx_to->vertprog)
      ctx_to->dirty &= ~NV50_NEW_VERTPROG;
   if (!ctx_to->fragprog)
      ctx_to->dirty &= ~NV50_NEW_FRAGPROG;

   if (!ctx_to->blend)
      ctx_to->dirty &= ~NV50_NEW_BLEND;
   if (!ctx_to->rast)
#ifdef NV50_SCISSORS_CLIPPING
      ctx_to->dirty &= ~(NV50_NEW_RASTERIZER | NV50_NEW_SCISSOR);
#else
      ctx_to->dirty &= ~NV50_NEW_RASTERIZER;
#endif
   if (!ctx_to->zsa)
      ctx_to->dirty &= ~NV50_NEW_ZSA;

   ctx_to->screen->cur_ctx = ctx_to;
}

static struct state_validate {
    void (*func)(struct nv50_context *);
    uint32_t states;
} validate_list[] = {
    { nv50_validate_fb,            NV50_NEW_FRAMEBUFFER },
    { nv50_validate_blend,         NV50_NEW_BLEND },
    { nv50_validate_zsa,           NV50_NEW_ZSA },
    { nv50_validate_sample_mask,   NV50_NEW_SAMPLE_MASK },
    { nv50_validate_rasterizer,    NV50_NEW_RASTERIZER },
    { nv50_validate_blend_colour,  NV50_NEW_BLEND_COLOUR },
    { nv50_validate_stencil_ref,   NV50_NEW_STENCIL_REF },
    { nv50_validate_stipple,       NV50_NEW_STIPPLE },
#ifdef NV50_SCISSORS_CLIPPING
    { nv50_validate_scissor,       NV50_NEW_SCISSOR | NV50_NEW_VIEWPORT |
                                   NV50_NEW_RASTERIZER |
                                   NV50_NEW_FRAMEBUFFER },
#else
    { nv50_validate_scissor,       NV50_NEW_SCISSOR },
#endif
    { nv50_validate_viewport,      NV50_NEW_VIEWPORT },
    { nv50_vertprog_validate,      NV50_NEW_VERTPROG },
    { nv50_gmtyprog_validate,      NV50_NEW_GMTYPROG },
    { nv50_fragprog_validate,      NV50_NEW_FRAGPROG },
    { nv50_fp_linkage_validate,    NV50_NEW_FRAGPROG | NV50_NEW_VERTPROG |
                                   NV50_NEW_GMTYPROG | NV50_NEW_RASTERIZER },
    { nv50_gp_linkage_validate,    NV50_NEW_GMTYPROG | NV50_NEW_VERTPROG },
    { nv50_validate_derived_rs,    NV50_NEW_FRAGPROG | NV50_NEW_RASTERIZER |
                                   NV50_NEW_VERTPROG | NV50_NEW_GMTYPROG },
    { nv50_validate_clip,          NV50_NEW_CLIP | NV50_NEW_RASTERIZER |
                                   NV50_NEW_VERTPROG | NV50_NEW_GMTYPROG },
    { nv50_constbufs_validate,     NV50_NEW_CONSTBUF },
    { nv50_validate_textures,      NV50_NEW_TEXTURES },
    { nv50_validate_samplers,      NV50_NEW_SAMPLERS },
    { nv50_stream_output_validate, NV50_NEW_STRMOUT |
                                   NV50_NEW_VERTPROG | NV50_NEW_GMTYPROG },
    { nv50_vertex_arrays_validate, NV50_NEW_VERTEX | NV50_NEW_ARRAYS }
};
#define validate_list_len (sizeof(validate_list) / sizeof(validate_list[0]))

boolean
nv50_state_validate(struct nv50_context *nv50, uint32_t mask, unsigned words)
{
   uint32_t state_mask;
   int ret;
   unsigned i;

   if (nv50->screen->cur_ctx != nv50)
      nv50_switch_pipe_context(nv50);

   state_mask = nv50->dirty & mask;

   if (state_mask) {
      for (i = 0; i < validate_list_len; ++i) {
         struct state_validate *validate = &validate_list[i];

         if (state_mask & validate->states)
            validate->func(nv50);
      }
      nv50->dirty &= ~state_mask;

      if (nv50->state.rt_serialize) {
         nv50->state.rt_serialize = FALSE;
         BEGIN_NV04(nv50->base.pushbuf, SUBC_3D(NV50_GRAPH_SERIALIZE), 1);
         PUSH_DATA (nv50->base.pushbuf, 0);
      }

      nv50_bufctx_fence(nv50->bufctx_3d, FALSE);
   }
   nouveau_pushbuf_bufctx(nv50->base.pushbuf, nv50->bufctx_3d);
   ret = nouveau_pushbuf_validate(nv50->base.pushbuf);

   if (unlikely(nv50->state.flushed)) {
      nv50->state.flushed = FALSE;
      nv50_bufctx_fence(nv50->bufctx_3d, TRUE);
   }
   return !ret;
}
