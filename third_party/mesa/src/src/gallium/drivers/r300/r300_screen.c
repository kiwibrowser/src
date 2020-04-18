/*
 * Copyright 2008 Corbin Simpson <MostAwesomeDude@gmail.com>
 * Copyright 2010 Marek Olšák <maraeo@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE. */

#include "util/u_format.h"
#include "util/u_format_s3tc.h"
#include "util/u_memory.h"
#include "os/os_time.h"
#include "vl/vl_decoder.h"
#include "vl/vl_video_buffer.h"

#include "r300_context.h"
#include "r300_texture.h"
#include "r300_screen_buffer.h"
#include "r300_state_inlines.h"
#include "r300_public.h"

#include "draw/draw_context.h"

/* Return the identifier behind whom the brave coders responsible for this
 * amalgamation of code, sweat, and duct tape, routinely obscure their names.
 *
 * ...I should have just put "Corbin Simpson", but I'm not that cool.
 *
 * (Or egotistical. Yet.) */
static const char* r300_get_vendor(struct pipe_screen* pscreen)
{
    return "X.Org R300 Project";
}

static const char* chip_families[] = {
    "ATI R300",
    "ATI R350",
    "ATI RV350",
    "ATI RV370",
    "ATI RV380",
    "ATI RS400",
    "ATI RC410",
    "ATI RS480",
    "ATI R420",
    "ATI R423",
    "ATI R430",
    "ATI R480",
    "ATI R481",
    "ATI RV410",
    "ATI RS600",
    "ATI RS690",
    "ATI RS740",
    "ATI RV515",
    "ATI R520",
    "ATI RV530",
    "ATI R580",
    "ATI RV560",
    "ATI RV570"
};

static const char* r300_get_name(struct pipe_screen* pscreen)
{
    struct r300_screen* r300screen = r300_screen(pscreen);

    return chip_families[r300screen->caps.family];
}

