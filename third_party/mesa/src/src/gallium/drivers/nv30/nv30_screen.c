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

#include "util/u_format_s3tc.h"

#include "nouveau/nv_object.xml.h"
#include "nouveau/nv_m2mf.xml.h"
#include "nv30-40_3d.xml.h"
#include "nv01_2d.xml.h"

#include "nouveau/nouveau_fence.h"
#include "nv30_screen.h"
#include "nv30_context.h"
#include "nv30_resource.h"
#include "nv30_format.h"

#define RANKINE_0397_CHIPSET 0x00000003
#define RANKINE_0497_CHIPSET 0x000001e0
#define RANKINE_0697_CHIPSET 0x00000010
#define CURIE_4097_CHIPSET   0x00000baf
#define CURIE_4497_CHIPSET   0x00005450
#define CURIE_4497_CHIPSET6X 0x00000088

static int
nv30_screen_get_param(struct pipe_screen *pscreen, enum pipe_cap param)
{
   struct nv30_screen *screen = nv30_screen(pscreen);
   struct nouveau_object *eng3d = screen->eng3d;

   switch (param) {
   /* non-boolean capabilities */
   case PIPE_CAP_MAX_RENDER_TARGETS:
      return (eng3d->oclass >= NV40_3D_CLASS) ? 4 : 1;
   case PIPE_CAP_MAX_TEXTURE_2D_LEVELS:
      return 13;
   case PIPE_CAP_MAX_TEXTURE_3D_LEVELS:
      return 10;
   case PIPE_CAP_MAX_TEXTURE_CUBE_LEVELS:
      return 13;
   case PIPE_CAP_MAX_COMBINED_SAMPLERS:
      return 16;
   case PIPE_CAP_GLSL_FEATURE_LEVEL:
      return 120;
   /* supported capabilities */
   case PIPE_CAP_TWO_SIDED_STENCIL:
   case PIPE_CAP_ANISOTROPIC_FILTER:
   case PIPE_CAP_POINT_SPRITE:
   case PIPE_CAP_SCALED_RESOLVE:
   case PIPE_CAP_OCCLUSION_QUERY:
   case PIPE_CAP_TIMER_QUERY:
   case PIPE_CAP_QUERY_TIMESTAMP:
   case PIPE_CAP_TEXTURE_SHADOW_MAP:
   case PIPE_CAP_TEXTURE_SWIZZLE:
   case PIPE_CAP_DEPTHSTENCIL_CLEAR_SEPARATE:
   case PIPE_CAP_DEPTH_CLIP_DISABLE:
   case PIPE_CAP_TGSI_FS_COORD_ORIGIN_UPPER_LEFT:
   case PIPE_CAP_TGSI_FS_COORD_ORIGIN_LOWER_LEFT:
   case PIPE_CAP_TGSI_FS_COORD_PIXEL_CENTER_HALF_INTEGER:
   case PIPE_CAP_TGSI_FS_COORD_PIXEL_CENTER_INTEGER:
   case PIPE_CAP_USER_CONSTANT_BUFFERS:
   case PIPE_CAP_USER_INDEX_BUFFERS:
      return 1;
   case PIPE_CAP_USER_VERTEX_BUFFERS:
      return 0;
   case PIPE_CAP_CONSTANT_BUFFER_OFFSET_ALIGNMENT:
      return 16;
   /* nv4x capabilities */
   case PIPE_CAP_BLEND_EQUATION_SEPARATE:
   case PIPE_CAP_NPOT_TEXTURES:
   case PIPE_CAP_CONDITIONAL_RENDER:
   case PIPE_CAP_TEXTURE_MIRROR_CLAMP:
   case PIPE_CAP_PRIMITIVE_RESTART:
      return (eng3d->oclass >= NV40_3D_CLASS) ? 1 : 0;
   /* unsupported */
   case PIPE_CAP_MAX_DUAL_SOURCE_RENDER_TARGETS:
   case PIPE_CAP_SM3:
   case PIPE_CAP_INDEP_BLEND_ENABLE:
   case PIPE_CAP_INDEP_BLEND_FUNC:
   case PIPE_CAP_MAX_TEXTURE_ARRAY_LAYERS:
   case PIPE_CAP_SHADER_STENCIL_EXPORT:
   case PIPE_CAP_TGSI_INSTANCEID:
   case PIPE_CAP_VERTEX_ELEMENT_INSTANCE_DIVISOR: /* XXX: yes? */
   case PIPE_CAP_MAX_STREAM_OUTPUT_BUFFERS:
   case PIPE_CAP_STREAM_OUTPUT_PAUSE_RESUME:
   case PIPE_CAP_MIN_TEXEL_OFFSET:
   case PIPE_CAP_MAX_TEXEL_OFFSET:
   case PIPE_CAP_MAX_STREAM_OUTPUT_SEPARATE_COMPONENTS:
   case PIPE_CAP_MAX_STREAM_OUTPUT_INTERLEAVED_COMPONENTS:
   case PIPE_CAP_TGSI_CAN_COMPACT_VARYINGS:
   case PIPE_CAP_TGSI_CAN_COMPACT_CONSTANTS:
   case PIPE_CAP_TEXTURE_BARRIER:
   case PIPE_CAP_SEAMLESS_CUBE_MAP:
   case PIPE_CAP_SEAMLESS_CUBE_MAP_PER_TEXTURE:
   case PIPE_CAP_VERTEX_COLOR_UNCLAMPED:
   case PIPE_CAP_QUADS_FOLLOW_PROVOKING_VERTEX_CONVENTION:
   case PIPE_CAP_MIXED_COLORBUFFER_FORMATS:
   case PIPE_CAP_START_INSTANCE:
      return 0;
   case PIPE_CAP_VERTEX_BUFFER_OFFSET_4BYTE_ALIGNED_ONLY:
   case PIPE_CAP_VERTEX_BUFFER_STRIDE_4BYTE_ALIGNED_ONLY:
   case PIPE_CAP_VERTEX_ELEMENT_SRC_OFFSET_4BYTE_ALIGNED_ONLY:
      return 1;
   default:
      debug_printf("unknown param %d\n", param);
      return 0;
   }
}

