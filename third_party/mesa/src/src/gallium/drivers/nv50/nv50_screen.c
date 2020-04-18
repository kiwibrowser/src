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

#include "nv50_context.h"
#include "nv50_screen.h"

#include "nouveau/nv_object.xml.h"
#include <errno.h>

#ifndef NOUVEAU_GETPARAM_GRAPH_UNITS
# define NOUVEAU_GETPARAM_GRAPH_UNITS 13
#endif

/* affected by LOCAL_WARPS_LOG_ALLOC / LOCAL_WARPS_NO_CLAMP */
#define LOCAL_WARPS_ALLOC 32
/* affected by STACK_WARPS_LOG_ALLOC / STACK_WARPS_NO_CLAMP */
#define STACK_WARPS_ALLOC 32

#define THREADS_IN_WARP 32

#define ONE_TEMP_SIZE (4/*vector*/ * sizeof(float))

static boolean
nv50_screen_is_format_supported(struct pipe_screen *pscreen,
                                enum pipe_format format,
                                enum pipe_texture_target target,
                                unsigned sample_count,
                                unsigned bindings)
{
   if (sample_count > 8)
      return FALSE;
   if (!(0x117 & (1 << sample_count))) /* 0, 1, 2, 4 or 8 */
      return FALSE;
   if (sample_count == 8 && util_format_get_blocksizebits(format) >= 128)
      return FALSE;

   if (!util_format_is_supported(format, bindings))
      return FALSE;

   switch (format) {
   case PIPE_FORMAT_Z16_UNORM:
      if (nv50_screen(pscreen)->tesla->oclass < NVA0_3D_CLASS)
         return FALSE;
      break;
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

   return (nv50_format_table[format].usage & bindings) == bindings;
}

static int
nv50_screen_get_param(struct pipe_screen *pscreen, enum pipe_cap param)
{
   const uint16_t class_3d = nouveau_screen(pscreen)->class_3d;

   switch (param) {
   case PIPE_CAP_MAX_COMBINED_SAMPLERS:
      return 64;
   case PIPE_CAP_MAX_TEXTURE_2D_LEVELS:
      return 14;
   case PIPE_CAP_MAX_TEXTURE_3D_LEVELS:
      return 12;
   case PIPE_CAP_MAX_TEXTURE_CUBE_LEVELS:
      return 14;
   case PIPE_CAP_MAX_TEXTURE_ARRAY_LAYERS:
      return 512;
   case PIPE_CAP_MIN_TEXEL_OFFSET:
      return -8;
   case PIPE_CAP_MAX_TEXEL_OFFSET:
      return 7;
   case PIPE_CAP_TEXTURE_MIRROR_CLAMP:
   case PIPE_CAP_TEXTURE_SWIZZLE:
   case PIPE_CAP_TEXTURE_SHADOW_MAP:
   case PIPE_CAP_NPOT_TEXTURES:
   case PIPE_CAP_ANISOTROPIC_FILTER:
   case PIPE_CAP_SCALED_RESOLVE:
      return 1;
   case PIPE_CAP_SEAMLESS_CUBE_MAP:
      return nv50_screen(pscreen)->tesla->oclass >= NVA0_3D_CLASS;
   case PIPE_CAP_SEAMLESS_CUBE_MAP_PER_TEXTURE:
      return 0;
   case PIPE_CAP_TWO_SIDED_STENCIL:
   case PIPE_CAP_DEPTH_CLIP_DISABLE:
   case PIPE_CAP_DEPTHSTENCIL_CLEAR_SEPARATE:
   case PIPE_CAP_POINT_SPRITE:
      return 1;
   case PIPE_CAP_SM3:
      return 1;
   case PIPE_CAP_GLSL_FEATURE_LEVEL:
      return 130;
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
      return 1;
   case PIPE_CAP_MAX_STREAM_OUTPUT_BUFFERS:
      return 4;
   case PIPE_CAP_MAX_STREAM_OUTPUT_INTERLEAVED_COMPONENTS:
   case PIPE_CAP_MAX_STREAM_OUTPUT_SEPARATE_COMPONENTS:
      return 64;
   case PIPE_CAP_STREAM_OUTPUT_PAUSE_RESUME:
      return (class_3d >= NVA0_3D_CLASS) ? 1 : 0;
   case PIPE_CAP_BLEND_EQUATION_SEPARATE:
   case PIPE_CAP_INDEP_BLEND_ENABLE:
      return 1;
   case PIPE_CAP_INDEP_BLEND_FUNC:
      return nv50_screen(pscreen)->tesla->oclass >= NVA3_3D_CLASS;
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
nv50_screen_get_shader_param(struct pipe_screen *pscreen, unsigned shader,
                             enum pipe_shader_cap param)
{
   switch (shader) {
   case PIPE_SHADER_VERTEX:
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
      return 4;
   case PIPE_SHADER_CAP_MAX_INPUTS:
      if (shader == PIPE_SHADER_VERTEX)
         return 32;
      return 0x300 / 16;
   case PIPE_SHADER_CAP_MAX_CONSTS:
      return 65536 / 16;
   case PIPE_SHADER_CAP_MAX_CONST_BUFFERS:
      return NV50_MAX_PIPE_CONSTBUFS;
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
      return nv50_screen(pscreen)->max_tls_space / ONE_TEMP_SIZE;
   case PIPE_SHADER_CAP_TGSI_CONT_SUPPORTED:
      return 1;
   case PIPE_SHADER_CAP_SUBROUTINES:
      return 0; /* please inline, or provide function declarations */
   case PIPE_SHADER_CAP_INTEGERS:
      return 1;
   case PIPE_SHADER_CAP_MAX_TEXTURE_SAMPLERS:
      return 32;
   default:
      NOUVEAU_ERR("unknown PIPE_SHADER_CAP %d\n", param);
      return 0;
   }
}

static float
nv50_screen_get_paramf(struct pipe_screen *pscreen, enum pipe_capf param)
{
   switch (param) {
   case PIPE_CAPF_MAX_LINE_WIDTH:
   case PIPE_CAPF_MAX_LINE_WIDTH_AA:
      return 10.0f;
   case PIPE_CAPF_MAX_POINT_WIDTH:
   case PIPE_CAPF_MAX_POINT_WIDTH_AA:
      return 64.0f;
   case PIPE_CAPF_MAX_TEXTURE_ANISOTROPY:
      return 16.0f;
   case PIPE_CAPF_MAX_TEXTURE_LOD_BIAS:
      return 4.0f;
   default:
      NOUVEAU_ERR("unknown PIPE_CAP %d\n", param);
      return 0.0f;
   }
}

static void
nv50_screen_destroy(struct pipe_screen *pscreen)
{
   struct nv50_screen *screen = nv50_screen(pscreen);

   if (screen->base.fence.current) {
      nouveau_fence_wait(screen->base.fence.current);
      nouveau_fence_ref (NULL, &screen->base.fence.current);
   }
   if (screen->base.pushbuf)
      screen->base.pushbuf->user_priv = NULL;

   if (screen->blitctx)
      FREE(screen->blitctx);

   nouveau_bo_ref(NULL, &screen->code);
   nouveau_bo_ref(NULL, &screen->tls_bo);
   nouveau_bo_ref(NULL, &screen->stack_bo);
   nouveau_bo_ref(NULL, &screen->txc);
   nouveau_bo_ref(NULL, &screen->uniforms);
   nouveau_bo_ref(NULL, &screen->fence.bo);

   nouveau_heap_destroy(&screen->vp_code_heap);
   nouveau_heap_destroy(&screen->gp_code_heap);
   nouveau_heap_destroy(&screen->fp_code_heap);

   if (screen->tic.entries)
      FREE(screen->tic.entries);

   nouveau_object_del(&screen->tesla);
   nouveau_object_del(&screen->eng2d);
   nouveau_object_del(&screen->m2mf);
   nouveau_object_del(&screen->sync);

   nouveau_screen_fini(&screen->base);

   FREE(screen);
}

static void
nv50_screen_fence_emit(struct pipe_screen *pscreen, u32 *sequence)
{
   struct nv50_screen *screen = nv50_screen(pscreen);
   struct nouveau_pushbuf *push = screen->base.pushbuf;

   /* we need to do it after possible flush in MARK_RING */
   *sequence = ++screen->base.fence.sequence;

   PUSH_DATA (push, NV50_FIFO_PKHDR(NV50_3D(QUERY_ADDRESS_HIGH), 4));
   PUSH_DATAh(push, screen->fence.bo->offset);
   PUSH_DATA (push, screen->fence.bo->offset);
   PUSH_DATA (push, *sequence);
   PUSH_DATA (push, NV50_3D_QUERY_GET_MODE_WRITE_UNK0 |
                    NV50_3D_QUERY_GET_UNK4 |
                    NV50_3D_QUERY_GET_UNIT_CROP |
                    NV50_3D_QUERY_GET_TYPE_QUERY |
                    NV50_3D_QUERY_GET_QUERY_SELECT_ZERO |
                    NV50_3D_QUERY_GET_SHORT);
}

static u32
nv50_screen_fence_update(struct pipe_screen *pscreen)
{
   return nv50_screen(pscreen)->fence.map[0];
}

static void
nv50_screen_init_hwctx(struct nv50_screen *screen)
{
   struct nouveau_pushbuf *push = screen->base.pushbuf;
   struct nv04_fifo *fifo;
   unsigned i;

   fifo = (struct nv04_fifo *)screen->base.channel->data;

   BEGIN_NV04(push, SUBC_M2MF(NV01_SUBCHAN_OBJECT), 1);
   PUSH_DATA (push, screen->m2mf->handle);
   BEGIN_NV04(push, SUBC_M2MF(NV03_M2MF_DMA_NOTIFY), 3);
   PUSH_DATA (push, screen->sync->handle);
   PUSH_DATA (push, fifo->vram);
   PUSH_DATA (push, fifo->vram);

   BEGIN_NV04(push, SUBC_2D(NV01_SUBCHAN_OBJECT), 1);
   PUSH_DATA (push, screen->eng2d->handle);
   BEGIN_NV04(push, NV50_2D(DMA_NOTIFY), 4);
   PUSH_DATA (push, screen->sync->handle);
   PUSH_DATA (push, fifo->vram);
   PUSH_DATA (push, fifo->vram);
   PUSH_DATA (push, fifo->vram);
   BEGIN_NV04(push, NV50_2D(OPERATION), 1);
   PUSH_DATA (push, NV50_2D_OPERATION_SRCCOPY);
   BEGIN_NV04(push, NV50_2D(CLIP_ENABLE), 1);
   PUSH_DATA (push, 0);
   BEGIN_NV04(push, NV50_2D(COLOR_KEY_ENABLE), 1);
   PUSH_DATA (push, 0);
   BEGIN_NV04(push, SUBC_2D(0x0888), 1);
   PUSH_DATA (push, 1);

   BEGIN_NV04(push, SUBC_3D(NV01_SUBCHAN_OBJECT), 1);
   PUSH_DATA (push, screen->tesla->handle);

   BEGIN_NV04(push, NV50_3D(COND_MODE), 1);
   PUSH_DATA (push, NV50_3D_COND_MODE_ALWAYS);

   BEGIN_NV04(push, NV50_3D(DMA_NOTIFY), 1);
   PUSH_DATA (push, screen->sync->handle);
   BEGIN_NV04(push, NV50_3D(DMA_ZETA), 11);
   for (i = 0; i < 11; ++i)
      PUSH_DATA(push, fifo->vram);
   BEGIN_NV04(push, NV50_3D(DMA_COLOR(0)), NV50_3D_DMA_COLOR__LEN);
   for (i = 0; i < NV50_3D_DMA_COLOR__LEN; ++i)
      PUSH_DATA(push, fifo->vram);

   BEGIN_NV04(push, NV50_3D(REG_MODE), 1);
   PUSH_DATA (push, NV50_3D_REG_MODE_STRIPED);
   BEGIN_NV04(push, NV50_3D(UNK1400_LANES), 1);
   PUSH_DATA (push, 0xf);

   if (debug_get_bool_option("NOUVEAU_SHADER_WATCHDOG", TRUE)) {
      BEGIN_NV04(push, NV50_3D(WATCHDOG_TIMER), 1);
      PUSH_DATA (push, 0x18);
   }

   BEGIN_NV04(push, NV50_3D(RT_CONTROL), 1);
   PUSH_DATA (push, 1);

   BEGIN_NV04(push, NV50_3D(CSAA_ENABLE), 1);
   PUSH_DATA (push, 0);
   BEGIN_NV04(push, NV50_3D(MULTISAMPLE_ENABLE), 1);
   PUSH_DATA (push, 0);
   BEGIN_NV04(push, NV50_3D(MULTISAMPLE_MODE), 1);
   PUSH_DATA (push, NV50_3D_MULTISAMPLE_MODE_MS1);
   BEGIN_NV04(push, NV50_3D(MULTISAMPLE_CTRL), 1);
   PUSH_DATA (push, 0);
   BEGIN_NV04(push, NV50_3D(LINE_LAST_PIXEL), 1);
   PUSH_DATA (push, 0);
   BEGIN_NV04(push, NV50_3D(BLEND_SEPARATE_ALPHA), 1);
   PUSH_DATA (push, 1);

   if (screen->tesla->oclass >= NVA0_3D_CLASS) {
      BEGIN_NV04(push, SUBC_3D(NVA0_3D_TEX_MISC), 1);
      PUSH_DATA (push, NVA0_3D_TEX_MISC_SEAMLESS_CUBE_MAP);
   }

   BEGIN_NV04(push, NV50_3D(SCREEN_Y_CONTROL), 1);
   PUSH_DATA (push, 0);
   BEGIN_NV04(push, NV50_3D(WINDOW_OFFSET_X), 2);
   PUSH_DATA (push, 0);
   PUSH_DATA (push, 0);
   BEGIN_NV04(push, NV50_3D(ZCULL_REGION), 1);
   PUSH_DATA (push, 0x3f);

   BEGIN_NV04(push, NV50_3D(VP_ADDRESS_HIGH), 2);
   PUSH_DATAh(push, screen->code->offset + (0 << NV50_CODE_BO_SIZE_LOG2));
   PUSH_DATA (push, screen->code->offset + (0 << NV50_CODE_BO_SIZE_LOG2));

   BEGIN_NV04(push, NV50_3D(FP_ADDRESS_HIGH), 2);
   PUSH_DATAh(push, screen->code->offset + (1 << NV50_CODE_BO_SIZE_LOG2));
   PUSH_DATA (push, screen->code->offset + (1 << NV50_CODE_BO_SIZE_LOG2));

   BEGIN_NV04(push, NV50_3D(GP_ADDRESS_HIGH), 2);
   PUSH_DATAh(push, screen->code->offset + (2 << NV50_CODE_BO_SIZE_LOG2));
   PUSH_DATA (push, screen->code->offset + (2 << NV50_CODE_BO_SIZE_LOG2));

   BEGIN_NV04(push, NV50_3D(LOCAL_ADDRESS_HIGH), 3);
   PUSH_DATAh(push, screen->tls_bo->offset);
   PUSH_DATA (push, screen->tls_bo->offset);
   PUSH_DATA (push, util_logbase2(screen->cur_tls_space / 8));

   BEGIN_NV04(push, NV50_3D(STACK_ADDRESS_HIGH), 3);
   PUSH_DATAh(push, screen->stack_bo->offset);
   PUSH_DATA (push, screen->stack_bo->offset);
   PUSH_DATA (push, 4);

   BEGIN_NV04(push, NV50_3D(CB_DEF_ADDRESS_HIGH), 3);
   PUSH_DATAh(push, screen->uniforms->offset + (0 << 16));
   PUSH_DATA (push, screen->uniforms->offset + (0 << 16));
   PUSH_DATA (push, (NV50_CB_PVP << 16) | 0x0000);

   BEGIN_NV04(push, NV50_3D(CB_DEF_ADDRESS_HIGH), 3);
   PUSH_DATAh(push, screen->uniforms->offset + (1 << 16));
   PUSH_DATA (push, screen->uniforms->offset + (1 << 16));
   PUSH_DATA (push, (NV50_CB_PGP << 16) | 0x0000);

   BEGIN_NV04(push, NV50_3D(CB_DEF_ADDRESS_HIGH), 3);
   PUSH_DATAh(push, screen->uniforms->offset + (2 << 16));
   PUSH_DATA (push, screen->uniforms->offset + (2 << 16));
   PUSH_DATA (push, (NV50_CB_PFP << 16) | 0x0000);

   BEGIN_NV04(push, NV50_3D(CB_DEF_ADDRESS_HIGH), 3);
   PUSH_DATAh(push, screen->uniforms->offset + (3 << 16));
   PUSH_DATA (push, screen->uniforms->offset + (3 << 16));
   PUSH_DATA (push, (NV50_CB_AUX << 16) | 0x0200);

   BEGIN_NI04(push, NV50_3D(SET_PROGRAM_CB), 3);
   PUSH_DATA (push, (NV50_CB_AUX << 12) | 0xf01);
   PUSH_DATA (push, (NV50_CB_AUX << 12) | 0xf21);
   PUSH_DATA (push, (NV50_CB_AUX << 12) | 0xf31);

   /* return { 0.0, 0.0, 0.0, 0.0 } on out-of-bounds vtxbuf access */
   BEGIN_NV04(push, NV50_3D(CB_ADDR), 1);
   PUSH_DATA (push, ((1 << 9) << 6) | NV50_CB_AUX);
   BEGIN_NI04(push, NV50_3D(CB_DATA(0)), 4);
   PUSH_DATAf(push, 0.0f);
   PUSH_DATAf(push, 0.0f);
   PUSH_DATAf(push, 0.0f);
   PUSH_DATAf(push, 0.0f);
   BEGIN_NV04(push, NV50_3D(VERTEX_RUNOUT_ADDRESS_HIGH), 2);
   PUSH_DATAh(push, screen->uniforms->offset + (3 << 16) + (1 << 9));
   PUSH_DATA (push, screen->uniforms->offset + (3 << 16) + (1 << 9));

   /* max TIC (bits 4:8) & TSC bindings, per program type */
   for (i = 0; i < 3; ++i) {
      BEGIN_NV04(push, NV50_3D(TEX_LIMITS(i)), 1);
      PUSH_DATA (push, 0x54);
   }

   BEGIN_NV04(push, NV50_3D(TIC_ADDRESS_HIGH), 3);
   PUSH_DATAh(push, screen->txc->offset);
   PUSH_DATA (push, screen->txc->offset);
   PUSH_DATA (push, NV50_TIC_MAX_ENTRIES - 1);

   BEGIN_NV04(push, NV50_3D(TSC_ADDRESS_HIGH), 3);
   PUSH_DATAh(push, screen->txc->offset + 65536);
   PUSH_DATA (push, screen->txc->offset + 65536);
   PUSH_DATA (push, NV50_TSC_MAX_ENTRIES - 1);

   BEGIN_NV04(push, NV50_3D(LINKED_TSC), 1);
   PUSH_DATA (push, 0);

   BEGIN_NV04(push, NV50_3D(CLIP_RECTS_EN), 1);
   PUSH_DATA (push, 0);
   BEGIN_NV04(push, NV50_3D(CLIP_RECTS_MODE), 1);
   PUSH_DATA (push, NV50_3D_CLIP_RECTS_MODE_INSIDE_ANY);
   BEGIN_NV04(push, NV50_3D(CLIP_RECT_HORIZ(0)), 8 * 2);
   for (i = 0; i < 8 * 2; ++i)
      PUSH_DATA(push, 0);
   BEGIN_NV04(push, NV50_3D(CLIPID_ENABLE), 1);
   PUSH_DATA (push, 0);

   BEGIN_NV04(push, NV50_3D(VIEWPORT_TRANSFORM_EN), 1);
   PUSH_DATA (push, 1);
   BEGIN_NV04(push, NV50_3D(DEPTH_RANGE_NEAR(0)), 2);
   PUSH_DATAf(push, 0.0f);
   PUSH_DATAf(push, 1.0f);

   BEGIN_NV04(push, NV50_3D(VIEW_VOLUME_CLIP_CTRL), 1);
#ifdef NV50_SCISSORS_CLIPPING
   PUSH_DATA (push, 0x0000);
#else
   PUSH_DATA (push, 0x1080);
#endif

   BEGIN_NV04(push, NV50_3D(CLEAR_FLAGS), 1);
   PUSH_DATA (push, NV50_3D_CLEAR_FLAGS_CLEAR_RECT_VIEWPORT);

   /* We use scissors instead of exact view volume clipping,
    * so they're always enabled.
    */
   BEGIN_NV04(push, NV50_3D(SCISSOR_ENABLE(0)), 3);
   PUSH_DATA (push, 1);
   PUSH_DATA (push, 8192 << 16);
   PUSH_DATA (push, 8192 << 16);

   BEGIN_NV04(push, NV50_3D(RASTERIZE_ENABLE), 1);
   PUSH_DATA (push, 1);
   BEGIN_NV04(push, NV50_3D(POINT_RASTER_RULES), 1);
   PUSH_DATA (push, NV50_3D_POINT_RASTER_RULES_OGL);
   BEGIN_NV04(push, NV50_3D(FRAG_COLOR_CLAMP_EN), 1);
   PUSH_DATA (push, 0x11111111);
   BEGIN_NV04(push, NV50_3D(EDGEFLAG), 1);
   PUSH_DATA (push, 1);

   PUSH_KICK (push);
}

static int nv50_tls_alloc(struct nv50_screen *screen, unsigned tls_space,
      uint64_t *tls_size)
{
   struct nouveau_device *dev = screen->base.device;
   int ret;

   screen->cur_tls_space = util_next_power_of_two(tls_space / ONE_TEMP_SIZE) *
         ONE_TEMP_SIZE;
   if (nouveau_mesa_debug)
      debug_printf("allocating space for %u temps\n",
            util_next_power_of_two(tls_space / ONE_TEMP_SIZE));
   *tls_size = screen->cur_tls_space * util_next_power_of_two(screen->TPs) *
         screen->MPsInTP * LOCAL_WARPS_ALLOC * THREADS_IN_WARP;

   ret = nouveau_bo_new(dev, NOUVEAU_BO_VRAM, 1 << 16,
                        *tls_size, NULL, &screen->tls_bo);
   if (ret) {
      NOUVEAU_ERR("Failed to allocate local bo: %d\n", ret);
      return ret;
   }

   return 0;
}

int nv50_tls_realloc(struct nv50_screen *screen, unsigned tls_space)
{
   struct nouveau_pushbuf *push = screen->base.pushbuf;
   int ret;
   uint64_t tls_size;

   if (tls_space < screen->cur_tls_space)
      return 0;
   if (tls_space > screen->max_tls_space) {
      /* fixable by limiting number of warps (LOCAL_WARPS_LOG_ALLOC /
       * LOCAL_WARPS_NO_CLAMP) */
      NOUVEAU_ERR("Unsupported number of temporaries (%u > %u). Fixable if someone cares.\n",
            (unsigned)(tls_space / ONE_TEMP_SIZE),
            (unsigned)(screen->max_tls_space / ONE_TEMP_SIZE));
      return -ENOMEM;
   }

   nouveau_bo_ref(NULL, &screen->tls_bo);
   ret = nv50_tls_alloc(screen, tls_space, &tls_size);
   if (ret)
      return ret;

   BEGIN_NV04(push, NV50_3D(LOCAL_ADDRESS_HIGH), 3);
   PUSH_DATAh(push, screen->tls_bo->offset);
   PUSH_DATA (push, screen->tls_bo->offset);
   PUSH_DATA (push, util_logbase2(screen->cur_tls_space / 8));

   return 1;
}

struct pipe_screen *
nv50_screen_create(struct nouveau_device *dev)
{
   struct nv50_screen *screen;
   struct pipe_screen *pscreen;
   struct nouveau_object *chan;
   uint64_t value;
   uint32_t tesla_class;
   unsigned stack_size;
   int ret;

   screen = CALLOC_STRUCT(nv50_screen);
   if (!screen)
      return NULL;
   pscreen = &screen->base.base;

   ret = nouveau_screen_init(&screen->base, dev);
   if (ret) {
      NOUVEAU_ERR("nouveau_screen_init failed: %d\n", ret);
      goto fail;
   }

   /* TODO: Prevent FIFO prefetch before transfer of index buffers and
    *  admit them to VRAM.
    */
   screen->base.vidmem_bindings |= PIPE_BIND_CONSTANT_BUFFER |
      PIPE_BIND_VERTEX_BUFFER;
   screen->base.sysmem_bindings |=
      PIPE_BIND_VERTEX_BUFFER | PIPE_BIND_INDEX_BUFFER;

   screen->base.pushbuf->user_priv = screen;
   screen->base.pushbuf->rsvd_kick = 5;

   chan = screen->base.channel;

   pscreen->destroy = nv50_screen_destroy;
   pscreen->context_create = nv50_create;
   pscreen->is_format_supported = nv50_screen_is_format_supported;
   pscreen->get_param = nv50_screen_get_param;
   pscreen->get_shader_param = nv50_screen_get_shader_param;
   pscreen->get_paramf = nv50_screen_get_paramf;

   nv50_screen_init_resource_functions(pscreen);

   nouveau_screen_init_vdec(&screen->base);

   ret = nouveau_bo_new(dev, NOUVEAU_BO_GART | NOUVEAU_BO_MAP, 0, 4096,
                        NULL, &screen->fence.bo);
   if (ret) {
      NOUVEAU_ERR("Failed to allocate fence bo: %d\n", ret);
      goto fail;
   }

   nouveau_bo_map(screen->fence.bo, 0, NULL);
   screen->fence.map = screen->fence.bo->map;
   screen->base.fence.emit = nv50_screen_fence_emit;
   screen->base.fence.update = nv50_screen_fence_update;

   ret = nouveau_object_new(chan, 0xbeef0301, NOUVEAU_NOTIFIER_CLASS,
                            &(struct nv04_notify){ .length = 32 },
                            sizeof(struct nv04_notify), &screen->sync);
   if (ret) {
      NOUVEAU_ERR("Failed to allocate notifier: %d\n", ret);
      goto fail;
   }

   ret = nouveau_object_new(chan, 0xbeef5039, NV50_M2MF_CLASS,
                            NULL, 0, &screen->m2mf);
   if (ret) {
      NOUVEAU_ERR("Failed to allocate PGRAPH context for M2MF: %d\n", ret);
      goto fail;
   }

   ret = nouveau_object_new(chan, 0xbeef502d, NV50_2D_CLASS,
                            NULL, 0, &screen->eng2d);
   if (ret) {
      NOUVEAU_ERR("Failed to allocate PGRAPH context for 2D: %d\n", ret);
      goto fail;
   }

   switch (dev->chipset & 0xf0) {
   case 0x50:
      tesla_class = NV50_3D_CLASS;
      break;
   case 0x80:
   case 0x90:
      tesla_class = NV84_3D_CLASS;
      break;
   case 0xa0:
      switch (dev->chipset) {
      case 0xa0:
      case 0xaa:
      case 0xac:
         tesla_class = NVA0_3D_CLASS;
         break;
      case 0xaf:
         tesla_class = NVAF_3D_CLASS;
         break;
      default:
         tesla_class = NVA3_3D_CLASS;
         break;
      }
      break;
   default:
      NOUVEAU_ERR("Not a known NV50 chipset: NV%02x\n", dev->chipset);
      goto fail;
   }
   screen->base.class_3d = tesla_class;

   ret = nouveau_object_new(chan, 0xbeef5097, tesla_class,
                            NULL, 0, &screen->tesla);
   if (ret) {
      NOUVEAU_ERR("Failed to allocate PGRAPH context for 3D: %d\n", ret);
      goto fail;
   }

   ret = nouveau_bo_new(dev, NOUVEAU_BO_VRAM, 1 << 16,
                        3 << NV50_CODE_BO_SIZE_LOG2, NULL, &screen->code);
   if (ret) {
      NOUVEAU_ERR("Failed to allocate code bo: %d\n", ret);
      goto fail;
   }

   nouveau_heap_init(&screen->vp_code_heap, 0, 1 << NV50_CODE_BO_SIZE_LOG2);
   nouveau_heap_init(&screen->gp_code_heap, 0, 1 << NV50_CODE_BO_SIZE_LOG2);
   nouveau_heap_init(&screen->fp_code_heap, 0, 1 << NV50_CODE_BO_SIZE_LOG2);

   nouveau_getparam(dev, NOUVEAU_GETPARAM_GRAPH_UNITS, &value);

   screen->TPs = util_bitcount(value & 0xffff);
   screen->MPsInTP = util_bitcount((value >> 24) & 0xf);

   stack_size = util_next_power_of_two(screen->TPs) * screen->MPsInTP *
         STACK_WARPS_ALLOC * 64 * 8;

   ret = nouveau_bo_new(dev, NOUVEAU_BO_VRAM, 1 << 16, stack_size, NULL,
                        &screen->stack_bo);
   if (ret) {
      NOUVEAU_ERR("Failed to allocate stack bo: %d\n", ret);
      goto fail;
   }

   uint64_t size_of_one_temp = util_next_power_of_two(screen->TPs) *
         screen->MPsInTP * LOCAL_WARPS_ALLOC *  THREADS_IN_WARP *
         ONE_TEMP_SIZE;
   screen->max_tls_space = dev->vram_size / size_of_one_temp * ONE_TEMP_SIZE;
   screen->max_tls_space /= 2; /* half of vram */

   /* hw can address max 64 KiB */
   screen->max_tls_space = MIN2(screen->max_tls_space, 64 << 10);

   uint64_t tls_size;
   unsigned tls_space = 4/*temps*/ * ONE_TEMP_SIZE;
   ret = nv50_tls_alloc(screen, tls_space, &tls_size);
   if (ret)
      goto fail;

   if (nouveau_mesa_debug)
      debug_printf("TPs = %u, MPsInTP = %u, VRAM = %"PRIu64" MiB, tls_size = %"PRIu64" KiB\n",
            screen->TPs, screen->MPsInTP, dev->vram_size >> 20, tls_size >> 10);

   ret = nouveau_bo_new(dev, NOUVEAU_BO_VRAM, 1 << 16, 4 << 16, NULL,
                        &screen->uniforms);
   if (ret) {
      NOUVEAU_ERR("Failed to allocate uniforms bo: %d\n", ret);
      goto fail;
   }

   ret = nouveau_bo_new(dev, NOUVEAU_BO_VRAM, 1 << 16, 3 << 16, NULL,
                        &screen->txc);
   if (ret) {
      NOUVEAU_ERR("Failed to allocate TIC/TSC bo: %d\n", ret);
      goto fail;
   }

   screen->tic.entries = CALLOC(4096, sizeof(void *));
   screen->tsc.entries = screen->tic.entries + 2048;

   if (!nv50_blitctx_create(screen))
      goto fail;

   nv50_screen_init_hwctx(screen);

   nouveau_fence_new(&screen->base, &screen->base.fence.current, FALSE);

   return pscreen;

fail:
   nv50_screen_destroy(pscreen);
   return NULL;
}

int
nv50_screen_tic_alloc(struct nv50_screen *screen, void *entry)
{
   int i = screen->tic.next;

   while (screen->tic.lock[i / 32] & (1 << (i % 32)))
      i = (i + 1) & (NV50_TIC_MAX_ENTRIES - 1);

   screen->tic.next = (i + 1) & (NV50_TIC_MAX_ENTRIES - 1);

   if (screen->tic.entries[i])
      nv50_tic_entry(screen->tic.entries[i])->id = -1;

   screen->tic.entries[i] = entry;
   return i;
}

int
nv50_screen_tsc_alloc(struct nv50_screen *screen, void *entry)
{
   int i = screen->tsc.next;

   while (screen->tsc.lock[i / 32] & (1 << (i % 32)))
      i = (i + 1) & (NV50_TSC_MAX_ENTRIES - 1);

   screen->tsc.next = (i + 1) & (NV50_TSC_MAX_ENTRIES - 1);

   if (screen->tsc.entries[i])
      nv50_tsc_entry(screen->tsc.entries[i])->id = -1;

   screen->tsc.entries[i] = entry;
   return i;
}