static int r300_get_param(struct pipe_screen* pscreen, enum pipe_cap param)
{
    struct r300_screen* r300screen = r300_screen(pscreen);
    boolean is_r500 = r300screen->caps.is_r500;

    switch (param) {
        /* Supported features (boolean caps). */
        case PIPE_CAP_NPOT_TEXTURES:
        case PIPE_CAP_TWO_SIDED_STENCIL:
        case PIPE_CAP_ANISOTROPIC_FILTER:
        case PIPE_CAP_POINT_SPRITE:
        case PIPE_CAP_OCCLUSION_QUERY:
        case PIPE_CAP_TEXTURE_SHADOW_MAP:
        case PIPE_CAP_TEXTURE_MIRROR_CLAMP:
        case PIPE_CAP_BLEND_EQUATION_SEPARATE:
        case PIPE_CAP_VERTEX_ELEMENT_INSTANCE_DIVISOR:
        case PIPE_CAP_TGSI_FS_COORD_ORIGIN_UPPER_LEFT:
        case PIPE_CAP_TGSI_FS_COORD_PIXEL_CENTER_HALF_INTEGER:
        case PIPE_CAP_CONDITIONAL_RENDER:
        case PIPE_CAP_TEXTURE_BARRIER:
        case PIPE_CAP_TGSI_CAN_COMPACT_VARYINGS:
        case PIPE_CAP_TGSI_CAN_COMPACT_CONSTANTS:
        case PIPE_CAP_VERTEX_COLOR_CLAMPED:
        case PIPE_CAP_USER_INDEX_BUFFERS:
        case PIPE_CAP_USER_CONSTANT_BUFFERS:
            return 1;

        case PIPE_CAP_CONSTANT_BUFFER_OFFSET_ALIGNMENT:
            return 16;

        case PIPE_CAP_GLSL_FEATURE_LEVEL:
            return 120;

        /* r300 cannot do swizzling of compressed textures. Supported otherwise. */
        case PIPE_CAP_TEXTURE_SWIZZLE:
            return util_format_s3tc_enabled ? r300screen->caps.dxtc_swizzle : 1;

        /* Supported on r500 only. */
        case PIPE_CAP_VERTEX_COLOR_UNCLAMPED:
        case PIPE_CAP_MIXED_COLORBUFFER_FORMATS:
        case PIPE_CAP_SM3:
            return is_r500 ? 1 : 0;

        /* Unsupported features. */
        case PIPE_CAP_TIMER_QUERY:
        case PIPE_CAP_MAX_DUAL_SOURCE_RENDER_TARGETS:
        case PIPE_CAP_INDEP_BLEND_ENABLE:
        case PIPE_CAP_INDEP_BLEND_FUNC:
        case PIPE_CAP_DEPTH_CLIP_DISABLE:
        case PIPE_CAP_DEPTHSTENCIL_CLEAR_SEPARATE:
        case PIPE_CAP_SHADER_STENCIL_EXPORT:
        case PIPE_CAP_MAX_TEXTURE_ARRAY_LAYERS:
        case PIPE_CAP_TGSI_INSTANCEID:
        case PIPE_CAP_TGSI_FS_COORD_ORIGIN_LOWER_LEFT:
        case PIPE_CAP_TGSI_FS_COORD_PIXEL_CENTER_INTEGER:
        case PIPE_CAP_SEAMLESS_CUBE_MAP:
        case PIPE_CAP_SEAMLESS_CUBE_MAP_PER_TEXTURE:
        case PIPE_CAP_SCALED_RESOLVE:
        case PIPE_CAP_MIN_TEXEL_OFFSET:
        case PIPE_CAP_MAX_TEXEL_OFFSET:
        case PIPE_CAP_MAX_STREAM_OUTPUT_BUFFERS:
        case PIPE_CAP_MAX_STREAM_OUTPUT_SEPARATE_COMPONENTS:
        case PIPE_CAP_MAX_STREAM_OUTPUT_INTERLEAVED_COMPONENTS:
        case PIPE_CAP_STREAM_OUTPUT_PAUSE_RESUME:
        case PIPE_CAP_FRAGMENT_COLOR_CLAMPED:
        case PIPE_CAP_QUADS_FOLLOW_PROVOKING_VERTEX_CONVENTION:
        case PIPE_CAP_COMPUTE:
        case PIPE_CAP_START_INSTANCE:
        case PIPE_CAP_QUERY_TIMESTAMP:
            return 0;

        /* SWTCL-only features. */
        case PIPE_CAP_PRIMITIVE_RESTART:
        case PIPE_CAP_USER_VERTEX_BUFFERS:
            return !r300screen->caps.has_tcl;

        /* HWTCL-only features / limitations. */
        case PIPE_CAP_VERTEX_BUFFER_OFFSET_4BYTE_ALIGNED_ONLY:
        case PIPE_CAP_VERTEX_BUFFER_STRIDE_4BYTE_ALIGNED_ONLY:
        case PIPE_CAP_VERTEX_ELEMENT_SRC_OFFSET_4BYTE_ALIGNED_ONLY:
            return r300screen->caps.has_tcl;

        /* Texturing. */
        case PIPE_CAP_MAX_COMBINED_SAMPLERS:
            return r300screen->caps.num_tex_units;
        case PIPE_CAP_MAX_TEXTURE_2D_LEVELS:
        case PIPE_CAP_MAX_TEXTURE_3D_LEVELS:
        case PIPE_CAP_MAX_TEXTURE_CUBE_LEVELS:
            /* 13 == 4096, 12 == 2048 */
            return is_r500 ? 13 : 12;

        /* Render targets. */
        case PIPE_CAP_MAX_RENDER_TARGETS:
            return 4;
    }
    return 0;
}