static float
nv30_screen_get_paramf(struct pipe_screen *pscreen, enum pipe_capf param)
{
   struct nv30_screen *screen = nv30_screen(pscreen);
   struct nouveau_object *eng3d = screen->eng3d;

   switch (param) {
   case PIPE_CAPF_MAX_LINE_WIDTH:
   case PIPE_CAPF_MAX_LINE_WIDTH_AA:
      return 10.0;
   case PIPE_CAPF_MAX_POINT_WIDTH:
   case PIPE_CAPF_MAX_POINT_WIDTH_AA:
      return 64.0;
   case PIPE_CAPF_MAX_TEXTURE_ANISOTROPY:
      return (eng3d->oclass >= NV40_3D_CLASS) ? 16.0 : 8.0;
   case PIPE_CAPF_MAX_TEXTURE_LOD_BIAS:
      return 15.0;
   default:
      debug_printf("unknown paramf %d\n", param);
      return 0;
   }
}

static int
nv30_screen_get_shader_param(struct pipe_screen *pscreen, unsigned shader,
                             enum pipe_shader_cap param)
{
   struct nv30_screen *screen = nv30_screen(pscreen);
   struct nouveau_object *eng3d = screen->eng3d;

   switch (shader) {
   case PIPE_SHADER_VERTEX:
      switch (param) {
      case PIPE_SHADER_CAP_MAX_INSTRUCTIONS:
      case PIPE_SHADER_CAP_MAX_ALU_INSTRUCTIONS:
         return (eng3d->oclass >= NV40_3D_CLASS) ? 512 : 256;
      case PIPE_SHADER_CAP_MAX_TEX_INSTRUCTIONS:
      case PIPE_SHADER_CAP_MAX_TEX_INDIRECTIONS:
         return (eng3d->oclass >= NV40_3D_CLASS) ? 512 : 0;
      case PIPE_SHADER_CAP_MAX_CONTROL_FLOW_DEPTH:
         return 0;
      case PIPE_SHADER_CAP_MAX_INPUTS:
         return 16;
      case PIPE_SHADER_CAP_MAX_CONSTS:
         return (eng3d->oclass >= NV40_3D_CLASS) ? (468 - 6): (256 - 6);
      case PIPE_SHADER_CAP_MAX_CONST_BUFFERS:
         return 1;
      case PIPE_SHADER_CAP_MAX_TEMPS:
         return (eng3d->oclass >= NV40_3D_CLASS) ? 32 : 13;
      case PIPE_SHADER_CAP_MAX_TEXTURE_SAMPLERS:
         return 0;
      case PIPE_SHADER_CAP_MAX_ADDRS:
         return 2;
      case PIPE_SHADER_CAP_MAX_PREDS:
      case PIPE_SHADER_CAP_TGSI_CONT_SUPPORTED:
      case PIPE_SHADER_CAP_INDIRECT_INPUT_ADDR:
      case PIPE_SHADER_CAP_INDIRECT_OUTPUT_ADDR:
      case PIPE_SHADER_CAP_INDIRECT_TEMP_ADDR:
      case PIPE_SHADER_CAP_INDIRECT_CONST_ADDR:
      case PIPE_SHADER_CAP_SUBROUTINES:
      case PIPE_SHADER_CAP_INTEGERS:
         return 0;
      default:
         debug_printf("unknown vertex shader param %d\n", param);
         return 0;
      }
      break;
   case PIPE_SHADER_FRAGMENT:
      switch (param) {
      case PIPE_SHADER_CAP_MAX_INSTRUCTIONS:
      case PIPE_SHADER_CAP_MAX_ALU_INSTRUCTIONS:
      case PIPE_SHADER_CAP_MAX_TEX_INSTRUCTIONS:
      case PIPE_SHADER_CAP_MAX_TEX_INDIRECTIONS:
         return 4096;
      case PIPE_SHADER_CAP_MAX_CONTROL_FLOW_DEPTH:
         return 0;
      case PIPE_SHADER_CAP_MAX_INPUTS:
         return (eng3d->oclass >= NV40_3D_CLASS) ? 12 : 10;
      case PIPE_SHADER_CAP_MAX_CONSTS:
         return (eng3d->oclass >= NV40_3D_CLASS) ? 224 : 32;
      case PIPE_SHADER_CAP_MAX_CONST_BUFFERS:
         return 1;
      case PIPE_SHADER_CAP_MAX_TEMPS:
         return 32;
      case PIPE_SHADER_CAP_MAX_ADDRS:
         return (eng3d->oclass >= NV40_3D_CLASS) ? 1 : 0;
      case PIPE_SHADER_CAP_MAX_TEXTURE_SAMPLERS:
         return 16;
      case PIPE_SHADER_CAP_MAX_PREDS:
      case PIPE_SHADER_CAP_TGSI_CONT_SUPPORTED:
      case PIPE_SHADER_CAP_INDIRECT_INPUT_ADDR:
      case PIPE_SHADER_CAP_INDIRECT_OUTPUT_ADDR:
      case PIPE_SHADER_CAP_INDIRECT_TEMP_ADDR:
      case PIPE_SHADER_CAP_INDIRECT_CONST_ADDR:
      case PIPE_SHADER_CAP_SUBROUTINES:
         return 0;
      default:
         debug_printf("unknown fragment shader param %d\n", param);
         return 0;
      }
      break;
   default:
      return 0;
   }
}

