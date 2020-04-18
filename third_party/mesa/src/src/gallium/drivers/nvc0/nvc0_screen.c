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

#include "util/u_format.h"
#include "util/u_format_s3tc.h"
#include "pipe/p_screen.h"

#include "vl/vl_decoder.h"
#include "vl/vl_video_buffer.h"

#include "nvc0_context.h"
#include "nvc0_screen.h"

#include "nvc0_graph_macros.h"

static boolean
nvc0_screen_is_format_supported(struct pipe_screen *pscreen,
                                enum pipe_format format,
                                enum pipe_texture_target target,
                                unsigned sample_count,
                                unsigned bindings)
{
   if (sample_count > 8)
      return FALSE;
   if (!(0x117 & (1 << sample_count))) /* 0, 1, 2, 4 or 8 */
      return FALSE;

   if (!util_format_is_supported(format, bindings))
      return FALSE;

   switch (format) {
   case PIPE_FORMAT_R8G8B8A8_UNORM:
   case PIPE_FORMAT_R8G8B8X8_UNORM:
      /* HACK: GL requires equal formats for MS resolve and window is BGRA */
      if (bindings & PIPE_BIND_RENDER_TARGET)
         return FALSE;
   default:
      break;
   }

   /* transfers & shared are always supported */
   bindings &= ~(PIPE_BIND_TRANSFER_READ |
                 PIPE_BIND_TRANSFER_WRITE |
                 PIPE_BIND_SHARED);

   return (nvc0_format_table[format].usage & bindings) == bindings;
}

static int
nvc0_screen_get_param(struct pipe_screen *pscreen, enum pipe_cap param)
{
   const uint16_t class_3d = nouveau_screen(pscreen)->class_3d;

   switch (param) {
   case PIPE_CAP_MAX_COMBINED_SAMPLERS:
      return 16 * PIPE_SHADER_TYPES; /* NOTE: should not count COMPUTE */
   case PIPE_CAP_MAX_TEXTURE_2D_LEVELS:
   case PIPE_CAP_MAX_TEXTURE_CUBE_LEVELS:
      return 15;
   case PIPE_CAP_MAX_TEXTURE_3D_LEVELS:
      return 12;
   case PIPE_CAP_MAX_TEXTURE_ARRAY_LAYERS:
      return 2048;
   case PIPE_CAP_MIN_TEXEL_OFFSET:
      return -8;
   case PIPE_CAP_MAX_TEXEL_OFFSET:
      return 7;
   case PIPE_CAP_TEXTURE_MIRROR_CLAMP:
   case PIPE_CAP_TEXTURE_SWIZZLE:
   case PIPE_CAP_TEXTURE_SHADOW_MAP:
   case PIPE_CAP_NPOT_TEXTURES:
   case PIPE_CAP_ANISOTROPIC_FILTER:
   case PIPE_CAP_SEAMLESS_CUBE_MAP:
      return 1;
   case PIPE_CAP_SEAMLESS_CUBE_MAP_PER_TEXTURE:
      return (class_3d >= NVE4_3D_CLASS) ? 1 : 0;
   case PIPE_CAP_TWO_SIDED_STENCIL:
   case PIPE_CAP_DEPTH_CLIP_DISABLE:
   case PIPE_CAP_DEPTHSTENCIL_CLEAR_SEPARATE:
   case PIPE_CAP_POINT_SPRITE:
      return 1;
   case PIPE_CAP_SM3:
      return 1;
   case PIPE_CAP_GLSL_FEATURE_LEVEL:
      return 150;
   case PIPE_CAP_MAX_RENDER_TARGETS:
      return 8;
   case PIPE_CAP_MAX_DUAL_SOURCE_RENDER_TARGETS:
      return 1;
   case PIPE_CAP_FRAGMENT_COLOR_CLAMPED:
   case PIPE_CAP_VERTEX_COLOR_UNCLAMPED:
   case PIPE_CAP_VERTEX_COLOR_CLAMPED:
      return 1;
   case PIPE_CAP_QUERY_TIMESTAMP:
   case PIPE_CAP_TIMER_QUERY:
   case PIPE_CAP_OCCLUSION_QUERY:
   case PIPE_CAP_STREAM_OUTPUT_PAUSE_RESUME:
      return 1;
   case PIPE_CAP_MAX_STREAM_OUTPUT_BUFFERS:
      return 4;
   case PIPE_CAP_MAX_STREAM_OUTPUT_SEPARATE_COMPONENTS:
   case PIPE_CAP_MAX_STREAM_OUTPUT_INTERLEAVED_COMPONENTS:
      return 128;
   case PIPE_CAP_BLEND_EQUATION_SEPARATE:
   case PIPE_CAP_INDEP_BLEND_ENABLE:
   case PIPE_CAP_INDEP_BLEND_FUNC:
      return 1;
   case PIPE_CAP_TGSI_FS_COORD_ORIGIN_UPPER_LEFT:
   case PIPE_CAP_TGSI_FS_COORD_PIXEL_CENTER_HALF_INTEGER:
      return 1;
   case PIPE_CAP_TGSI_FS_COORD_ORIGIN_LOWER_LEFT:
   case PIPE_CAP_TGSI_FS_COORD_PIXEL_CENTER_INTEGER:
      return 0;
   case PIPE_CAP_SHADER_STENCIL_EXPORT:
      return 0;
   case PIPE_CAP_PRIMITIVE_RESTART:
   case PIPE_CAP_TGSI_INSTANCEID:
   case PIPE_CAP_VERTEX_ELEMENT_INSTANCE_DIVISOR:
   case PIPE_CAP_MIXED_COLORBUFFER_FORMATS:
   case PIPE_CAP_CONDITIONAL_RENDER:
   case PIPE_CAP_TEXTURE_BARRIER:
   case PIPE_CAP_QUADS_FOLLOW_PROVOKING_VERTEX_CONVENTION:
   case PIPE_CAP_START_INSTANCE:
      return 1;
   case PIPE_CAP_TGSI_CAN_COMPACT_VARYINGS:
   case PIPE_CAP_TGSI_CAN_COMPACT_CONSTANTS:
      return 0; /* state trackers will know better */
   case PIPE_CAP_USER_CONSTANT_BUFFERS:
   case PIPE_CAP_USER_INDEX_BUFFERS:
   case PIPE_CAP_USER_VERTEX_BUFFERS:
      return 1;
   case PIPE_CAP_CONSTANT_BUFFER_OFFSET_ALIGNMENT:
      return 256;
   case PIPE_CAP_VERTEX_BUFFER_OFFSET_4BYTE_ALIGNED_ONLY:
   case PIPE_CAP_VERTEX_BUFFER_STRIDE_4BYTE_ALIGNED_ONLY:
   case PIPE_CAP_VERTEX_ELEMENT_SRC_OFFSET_4BYTE_ALIGNED_ONLY:
      return 0;
   default:
      NOUVEAU_ERR("unknown PIPE_CAP %d\n", param);
      return 0;
   }
}