static int r300_get_shader_param(struct pipe_screen *pscreen, unsigned shader, enum pipe_shader_cap param)
{
   struct r300_screen* r300screen = r300_screen(pscreen);
   boolean is_r400 = r300screen->caps.is_r400;
   boolean is_r500 = r300screen->caps.is_r500;

   switch (shader) {
    case PIPE_SHADER_FRAGMENT:
        switch (param)
        {
        case PIPE_SHADER_CAP_MAX_INSTRUCTIONS:
            return is_r500 || is_r400 ? 512 : 96;
        case PIPE_SHADER_CAP_MAX_ALU_INSTRUCTIONS:
            return is_r500 || is_r400 ? 512 : 64;
        case PIPE_SHADER_CAP_MAX_TEX_INSTRUCTIONS:
            return is_r500 || is_r400 ? 512 : 32;
        case PIPE_SHADER_CAP_MAX_TEX_INDIRECTIONS:
            return is_r500 ? 511 : 4;
        case PIPE_SHADER_CAP_MAX_CONTROL_FLOW_DEPTH:
            return is_r500 ? 64 : 0; /* Actually unlimited on r500. */
            /* Fragment shader limits. */
        case PIPE_SHADER_CAP_MAX_INPUTS:
            /* 2 colors + 8 texcoords are always supported
             * (minus fog and wpos).
             *
             * R500 has the ability to turn 3rd and 4th color into
             * additional texcoords but there is no two-sided color
             * selection then. However the facing bit can be used instead. */
            return 10;
        case PIPE_SHADER_CAP_MAX_CONSTS:
            return is_r500 ? 256 : 32;
        case PIPE_SHADER_CAP_MAX_CONST_BUFFERS:
            return 1;
        case PIPE_SHADER_CAP_MAX_TEMPS:
            return is_r500 ? 128 : is_r400 ? 64 : 32;
        case PIPE_SHADER_CAP_MAX_PREDS:
            return is_r500 ? 1 : 0;
        case PIPE_SHADER_CAP_MAX_TEXTURE_SAMPLERS:
           return r300screen->caps.num_tex_units;
        case PIPE_SHADER_CAP_MAX_ADDRS:
        case PIPE_SHADER_CAP_TGSI_CONT_SUPPORTED:
        case PIPE_SHADER_CAP_INDIRECT_INPUT_ADDR:
        case PIPE_SHADER_CAP_INDIRECT_OUTPUT_ADDR:
        case PIPE_SHADER_CAP_INDIRECT_TEMP_ADDR:
        case PIPE_SHADER_CAP_INDIRECT_CONST_ADDR:
        case PIPE_SHADER_CAP_SUBROUTINES:
        case PIPE_SHADER_CAP_INTEGERS:
            return 0;
        case PIPE_SHADER_CAP_PREFERRED_IR:
            return PIPE_SHADER_IR_TGSI;
        }
        break;
    case PIPE_SHADER_VERTEX:
        switch (param)
        {
        case PIPE_SHADER_CAP_MAX_TEXTURE_SAMPLERS:
        case PIPE_SHADER_CAP_SUBROUTINES:
            return 0;
        default:;
        }

        if (!r300screen->caps.has_tcl) {
            return draw_get_shader_param(shader, param);
        }

        switch (param)
        {
        case PIPE_SHADER_CAP_MAX_INSTRUCTIONS:
        case PIPE_SHADER_CAP_MAX_ALU_INSTRUCTIONS:
            return is_r500 ? 1024 : 256;
        case PIPE_SHADER_CAP_MAX_CONTROL_FLOW_DEPTH:
            return is_r500 ? 4 : 0; /* For loops; not sure about conditionals. */
        case PIPE_SHADER_CAP_MAX_INPUTS:
            return 16;
        case PIPE_SHADER_CAP_MAX_CONSTS:
            return 256;
        case PIPE_SHADER_CAP_MAX_CONST_BUFFERS:
            return 1;
        case PIPE_SHADER_CAP_MAX_TEMPS:
            return 32;
        case PIPE_SHADER_CAP_MAX_ADDRS:
            return 1; /* XXX guessed */
        case PIPE_SHADER_CAP_MAX_PREDS:
            return is_r500 ? 4 : 0; /* XXX guessed. */
        case PIPE_SHADER_CAP_INDIRECT_CONST_ADDR:
            return 1;
        case PIPE_SHADER_CAP_MAX_TEX_INSTRUCTIONS:
        case PIPE_SHADER_CAP_MAX_TEX_INDIRECTIONS:
        case PIPE_SHADER_CAP_TGSI_CONT_SUPPORTED:
        case PIPE_SHADER_CAP_INDIRECT_INPUT_ADDR:
        case PIPE_SHADER_CAP_INDIRECT_OUTPUT_ADDR:
        case PIPE_SHADER_CAP_INDIRECT_TEMP_ADDR:
        case PIPE_SHADER_CAP_SUBROUTINES:
        case PIPE_SHADER_CAP_INTEGERS:
        case PIPE_SHADER_CAP_MAX_TEXTURE_SAMPLERS:
            return 0;
        case PIPE_SHADER_CAP_PREFERRED_IR:
            return PIPE_SHADER_IR_TGSI;
        }
        break;
    }
    return 0;
}

