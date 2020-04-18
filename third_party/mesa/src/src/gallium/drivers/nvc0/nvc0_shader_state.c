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

#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_state.h"
#include "util/u_inlines.h"

#include "nvc0_context.h"

static INLINE void
nvc0_program_update_context_state(struct nvc0_context *nvc0,
                                  struct nvc0_program *prog, int stage)
{
   struct nouveau_pushbuf *push = nvc0->base.pushbuf;

   if (prog && prog->need_tls) {
      const uint32_t flags = NOUVEAU_BO_VRAM | NOUVEAU_BO_RDWR;
      if (!nvc0->state.tls_required)
         BCTX_REFN_bo(nvc0->bufctx_3d, TLS, flags, nvc0->screen->tls);
      nvc0->state.tls_required |= 1 << stage;
   } else {
      if (nvc0->state.tls_required == (1 << stage))
         nouveau_bufctx_reset(nvc0->bufctx_3d, NVC0_BIND_TLS);
      nvc0->state.tls_required &= ~(1 << stage);
   }

   if (prog && prog->immd_size) {
      BEGIN_NVC0(push, NVC0_3D(CB_SIZE), 3);
      /* NOTE: may overlap code of a different shader */
      PUSH_DATA (push, align(prog->immd_size, 0x100));
      PUSH_DATAh(push, nvc0->screen->text->offset + prog->immd_base);
      PUSH_DATA (push, nvc0->screen->text->offset + prog->immd_base);
      BEGIN_NVC0(push, NVC0_3D(CB_BIND(stage)), 1);
      PUSH_DATA (push, (14 << 4) | 1);

      nvc0->state.c14_bound |= 1 << stage;
   } else
   if (nvc0->state.c14_bound & (1 << stage)) {
      BEGIN_NVC0(push, NVC0_3D(CB_BIND(stage)), 1);
      PUSH_DATA (push, (14 << 4) | 0);

      nvc0->state.c14_bound &= ~(1 << stage);
   }
}

static INLINE boolean
nvc0_program_validate(struct nvc0_context *nvc0, struct nvc0_program *prog)
{
   if (prog->mem)
      return TRUE;

   if (!prog->translated) {
      prog->translated = nvc0_program_translate(
         prog, nvc0->screen->base.device->chipset);
      if (!prog->translated)
         return FALSE;
   }

   if (likely(prog->code_size))
      return nvc0_program_upload_code(nvc0, prog);
   return TRUE; /* stream output info only */
}

void
nvc0_vertprog_validate(struct nvc0_context *nvc0)
{
   struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   struct nvc0_program *vp = nvc0->vertprog;

   if (!nvc0_program_validate(nvc0, vp))
         return;
   nvc0_program_update_context_state(nvc0, vp, 0);

   BEGIN_NVC0(push, NVC0_3D(SP_SELECT(1)), 2);
   PUSH_DATA (push, 0x11);
   PUSH_DATA (push, vp->code_base);
   BEGIN_NVC0(push, NVC0_3D(SP_GPR_ALLOC(1)), 1);
   PUSH_DATA (push, vp->max_gpr);

   // BEGIN_NVC0(push, NVC0_3D_(0x163c), 1);
   // PUSH_DATA (push, 0);
}

void
nvc0_fragprog_validate(struct nvc0_context *nvc0)
{
   struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   struct nvc0_program *fp = nvc0->fragprog;

   if (!nvc0_program_validate(nvc0, fp))
         return;
   nvc0_program_update_context_state(nvc0, fp, 4);

   if (fp->fp.early_z != nvc0->state.early_z_forced) {
      nvc0->state.early_z_forced = fp->fp.early_z;
      IMMED_NVC0(push, NVC0_3D(FORCE_EARLY_FRAGMENT_TESTS), fp->fp.early_z);
   }

   BEGIN_NVC0(push, NVC0_3D(SP_SELECT(5)), 2);
   PUSH_DATA (push, 0x51);
   PUSH_DATA (push, fp->code_base);
   BEGIN_NVC0(push, NVC0_3D(SP_GPR_ALLOC(5)), 1);
   PUSH_DATA (push, fp->max_gpr);

   BEGIN_NVC0(push, SUBC_3D(0x0360), 2);
   PUSH_DATA (push, 0x20164010);
   PUSH_DATA (push, 0x20);
   BEGIN_NVC0(push, NVC0_3D(ZCULL_TEST_MASK), 1);
   PUSH_DATA (push, fp->flags[0]);
}

void
nvc0_tctlprog_validate(struct nvc0_context *nvc0)
{
   struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   struct nvc0_program *tp = nvc0->tctlprog;

   if (tp && nvc0_program_validate(nvc0, tp)) {
      if (tp->tp.tess_mode != ~0) {
         BEGIN_NVC0(push, NVC0_3D(TESS_MODE), 1);
         PUSH_DATA (push, tp->tp.tess_mode);
      }
      BEGIN_NVC0(push, NVC0_3D(SP_SELECT(2)), 2);
      PUSH_DATA (push, 0x21);
      PUSH_DATA (push, tp->code_base);
      BEGIN_NVC0(push, NVC0_3D(SP_GPR_ALLOC(2)), 1);
      PUSH_DATA (push, tp->max_gpr);

      if (tp->tp.input_patch_size <= 32)
         IMMED_NVC0(push, NVC0_3D(PATCH_VERTICES), tp->tp.input_patch_size);
   } else {
      BEGIN_NVC0(push, NVC0_3D(SP_SELECT(2)), 1);
      PUSH_DATA (push, 0x20);
   }
   nvc0_program_update_context_state(nvc0, tp, 1);
}

