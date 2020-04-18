/**********************************************************
 * Copyright 2011 VMware, Inc.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 **********************************************************/


#include "pipe/p_format.h"
#include "util/u_debug.h"
#include "util/u_memory.h"

#include "svga_winsys.h"
#include "svga_screen.h"
#include "svga_format.h"


/*
 * Translate from gallium format to SVGA3D format.
 */
SVGA3dSurfaceFormat
svga_translate_format(struct svga_screen *ss,
                      enum pipe_format format,
                      unsigned bind)
{
   switch(format) {

   case PIPE_FORMAT_B8G8R8A8_UNORM:
      return SVGA3D_A8R8G8B8;
   case PIPE_FORMAT_B8G8R8X8_UNORM:
      return SVGA3D_X8R8G8B8;

   /* sRGB required for GL2.1 */
   case PIPE_FORMAT_B8G8R8A8_SRGB:
      return SVGA3D_A8R8G8B8;
   case PIPE_FORMAT_DXT1_SRGB:
   case PIPE_FORMAT_DXT1_SRGBA:
      return SVGA3D_DXT1;
   case PIPE_FORMAT_DXT3_SRGBA:
      return SVGA3D_DXT3;
   case PIPE_FORMAT_DXT5_SRGBA:
      return SVGA3D_DXT5;

   case PIPE_FORMAT_B5G6R5_UNORM:
      return SVGA3D_R5G6B5;
   case PIPE_FORMAT_B5G5R5A1_UNORM:
      return SVGA3D_A1R5G5B5;
   case PIPE_FORMAT_B4G4R4A4_UNORM:
      return SVGA3D_A4R4G4B4;

   case PIPE_FORMAT_Z16_UNORM:
      return bind & PIPE_BIND_SAMPLER_VIEW ? ss->depth.z16 : SVGA3D_Z_D16;
   case PIPE_FORMAT_S8_UINT_Z24_UNORM:
      return bind & PIPE_BIND_SAMPLER_VIEW ? ss->depth.s8z24 : SVGA3D_Z_D24S8;
   case PIPE_FORMAT_X8Z24_UNORM:
      return bind & PIPE_BIND_SAMPLER_VIEW ? ss->depth.x8z24 : SVGA3D_Z_D24X8;

   case PIPE_FORMAT_A8_UNORM:
      return SVGA3D_ALPHA8;
   case PIPE_FORMAT_L8_UNORM:
      return SVGA3D_LUMINANCE8;

   case PIPE_FORMAT_DXT1_RGB:
   case PIPE_FORMAT_DXT1_RGBA:
      return SVGA3D_DXT1;
   case PIPE_FORMAT_DXT3_RGBA:
      return SVGA3D_DXT3;
   case PIPE_FORMAT_DXT5_RGBA:
      return SVGA3D_DXT5;

   /* Float formats (only 1, 2 and 4-component formats supported) */
   case PIPE_FORMAT_R32_FLOAT:
      return SVGA3D_R_S23E8;
   case PIPE_FORMAT_R32G32_FLOAT:
      return SVGA3D_RG_S23E8;
   case PIPE_FORMAT_R32G32B32A32_FLOAT:
      return SVGA3D_ARGB_S23E8;
   case PIPE_FORMAT_R16_FLOAT:
      return SVGA3D_R_S10E5;
   case PIPE_FORMAT_R16G16_FLOAT:
      return SVGA3D_RG_S10E5;
   case PIPE_FORMAT_R16G16B16A16_FLOAT:
      return SVGA3D_ARGB_S10E5;

   case PIPE_FORMAT_Z32_UNORM:
      /* SVGA3D_Z_D32 is not yet unsupported */
      /* fall-through */
   default:
      return SVGA3D_FORMAT_INVALID;
   }
}


/*
 * Format capability description entry.
 */
struct format_cap {
   SVGA3dSurfaceFormat format;

   /*
    * Capability index corresponding to the format.
    */
   SVGA3dDevCapIndex index;