static float r300_get_paramf(struct pipe_screen* pscreen,
                             enum pipe_capf param)
{
    struct r300_screen* r300screen = r300_screen(pscreen);

    switch (param) {
        case PIPE_CAPF_MAX_LINE_WIDTH:
        case PIPE_CAPF_MAX_LINE_WIDTH_AA:
        case PIPE_CAPF_MAX_POINT_WIDTH:
        case PIPE_CAPF_MAX_POINT_WIDTH_AA:
            /* The maximum dimensions of the colorbuffer are our practical
             * rendering limits. 2048 pixels should be enough for anybody. */
            if (r300screen->caps.is_r500) {
                return 4096.0f;
            } else if (r300screen->caps.is_r400) {
                return 4021.0f;
            } else {
                return 2560.0f;
            }
        case PIPE_CAPF_MAX_TEXTURE_ANISOTROPY:
            return 16.0f;
        case PIPE_CAPF_MAX_TEXTURE_LOD_BIAS:
            return 16.0f;
        case PIPE_CAPF_GUARD_BAND_LEFT:
        case PIPE_CAPF_GUARD_BAND_TOP:
        case PIPE_CAPF_GUARD_BAND_RIGHT:
        case PIPE_CAPF_GUARD_BAND_BOTTOM:
            /* XXX I don't know what these should be but the least we can do is
             * silence the potential error message */
            return 0.0f;
        default:
            debug_printf("r300: Warning: Unknown CAP %d in get_paramf.\n",
                         param);
            return 0.0f;
    }
}

static int r300_get_video_param(struct pipe_screen *screen,
				enum pipe_video_profile profile,
				enum pipe_video_cap param)
{
   switch (param) {
      case PIPE_VIDEO_CAP_SUPPORTED:
         return vl_profile_supported(screen, profile);
      case PIPE_VIDEO_CAP_NPOT_TEXTURES:
         return 0;
      case PIPE_VIDEO_CAP_MAX_WIDTH:
      case PIPE_VIDEO_CAP_MAX_HEIGHT:
         return vl_video_buffer_max_size(screen);
      case PIPE_VIDEO_CAP_PREFERED_FORMAT:
         return PIPE_FORMAT_NV12;
      case PIPE_VIDEO_CAP_PREFERS_INTERLACED:
         return false;
      case PIPE_VIDEO_CAP_SUPPORTS_INTERLACED:
         return false;
      case PIPE_VIDEO_CAP_SUPPORTS_PROGRESSIVE:
         return true;
      default:
         return 0;
   }
}