void
nvc0_tevlprog_validate(struct nvc0_context *nvc0)
{
   struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   struct nvc0_program *tp = nvc0->tevlprog;

   if (tp && nvc0_program_validate(nvc0, tp)) {
      if (tp->tp.tess_mode != ~0) {
         BEGIN_NVC0(push, NVC0_3D(TESS_MODE), 1);
         PUSH_DATA (push, tp->tp.tess_mode);
      }
      BEGIN_NVC0(push, NVC0_3D(MACRO_TEP_SELECT), 1);
      PUSH_DATA (push, 0x31);
      BEGIN_NVC0(push, NVC0_3D(SP_START_ID(3)), 1);
      PUSH_DATA (push, tp->code_base);
      BEGIN_NVC0(push, NVC0_3D(SP_GPR_ALLOC(3)), 1);
      PUSH_DATA (push, tp->max_gpr);
   } else {
      BEGIN_NVC0(push, NVC0_3D(MACRO_TEP_SELECT), 1);
      PUSH_DATA (push, 0x30);
   }
   nvc0_program_update_context_state(nvc0, tp, 2);
}

void
nvc0_gmtyprog_validate(struct nvc0_context *nvc0)
{
   struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   struct nvc0_program *gp = nvc0->gmtyprog;

   if (gp)
      nvc0_program_validate(nvc0, gp);

   /* we allow GPs with no code for specifying stream output state only */
   if (gp && gp->code_size) {
      const boolean gp_selects_layer = gp->hdr[13] & (1 << 9);

      BEGIN_NVC0(push, NVC0_3D(MACRO_GP_SELECT), 1);
      PUSH_DATA (push, 0x41);
      BEGIN_NVC0(push, NVC0_3D(SP_START_ID(4)), 1);
      PUSH_DATA (push, gp->code_base);
      BEGIN_NVC0(push, NVC0_3D(SP_GPR_ALLOC(4)), 1);
      PUSH_DATA (push, gp->max_gpr);
      BEGIN_NVC0(push, NVC0_3D(LAYER), 1);
      PUSH_DATA (push, gp_selects_layer ? NVC0_3D_LAYER_USE_GP : 0);
   } else {
      IMMED_NVC0(push, NVC0_3D(LAYER), 0);
      BEGIN_NVC0(push, NVC0_3D(MACRO_GP_SELECT), 1);
      PUSH_DATA (push, 0x40);
   }
   nvc0_program_update_context_state(nvc0, gp, 3);
}

void
nvc0_tfb_validate(struct nvc0_context *nvc0)
{
   struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   struct nvc0_transform_feedback_state *tfb;
   unsigned b;

   if (nvc0->gmtyprog) tfb = nvc0->gmtyprog->tfb;
   else
   if (nvc0->tevlprog) tfb = nvc0->tevlprog->tfb;
   else
      tfb = nvc0->vertprog->tfb;

   IMMED_NVC0(push, NVC0_3D(TFB_ENABLE), (tfb && nvc0->num_tfbbufs) ? 1 : 0);

   if (tfb && tfb != nvc0->state.tfb) {
      for (b = 0; b < 4; ++b) {
         if (tfb->varying_count[b]) {
            unsigned n = (tfb->varying_count[b] + 3) / 4;

            BEGIN_NVC0(push, NVC0_3D(TFB_STREAM(b)), 3);
            PUSH_DATA (push, 0);
            PUSH_DATA (push, tfb->varying_count[b]);
            PUSH_DATA (push, tfb->stride[b]);
            BEGIN_NVC0(push, NVC0_3D(TFB_VARYING_LOCS(b, 0)), n);
            PUSH_DATAp(push, tfb->varying_index[b], n);

            if (nvc0->tfbbuf[b])
               nvc0_so_target(nvc0->tfbbuf[b])->stride = tfb->stride[b];
         } else {
            IMMED_NVC0(push, NVC0_3D(TFB_VARYING_COUNT(b)), 0);
         }
      }
   }
   nvc0->state.tfb = tfb;

   if (!(nvc0->dirty & NVC0_NEW_TFB_TARGETS))
      return;
   nouveau_bufctx_reset(nvc0->bufctx_3d, NVC0_BIND_TFB);

   for (b = 0; b < nvc0->num_tfbbufs; ++b) {
      struct nvc0_so_target *targ = nvc0_so_target(nvc0->tfbbuf[b]);
      struct nv04_resource *buf = nv04_resource(targ->pipe.buffer);

      if (tfb)
         targ->stride = tfb->stride[b];

      if (!(nvc0->tfbbuf_dirty & (1 << b)))
         continue;

      if (!targ->clean)
         nvc0_query_fifo_wait(push, targ->pq);
      BEGIN_NVC0(push, NVC0_3D(TFB_BUFFER_ENABLE(b)), 5);
      PUSH_DATA (push, 1);
      PUSH_DATAh(push, buf->address + targ->pipe.buffer_offset);
      PUSH_DATA (push, buf->address + targ->pipe.buffer_offset);
      PUSH_DATA (push, targ->pipe.buffer_size);
      if (!targ->clean) {
         nvc0_query_pushbuf_submit(push, targ->pq, 0x4);
      } else {
         PUSH_DATA(push, 0); /* TFB_BUFFER_OFFSET */
         targ->clean = FALSE;
      }
      BCTX_REFN(nvc0->bufctx_3d, TFB, buf, WR);
   }
   for (; b < 4; ++b)
      IMMED_NVC0(push, NVC0_3D(TFB_BUFFER_ENABLE(b)), 0);
}
