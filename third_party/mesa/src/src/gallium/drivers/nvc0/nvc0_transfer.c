
#include "util/u_format.h"

#include "nvc0_context.h"

#include "nv50/nv50_defs.xml.h"

struct nvc0_transfer {
   struct pipe_transfer base;
   struct nv50_m2mf_rect rect[2];
   uint32_t nblocksx;
   uint16_t nblocksy;
   uint16_t nlayers;
};

static void
nvc0_m2mf_transfer_rect(struct nvc0_context *nvc0,
                        const struct nv50_m2mf_rect *dst,
                        const struct nv50_m2mf_rect *src,
                        uint32_t nblocksx, uint32_t nblocksy)
{
   struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   struct nouveau_bufctx *bctx = nvc0->bufctx;
   const int cpp = dst->cpp;
   uint32_t src_ofst = src->base;
   uint32_t dst_ofst = dst->base;
   uint32_t height = nblocksy;
   uint32_t sy = src->y;
   uint32_t dy = dst->y;
   uint32_t exec = (1 << 20);

   assert(dst->cpp == src->cpp);

   nouveau_bufctx_refn(bctx, 0, src->bo, src->domain | NOUVEAU_BO_RD);
   nouveau_bufctx_refn(bctx, 0, dst->bo, dst->domain | NOUVEAU_BO_WR);
   nouveau_pushbuf_bufctx(push, bctx);
   nouveau_pushbuf_validate(push);

   if (nouveau_bo_memtype(src->bo)) {
      BEGIN_NVC0(push, NVC0_M2MF(TILING_MODE_IN), 5);
      PUSH_DATA (push, src->tile_mode);
      PUSH_DATA (push, src->width * cpp);
      PUSH_DATA (push, src->height);
      PUSH_DATA (push, src->depth);
      PUSH_DATA (push, src->z);
   } else {
      src_ofst += src->y * src->pitch + src->x * cpp;

      BEGIN_NVC0(push, NVC0_M2MF(PITCH_IN), 1);
      PUSH_DATA (push, src->width * cpp);

      exec |= NVC0_M2MF_EXEC_LINEAR_IN;
   }

   if (nouveau_bo_memtype(dst->bo)) {
      BEGIN_NVC0(push, NVC0_M2MF(TILING_MODE_OUT), 5);
      PUSH_DATA (push, dst->tile_mode);
      PUSH_DATA (push, dst->width * cpp);
      PUSH_DATA (push, dst->height);
      PUSH_DATA (push, dst->depth);
      PUSH_DATA (push, dst->z);
   } else {
      dst_ofst += dst->y * dst->pitch + dst->x * cpp;

      BEGIN_NVC0(push, NVC0_M2MF(PITCH_OUT), 1);
      PUSH_DATA (push, dst->width * cpp);

      exec |= NVC0_M2MF_EXEC_LINEAR_OUT;
   }

   while (height) {
      int line_count = height > 2047 ? 2047 : height;

      BEGIN_NVC0(push, NVC0_M2MF(OFFSET_IN_HIGH), 2);
      PUSH_DATAh(push, src->bo->offset + src_ofst);
      PUSH_DATA (push, src->bo->offset + src_ofst);

      BEGIN_NVC0(push, NVC0_M2MF(OFFSET_OUT_HIGH), 2);
      PUSH_DATAh(push, dst->bo->offset + dst_ofst);
      PUSH_DATA (push, dst->bo->offset + dst_ofst);

      if (!(exec & NVC0_M2MF_EXEC_LINEAR_IN)) {
         BEGIN_NVC0(push, NVC0_M2MF(TILING_POSITION_IN_X), 2);
         PUSH_DATA (push, src->x * cpp);
         PUSH_DATA (push, sy);
      } else {
         src_ofst += line_count * src->pitch;
      }
      if (!(exec & NVC0_M2MF_EXEC_LINEAR_OUT)) {
         BEGIN_NVC0(push, NVC0_M2MF(TILING_POSITION_OUT_X), 2);
         PUSH_DATA (push, dst->x * cpp);
         PUSH_DATA (push, dy);
      } else {
         dst_ofst += line_count * dst->pitch;
      }

      BEGIN_NVC0(push, NVC0_M2MF(LINE_LENGTH_IN), 2);
      PUSH_DATA (push, nblocksx * cpp);
      PUSH_DATA (push, line_count);
      BEGIN_NVC0(push, NVC0_M2MF(EXEC), 1);
      PUSH_DATA (push, exec);

      height -= line_count;
      sy += line_count;
      dy += line_count;
   }

   nouveau_bufctx_reset(bctx, 0);
}