static boolean r300_is_format_supported(struct pipe_screen* screen,
                                        enum pipe_format format,
                                        enum pipe_texture_target target,
                                        unsigned sample_count,
                                        unsigned usage)
{
    uint32_t retval = 0;
    boolean drm_2_8_0 = r300_screen(screen)->info.drm_minor >= 8;
    boolean is_r500 = r300_screen(screen)->caps.is_r500;
    boolean is_r400 = r300_screen(screen)->caps.is_r400;
    boolean is_color2101010 = format == PIPE_FORMAT_R10G10B10A2_UNORM ||
                              format == PIPE_FORMAT_R10G10B10X2_SNORM ||
                              format == PIPE_FORMAT_B10G10R10A2_UNORM ||
                              format == PIPE_FORMAT_R10SG10SB10SA2U_NORM;
    boolean is_ati1n = format == PIPE_FORMAT_RGTC1_UNORM ||
                       format == PIPE_FORMAT_RGTC1_SNORM ||
                       format == PIPE_FORMAT_LATC1_UNORM ||
                       format == PIPE_FORMAT_LATC1_SNORM;
    boolean is_ati2n = format == PIPE_FORMAT_RGTC2_UNORM ||
                       format == PIPE_FORMAT_RGTC2_SNORM ||
                       format == PIPE_FORMAT_LATC2_UNORM ||
                       format == PIPE_FORMAT_LATC2_SNORM;
    boolean is_x16f_xy16f = format == PIPE_FORMAT_R16_FLOAT ||
                            format == PIPE_FORMAT_R16G16_FLOAT ||
                            format == PIPE_FORMAT_A16_FLOAT ||
                            format == PIPE_FORMAT_L16_FLOAT ||
                            format == PIPE_FORMAT_L16A16_FLOAT ||
                            format == PIPE_FORMAT_I16_FLOAT;
    boolean is_half_float = format == PIPE_FORMAT_R16_FLOAT ||
                            format == PIPE_FORMAT_R16G16_FLOAT ||
                            format == PIPE_FORMAT_R16G16B16_FLOAT ||
                            format == PIPE_FORMAT_R16G16B16A16_FLOAT;

    if (!util_format_is_supported(format, usage))
       return FALSE;

    /* Check multisampling support. */
    switch (sample_count) {
        case 0:
        case 1:
            break;
        case 2:
        case 3:
        case 4:
        case 6:
            return FALSE;
#if 0
            if (usage != PIPE_BIND_RENDER_TARGET ||
                !util_format_is_rgba8_variant(
                    util_format_description(format))) {
                return FALSE;
            }
#endif
            break;
        default:
            return FALSE;
    }

    /* Check sampler format support. */
    if ((usage & PIPE_BIND_SAMPLER_VIEW) &&
        /* ATI1N is r5xx-only. */
        (is_r500 || !is_ati1n) &&
        /* ATI2N is supported on r4xx-r5xx. */
        (is_r400 || is_r500 || !is_ati2n) &&
        /* R16F and RG16F texture support was added in as late as DRM 2.8.0 */
        (drm_2_8_0 || !is_x16f_xy16f) &&
        r300_is_sampler_format_supported(format)) {
        retval |= PIPE_BIND_SAMPLER_VIEW;
    }

    /* Check colorbuffer format support. */
    if ((usage & (PIPE_BIND_RENDER_TARGET |
                  PIPE_BIND_DISPLAY_TARGET |
                  PIPE_BIND_SCANOUT |
                  PIPE_BIND_SHARED)) &&
        /* 2101010 cannot be rendered to on non-r5xx. */
        (!is_color2101010 || (is_r500 && drm_2_8_0)) &&
        r300_is_colorbuffer_format_supported(format)) {
        retval |= usage &
            (PIPE_BIND_RENDER_TARGET |
             PIPE_BIND_DISPLAY_TARGET |
             PIPE_BIND_SCANOUT |
             PIPE_BIND_SHARED);
    }

    /* Check depth-stencil format support. */
    if (usage & PIPE_BIND_DEPTH_STENCIL &&
        r300_is_zs_format_supported(format)) {
        retval |= PIPE_BIND_DEPTH_STENCIL;
    }

    /* Check vertex buffer format support. */
    if (usage & PIPE_BIND_VERTEX_BUFFER) {
        if (r300_screen(screen)->caps.has_tcl) {
            /* Half float is supported on >= R400. */
            if ((is_r400 || is_r500 || !is_half_float) &&
                r300_translate_vertex_data_type(format) != R300_INVALID_FORMAT) {
                retval |= PIPE_BIND_VERTEX_BUFFER;
            }
        } else {
            /* SW TCL */
            if (!util_format_is_pure_integer(format)) {
                retval |= PIPE_BIND_VERTEX_BUFFER;
            }
        }
    }

    /* Transfers are always supported. */
    if (usage & PIPE_BIND_TRANSFER_READ)
        retval |= PIPE_BIND_TRANSFER_READ;
    if (usage & PIPE_BIND_TRANSFER_WRITE)
        retval |= PIPE_BIND_TRANSFER_WRITE;

    return retval == usage;
}