static int
nvc0_screen_get_shader_param(struct pipe_screen *pscreen, unsigned shader,
                             enum pipe_shader_cap param)
{
   switch (shader) {
   case PIPE_SHADER_VERTEX:
      /*
   case PIPE_SHADER_TESSELLATION_CONTROL:
   case PIPE_SHADER_TESSELLATION_EVALUATION:
      */
   case PIPE_SHADER_GEOMETRY:
   case PIPE_SHADER_FRAGMENT:
      break;
   default:
      return 0;
   }
   
   switch (param) {
   case PIPE_SHADER_CAP_MAX_INSTRUCTIONS:
   case PIPE_SHADER_CAP_MAX_ALU_INSTRUCTIONS:
   case PIPE_SHADER_CAP_MAX_TEX_INSTRUCTIONS:
   case PIPE_SHADER_CAP_MAX_TEX_INDIRECTIONS:
      return 16384;
   case PIPE_SHADER_CAP_MAX_CONTROL_FLOW_DEPTH:
      return 16;
   case PIPE_SHADER_CAP_MAX_INPUTS:
      if (shader == PIPE_SHADER_VERTEX)
         return 32;
      if (shader == PIPE_SHADER_FRAGMENT)
         return (0x200 + 0x20 + 0x80) / 16; /* generic + colors + TexCoords */
      return (0x200 + 0x40 + 0x80) / 16; /* without 0x60 for per-patch inputs */
   case PIPE_SHADER_CAP_MAX_CONSTS:
      return 65536 / 16;
   case PIPE_SHADER_CAP_MAX_CONST_BUFFERS:
      return NVC0_MAX_PIPE_CONSTBUFS;
   case PIPE_SHADER_CAP_MAX_ADDRS:
      return 1;
   case PIPE_SHADER_CAP_INDIRECT_INPUT_ADDR:
   case PIPE_SHADER_CAP_INDIRECT_OUTPUT_ADDR:
      return shader != PIPE_SHADER_FRAGMENT;
   case PIPE_SHADER_CAP_INDIRECT_TEMP_ADDR:
   case PIPE_SHADER_CAP_INDIRECT_CONST_ADDR:
      return 1;
   case PIPE_SHADER_CAP_MAX_PREDS:
      return 0;
   case PIPE_SHADER_CAP_MAX_TEMPS:
      return NVC0_CAP_MAX_PROGRAM_TEMPS;
   case PIPE_SHADER_CAP_TGSI_CONT_SUPPORTED:
      return 1;
   case PIPE_SHADER_CAP_SUBROUTINES:
      return 1; /* but inlining everything, we need function declarations */
   case PIPE_SHADER_CAP_INTEGERS:
      return 1;
   case PIPE_SHADER_CAP_MAX_TEXTURE_SAMPLERS:
      return 16; /* would be 32 in linked (OpenGL-style) mode */
      /*
   case PIPE_SHADER_CAP_MAX_TEXTURE_SAMPLER_VIEWS:
      return 32;
      */
   default:
      NOUVEAU_ERR("unknown PIPE_SHADER_CAP %d\n", param);
      return 0;
   }
}