static boolean
nv30_screen_is_format_supported(struct pipe_screen *pscreen,
                                enum pipe_format format,
                                enum pipe_texture_target target,
                                unsigned sample_count,
                                unsigned bindings)
{
   if (sample_count > 4)
      return FALSE;
   if (!(0x00000017 & (1 << sample_count)))
      return FALSE;

   if (!util_format_s3tc_enabled) {
      switch (format) {
      case PIPE_FORMAT_DXT1_RGB:
      case PIPE_FORMAT_DXT1_RGBA:
      case PIPE_FORMAT_DXT3_RGBA:
      case PIPE_FORMAT_DXT5_RGBA:
         return FALSE;
      default:
         break;
      }
   }

   /* transfers & shared are always supported */
   bindings &= ~(PIPE_BIND_TRANSFER_READ |
                 PIPE_BIND_TRANSFER_WRITE |
                 PIPE_BIND_SHARED);

   return (nv30_format_info(pscreen, format)->bindings & bindings) == bindings;
}

static void
nv30_screen_fence_emit(struct pipe_screen *pscreen, uint32_t *sequence)
{
   struct nv30_screen *screen = nv30_screen(pscreen);
   struct nouveau_pushbuf *push = screen->base.pushbuf;

   *sequence = ++screen->base.fence.sequence;

   BEGIN_NV04(push, NV30_3D(FENCE_OFFSET), 2);
   PUSH_DATA (push, 0);
   PUSH_DATA (push, *sequence);
}