static void
nve4_m2mf_transfer_rect(struct nvc0_context *nvc0,
                        const struct nv50_m2mf_rect *dst,
                        const struct nv50_m2mf_rect *src,
                        uint32_t nblocksx, uint32_t nblocksy)
{
   struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   struct nouveau_bufctx *bctx = nvc0->bufctx;
   uint32_t exec;
   uint32_t src_base = src->base;
   uint32_t dst_base = dst->base;
   const int cpp = dst->cpp;

   assert(dst->cpp == src->cpp);

   nouveau_bufctx_refn(bctx, 0, dst->bo, dst->domain | NOUVEAU_BO_WR);
   nouveau_bufctx_refn(bctx, 0, src->bo, src->domain | NOUVEAU_BO_RD);
   nouveau_pushbuf_bufctx(push, bctx);
   nouveau_pushbuf_validate(push);

   exec = 0x200 /* 2D_ENABLE */ | 0x6 /* UNK */;

   if (!nouveau_bo_memtype(dst->bo)) {
      assert(!dst->z);
      dst_base += dst->y * dst->pitch + dst->x * cpp;
      exec |= 0x100; /* DST_MODE_2D_LINEAR */
   }
   if (!nouveau_bo_memtype(src->bo)) {
      assert(!src->z);
      src_base += src->y * src->pitch + src->x * cpp;
      exec |= 0x080; /* SRC_MODE_2D_LINEAR */
   }

   BEGIN_NVC0(push, SUBC_COPY(0x070c), 6);
   PUSH_DATA (push, 0x1000 | dst->tile_mode);
   PUSH_DATA (push, dst->pitch);
   PUSH_DATA (push, dst->height);
   PUSH_DATA (push, dst->depth);
   PUSH_DATA (push, dst->z);
   PUSH_DATA (push, (dst->y << 16) | (dst->x * cpp));

   BEGIN_NVC0(push, SUBC_COPY(0x0728), 6);
   PUSH_DATA (push, 0x1000 | src->tile_mode);
   PUSH_DATA (push, src->pitch);
   PUSH_DATA (push, src->height);
   PUSH_DATA (push, src->depth);
   PUSH_DATA (push, src->z);
   PUSH_DATA (push, (src->y << 16) | (src->x * cpp));

   BEGIN_NVC0(push, SUBC_COPY(0x0400), 8);
   PUSH_DATAh(push, src->bo->offset + src_base);
   PUSH_DATA (push, src->bo->offset + src_base);
   PUSH_DATAh(push, dst->bo->offset + dst_base);
   PUSH_DATA (push, dst->bo->offset + dst_base);
   PUSH_DATA (push, src->pitch);
   PUSH_DATA (push, dst->pitch);
   PUSH_DATA (push, nblocksx * cpp);
   PUSH_DATA (push, nblocksy);

   BEGIN_NVC0(push, SUBC_COPY(0x0300), 1);
   PUSH_DATA (push, exec);

   nouveau_bufctx_reset(bctx, 0);
}

void
nvc0_m2mf_push_linear(struct nouveau_context *nv,
                      struct nouveau_bo *dst, unsigned offset, unsigned domain,
                      unsigned size, const void *data)
{
   struct nvc0_context *nvc0 = nvc0_context(&nv->pipe);
   struct nouveau_pushbuf *push = nv->pushbuf;
   uint32_t *src = (uint32_t *)data;
   unsigned count = (size + 3) / 4;

   nouveau_bufctx_refn(nvc0->bufctx, 0, dst, domain | NOUVEAU_BO_WR);
   nouveau_pushbuf_bufctx(push, nvc0->bufctx);
   nouveau_pushbuf_validate(push);

   while (count) {
      unsigned nr;

      if (!PUSH_SPACE(push, 16))
         break;
      nr = PUSH_AVAIL(push);
      assert(nr >= 16);
      nr = MIN2(count, nr - 9);
      nr = MIN2(nr, NV04_PFIFO_MAX_PACKET_LEN);

      BEGIN_NVC0(push, NVC0_M2MF(OFFSET_OUT_HIGH), 2);
      PUSH_DATAh(push, dst->offset + offset);
      PUSH_DATA (push, dst->offset + offset);
      BEGIN_NVC0(push, NVC0_M2MF(LINE_LENGTH_IN), 2);
      PUSH_DATA (push, nr * 4);
      PUSH_DATA (push, 1);
      BEGIN_NVC0(push, NVC0_M2MF(EXEC), 1);
      PUSH_DATA (push, 0x100111);

      /* must not be interrupted (trap on QUERY fence, 0x50 works however) */
      BEGIN_NIC0(push, NVC0_M2MF(DATA), nr);
      PUSH_DATAp(push, src, nr);

      count -= nr;
      src += nr;
      offset += nr * 4;
   }

   nouveau_bufctx_reset(nvc0->bufctx, 0);
}