static float
nvc0_screen_get_paramf(struct pipe_screen *pscreen, enum pipe_capf param)
{
   switch (param) {
   case PIPE_CAPF_MAX_LINE_WIDTH:
   case PIPE_CAPF_MAX_LINE_WIDTH_AA:
      return 10.0f;
   case PIPE_CAPF_MAX_POINT_WIDTH:
      return 63.0f;
   case PIPE_CAPF_MAX_POINT_WIDTH_AA:
      return 63.375f;
   case PIPE_CAPF_MAX_TEXTURE_ANISOTROPY:
      return 16.0f;
   case PIPE_CAPF_MAX_TEXTURE_LOD_BIAS:
      return 15.0f;
   default:
      NOUVEAU_ERR("unknown PIPE_CAP %d\n", param);
      return 0.0f;
   }
}

static void
nvc0_screen_destroy(struct pipe_screen *pscreen)
{
   struct nvc0_screen *screen = nvc0_screen(pscreen);

   if (screen->base.fence.current) {
      nouveau_fence_wait(screen->base.fence.current);
      nouveau_fence_ref(NULL, &screen->base.fence.current);
   }
   if (screen->base.pushbuf)
      screen->base.pushbuf->user_priv = NULL;

   if (screen->blitctx)
      FREE(screen->blitctx);

   nouveau_bo_ref(NULL, &screen->text);
   nouveau_bo_ref(NULL, &screen->uniform_bo);
   nouveau_bo_ref(NULL, &screen->tls);
   nouveau_bo_ref(NULL, &screen->txc);
   nouveau_bo_ref(NULL, &screen->fence.bo);
   nouveau_bo_ref(NULL, &screen->poly_cache);

   nouveau_heap_destroy(&screen->lib_code);
   nouveau_heap_destroy(&screen->text_heap);

   if (screen->tic.entries)
      FREE(screen->tic.entries);

   nouveau_mm_destroy(screen->mm_VRAM_fe0);

   nouveau_object_del(&screen->eng3d);
   nouveau_object_del(&screen->eng2d);
   nouveau_object_del(&screen->m2mf);

   nouveau_screen_fini(&screen->base);

   FREE(screen);
}

static int
nvc0_graph_set_macro(struct nvc0_screen *screen, uint32_t m, unsigned pos,
                     unsigned size, const uint32_t *data)
{
   struct nouveau_pushbuf *push = screen->base.pushbuf;

   size /= 4;

   BEGIN_NVC0(push, SUBC_3D(NVC0_GRAPH_MACRO_ID), 2);
   PUSH_DATA (push, (m - 0x3800) / 8);
   PUSH_DATA (push, pos);
   BEGIN_1IC0(push, SUBC_3D(NVC0_GRAPH_MACRO_UPLOAD_POS), size + 1);
   PUSH_DATA (push, pos);
   PUSH_DATAp(push, data, size);

   return pos + size;
}