static uint32_t
nv30_screen_fence_update(struct pipe_screen *pscreen)
{
   struct nv30_screen *screen = nv30_screen(pscreen);
   struct nv04_notify *fence = screen->fence->data;
   return *(uint32_t *)((char *)screen->notify->map + fence->offset);
}

static void
nv30_screen_destroy(struct pipe_screen *pscreen)
{
   struct nv30_screen *screen = nv30_screen(pscreen);

   if (screen->base.fence.current &&
       screen->base.fence.current->state >= NOUVEAU_FENCE_STATE_EMITTED) {
      nouveau_fence_wait(screen->base.fence.current);
      nouveau_fence_ref (NULL, &screen->base.fence.current);
   }

   nouveau_object_del(&screen->query);
   nouveau_object_del(&screen->fence);
   nouveau_object_del(&screen->ntfy);

   nouveau_object_del(&screen->sifm);
   nouveau_object_del(&screen->swzsurf);
   nouveau_object_del(&screen->surf2d);
   nouveau_object_del(&screen->m2mf);
   nouveau_object_del(&screen->eng3d);
   nouveau_object_del(&screen->null);

   nouveau_screen_fini(&screen->base);
   FREE(screen);
}

#define FAIL_SCREEN_INIT(str, err)                    \
   do {                                               \
      NOUVEAU_ERR(str, err);                          \
      nv30_screen_destroy(pscreen);                   \
      return NULL;                                    \
   } while(0)