   /*
    * Mask of supported SVGA3dFormatOp operations, to be inferred when the
    * capability is not explicitly present.
    */
   uint32 defaultOperations;
};


/*
 * Format capability description table.
 *
 * Ordererd by increasing SVGA3dSurfaceFormat value, but with gaps.
 */
static const struct format_cap format_cap_table[] = {
   {
      SVGA3D_X8R8G8B8,
      SVGA3D_DEVCAP_SURFACEFMT_X8R8G8B8,
      SVGA3DFORMAT_OP_TEXTURE |
      SVGA3DFORMAT_OP_CUBETEXTURE |
      SVGA3DFORMAT_OP_VOLUMETEXTURE |
      SVGA3DFORMAT_OP_DISPLAYMODE |
      SVGA3DFORMAT_OP_3DACCELERATION |
      SVGA3DFORMAT_OP_CONVERT_TO_ARGB |
      SVGA3DFORMAT_OP_MEMBEROFGROUP_ARGB |
      SVGA3DFORMAT_OP_OFFSCREENPLAIN |
      SVGA3DFORMAT_OP_SRGBREAD |
      SVGA3DFORMAT_OP_SRGBWRITE |
      SVGA3DFORMAT_OP_SAME_FORMAT_RENDERTARGET |
      SVGA3DFORMAT_OP_OFFSCREEN_RENDERTARGET
   },
   {
      SVGA3D_A8R8G8B8,
      SVGA3D_DEVCAP_SURFACEFMT_A8R8G8B8,
      SVGA3DFORMAT_OP_TEXTURE |
      SVGA3DFORMAT_OP_CUBETEXTURE |
      SVGA3DFORMAT_OP_VOLUMETEXTURE |
      SVGA3DFORMAT_OP_CONVERT_TO_ARGB |
      SVGA3DFORMAT_OP_MEMBEROFGROUP_ARGB |
      SVGA3DFORMAT_OP_OFFSCREENPLAIN |
      SVGA3DFORMAT_OP_SRGBREAD |
      SVGA3DFORMAT_OP_SRGBWRITE |
      SVGA3DFORMAT_OP_SAME_FORMAT_RENDERTARGET |
      SVGA3DFORMAT_OP_SAME_FORMAT_UP_TO_ALPHA_RENDERTARGET |
      SVGA3DFORMAT_OP_OFFSCREEN_RENDERTARGET
   },
   {
      SVGA3D_R5G6B5,
      SVGA3D_DEVCAP_SURFACEFMT_R5G6B5,
      SVGA3DFORMAT_OP_TEXTURE |
      SVGA3DFORMAT_OP_CUBETEXTURE |
      SVGA3DFORMAT_OP_VOLUMETEXTURE |
      SVGA3DFORMAT_OP_DISPLAYMODE |
      SVGA3DFORMAT_OP_3DACCELERATION |
      SVGA3DFORMAT_OP_CONVERT_TO_ARGB |
      SVGA3DFORMAT_OP_MEMBEROFGROUP_ARGB |
      SVGA3DFORMAT_OP_OFFSCREENPLAIN |
      SVGA3DFORMAT_OP_SRGBREAD |
      SVGA3DFORMAT_OP_SRGBWRITE |
      SVGA3DFORMAT_OP_SAME_FORMAT_RENDERTARGET |
      SVGA3DFORMAT_OP_OFFSCREEN_RENDERTARGET
   },
   {
      SVGA3D_X1R5G5B5,
      SVGA3D_DEVCAP_SURFACEFMT_X1R5G5B5,
      SVGA3DFORMAT_OP_TEXTURE |
      SVGA3DFORMAT_OP_CUBETEXTURE |
      SVGA3DFORMAT_OP_VOLUMETEXTURE |
      SVGA3DFORMAT_OP_CONVERT_TO_ARGB |
      SVGA3DFORMAT_OP_MEMBEROFGROUP_ARGB |
      SVGA3DFORMAT_OP_OFFSCREENPLAIN |
      SVGA3DFORMAT_OP_SRGBREAD |
      SVGA3DFORMAT_OP_SRGBWRITE |
      SVGA3DFORMAT_OP_SAME_FORMAT_RENDERTARGET |
      SVGA3DFORMAT_OP_OFFSCREEN_RENDERTARGET
   },
   {
      SVGA3D_A1R5G5B5,
      SVGA3D_DEVCAP_SURFACEFMT_A1R5G5B5,
      SVGA3DFORMAT_OP_TEXTURE |
      SVGA3DFORMAT_OP_CUBETEXTURE |
      SVGA3DFORMAT_OP_VOLUMETEXTURE |
      SVGA3DFORMAT_OP_CONVERT_TO_ARGB |
      SVGA3DFORMAT_OP_MEMBEROFGROUP_ARGB |
      SVGA3DFORMAT_OP_OFFSCREENPLAIN |
      SVGA3DFORMAT_OP_SRGBREAD |
      SVGA3DFORMAT_OP_SRGBWRITE |
      SVGA3DFORMAT_OP_SAME_FORMAT_RENDERTARGET |
      SVGA3DFORMAT_OP_SAME_FORMAT_UP_TO_ALPHA_RENDERTARGET |
      SVGA3DFORMAT_OP_OFFSCREEN_RENDERTARGET
   },
   {
      SVGA3D_A4R4G4B4,
      SVGA3D_DEVCAP_SURFACEFMT_A4R4G4B4,
      SVGA3DFORMAT_OP_TEXTURE |
      SVGA3DFORMAT_OP_CUBETEXTURE |
      SVGA3DFORMAT_OP_VOLUMETEXTURE |
      SVGA3DFORMAT_OP_OFFSCREENPLAIN |
      SVGA3DFORMAT_OP_SRGBREAD |
      SVGA3DFORMAT_OP_SRGBWRITE |
      SVGA3DFORMAT_OP_SAME_FORMAT_RENDERTARGET |
      SVGA3DFORMAT_OP_SAME_FORMAT_UP_TO_ALPHA_RENDERTARGET |
      SVGA3DFORMAT_OP_OFFSCREEN_RENDERTARGET
   },
   /*
    * SVGA3D_Z_D32 is not yet supported, and has no corresponding
    * SVGA3D_DEVCAP_xxx.
    */
   {
      SVGA3D_Z_D16,
      SVGA3D_DEVCAP_SURFACEFMT_Z_D16,
      SVGA3DFORMAT_OP_ZSTENCIL |
      SVGA3DFORMAT_OP_ZSTENCIL_WITH_ARBITRARY_COLOR_DEPTH
   },
   {
      SVGA3D_Z_D24S8,
      SVGA3D_DEVCAP_SURFACEFMT_Z_D24S8,
      SVGA3DFORMAT_OP_ZSTENCIL |
      SVGA3DFORMAT_OP_ZSTENCIL_WITH_ARBITRARY_COLOR_DEPTH
   },
   {
      SVGA3D_Z_D15S1,
      SVGA3D_DEVCAP_MAX,
      SVGA3DFORMAT_OP_ZSTENCIL |
      SVGA3DFORMAT_OP_ZSTENCIL_WITH_ARBITRARY_COLOR_DEPTH
   },
   {
      SVGA3D_LUMINANCE8,
      SVGA3D_DEVCAP_SURFACEFMT_LUMINANCE8,
      SVGA3DFORMAT_OP_TEXTURE |
      SVGA3DFORMAT_OP_CUBETEXTURE |
      SVGA3DFORMAT_OP_VOLUMETEXTURE |
      SVGA3DFORMAT_OP_OFFSCREENPLAIN
   },
   {
      SVGA3D_LUMINANCE8_ALPHA8,
      SVGA3D_DEVCAP_SURFACEFMT_LUMINANCE8_ALPHA8,
      SVGA3DFORMAT_OP_TEXTURE |
      SVGA3DFORMAT_OP_CUBETEXTURE |
      SVGA3DFORMAT_OP_VOLUMETEXTURE |
      SVGA3DFORMAT_OP_OFFSCREENPLAIN
   },
   /*
    * SVGA3D_LUMINANCE4_ALPHA4 is not supported, and has no corresponding
    * SVGA3D_DEVCAP_xxx.
    */
   {
      SVGA3D_LUMINANCE16,
      SVGA3D_DEVCAP_SURFACEFMT_LUMINANCE16,
      SVGA3DFORMAT_OP_TEXTURE |
      SVGA3DFORMAT_OP_CUBETEXTURE |
      SVGA3DFORMAT_OP_VOLUMETEXTURE |
      SVGA3DFORMAT_OP_OFFSCREENPLAIN
   },
   {
      SVGA3D_DXT1,
      SVGA3D_DEVCAP_SURFACEFMT_DXT1,
      SVGA3DFORMAT_OP_TEXTURE |
      SVGA3DFORMAT_OP_SRGBREAD |
      SVGA3DFORMAT_OP_CUBETEXTURE |
      SVGA3DFORMAT_OP_OFFSCREENPLAIN
   },
   {
      SVGA3D_DXT2,
      SVGA3D_DEVCAP_SURFACEFMT_DXT2,
      SVGA3DFORMAT_OP_TEXTURE |
      SVGA3DFORMAT_OP_SRGBREAD |
      SVGA3DFORMAT_OP_CUBETEXTURE |
      SVGA3DFORMAT_OP_OFFSCREENPLAIN
   },
   {
      SVGA3D_DXT3,
      SVGA3D_DEVCAP_SURFACEFMT_DXT3,
      SVGA3DFORMAT_OP_TEXTURE |
      SVGA3DFORMAT_OP_SRGBREAD |
      SVGA3DFORMAT_OP_CUBETEXTURE |
      SVGA3DFORMAT_OP_OFFSCREENPLAIN
   },
   {
      SVGA3D_DXT4,
      SVGA3D_DEVCAP_SURFACEFMT_DXT4,
      SVGA3DFORMAT_OP_TEXTURE |
      SVGA3DFORMAT_OP_SRGBREAD |
      SVGA3DFORMAT_OP_CUBETEXTURE |
      SVGA3DFORMAT_OP_OFFSCREENPLAIN
   },
   {
      SVGA3D_DXT5,
      SVGA3D_DEVCAP_SURFACEFMT_DXT5,
      SVGA3DFORMAT_OP_TEXTURE |
      SVGA3DFORMAT_OP_SRGBREAD |
      SVGA3DFORMAT_OP_CUBETEXTURE |
      SVGA3DFORMAT_OP_OFFSCREENPLAIN
   },
   {
      SVGA3D_BUMPU8V8,
      SVGA3D_DEVCAP_SURFACEFMT_BUMPU8V8,
      SVGA3DFORMAT_OP_TEXTURE |
      SVGA3DFORMAT_OP_CUBETEXTURE |
      SVGA3DFORMAT_OP_VOLUMETEXTURE |
      SVGA3DFORMAT_OP_BUMPMAP |
      SVGA3DFORMAT_OP_OFFSCREENPLAIN
   },
   /*
    * SVGA3D_BUMPL6V5U5 is unsupported; it has no corresponding
    * SVGA3D_DEVCAP_xxx.
    */
   {
      SVGA3D_BUMPX8L8V8U8,
      SVGA3D_DEVCAP_SURFACEFMT_BUMPX8L8V8U8,
      SVGA3DFORMAT_OP_TEXTURE |
      SVGA3DFORMAT_OP_CUBETEXTURE |
      SVGA3DFORMAT_OP_BUMPMAP |
      SVGA3DFORMAT_OP_OFFSCREENPLAIN
   },
   /*
    * SVGA3D_BUMPL8V8U8 is unsupported; it has no corresponding
    * SVGA3D_DEVCAP_xxx. SVGA3D_BUMPX8L8V8U8 should be used instead.
    */
   {
      SVGA3D_ARGB_S10E5,
      SVGA3D_DEVCAP_SURFACEFMT_ARGB_S10E5,
      SVGA3DFORMAT_OP_TEXTURE |
      SVGA3DFORMAT_OP_CUBETEXTURE |
      SVGA3DFORMAT_OP_VOLUMETEXTURE |
      SVGA3DFORMAT_OP_OFFSCREENPLAIN |
      SVGA3DFORMAT_OP_SRGBREAD |
      SVGA3DFORMAT_OP_SRGBWRITE |
      SVGA3DFORMAT_OP_SAME_FORMAT_RENDERTARGET |
      SVGA3DFORMAT_OP_OFFSCREEN_RENDERTARGET
   },
   {
      SVGA3D_ARGB_S23E8,
      SVGA3D_DEVCAP_SURFACEFMT_ARGB_S23E8,
      SVGA3DFORMAT_OP_TEXTURE |
      SVGA3DFORMAT_OP_CUBETEXTURE |
      SVGA3DFORMAT_OP_VOLUMETEXTURE |
      SVGA3DFORMAT_OP_OFFSCREENPLAIN |
      SVGA3DFORMAT_OP_SRGBREAD |
      SVGA3DFORMAT_OP_SRGBWRITE |
      SVGA3DFORMAT_OP_SAME_FORMAT_RENDERTARGET |
      SVGA3DFORMAT_OP_OFFSCREEN_RENDERTARGET
   },
   {
      SVGA3D_A2R10G10B10,
      SVGA3D_DEVCAP_SURFACEFMT_A2R10G10B10,
      SVGA3DFORMAT_OP_TEXTURE |
      SVGA3DFORMAT_OP_CUBETEXTURE |
      SVGA3DFORMAT_OP_VOLUMETEXTURE |
      SVGA3DFORMAT_OP_CONVERT_TO_ARGB |
      SVGA3DFORMAT_OP_MEMBEROFGROUP_ARGB |
      SVGA3DFORMAT_OP_OFFSCREENPLAIN |
      SVGA3DFORMAT_OP_SRGBREAD |
      SVGA3DFORMAT_OP_SRGBWRITE |
      SVGA3DFORMAT_OP_SAME_FORMAT_RENDERTARGET |
      SVGA3DFORMAT_OP_OFFSCREEN_RENDERTARGET
   },
   /*
    * SVGA3D_V8U8 is unsupported; it has no corresponding
    * SVGA3D_DEVCAP_xxx. SVGA3D_BUMPU8V8 should be used instead.
    */
   {
      SVGA3D_Q8W8V8U8,
      SVGA3D_DEVCAP_SURFACEFMT_Q8W8V8U8,
      SVGA3DFORMAT_OP_TEXTURE |
      SVGA3DFORMAT_OP_CUBETEXTURE |
      SVGA3DFORMAT_OP_BUMPMAP |
      SVGA3DFORMAT_OP_OFFSCREENPLAIN
   },
   {
      SVGA3D_CxV8U8,
      SVGA3D_DEVCAP_SURFACEFMT_CxV8U8,
      SVGA3DFORMAT_OP_TEXTURE |
      SVGA3DFORMAT_OP_BUMPMAP |
      SVGA3DFORMAT_OP_OFFSCREENPLAIN
   },
   /*
    * SVGA3D_X8L8V8U8 is unsupported; it has no corresponding
    * SVGA3D_DEVCAP_xxx. SVGA3D_BUMPX8L8V8U8 should be used instead.
    */
   {
      SVGA3D_A2W10V10U10,
      SVGA3D_DEVCAP_SURFACEFMT_A2W10V10U10,
      SVGA3DFORMAT_OP_TEXTURE |
      SVGA3DFORMAT_OP_BUMPMAP |
      SVGA3DFORMAT_OP_OFFSCREENPLAIN
   },
   {
      SVGA3D_ALPHA8,
      SVGA3D_DEVCAP_SURFACEFMT_ALPHA8,
      SVGA3DFORMAT_OP_TEXTURE |
      SVGA3DFORMAT_OP_CUBETEXTURE |
      SVGA3DFORMAT_OP_VOLUMETEXTURE |
      SVGA3DFORMAT_OP_OFFSCREENPLAIN
   },
   {
      SVGA3D_R_S10E5,
      SVGA3D_DEVCAP_SURFACEFMT_R_S10E5,
      SVGA3DFORMAT_OP_TEXTURE |
      SVGA3DFORMAT_OP_VOLUMETEXTURE |
      SVGA3DFORMAT_OP_CUBETEXTURE |
      SVGA3DFORMAT_OP_OFFSCREENPLAIN |
      SVGA3DFORMAT_OP_SRGBREAD |
      SVGA3DFORMAT_OP_SRGBWRITE |
      SVGA3DFORMAT_OP_SAME_FORMAT_RENDERTARGET |
      SVGA3DFORMAT_OP_OFFSCREEN_RENDERTARGET
   },
   {
      SVGA3D_R_S23E8,
      SVGA3D_DEVCAP_SURFACEFMT_R_S23E8,
      SVGA3DFORMAT_OP_TEXTURE |
      SVGA3DFORMAT_OP_VOLUMETEXTURE |
      SVGA3DFORMAT_OP_CUBETEXTURE |
      SVGA3DFORMAT_OP_OFFSCREENPLAIN |
      SVGA3DFORMAT_OP_SRGBREAD |
      SVGA3DFORMAT_OP_SRGBWRITE |
      SVGA3DFORMAT_OP_SAME_FORMAT_RENDERTARGET |
      SVGA3DFORMAT_OP_OFFSCREEN_RENDERTARGET
   },
   {
      SVGA3D_RG_S10E5,
      SVGA3D_DEVCAP_SURFACEFMT_RG_S10E5,
      SVGA3DFORMAT_OP_TEXTURE |
      SVGA3DFORMAT_OP_VOLUMETEXTURE |
      SVGA3DFORMAT_OP_CUBETEXTURE |
      SVGA3DFORMAT_OP_OFFSCREENPLAIN |
      SVGA3DFORMAT_OP_SRGBREAD |
      SVGA3DFORMAT_OP_SRGBWRITE |
      SVGA3DFORMAT_OP_SAME_FORMAT_RENDERTARGET |
      SVGA3DFORMAT_OP_OFFSCREEN_RENDERTARGET
   },
   {
      SVGA3D_RG_S23E8,
      SVGA3D_DEVCAP_SURFACEFMT_RG_S23E8,
      SVGA3DFORMAT_OP_TEXTURE |
      SVGA3DFORMAT_OP_VOLUMETEXTURE |
      SVGA3DFORMAT_OP_CUBETEXTURE |
      SVGA3DFORMAT_OP_OFFSCREENPLAIN |
      SVGA3DFORMAT_OP_SRGBREAD |
      SVGA3DFORMAT_OP_SRGBWRITE |
      SVGA3DFORMAT_OP_SAME_FORMAT_RENDERTARGET |
      SVGA3DFORMAT_OP_OFFSCREEN_RENDERTARGET
   },
   /*
    * SVGA3D_BUFFER is a placeholder format for index/vertex buffers.
    */
   {
      SVGA3D_Z_D24X8,
      SVGA3D_DEVCAP_SURFACEFMT_Z_D24X8,
      SVGA3DFORMAT_OP_ZSTENCIL |
      SVGA3DFORMAT_OP_ZSTENCIL_WITH_ARBITRARY_COLOR_DEPTH
   },
   {
      SVGA3D_V16U16,
      SVGA3D_DEVCAP_SURFACEFMT_V16U16,
      SVGA3DFORMAT_OP_TEXTURE |
      SVGA3DFORMAT_OP_CUBETEXTURE |
      SVGA3DFORMAT_OP_VOLUMETEXTURE |
      SVGA3DFORMAT_OP_BUMPMAP |
      SVGA3DFORMAT_OP_OFFSCREENPLAIN
   },
   {
      SVGA3D_G16R16,
      SVGA3D_DEVCAP_SURFACEFMT_G16R16,
      SVGA3DFORMAT_OP_TEXTURE |
      SVGA3DFORMAT_OP_CUBETEXTURE |
      SVGA3DFORMAT_OP_VOLUMETEXTURE |
      SVGA3DFORMAT_OP_OFFSCREENPLAIN |
      SVGA3DFORMAT_OP_SAME_FORMAT_RENDERTARGET |
      SVGA3DFORMAT_OP_OFFSCREEN_RENDERTARGET
   },
   {
      SVGA3D_A16B16G16R16,
      SVGA3D_DEVCAP_SURFACEFMT_A16B16G16R16,
      SVGA3DFORMAT_OP_TEXTURE |
      SVGA3DFORMAT_OP_CUBETEXTURE |
      SVGA3DFORMAT_OP_VOLUMETEXTURE |
      SVGA3DFORMAT_OP_OFFSCREENPLAIN |
      SVGA3DFORMAT_OP_SAME_FORMAT_RENDERTARGET |
      SVGA3DFORMAT_OP_OFFSCREEN_RENDERTARGET
   },
   {
      SVGA3D_UYVY,
      SVGA3D_DEVCAP_SURFACEFMT_UYVY,
      0
   },
   {
      SVGA3D_YUY2,
      SVGA3D_DEVCAP_SURFACEFMT_YUY2,
      0
   },
   {
      SVGA3D_NV12,
      SVGA3D_DEVCAP_SURFACEFMT_NV12,
      0
   },
   {
      SVGA3D_AYUV,
      SVGA3D_DEVCAP_SURFACEFMT_AYUV,
      0
   },
   {
      SVGA3D_BC4_UNORM,
      SVGA3D_DEVCAP_SURFACEFMT_BC4_UNORM,
      0
   },
   {
      SVGA3D_BC5_UNORM,
      SVGA3D_DEVCAP_SURFACEFMT_BC5_UNORM,
      0
   },
   {
      SVGA3D_Z_DF16,
      SVGA3D_DEVCAP_SURFACEFMT_Z_DF16,
      0
   },
   {
      SVGA3D_Z_DF24,
      SVGA3D_DEVCAP_SURFACEFMT_Z_DF24,
      0
   },
   {
      SVGA3D_Z_D24S8_INT,
      SVGA3D_DEVCAP_SURFACEFMT_Z_D24S8_INT,
      0
   },
};