void
nve4_p2mf_push_linear(struct nouveau_context *nv,
                      struct nouveau_bo *dst, unsigned offset, unsigned domain,
                      unsigned size, const void *data)
{
   struct nvc0_context *nvc0 = nvc0_context(&nv->pipe);
   struct nouveau_pushbuf *push = nv->pushbuf;
   uint32_t *src = (uint32_t *)data;
   unsigned count = (size + 3) / 4;

   nouveau_bufctx_refn(nvc0->bufctx, 0, dst, domain | NOUVEAU_BO_WR);
   nouveau_pushbuf_bufctx(push, nvc0->bufctx);
   nouveau_pushbuf_validate(push);

   while (count) {
      unsigned nr;

      if (!PUSH_SPACE(push, 16))
         break;
      nr = PUSH_AVAIL(push);
      assert(nr >= 16);
      nr = MIN2(count, nr - 8);
      nr = MIN2(nr, (NV04_PFIFO_MAX_PACKET_LEN - 1));

      BEGIN_NVC0(push, NVE4_P2MF(DST_ADDRESS_HIGH), 2);
      PUSH_DATAh(push, dst->offset + offset);
      PUSH_DATA (push, dst->offset + offset);
      BEGIN_NVC0(push, NVE4_P2MF(LINE_LENGTH_IN), 2);
      PUSH_DATA (push, nr * 4);
      PUSH_DATA (push, 1);
      /* must not be interrupted (trap on QUERY fence, 0x50 works however) */
      BEGIN_1IC0(push, NVE4_P2MF(EXEC), nr + 1);
      PUSH_DATA (push, 0x1001);
      PUSH_DATAp(push, src, nr);

      count -= nr;
      src += nr;
      offset += nr * 4;
   }

   nouveau_bufctx_reset(nvc0->bufctx, 0);
}

static void
nvc0_m2mf_copy_linear(struct nouveau_context *nv,
                      struct nouveau_bo *dst, unsigned dstoff, unsigned dstdom,
                      struct nouveau_bo *src, unsigned srcoff, unsigned srcdom,
                      unsigned size)
{
   struct nouveau_pushbuf *push = nv->pushbuf;
   struct nouveau_bufctx *bctx = nvc0_context(&nv->pipe)->bufctx;

   nouveau_bufctx_refn(bctx, 0, src, srcdom | NOUVEAU_BO_RD);
   nouveau_bufctx_refn(bctx, 0, dst, dstdom | NOUVEAU_BO_WR);
   nouveau_pushbuf_bufctx(push, bctx);
   nouveau_pushbuf_validate(push);

   while (size) {
      unsigned bytes = MIN2(size, 1 << 17);

      BEGIN_NVC0(push, NVC0_M2MF(OFFSET_OUT_HIGH), 2);
      PUSH_DATAh(push, dst->offset + dstoff);
      PUSH_DATA (push, dst->offset + dstoff);
      BEGIN_NVC0(push, NVC0_M2MF(OFFSET_IN_HIGH), 2);
      PUSH_DATAh(push, src->offset + srcoff);
      PUSH_DATA (push, src->offset + srcoff);
      BEGIN_NVC0(push, NVC0_M2MF(LINE_LENGTH_IN), 2);
      PUSH_DATA (push, bytes);
      PUSH_DATA (push, 1);
      BEGIN_NVC0(push, NVC0_M2MF(EXEC), 1);
      PUSH_DATA (push, (1 << NVC0_M2MF_EXEC_INC__SHIFT) |
                 NVC0_M2MF_EXEC_LINEAR_IN | NVC0_M2MF_EXEC_LINEAR_OUT);

      srcoff += bytes;
      dstoff += bytes;
      size -= bytes;
   }

   nouveau_bufctx_reset(bctx, 0);
}