static void
nvc0_magic_3d_init(struct nouveau_pushbuf *push, uint16_t obj_class)
{
   BEGIN_NVC0(push, SUBC_3D(0x10cc), 1);
   PUSH_DATA (push, 0xff);
   BEGIN_NVC0(push, SUBC_3D(0x10e0), 2);
   PUSH_DATA (push, 0xff);
   PUSH_DATA (push, 0xff);
   BEGIN_NVC0(push, SUBC_3D(0x10ec), 2);
   PUSH_DATA (push, 0xff);
   PUSH_DATA (push, 0xff);
   BEGIN_NVC0(push, SUBC_3D(0x074c), 1);
   PUSH_DATA (push, 0x3f);

   BEGIN_NVC0(push, SUBC_3D(0x16a8), 1);
   PUSH_DATA (push, (3 << 16) | 3);
   BEGIN_NVC0(push, SUBC_3D(0x1794), 1);
   PUSH_DATA (push, (2 << 16) | 2);
   BEGIN_NVC0(push, SUBC_3D(0x0de8), 1);
   PUSH_DATA (push, 1);

   BEGIN_NVC0(push, SUBC_3D(0x12ac), 1);
   PUSH_DATA (push, 0);
   BEGIN_NVC0(push, SUBC_3D(0x0218), 1);
   PUSH_DATA (push, 0x10);
   BEGIN_NVC0(push, SUBC_3D(0x10fc), 1);
   PUSH_DATA (push, 0x10);
   BEGIN_NVC0(push, SUBC_3D(0x1290), 1);
   PUSH_DATA (push, 0x10);
   BEGIN_NVC0(push, SUBC_3D(0x12d8), 2);
   PUSH_DATA (push, 0x10);
   PUSH_DATA (push, 0x10);
   BEGIN_NVC0(push, SUBC_3D(0x1140), 1);
   PUSH_DATA (push, 0x10);
   BEGIN_NVC0(push, SUBC_3D(0x1610), 1);
   PUSH_DATA (push, 0xe);

   BEGIN_NVC0(push, SUBC_3D(0x164c), 1);
   PUSH_DATA (push, 1 << 12);
   BEGIN_NVC0(push, SUBC_3D(0x030c), 1);
   PUSH_DATA (push, 0);
   BEGIN_NVC0(push, SUBC_3D(0x0300), 1);
   PUSH_DATA (push, 3);

   BEGIN_NVC0(push, SUBC_3D(0x02d0), 1);
   PUSH_DATA (push, 0x3fffff);
   BEGIN_NVC0(push, SUBC_3D(0x0fdc), 1);
   PUSH_DATA (push, 1);
   BEGIN_NVC0(push, SUBC_3D(0x19c0), 1);
   PUSH_DATA (push, 1);
   BEGIN_NVC0(push, SUBC_3D(0x075c), 1);
   PUSH_DATA (push, 3);

   if (obj_class >= NVE4_3D_CLASS) {
      BEGIN_NVC0(push, SUBC_3D(0x07fc), 1);
      PUSH_DATA (push, 1);
   }

   /* TODO: find out what software methods 0x1528, 0x1280 and (on nve4) 0x02dc
    * are supposed to do */
}

static void
nvc0_screen_fence_emit(struct pipe_screen *pscreen, u32 *sequence)
{
   struct nvc0_screen *screen = nvc0_screen(pscreen);
   struct nouveau_pushbuf *push = screen->base.pushbuf;

   /* we need to do it after possible flush in MARK_RING */
   *sequence = ++screen->base.fence.sequence;

   BEGIN_NVC0(push, NVC0_3D(QUERY_ADDRESS_HIGH), 4);
   PUSH_DATAh(push, screen->fence.bo->offset);
   PUSH_DATA (push, screen->fence.bo->offset);
   PUSH_DATA (push, *sequence);
   PUSH_DATA (push, NVC0_3D_QUERY_GET_FENCE | NVC0_3D_QUERY_GET_SHORT |
              (0xf << NVC0_3D_QUERY_GET_UNIT__SHIFT));
}

static u32
nvc0_screen_fence_update(struct pipe_screen *pscreen)
{
   struct nvc0_screen *screen = nvc0_screen(pscreen);
   return screen->fence.map[0];
}

#define FAIL_SCREEN_INIT(str, err)                    \
   do {                                               \
      NOUVEAU_ERR(str, err);                          \
      nvc0_screen_destroy(pscreen);                   \
      return NULL;                                    \
   } while(0)