static void r300_destroy_screen(struct pipe_screen* pscreen)
{
    struct r300_screen* r300screen = r300_screen(pscreen);
    struct radeon_winsys *rws = radeon_winsys(pscreen);

    if (rws)
      rws->destroy(rws);

    FREE(r300screen);
}

static void r300_fence_reference(struct pipe_screen *screen,
                                 struct pipe_fence_handle **ptr,
                                 struct pipe_fence_handle *fence)
{
    pb_reference((struct pb_buffer**)ptr,
                             (struct pb_buffer*)fence);
}

static boolean r300_fence_signalled(struct pipe_screen *screen,
                                    struct pipe_fence_handle *fence)
{
    struct radeon_winsys *rws = r300_screen(screen)->rws;
    struct pb_buffer *rfence = (struct pb_buffer*)fence;

    return !rws->buffer_is_busy(rfence, RADEON_USAGE_READWRITE);
}

static boolean r300_fence_finish(struct pipe_screen *screen,
                                 struct pipe_fence_handle *fence,
                                 uint64_t timeout)
{
    struct radeon_winsys *rws = r300_screen(screen)->rws;
    struct pb_buffer *rfence = (struct pb_buffer*)fence;

    if (timeout != PIPE_TIMEOUT_INFINITE) {
        int64_t start_time = os_time_get();

        /* Convert to microseconds. */
        timeout /= 1000;

        /* Wait in a loop. */
        while (rws->buffer_is_busy(rfence, RADEON_USAGE_READWRITE)) {
            if (os_time_get() - start_time >= timeout) {
                return FALSE;
            }
            os_time_sleep(10);
        }
        return TRUE;
    }

    rws->buffer_wait(rfence, RADEON_USAGE_READWRITE);
    return TRUE;
}

struct pipe_screen* r300_screen_create(struct radeon_winsys *rws)
{
    struct r300_screen *r300screen = CALLOC_STRUCT(r300_screen);

    if (!r300screen) {
        FREE(r300screen);
        return NULL;
    }

    rws->query_info(rws, &r300screen->info);

    r300_init_debug(r300screen);
    r300_parse_chipset(r300screen->info.pci_id, &r300screen->caps);

    if (SCREEN_DBG_ON(r300screen, DBG_NO_ZMASK))
        r300screen->caps.zmask_ram = 0;
    if (SCREEN_DBG_ON(r300screen, DBG_NO_HIZ))
        r300screen->caps.hiz_ram = 0;

    if (r300screen->info.drm_minor < 8)
        r300screen->caps.has_us_format = FALSE;

    r300screen->rws = rws;
    r300screen->screen.destroy = r300_destroy_screen;
    r300screen->screen.get_name = r300_get_name;
    r300screen->screen.get_vendor = r300_get_vendor;
    r300screen->screen.get_param = r300_get_param;
    r300screen->screen.get_shader_param = r300_get_shader_param;
    r300screen->screen.get_paramf = r300_get_paramf;
    r300screen->screen.get_video_param = r300_get_video_param;
    r300screen->screen.is_format_supported = r300_is_format_supported;
    r300screen->screen.is_video_format_supported = vl_video_buffer_is_format_supported;
    r300screen->screen.context_create = r300_create_context;
    r300screen->screen.fence_reference = r300_fence_reference;
    r300screen->screen.fence_signalled = r300_fence_signalled;
    r300screen->screen.fence_finish = r300_fence_finish;

    r300_init_screen_resource_functions(r300screen);

    util_format_s3tc_init();

    return &r300screen->screen;
}