static void
nve4_m2mf_copy_linear(struct nouveau_context *nv,
                      struct nouveau_bo *dst, unsigned dstoff, unsigned dstdom,
                      struct nouveau_bo *src, unsigned srcoff, unsigned srcdom,
                      unsigned size)
{
   struct nouveau_pushbuf *push = nv->pushbuf;
   struct nouveau_bufctx *bctx = nvc0_context(&nv->pipe)->bufctx;

   nouveau_bufctx_refn(bctx, 0, src, srcdom | NOUVEAU_BO_RD);
   nouveau_bufctx_refn(bctx, 0, dst, dstdom | NOUVEAU_BO_WR);
   nouveau_pushbuf_bufctx(push, bctx);
   nouveau_pushbuf_validate(push);

   BEGIN_NVC0(push, SUBC_COPY(0x0400), 4);
   PUSH_DATAh(push, src->offset + srcoff);
   PUSH_DATA (push, src->offset + srcoff);
   PUSH_DATAh(push, dst->offset + dstoff);
   PUSH_DATA (push, dst->offset + dstoff);
   BEGIN_NVC0(push, SUBC_COPY(0x0418), 1);
   PUSH_DATA (push, size);
   BEGIN_NVC0(push, SUBC_COPY(0x0300), 1);
   PUSH_DATA (push, 0x186);

   nouveau_bufctx_reset(bctx, 0);
}

struct pipe_transfer *
nvc0_miptree_transfer_new(struct pipe_context *pctx,
                          struct pipe_resource *res,
                          unsigned level,
                          unsigned usage,
                          const struct pipe_box *box)
{
   struct nvc0_context *nvc0 = nvc0_context(pctx);
   struct nouveau_device *dev = nvc0->screen->base.device;
   struct nv50_miptree *mt = nv50_miptree(res);
   struct nvc0_transfer *tx;
   uint32_t size;
   int ret;

   if (usage & PIPE_TRANSFER_MAP_DIRECTLY)
      return NULL;

   tx = CALLOC_STRUCT(nvc0_transfer);
   if (!tx)
      return NULL;

   pipe_resource_reference(&tx->base.resource, res);

   tx->base.level = level;
   tx->base.usage = usage;
   tx->base.box = *box;

   if (util_format_is_plain(res->format)) {
      tx->nblocksx = box->width << mt->ms_x;
      tx->nblocksy = box->height << mt->ms_y;
   } else {
      tx->nblocksx = util_format_get_nblocksx(res->format, box->width);
      tx->nblocksy = util_format_get_nblocksy(res->format, box->height);
   }
   tx->nlayers = box->depth;

   tx->base.stride = tx->nblocksx * util_format_get_blocksize(res->format);
   tx->base.layer_stride = tx->nblocksy * tx->base.stride;

   nv50_m2mf_rect_setup(&tx->rect[0], res, level, box->x, box->y, box->z);

   size = tx->base.layer_stride;

   ret = nouveau_bo_new(dev, NOUVEAU_BO_GART | NOUVEAU_BO_MAP, 0,
                        size * tx->nlayers, NULL, &tx->rect[1].bo);
   if (ret) {
      FREE(tx);
      return NULL;
   }

   tx->rect[1].cpp = tx->rect[0].cpp;
   tx->rect[1].width = tx->nblocksx;
   tx->rect[1].height = tx->nblocksy;
   tx->rect[1].depth = 1;
   tx->rect[1].pitch = tx->base.stride;
   tx->rect[1].domain = NOUVEAU_BO_GART;

   if (usage & PIPE_TRANSFER_READ) {
      unsigned base = tx->rect[0].base;
      unsigned z = tx->rect[0].z;
      unsigned i;
      for (i = 0; i < tx->nlayers; ++i) {
         nvc0->m2mf_copy_rect(nvc0, &tx->rect[1], &tx->rect[0],
                              tx->nblocksx, tx->nblocksy);
         if (mt->layout_3d)
            tx->rect[0].z++;
         else
            tx->rect[0].base += mt->layer_stride;
         tx->rect[1].base += size;
      }
      tx->rect[0].z = z;
      tx->rect[0].base = base;
      tx->rect[1].base = 0;
   }

   return &tx->base;
}