struct pipe_screen *
nv30_screen_create(struct nouveau_device *dev)
{
   struct nv30_screen *screen = CALLOC_STRUCT(nv30_screen);
   struct pipe_screen *pscreen;
   struct nouveau_pushbuf *push;
   struct nv04_fifo *fifo;
   unsigned oclass = 0;
   int ret, i;

   if (!screen)
      return NULL;

   switch (dev->chipset & 0xf0) {
   case 0x30:
      if (RANKINE_0397_CHIPSET & (1 << (dev->chipset & 0x0f)))
         oclass = NV30_3D_CLASS;
      else
      if (RANKINE_0697_CHIPSET & (1 << (dev->chipset & 0x0f)))
         oclass = NV34_3D_CLASS;
      else
      if (RANKINE_0497_CHIPSET & (1 << (dev->chipset & 0x0f)))
         oclass = NV35_3D_CLASS;
      break;
   case 0x40:
      if (CURIE_4097_CHIPSET & (1 << (dev->chipset & 0x0f)))
         oclass = NV40_3D_CLASS;
      else
      if (CURIE_4497_CHIPSET & (1 << (dev->chipset & 0x0f)))
         oclass = NV44_3D_CLASS;
      break;
   case 0x60:
      if (CURIE_4497_CHIPSET6X & (1 << (dev->chipset & 0x0f)))
         oclass = NV44_3D_CLASS;
      break;
   default:
      break;
   }

   if (!oclass) {
      NOUVEAU_ERR("unknown 3d class for 0x%02x\n", dev->chipset);
      return NULL;
   }

   pscreen = &screen->base.base;
   pscreen->destroy = nv30_screen_destroy;
   pscreen->get_param = nv30_screen_get_param;
   pscreen->get_paramf = nv30_screen_get_paramf;
   pscreen->get_shader_param = nv30_screen_get_shader_param;
   pscreen->context_create = nv30_context_create;
   pscreen->is_format_supported = nv30_screen_is_format_supported;
   nv30_resource_screen_init(pscreen);

   screen->base.fence.emit = nv30_screen_fence_emit;
   screen->base.fence.update = nv30_screen_fence_update;

   ret = nouveau_screen_init(&screen->base, dev);
   if (ret)
      FAIL_SCREEN_INIT("nv30_screen_init failed: %d\n", ret);

   screen->base.vidmem_bindings |= PIPE_BIND_VERTEX_BUFFER;
   screen->base.sysmem_bindings |= PIPE_BIND_VERTEX_BUFFER;
   if (oclass == NV40_3D_CLASS) {
      screen->base.vidmem_bindings |= PIPE_BIND_INDEX_BUFFER;
      screen->base.sysmem_bindings |= PIPE_BIND_INDEX_BUFFER;
   }

   fifo = screen->base.channel->data;
   push = screen->base.pushbuf;
   push->rsvd_kick = 16;

   ret = nouveau_object_new(screen->base.channel, 0x00000000, NV01_NULL_CLASS,
                            NULL, 0, &screen->null);
   if (ret)
      FAIL_SCREEN_INIT("error allocating null object: %d\n", ret);

   /* DMA_FENCE refuses to accept DMA objects with "adjust" filled in,
    * this means that the address pointed at by the DMA object must
    * be 4KiB aligned, which means this object needs to be the first
    * one allocated on the channel.
    */
   ret = nouveau_object_new(screen->base.channel, 0xbeef1e00,
                            NOUVEAU_NOTIFIER_CLASS, &(struct nv04_notify) {
                            .length = 32 }, sizeof(struct nv04_notify),
                            &screen->fence);
   if (ret)
      FAIL_SCREEN_INIT("error allocating fence notifier: %d\n", ret);

   /* DMA_NOTIFY object, we don't actually use this but M2MF fails without */
   ret = nouveau_object_new(screen->base.channel, 0xbeef0301,
                            NOUVEAU_NOTIFIER_CLASS, &(struct nv04_notify) {
                            .length = 32 }, sizeof(struct nv04_notify),
                            &screen->ntfy);
   if (ret)
      FAIL_SCREEN_INIT("error allocating sync notifier: %d\n", ret);

   /* DMA_QUERY, used to implement occlusion queries, we attempt to allocate
    * the remainder of the "notifier block" assigned by the kernel for
    * use as query objects
    */
   ret = nouveau_object_new(screen->base.channel, 0xbeef0351,
                            NOUVEAU_NOTIFIER_CLASS, &(struct nv04_notify) {
                            .length = 4096 - 128 }, sizeof(struct nv04_notify),
                            &screen->query);
   if (ret)
      FAIL_SCREEN_INIT("error allocating query notifier: %d\n", ret);

   ret = nouveau_heap_init(&screen->query_heap, 0, 4096 - 128);
   if (ret)
      FAIL_SCREEN_INIT("error creating query heap: %d\n", ret);

   LIST_INITHEAD(&screen->queries);

   /* Vertex program resources (code/data), currently 6 of the constant
    * slots are reserved to implement user clipping planes
    */
   if (oclass < NV40_3D_CLASS) {
      nouveau_heap_init(&screen->vp_exec_heap, 0, 256);
      nouveau_heap_init(&screen->vp_data_heap, 6, 256 - 6);
   } else {
      nouveau_heap_init(&screen->vp_exec_heap, 0, 512);
      nouveau_heap_init(&screen->vp_data_heap, 6, 468 - 6);
   }

   ret = nouveau_bo_wrap(screen->base.device, fifo->notify, &screen->notify);
   if (ret == 0)
      nouveau_bo_map(screen->notify, 0, screen->base.client);
   if (ret)
      FAIL_SCREEN_INIT("error mapping notifier memory: %d\n", ret);

   ret = nouveau_object_new(screen->base.channel, 0xbeef3097, oclass,
                            NULL, 0, &screen->eng3d);
   if (ret)
      FAIL_SCREEN_INIT("error allocating 3d object: %d\n", ret);

   BEGIN_NV04(push, NV01_SUBC(3D, OBJECT), 1);
   PUSH_DATA (push, screen->eng3d->handle);
   BEGIN_NV04(push, NV30_3D(DMA_NOTIFY), 13);
   PUSH_DATA (push, screen->ntfy->handle);
   PUSH_DATA (push, fifo->vram);     /* TEXTURE0 */
   PUSH_DATA (push, fifo->gart);     /* TEXTURE1 */
   PUSH_DATA (push, fifo->vram);     /* COLOR1 */
   PUSH_DATA (push, screen->null->handle);  /* UNK190 */
   PUSH_DATA (push, fifo->vram);     /* COLOR0 */
   PUSH_DATA (push, fifo->vram);     /* ZETA */
   PUSH_DATA (push, fifo->vram);     /* VTXBUF0 */
   PUSH_DATA (push, fifo->gart);     /* VTXBUF1 */
   PUSH_DATA (push, screen->fence->handle);  /* FENCE */
   PUSH_DATA (push, screen->query->handle);  /* QUERY - intr 0x80 if nullobj */
   PUSH_DATA (push, screen->null->handle);  /* UNK1AC */
   PUSH_DATA (push, screen->null->handle);  /* UNK1B0 */
   if (screen->eng3d->oclass < NV40_3D_CLASS) {
      BEGIN_NV04(push, SUBC_3D(0x03b0), 1);
      PUSH_DATA (push, 0x00100000);
      BEGIN_NV04(push, SUBC_3D(0x1d80), 1);
      PUSH_DATA (push, 3);

      BEGIN_NV04(push, SUBC_3D(0x1e98), 1);
      PUSH_DATA (push, 0);
      BEGIN_NV04(push, SUBC_3D(0x17e0), 3);
      PUSH_DATA (push, fui(0.0));
      PUSH_DATA (push, fui(0.0));
      PUSH_DATA (push, fui(1.0));
      BEGIN_NV04(push, SUBC_3D(0x1f80), 16);
      for (i = 0; i < 16; i++)
         PUSH_DATA (push, (i == 8) ? 0x0000ffff : 0);

      BEGIN_NV04(push, NV30_3D(RC_ENABLE), 1);
      PUSH_DATA (push, 0);
   } else {
      BEGIN_NV04(push, NV40_3D(DMA_COLOR2), 2);
      PUSH_DATA (push, fifo->vram);
      PUSH_DATA (push, fifo->vram);  /* COLOR3 */

      BEGIN_NV04(push, SUBC_3D(0x1450), 1);
      PUSH_DATA (push, 0x00000004);

      BEGIN_NV04(push, SUBC_3D(0x1ea4), 3); /* ZCULL */
      PUSH_DATA (push, 0x00000010);
      PUSH_DATA (push, 0x01000100);
      PUSH_DATA (push, 0xff800006);

      /* vtxprog output routing */
      BEGIN_NV04(push, SUBC_3D(0x1fc4), 1);
      PUSH_DATA (push, 0x06144321);
      BEGIN_NV04(push, SUBC_3D(0x1fc8), 2);
      PUSH_DATA (push, 0xedcba987);
      PUSH_DATA (push, 0x0000006f);
      BEGIN_NV04(push, SUBC_3D(0x1fd0), 1);
      PUSH_DATA (push, 0x00171615);
      BEGIN_NV04(push, SUBC_3D(0x1fd4), 1);
      PUSH_DATA (push, 0x001b1a19);

      BEGIN_NV04(push, SUBC_3D(0x1ef8), 1);
      PUSH_DATA (push, 0x0020ffff);
      BEGIN_NV04(push, SUBC_3D(0x1d64), 1);
      PUSH_DATA (push, 0x01d300d4);

      BEGIN_NV04(push, NV40_3D(MIPMAP_ROUNDING), 1);
      PUSH_DATA (push, NV40_3D_MIPMAP_ROUNDING_MODE_DOWN);
   }

   ret = nouveau_object_new(screen->base.channel, 0xbeef3901, NV03_M2MF_CLASS,
                            NULL, 0, &screen->m2mf);
   if (ret)
      FAIL_SCREEN_INIT("error allocating m2mf object: %d\n", ret);

   BEGIN_NV04(push, NV01_SUBC(M2MF, OBJECT), 1);
   PUSH_DATA (push, screen->m2mf->handle);
   BEGIN_NV04(push, NV03_M2MF(DMA_NOTIFY), 1);
   PUSH_DATA (push, screen->ntfy->handle);

   ret = nouveau_object_new(screen->base.channel, 0xbeef6201,
                            NV10_SURFACE_2D_CLASS, NULL, 0, &screen->surf2d);
   if (ret)
      FAIL_SCREEN_INIT("error allocating surf2d object: %d\n", ret);

   BEGIN_NV04(push, NV01_SUBC(SF2D, OBJECT), 1);
   PUSH_DATA (push, screen->surf2d->handle);
   BEGIN_NV04(push, NV04_SF2D(DMA_NOTIFY), 1);
   PUSH_DATA (push, screen->ntfy->handle);

   if (dev->chipset < 0x40)
      oclass = NV30_SURFACE_SWZ_CLASS;
   else
      oclass = NV40_SURFACE_SWZ_CLASS;

   ret = nouveau_object_new(screen->base.channel, 0xbeef5201, oclass,
                            NULL, 0, &screen->swzsurf);
   if (ret)
      FAIL_SCREEN_INIT("error allocating swizzled surface object: %d\n", ret);

   BEGIN_NV04(push, NV01_SUBC(SSWZ, OBJECT), 1);
   PUSH_DATA (push, screen->swzsurf->handle);
   BEGIN_NV04(push, NV04_SSWZ(DMA_NOTIFY), 1);
   PUSH_DATA (push, screen->ntfy->handle);

   if (dev->chipset < 0x40)
      oclass = NV30_SIFM_CLASS;
   else
      oclass = NV40_SIFM_CLASS;

   ret = nouveau_object_new(screen->base.channel, 0xbeef7701, oclass,
                            NULL, 0, &screen->sifm);
   if (ret)
      FAIL_SCREEN_INIT("error allocating scaled image object: %d\n", ret);

   BEGIN_NV04(push, NV01_SUBC(SIFM, OBJECT), 1);
   PUSH_DATA (push, screen->sifm->handle);
   BEGIN_NV04(push, NV03_SIFM(DMA_NOTIFY), 1);
   PUSH_DATA (push, screen->ntfy->handle);
   BEGIN_NV04(push, NV05_SIFM(COLOR_CONVERSION), 1);
   PUSH_DATA (push, NV05_SIFM_COLOR_CONVERSION_TRUNCATE);

   nouveau_pushbuf_kick(push, push->channel);

   nouveau_fence_new(&screen->base, &screen->base.fence.current, FALSE);
   return pscreen;
}