/*
 * Get format capabilities from the host.  It takes in consideration
 * deprecated/unsupported formats, and formats which are implicitely assumed to
 * be supported when the host does not provide an explicit capability entry.
 */
void
svga_get_format_cap(struct svga_screen *ss,
                    SVGA3dSurfaceFormat format,
                    SVGA3dSurfaceFormatCaps *caps)
{
   const struct format_cap *entry;

   for (entry = format_cap_table; entry < format_cap_table + Elements(format_cap_table); ++entry) {
      if (entry->format == format) {
         struct svga_winsys_screen *sws = ss->sws;
         SVGA3dDevCapResult result;

         if (sws->get_cap(sws, entry->index, &result)) {
            /* Explicitly advertised format */
            caps->value = result.u;
         } else {
            /* Implicitly advertised format -- use default caps */
            caps->value = entry->defaultOperations;
         }

         return;
      }
   }

   /* Unsupported format */
   caps->value = 0;
}


/**
 * Return block size and bytes per block for the given SVGA3D format.
 * block_width and block_height are one for uncompressed formats and
 * greater than one for compressed formats.
 * Note: we don't handle formats that are unsupported, according to
 * the format_cap_table above.
 */
void
svga_format_size(SVGA3dSurfaceFormat format,
                 unsigned *block_width,
                 unsigned *block_height,
                 unsigned *bytes_per_block)
{
   *block_width = *block_height = 1;

   switch (format) {
   case SVGA3D_X8R8G8B8:
   case SVGA3D_A8R8G8B8:
      *bytes_per_block = 4;
      return;

   case SVGA3D_R5G6B5:
   case SVGA3D_X1R5G5B5:
   case SVGA3D_A1R5G5B5:
   case SVGA3D_A4R4G4B4:
      *bytes_per_block = 2;
      return;

   case SVGA3D_Z_D32:
      *bytes_per_block = 4;
      return;

   case SVGA3D_Z_D16:
      *bytes_per_block = 2;
      return;

   case SVGA3D_Z_D24S8:
      *bytes_per_block = 4;
      return;

   case SVGA3D_Z_D15S1:
      *bytes_per_block = 2;
      return;

   case SVGA3D_LUMINANCE8:
   case SVGA3D_LUMINANCE4_ALPHA4:
      *bytes_per_block = 1;
      return;

   case SVGA3D_LUMINANCE16:
   case SVGA3D_LUMINANCE8_ALPHA8:
      *bytes_per_block = 2;
      return;

   case SVGA3D_DXT1:
   case SVGA3D_DXT2:
      *block_width = *block_height = 4;
      *bytes_per_block = 8;
      return;

   case SVGA3D_DXT3:
   case SVGA3D_DXT4:
   case SVGA3D_DXT5:
      *block_width = *block_height = 4;
      *bytes_per_block = 16;
      return;

   case SVGA3D_BUMPU8V8:
   case SVGA3D_BUMPL6V5U5:
      *bytes_per_block = 2;
      return;

   case SVGA3D_BUMPX8L8V8U8:
      *bytes_per_block = 4;
      return;

   case SVGA3D_ARGB_S10E5:
      *bytes_per_block = 8;
      return;

   case SVGA3D_ARGB_S23E8:
      *bytes_per_block = 16;
      return;

   case SVGA3D_A2R10G10B10:
      *bytes_per_block = 4;
      return;

   case SVGA3D_Q8W8V8U8:
      *bytes_per_block = 4;
      return;

   case SVGA3D_CxV8U8:
      *bytes_per_block = 2;
      return;

   case SVGA3D_X8L8V8U8:
   case SVGA3D_A2W10V10U10:
      *bytes_per_block = 4;
      return;

   case SVGA3D_ALPHA8:
      *bytes_per_block = 1;
      return;

   case SVGA3D_R_S10E5:
      *bytes_per_block = 2;
      return;
   case SVGA3D_R_S23E8:
      *bytes_per_block = 4;
      return;
   case SVGA3D_RG_S10E5:
      *bytes_per_block = 4;
      return;
   case SVGA3D_RG_S23E8:
      *bytes_per_block = 8;
      return;

   case SVGA3D_BUFFER:
      *bytes_per_block = 1;
      return;

   case SVGA3D_Z_D24X8:
      *bytes_per_block = 4;
      return;

   case SVGA3D_V16U16:
      *bytes_per_block = 4;
      return;

   case SVGA3D_G16R16:
      *bytes_per_block = 4;
      return;

   case SVGA3D_A16B16G16R16:
      *bytes_per_block = 8;
      return;

   case SVGA3D_Z_DF16:
      *bytes_per_block = 2;
      return;
   case SVGA3D_Z_DF24:
      *bytes_per_block = 4;
      return;
   case SVGA3D_Z_D24S8_INT:
      *bytes_per_block = 4;
      return;

   default:
      debug_printf("format %u\n", (unsigned) format);
      assert(!"unexpected format in svga_format_size()");
      *bytes_per_block = 4;
   }
}