void
nvc0_miptree_transfer_del(struct pipe_context *pctx,
                          struct pipe_transfer *transfer)
{
   struct nvc0_context *nvc0 = nvc0_context(pctx);
   struct nvc0_transfer *tx = (struct nvc0_transfer *)transfer;
   struct nv50_miptree *mt = nv50_miptree(tx->base.resource);
   unsigned i;

   if (tx->base.usage & PIPE_TRANSFER_WRITE) {
      for (i = 0; i < tx->nlayers; ++i) {
         nvc0->m2mf_copy_rect(nvc0, &tx->rect[0], &tx->rect[1],
                              tx->nblocksx, tx->nblocksy);
         if (mt->layout_3d)
            tx->rect[0].z++;
         else
            tx->rect[0].base += mt->layer_stride;
         tx->rect[1].base += tx->nblocksy * tx->base.stride;
      }
   }

   nouveau_bo_ref(NULL, &tx->rect[1].bo);
   pipe_resource_reference(&transfer->resource, NULL);

   FREE(tx);
}

void *
nvc0_miptree_transfer_map(struct pipe_context *pctx,
                          struct pipe_transfer *transfer)
{
   struct nvc0_context *nvc0 = nvc0_context(pctx);
   struct nvc0_transfer *tx = (struct nvc0_transfer *)transfer;
   int ret;
   unsigned flags = 0;

   if (tx->rect[1].bo->map)
      return tx->rect[1].bo->map;

   if (transfer->usage & PIPE_TRANSFER_READ)
      flags = NOUVEAU_BO_RD;
   if (transfer->usage & PIPE_TRANSFER_WRITE)
      flags |= NOUVEAU_BO_WR;

   ret = nouveau_bo_map(tx->rect[1].bo, flags, nvc0->screen->base.client);
   if (ret)
      return NULL;
   return tx->rect[1].bo->map;
}

void
nvc0_miptree_transfer_unmap(struct pipe_context *pctx,
                            struct pipe_transfer *transfer)
{
}

void
nvc0_cb_push(struct nouveau_context *nv,
             struct nouveau_bo *bo, unsigned domain,
             unsigned base, unsigned size,
             unsigned offset, unsigned words, const uint32_t *data)
{
   struct nouveau_bufctx *bctx = nvc0_context(&nv->pipe)->bufctx;
   struct nouveau_pushbuf *push = nv->pushbuf;

   assert(!(offset & 3));
   size = align(size, 0x100);

   nouveau_bufctx_refn(bctx, 0, bo, NOUVEAU_BO_WR | domain);
   nouveau_pushbuf_bufctx(push, bctx);
   nouveau_pushbuf_validate(push);

   BEGIN_NVC0(push, NVC0_3D(CB_SIZE), 3);
   PUSH_DATA (push, size);
   PUSH_DATAh(push, bo->offset + base);
   PUSH_DATA (push, bo->offset + base);

   while (words) {
      unsigned nr = PUSH_AVAIL(push);
      nr = MIN2(nr, words);
      nr = MIN2(nr, NV04_PFIFO_MAX_PACKET_LEN - 1);

      BEGIN_1IC0(push, NVC0_3D(CB_POS), nr + 1);
      PUSH_DATA (push, offset);
      PUSH_DATAp(push, data, nr);

      words -= nr;
      data += nr;
      offset += nr * 4;
   }

   nouveau_bufctx_reset(bctx, 0);
}

void
nvc0_init_transfer_functions(struct nvc0_context *nvc0)
{
   if (nvc0->screen->base.class_3d >= NVE4_3D_CLASS) {
      nvc0->m2mf_copy_rect = nve4_m2mf_transfer_rect;
      nvc0->base.copy_data = nve4_m2mf_copy_linear;
      nvc0->base.push_data = nve4_p2mf_push_linear;
   } else {
      nvc0->m2mf_copy_rect = nvc0_m2mf_transfer_rect;
      nvc0->base.copy_data = nvc0_m2mf_copy_linear;
      nvc0->base.push_data = nvc0_m2mf_push_linear;
   }
   nvc0->base.push_cb = nvc0_cb_push;
}