struct pipe_screen *
nvc0_screen_create(struct nouveau_device *dev)
{
   struct nvc0_screen *screen;
   struct pipe_screen *pscreen;
   struct nouveau_object *chan;
   struct nouveau_pushbuf *push;
   uint32_t obj_class;
   int ret;
   unsigned i;
   union nouveau_bo_config mm_config;

   switch (dev->chipset & ~0xf) {
   case 0xc0:
   case 0xd0:
   case 0xe0:
      break;
   default:
      return NULL;
   }

   screen = CALLOC_STRUCT(nvc0_screen);
   if (!screen)
      return NULL;
   pscreen = &screen->base.base;

   ret = nouveau_screen_init(&screen->base, dev);
   if (ret) {
      nvc0_screen_destroy(pscreen);
      return NULL;
   }
   chan = screen->base.channel;
   push = screen->base.pushbuf;
   push->user_priv = screen;

   screen->base.vidmem_bindings |= PIPE_BIND_CONSTANT_BUFFER |
      PIPE_BIND_VERTEX_BUFFER | PIPE_BIND_INDEX_BUFFER;
   screen->base.sysmem_bindings |=
      PIPE_BIND_VERTEX_BUFFER | PIPE_BIND_INDEX_BUFFER;

   pscreen->destroy = nvc0_screen_destroy;
   pscreen->context_create = nvc0_create;
   pscreen->is_format_supported = nvc0_screen_is_format_supported;
   pscreen->get_param = nvc0_screen_get_param;
   pscreen->get_shader_param = nvc0_screen_get_shader_param;
   pscreen->get_paramf = nvc0_screen_get_paramf;

   nvc0_screen_init_resource_functions(pscreen);

   nouveau_screen_init_vdec(&screen->base);

   ret = nouveau_bo_new(dev, NOUVEAU_BO_GART | NOUVEAU_BO_MAP, 0, 4096, NULL,
                        &screen->fence.bo);
   if (ret)
      goto fail;
   nouveau_bo_map(screen->fence.bo, 0, NULL);
   screen->fence.map = screen->fence.bo->map;
   screen->base.fence.emit = nvc0_screen_fence_emit;
   screen->base.fence.update = nvc0_screen_fence_update;

   switch (dev->chipset & 0xf0) {
   case 0xe0:
      obj_class = NVE4_P2MF_CLASS;
      break;
   default:
      obj_class = NVC0_M2MF_CLASS;
      break;
   }
   ret = nouveau_object_new(chan, 0xbeef323f, obj_class, NULL, 0,
                            &screen->m2mf);
   if (ret)
      FAIL_SCREEN_INIT("Error allocating PGRAPH context for M2MF: %d\n", ret);

   BEGIN_NVC0(push, SUBC_M2MF(NV01_SUBCHAN_OBJECT), 1);
   PUSH_DATA (push, screen->m2mf->oclass);
   if (screen->m2mf->oclass == NVE4_P2MF_CLASS) {
      BEGIN_NVC0(push, SUBC_COPY(NV01_SUBCHAN_OBJECT), 1);
      PUSH_DATA (push, 0xa0b5);
   }

   ret = nouveau_object_new(chan, 0xbeef902d, NVC0_2D_CLASS, NULL, 0,
                            &screen->eng2d);
   if (ret)
      FAIL_SCREEN_INIT("Error allocating PGRAPH context for 2D: %d\n", ret);

   BEGIN_NVC0(push, SUBC_2D(NV01_SUBCHAN_OBJECT), 1);
   PUSH_DATA (push, screen->eng2d->oclass);
   BEGIN_NVC0(push, NVC0_2D(OPERATION), 1);
   PUSH_DATA (push, NVC0_2D_OPERATION_SRCCOPY);
   BEGIN_NVC0(push, NVC0_2D(CLIP_ENABLE), 1);
   PUSH_DATA (push, 0);
   BEGIN_NVC0(push, NVC0_2D(COLOR_KEY_ENABLE), 1);
   PUSH_DATA (push, 0);
   BEGIN_NVC0(push, SUBC_2D(0x0884), 1);
   PUSH_DATA (push, 0x3f);
   BEGIN_NVC0(push, SUBC_2D(0x0888), 1);
   PUSH_DATA (push, 1);

   BEGIN_NVC0(push, SUBC_2D(NVC0_GRAPH_NOTIFY_ADDRESS_HIGH), 2);
   PUSH_DATAh(push, screen->fence.bo->offset + 16);
   PUSH_DATA (push, screen->fence.bo->offset + 16);

   switch (dev->chipset & 0xf0) {
   case 0xe0:
      obj_class = NVE4_3D_CLASS;
      break;
   case 0xd0:
   case 0xc0:
   default:
      switch (dev->chipset) {
      case 0xd9:
      case 0xc8:
         obj_class = NVC8_3D_CLASS;
         break;
      case 0xc1:
         obj_class = NVC1_3D_CLASS;
         break;
      default:
         obj_class = NVC0_3D_CLASS;
         break;
      }
      break;
   }
   ret = nouveau_object_new(chan, 0xbeef003d, obj_class, NULL, 0,
                            &screen->eng3d);
   if (ret)
      FAIL_SCREEN_INIT("Error allocating PGRAPH context for 3D: %d\n", ret);
   screen->base.class_3d = obj_class;

   BEGIN_NVC0(push, SUBC_3D(NV01_SUBCHAN_OBJECT), 1);
   PUSH_DATA (push, screen->eng3d->oclass);

   BEGIN_NVC0(push, NVC0_3D(COND_MODE), 1);
   PUSH_DATA (push, NVC0_3D_COND_MODE_ALWAYS);

   if (debug_get_bool_option("NOUVEAU_SHADER_WATCHDOG", TRUE)) {
      /* kill shaders after about 1 second (at 100 MHz) */
      BEGIN_NVC0(push, NVC0_3D(WATCHDOG_TIMER), 1);
      PUSH_DATA (push, 0x17);
   }

   BEGIN_NVC0(push, NVC0_3D(RT_CONTROL), 1);
   PUSH_DATA (push, 1);

   BEGIN_NVC0(push, NVC0_3D(CSAA_ENABLE), 1);
   PUSH_DATA (push, 0);
   BEGIN_NVC0(push, NVC0_3D(MULTISAMPLE_ENABLE), 1);
   PUSH_DATA (push, 0);
   BEGIN_NVC0(push, NVC0_3D(MULTISAMPLE_MODE), 1);
   PUSH_DATA (push, NVC0_3D_MULTISAMPLE_MODE_MS1);
   BEGIN_NVC0(push, NVC0_3D(MULTISAMPLE_CTRL), 1);
   PUSH_DATA (push, 0);
   BEGIN_NVC0(push, NVC0_3D(LINE_WIDTH_SEPARATE), 1);
   PUSH_DATA (push, 1);
   BEGIN_NVC0(push, NVC0_3D(LINE_LAST_PIXEL), 1);
   PUSH_DATA (push, 0);
   BEGIN_NVC0(push, NVC0_3D(BLEND_SEPARATE_ALPHA), 1);
   PUSH_DATA (push, 1);
   BEGIN_NVC0(push, NVC0_3D(BLEND_ENABLE_COMMON), 1);
   PUSH_DATA (push, 0);
   if (screen->eng3d->oclass < NVE4_3D_CLASS) {
      BEGIN_NVC0(push, NVC0_3D(TEX_MISC), 1);
      PUSH_DATA (push, NVC0_3D_TEX_MISC_SEAMLESS_CUBE_MAP);
   } else {
      BEGIN_NVC0(push, NVE4_3D(TEX_CB_INDEX), 1);
      PUSH_DATA (push, 15);
   }
   BEGIN_NVC0(push, NVC0_3D(CALL_LIMIT_LOG), 1);
   PUSH_DATA (push, 8); /* 128 */
   BEGIN_NVC0(push, NVC0_3D(ZCULL_STATCTRS_ENABLE), 1);
   PUSH_DATA (push, 1);
   if (screen->eng3d->oclass >= NVC1_3D_CLASS) {
      BEGIN_NVC0(push, NVC0_3D(CACHE_SPLIT), 1);
      PUSH_DATA (push, NVC0_3D_CACHE_SPLIT_48K_SHARED_16K_L1);
   }

   nvc0_magic_3d_init(push, screen->eng3d->oclass);

   ret = nouveau_bo_new(dev, NOUVEAU_BO_VRAM, 1 << 17, 1 << 20, NULL,
                        &screen->text);
   if (ret)
      goto fail;

   /* XXX: getting a page fault at the end of the code buffer every few
    *  launches, don't use the last 256 bytes to work around them - prefetch ?
    */
   nouveau_heap_init(&screen->text_heap, 0, (1 << 20) - 0x100);

   ret = nouveau_bo_new(dev, NOUVEAU_BO_VRAM, 1 << 12, 6 << 16, NULL,
                        &screen->uniform_bo);
   if (ret)
      goto fail;

   for (i = 0; i < 5; ++i) {
      /* TIC and TSC entries for each unit (nve4+ only) */
      /* auxiliary constants (6 user clip planes, base instance id) */
      BEGIN_NVC0(push, NVC0_3D(CB_SIZE), 3);
      PUSH_DATA (push, 512);
      PUSH_DATAh(push, screen->uniform_bo->offset + (5 << 16) + (i << 9));
      PUSH_DATA (push, screen->uniform_bo->offset + (5 << 16) + (i << 9));
      BEGIN_NVC0(push, NVC0_3D(CB_BIND(i)), 1);
      PUSH_DATA (push, (15 << 4) | 1);
      if (screen->eng3d->oclass >= NVE4_3D_CLASS) {
         unsigned j;
         BEGIN_1IC0(push, NVC0_3D(CB_POS), 9);
         PUSH_DATA (push, 0);
         for (j = 0; j < 8; ++j)
            PUSH_DATA(push, j);
      } else {
         BEGIN_NVC0(push, NVC0_3D(TEX_LIMITS(i)), 1);
         PUSH_DATA (push, 0x54);
      }
   }
   BEGIN_NVC0(push, NVC0_3D(LINKED_TSC), 1);
   PUSH_DATA (push, 0);

   /* return { 0.0, 0.0, 0.0, 0.0 } for out-of-bounds vtxbuf access */
   BEGIN_NVC0(push, NVC0_3D(CB_SIZE), 3);
   PUSH_DATA (push, 256);
   PUSH_DATAh(push, screen->uniform_bo->offset + (5 << 16) + (6 << 9));
   PUSH_DATA (push, screen->uniform_bo->offset + (5 << 16) + (6 << 9));
   BEGIN_1IC0(push, NVC0_3D(CB_POS), 5);
   PUSH_DATA (push, 0);
   PUSH_DATAf(push, 0.0f);
   PUSH_DATAf(push, 0.0f);
   PUSH_DATAf(push, 0.0f);
   PUSH_DATAf(push, 0.0f);
   BEGIN_NVC0(push, NVC0_3D(VERTEX_RUNOUT_ADDRESS_HIGH), 2);
   PUSH_DATAh(push, screen->uniform_bo->offset + (5 << 16) + (6 << 9));
   PUSH_DATA (push, screen->uniform_bo->offset + (5 << 16) + (6 << 9));

   /* max MPs * max warps per MP (TODO: ask kernel) */
   if (screen->eng3d->oclass >= NVE4_3D_CLASS)
      screen->tls_size = 8 * 64;
   else
      screen->tls_size = 16 * 48;
   screen->tls_size *= NVC0_CAP_MAX_PROGRAM_TEMPS * 16;
   screen->tls_size = align(screen->tls_size, 1 << 17);

   ret = nouveau_bo_new(dev, NOUVEAU_BO_VRAM, 1 << 17,
                        screen->tls_size, NULL, &screen->tls);
   if (ret)
      goto fail;

   BEGIN_NVC0(push, NVC0_3D(CODE_ADDRESS_HIGH), 2);
   PUSH_DATAh(push, screen->text->offset);
   PUSH_DATA (push, screen->text->offset);
   BEGIN_NVC0(push, NVC0_3D(TEMP_ADDRESS_HIGH), 4);
   PUSH_DATAh(push, screen->tls->offset);
   PUSH_DATA (push, screen->tls->offset);
   PUSH_DATA (push, screen->tls_size >> 32);
   PUSH_DATA (push, screen->tls_size);
   BEGIN_NVC0(push, NVC0_3D(WARP_TEMP_ALLOC), 1);
   PUSH_DATA (push, 0);
   BEGIN_NVC0(push, NVC0_3D(LOCAL_BASE), 1);
   PUSH_DATA (push, 0);

   ret = nouveau_bo_new(dev, NOUVEAU_BO_VRAM, 1 << 17, 1 << 20, NULL,
                        &screen->poly_cache);
   if (ret)
      goto fail;

   BEGIN_NVC0(push, NVC0_3D(VERTEX_QUARANTINE_ADDRESS_HIGH), 3);
   PUSH_DATAh(push, screen->poly_cache->offset);
   PUSH_DATA (push, screen->poly_cache->offset);
   PUSH_DATA (push, 3);

   ret = nouveau_bo_new(dev, NOUVEAU_BO_VRAM, 1 << 17, 1 << 17, NULL,
                        &screen->txc);
   if (ret)
      goto fail;

   BEGIN_NVC0(push, NVC0_3D(TIC_ADDRESS_HIGH), 3);
   PUSH_DATAh(push, screen->txc->offset);
   PUSH_DATA (push, screen->txc->offset);
   PUSH_DATA (push, NVC0_TIC_MAX_ENTRIES - 1);

   BEGIN_NVC0(push, NVC0_3D(TSC_ADDRESS_HIGH), 3);
   PUSH_DATAh(push, screen->txc->offset + 65536);
   PUSH_DATA (push, screen->txc->offset + 65536);
   PUSH_DATA (push, NVC0_TSC_MAX_ENTRIES - 1);

   BEGIN_NVC0(push, NVC0_3D(SCREEN_Y_CONTROL), 1);
   PUSH_DATA (push, 0);
   BEGIN_NVC0(push, NVC0_3D(WINDOW_OFFSET_X), 2);
   PUSH_DATA (push, 0);
   PUSH_DATA (push, 0);
   BEGIN_NVC0(push, NVC0_3D(ZCULL_REGION), 1); /* deactivate ZCULL */
   PUSH_DATA (push, 0x3f);

   BEGIN_NVC0(push, NVC0_3D(CLIP_RECTS_MODE), 1);
   PUSH_DATA (push, NVC0_3D_CLIP_RECTS_MODE_INSIDE_ANY);
   BEGIN_NVC0(push, NVC0_3D(CLIP_RECT_HORIZ(0)), 8 * 2);
   for (i = 0; i < 8 * 2; ++i)
      PUSH_DATA(push, 0);
   BEGIN_NVC0(push, NVC0_3D(CLIP_RECTS_EN), 1);
   PUSH_DATA (push, 0);
   BEGIN_NVC0(push, NVC0_3D(CLIPID_ENABLE), 1);
   PUSH_DATA (push, 0);

   /* neither scissors, viewport nor stencil mask should affect clears */
   BEGIN_NVC0(push, NVC0_3D(CLEAR_FLAGS), 1);
   PUSH_DATA (push, 0);

   BEGIN_NVC0(push, NVC0_3D(VIEWPORT_TRANSFORM_EN), 1);
   PUSH_DATA (push, 1);
   BEGIN_NVC0(push, NVC0_3D(DEPTH_RANGE_NEAR(0)), 2);
   PUSH_DATAf(push, 0.0f);
   PUSH_DATAf(push, 1.0f);
   BEGIN_NVC0(push, NVC0_3D(VIEW_VOLUME_CLIP_CTRL), 1);
   PUSH_DATA (push, NVC0_3D_VIEW_VOLUME_CLIP_CTRL_UNK1_UNK1);

   /* We use scissors instead of exact view volume clipping,
    * so they're always enabled.
    */
   BEGIN_NVC0(push, NVC0_3D(SCISSOR_ENABLE(0)), 3);
   PUSH_DATA (push, 1);
   PUSH_DATA (push, 8192 << 16);
   PUSH_DATA (push, 8192 << 16);

#define MK_MACRO(m, n) i = nvc0_graph_set_macro(screen, m, i, sizeof(n), n);

   i = 0;
   MK_MACRO(NVC0_3D_MACRO_VERTEX_ARRAY_PER_INSTANCE, nvc0_9097_per_instance_bf);
   MK_MACRO(NVC0_3D_MACRO_BLEND_ENABLES, nvc0_9097_blend_enables);
   MK_MACRO(NVC0_3D_MACRO_VERTEX_ARRAY_SELECT, nvc0_9097_vertex_array_select);
   MK_MACRO(NVC0_3D_MACRO_TEP_SELECT, nvc0_9097_tep_select);
   MK_MACRO(NVC0_3D_MACRO_GP_SELECT, nvc0_9097_gp_select);
   MK_MACRO(NVC0_3D_MACRO_POLYGON_MODE_FRONT, nvc0_9097_poly_mode_front);
   MK_MACRO(NVC0_3D_MACRO_POLYGON_MODE_BACK, nvc0_9097_poly_mode_back);

   BEGIN_NVC0(push, NVC0_3D(RASTERIZE_ENABLE), 1);
   PUSH_DATA (push, 1);
   BEGIN_NVC0(push, NVC0_3D(RT_SEPARATE_FRAG_DATA), 1);
   PUSH_DATA (push, 1);
   BEGIN_NVC0(push, NVC0_3D(MACRO_GP_SELECT), 1);
   PUSH_DATA (push, 0x40);
   BEGIN_NVC0(push, NVC0_3D(LAYER), 1);
   PUSH_DATA (push, 0);
   BEGIN_NVC0(push, NVC0_3D(MACRO_TEP_SELECT), 1);
   PUSH_DATA (push, 0x30);
   BEGIN_NVC0(push, NVC0_3D(PATCH_VERTICES), 1);
   PUSH_DATA (push, 3);
   BEGIN_NVC0(push, NVC0_3D(SP_SELECT(2)), 1);
   PUSH_DATA (push, 0x20);
   BEGIN_NVC0(push, NVC0_3D(SP_SELECT(0)), 1);
   PUSH_DATA (push, 0x00);

   BEGIN_NVC0(push, NVC0_3D(POINT_COORD_REPLACE), 1);
   PUSH_DATA (push, 0);
   BEGIN_NVC0(push, NVC0_3D(POINT_RASTER_RULES), 1);
   PUSH_DATA (push, NVC0_3D_POINT_RASTER_RULES_OGL);

   IMMED_NVC0(push, NVC0_3D(EDGEFLAG), 1);

   PUSH_KICK (push);

   screen->tic.entries = CALLOC(4096, sizeof(void *));
   screen->tsc.entries = screen->tic.entries + 2048;

   mm_config.nvc0.tile_mode = 0;
   mm_config.nvc0.memtype = 0xfe0;
   screen->mm_VRAM_fe0 = nouveau_mm_create(dev, NOUVEAU_BO_VRAM, &mm_config);

   if (!nvc0_blitctx_create(screen))
      goto fail;

   nouveau_fence_new(&screen->base, &screen->base.fence.current, FALSE);

   return pscreen;

fail:
   nvc0_screen_destroy(pscreen);
   return NULL;
}

int
nvc0_screen_tic_alloc(struct nvc0_screen *screen, void *entry)
{
   int i = screen->tic.next;

   while (screen->tic.lock[i / 32] & (1 << (i % 32)))
      i = (i + 1) & (NVC0_TIC_MAX_ENTRIES - 1);

   screen->tic.next = (i + 1) & (NVC0_TIC_MAX_ENTRIES - 1);

   if (screen->tic.entries[i])
      nv50_tic_entry(screen->tic.entries[i])->id = -1;

   screen->tic.entries[i] = entry;
   return i;
}

int
nvc0_screen_tsc_alloc(struct nvc0_screen *screen, void *entry)
{
   int i = screen->tsc.next;

   while (screen->tsc.lock[i / 32] & (1 << (i % 32)))
      i = (i + 1) & (NVC0_TSC_MAX_ENTRIES - 1);

   screen->tsc.next = (i + 1) & (NVC0_TSC_MAX_ENTRIES - 1);

   if (screen->tsc.entries[i])
      nv50_tsc_entry(screen->tsc.entries[i])->id = -1;

   screen->tsc.entries[i] = entry;
   return i;
}
