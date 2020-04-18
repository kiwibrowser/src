/**************************************************************************
 *
 * Copyright 2009 VMware, Inc.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#include "vg_translate.h"

#include "pipe/p_format.h"
#include "util/u_pack_color.h"

void _vega_pack_rgba_span_float(struct vg_context *ctx,
                                VGuint n, VGfloat rgba[][4],
                                VGImageFormat dstFormat,
                                void *dstAddr)
{
   VGint i;

   switch (dstFormat) {
   case VG_sRGBX_8888: {
      VGint *dst = (VGint *)dstAddr;
      for (i = 0; i < n; ++i) {
         VGubyte r, g, b ,a;
         r = float_to_ubyte(rgba[i][0]);
         g = float_to_ubyte(rgba[i][1]);
         b = float_to_ubyte(rgba[i][2]);
         a = 255;
         dst[i] = r << 24 | g << 16 | b << 8 | a;
      }
      return;
   }
      break;
   case VG_sRGBA_8888: {
      VGint *dst = (VGint *)dstAddr;
      for (i = 0; i < n; ++i) {
         VGubyte r, g, b ,a;
         r = float_to_ubyte(rgba[i][0]);
         g = float_to_ubyte(rgba[i][1]);
         b = float_to_ubyte(rgba[i][2]);
         a = float_to_ubyte(rgba[i][3]);
         dst[i] = r << 24 | g << 16 | b << 8 | a;
      }
      return;
   }
      break;
   case VG_sRGBA_8888_PRE: {
      VGint *dst = (VGint *)dstAddr;
      for (i = 0; i < n; ++i) {
         VGubyte r, g, b ,a;
         r = float_to_ubyte(rgba[i][0]);
         g = float_to_ubyte(rgba[i][1]);
         b = float_to_ubyte(rgba[i][2]);
         a = float_to_ubyte(rgba[i][3]);
         dst[i] = r << 24 | g << 16 | b << 8 | a;
      }
      return;
   }
      break;
   case VG_sRGB_565: {
      VGshort *dst = (VGshort *)dstAddr;
      for (i = 0; i < n; ++i) {
         VGubyte r, g, b;
         r = float_to_ubyte(rgba[i][0]);
         g = float_to_ubyte(rgba[i][1]);
         b = float_to_ubyte(rgba[i][2]);
         r = (r / 255.0) * 32;
         g = (g / 255.0) * 32;
         b = (b / 255.0) * 32;

         dst[i] = b | g << 5 | r << 11;
      }
      return;
   }
      break;
   case VG_sRGBA_5551: {
      VGshort *dst = (VGshort *)dstAddr;
      for (i = 0; i < n; ++i) {
         VGubyte r, g, b, a;
         r = float_to_ubyte(rgba[i][0]);
         g = float_to_ubyte(rgba[i][1]);
         b = float_to_ubyte(rgba[i][2]);
         a = float_to_ubyte(rgba[i][3]);
         r = (r / 255.0) * 32;
         g = (g / 255.0) * 32;
         b = (b / 255.0) * 32;
         a = (a / 255.0);

         dst[i] =  a | b << 1 | g << 6 | r << 11;
      }
      return;
   }
      break;
   case VG_sRGBA_4444: {
      VGshort *dst = (VGshort *)dstAddr;
      for (i = 0; i < n; ++i) {
         VGubyte r, g, b, a;
         r = float_to_ubyte(rgba[i][0]);
         g = float_to_ubyte(rgba[i][1]);
         b = float_to_ubyte(rgba[i][2]);
         a = float_to_ubyte(rgba[i][3]);
         r = (r / 255.0) * 16;
         g = (g / 255.0) * 16;
         b = (b / 255.0) * 16;
         a = (a / 255.0) * 16;

         dst[i] =  a | b << 4 | g << 8 | r << 12;
      }
      return;
   }
      break;
   case VG_sL_8: {
      VGubyte *dst = (VGubyte *)dstAddr;
      for (i = 0; i < n; ++i) {
         VGubyte a;
         a = float_to_ubyte(rgba[i][3]);

         dst[i] =  a;
      }
      return;
   }
      break;
   case VG_lRGBX_8888: {
      VGint *dst = (VGint *)dstAddr;
      for (i = 0; i < n; ++i) {
         VGubyte r, g, b ,a;
         r = float_to_ubyte(rgba[i][0]);
         g = float_to_ubyte(rgba[i][1]);
         b = float_to_ubyte(rgba[i][2]);
         a = 255;
         dst[i] = r << 24 | g << 16 | b << 8 | a;
      }
      return;
   }
      break;
   case VG_lRGBA_8888: {
      VGint *dst = (VGint *)dstAddr;
      for (i = 0; i < n; ++i) {
         VGubyte r, g, b ,a;
         r = float_to_ubyte(rgba[i][0]);
         g = float_to_ubyte(rgba[i][1]);
         b = float_to_ubyte(rgba[i][2]);
         a = float_to_ubyte(rgba[i][3]);
         dst[i] = r << 24 | g << 16 | b << 8 | a;
      }
      return;
   }
   case VG_lRGBA_8888_PRE: {
      VGint *dst = (VGint *)dstAddr;
      for (i = 0; i < n; ++i) {
         VGubyte r, g, b ,a;
         r = float_to_ubyte(rgba[i][0]);
         g = float_to_ubyte(rgba[i][1]);
         b = float_to_ubyte(rgba[i][2]);
         a = float_to_ubyte(rgba[i][3]);
         dst[i] = r << 24 | g << 16 | b << 8 | a;
      }
      return;
   }
      break;
   case VG_lL_8: {
      VGubyte *dst = (VGubyte *)dstAddr;
      for (i = 0; i < n; ++i) {
         VGubyte a;
         a = float_to_ubyte(rgba[i][3]);
         dst[i] = a;
      }
      return;
   }
      break;
   case VG_A_8: {
      VGubyte *dst = (VGubyte *)dstAddr;
      for (i = 0; i < n; ++i) {
         VGubyte a;
         a = float_to_ubyte(rgba[i][3]);

         dst[i] = a;
      }
      return;
   }
      break;
   case VG_BW_1: {
      VGshort *dst = (VGshort *)dstAddr;
      for (i = 0; i < n; ++i) {
         VGubyte r, g, b, a;
         VGubyte res;
         r = float_to_ubyte(rgba[i][0]);
         g = float_to_ubyte(rgba[i][1]);
         b = float_to_ubyte(rgba[i][2]);
         a = float_to_ubyte(rgba[i][3]);

         res = (r + g + b + a)/4;
         dst[i] =   (res & (128));
      }
      return;
   }
      break;
#ifdef OPENVG_VERSION_1_1
   case VG_A_1: {
      VGshort *dst = (VGshort *)dstAddr;
      for (i = 0; i < n; ++i) {
         VGubyte a;
         a = float_to_ubyte(rgba[i][3]);

         dst[i] =   (a & (128));
      }
      return;
   }
      break;
   case VG_A_4: {
      VGshort *dst = (VGshort *)dstAddr;
      for (i = 0; i < n; ++i) {
         VGubyte a;
         VGubyte res;
         a = float_to_ubyte(rgba[i][3]);

         res = a/4;
         dst[i] =   (res & (128));
      }
      return;
   }
      break;
#endif
   case VG_sXRGB_8888:
      break;
   case VG_sARGB_8888: {
      VGint *dst = (VGint *)dstAddr;
      for (i = 0; i < n; ++i) {
         VGubyte r, g, b ,a;
         r = float_to_ubyte(rgba[i][0]);
         g = float_to_ubyte(rgba[i][1]);
         b = float_to_ubyte(rgba[i][2]);
         a = float_to_ubyte(rgba[i][3]);
         dst[i] = a << 24 | r << 16 | g << 8 | b;
      }
      return;
   }
      break;
   case VG_sARGB_8888_PRE:  {
      VGint *dst = (VGint *)dstAddr;
      for (i = 0; i < n; ++i) {
         VGubyte r, g, b ,a;
         r = float_to_ubyte(rgba[i][0]);
         g = float_to_ubyte(rgba[i][1]);
         b = float_to_ubyte(rgba[i][2]);
         a = float_to_ubyte(rgba[i][3]);
         dst[i] = a << 24 | r << 16 | g << 8 | b;
      }
      return;
   }
      break;
   case VG_sARGB_1555:
      break;
   case VG_sARGB_4444:
      break;
   case VG_lXRGB_8888:
      break;
   case VG_lARGB_8888: {
      VGint *dst = (VGint *)dstAddr;
      for (i = 0; i < n; ++i) {
         VGubyte r, g, b ,a;
         r = float_to_ubyte(rgba[i][0]);
         g = float_to_ubyte(rgba[i][1]);
         b = float_to_ubyte(rgba[i][2]);
         a = float_to_ubyte(rgba[i][3]);
         dst[i] = a << 24 | r << 16 | g << 8 | b;
      }
      return;
   }
      break;
   case VG_lARGB_8888_PRE: {
      VGint *dst = (VGint *)dstAddr;
      for (i = 0; i < n; ++i) {
         VGubyte r, g, b ,a;
         r = float_to_ubyte(rgba[i][0]);
         g = float_to_ubyte(rgba[i][1]);
         b = float_to_ubyte(rgba[i][2]);
         a = float_to_ubyte(rgba[i][3]);
         dst[i] = a << 24 | r << 16 | g << 8 | b;
      }
      return;
   }
      break;
   case VG_sBGRX_8888: {
      VGint *dst = (VGint *)dstAddr;
      for (i = 0; i < n; ++i) {
         VGubyte r, g, b ,a;
         r = float_to_ubyte(rgba[i][0]);
         g = float_to_ubyte(rgba[i][1]);
         b = float_to_ubyte(rgba[i][2]);
         a = 0xff;
         dst[i] = b << 24 | g << 16 | r << 8 | a;
      }
      return;
   }
      break;
   case VG_sBGRA_8888: {
      VGint *dst = (VGint *)dstAddr;
      for (i = 0; i < n; ++i) {
         VGubyte r, g, b ,a;
         r = float_to_ubyte(rgba[i][0]);
         g = float_to_ubyte(rgba[i][1]);
         b = float_to_ubyte(rgba[i][2]);
         a = float_to_ubyte(rgba[i][3]);
         dst[i] = b << 24 | g << 16 | r << 8 | a;
      }
      return;
   }
      break;
   case VG_sBGRA_8888_PRE: {
      VGint *dst = (VGint *)dstAddr;
      for (i = 0; i < n; ++i) {
         VGubyte r, g, b ,a;
         r = float_to_ubyte(rgba[i][0]);
         g = float_to_ubyte(rgba[i][1]);
         b = float_to_ubyte(rgba[i][2]);
         a = float_to_ubyte(rgba[i][3]);
         dst[i] = b << 24 | g << 16 | r << 8 | a;
      }
      return;
   }
      break;
   case VG_sBGR_565:
      break;
   case VG_sBGRA_5551:
      break;
   case VG_sBGRA_4444:
      break;
   case VG_lBGRX_8888: {
      VGint *dst = (VGint *)dstAddr;
      for (i = 0; i < n; ++i) {
         VGubyte r, g, b ,a;
         r = float_to_ubyte(rgba[i][0]);
         g = float_to_ubyte(rgba[i][1]);
         b = float_to_ubyte(rgba[i][2]);
         a = 0xff;
         dst[i] = b << 24 | g << 16 | r << 8 | a;
      }
      return;
   }
      break;
   case VG_lBGRA_8888: {
      VGint *dst = (VGint *)dstAddr;
      for (i = 0; i < n; ++i) {
         VGubyte r, g, b ,a;
         r = float_to_ubyte(rgba[i][0]);
         g = float_to_ubyte(rgba[i][1]);
         b = float_to_ubyte(rgba[i][2]);
         a = float_to_ubyte(rgba[i][3]);
         dst[i] = b << 24 | g << 16 | r << 8 | a;
      }
      return;
   }
      break;
   case VG_lBGRA_8888_PRE: {
      VGint *dst = (VGint *)dstAddr;
      for (i = 0; i < n; ++i) {
         VGubyte r, g, b ,a;
         r = float_to_ubyte(rgba[i][0]);
         g = float_to_ubyte(rgba[i][1]);
         b = float_to_ubyte(rgba[i][2]);
         a = float_to_ubyte(rgba[i][3]);
         dst[i] = b << 24 | g << 16 | r << 8 | a;
      }
      return;
   }
      break;
   case VG_sXBGR_8888:
      break;
   case VG_sABGR_8888: {
      VGint *dst = (VGint *)dstAddr;
      for (i = 0; i < n; ++i) {
         VGubyte r, g, b ,a;
         r = float_to_ubyte(rgba[i][0]);
         g = float_to_ubyte(rgba[i][1]);
         b = float_to_ubyte(rgba[i][2]);
         a = float_to_ubyte(rgba[i][3]);
         dst[i] = a << 24 | b << 16 | g << 8 | r;
      }
      return;
   }
      break;
   case VG_sABGR_8888_PRE: {
      VGint *dst = (VGint *)dstAddr;
      for (i = 0; i < n; ++i) {
         VGubyte r, g, b ,a;
         r = float_to_ubyte(rgba[i][0]);
         g = float_to_ubyte(rgba[i][1]);
         b = float_to_ubyte(rgba[i][2]);
         a = float_to_ubyte(rgba[i][3]);
         dst[i] = a << 24 | b << 16 | g << 8 | r;
      }
      return;
   }
      break;
   case VG_sABGR_1555:
      break;
   case VG_sABGR_4444:
      break;
   case VG_lXBGR_8888:
      break;
   case VG_lABGR_8888: {
      VGint *dst = (VGint *)dstAddr;
      for (i = 0; i < n; ++i) {
         VGubyte r, g, b ,a;
         r = float_to_ubyte(rgba[i][0]);
         g = float_to_ubyte(rgba[i][1]);
         b = float_to_ubyte(rgba[i][2]);
         a = float_to_ubyte(rgba[i][3]);
         dst[i] = a << 24 | b << 16 | g << 8 | r;
      }
      return;
   }
      break;
   case VG_lABGR_8888_PRE: {
      VGint *dst = (VGint *)dstAddr;
      for (i = 0; i < n; ++i) {
         VGubyte r, g, b ,a;
         r = float_to_ubyte(rgba[i][0]);
         g = float_to_ubyte(rgba[i][1]);
         b = float_to_ubyte(rgba[i][2]);
         a = float_to_ubyte(rgba[i][3]);
         dst[i] = a << 24 | b << 16 | g << 8 | r;
      }
      return;
   }
      break;
   default:
      assert(!"Unknown ReadPixels format");
      break;
   }
   assert(!"Not implemented ReadPixels format");
}

void _vega_unpack_float_span_rgba(struct vg_context *ctx,
                                  VGuint n,
                                  VGuint offset,
                                  const void * data,
                                  VGImageFormat dataFormat,
                                  VGfloat rgba[][4])
{
   VGint i;
   union util_color uc;

   switch (dataFormat) {
   case VG_sRGBX_8888: {
      VGuint *src = (VGuint *)data;
      src += offset;
      for (i = 0; i < n; ++i) {
         VGubyte r, g, b ,a;
         r = (*src >> 24) & 0xff;
         g = (*src >> 16) & 0xff;
         b = (*src >>  8) & 0xff;
         a = 0xff;

         util_pack_color_ub(r, g, b, a, PIPE_FORMAT_R32G32B32A32_FLOAT, &uc);
         rgba[i][0] = uc.f[0];
         rgba[i][1] = uc.f[1];
         rgba[i][2] = uc.f[2];
         rgba[i][3] = uc.f[3];
         ++src;
      }
   }
      return;
   case VG_sRGBA_8888: {
      VGuint *src = (VGuint *)data;
      src += offset;
      for (i = 0; i < n; ++i) {
         VGubyte r, g, b ,a;
         r = (*src >> 24) & 0xff;
         g = (*src >> 16) & 0xff;
         b = (*src >>  8) & 0xff;
         a = (*src >>  0) & 0xff;

         util_pack_color_ub(r, g, b, a, PIPE_FORMAT_R32G32B32A32_FLOAT, &uc);
         rgba[i][0] = uc.f[0];
         rgba[i][1] = uc.f[1];
         rgba[i][2] = uc.f[2];
         rgba[i][3] = uc.f[3];
         ++src;
      }
      return;
   }
      break;
   case VG_sRGBA_8888_PRE: {
      VGint *src = (VGint *)data;
      src += offset;
      for (i = 0; i < n; ++i) {
         VGubyte r, g, b ,a;
         r = (*src >> 24) & 0xff;
         g = (*src >> 16) & 0xff;
         b = (*src >>  8) & 0xff;
         a = (*src >>  0) & 0xff;

         util_pack_color_ub(r, g, b, a, PIPE_FORMAT_R32G32B32A32_FLOAT, &uc);
         rgba[i][0] = uc.f[0];
         rgba[i][1] = uc.f[1];
         rgba[i][2] = uc.f[2];
         rgba[i][3] = uc.f[3];
         ++src;
      }
      return;
   }
      break;
   case VG_sRGB_565: {
      VGshort *src = (VGshort *)data;
      src += offset;
      for (i = 0; i < n; ++i) {
         VGfloat clr[4];
         clr[0] = ((*src >> 11) & 31)/31.;
         clr[1] = ((*src >>  5) & 63)/63.;
         clr[2] = ((*src >>  0) & 31)/31.;
         clr[3] = 1.f;

         util_pack_color(clr, PIPE_FORMAT_R32G32B32A32_FLOAT, &uc);
         rgba[i][0] = uc.f[0];
         rgba[i][1] = uc.f[1];
         rgba[i][2] = uc.f[2];
         rgba[i][3] = uc.f[3];
         ++src;
      }
   }
      return;
   case VG_sRGBA_5551: {
      VGshort *src = (VGshort *)data;
      src += offset;
      for (i = 0; i < n; ++i) {
         VGfloat clr[4];
         clr[0] = ((*src >> 10) & 31)/31.;
         clr[1] = ((*src >>  5) & 31)/31.;
         clr[2] = ((*src >>  1) & 31)/31.;
         clr[3] = ((*src >>  0) & 1)/1.;

         util_pack_color(clr, PIPE_FORMAT_R32G32B32A32_FLOAT, &uc);
         rgba[i][0] = uc.f[0];
         rgba[i][1] = uc.f[1];
         rgba[i][2] = uc.f[2];
         rgba[i][3] = uc.f[3];
         ++src;
      }
   }
      return;
   case VG_sRGBA_4444: {
      VGshort *src = (VGshort *)data;
      src += offset;
      for (i = 0; i < n; ++i) {
         VGfloat clr[4];
         clr[0] = ((*src >> 12) & 15)/15.;
         clr[1] = ((*src >>  8) & 15)/15.;
         clr[2] = ((*src >>  4) & 15)/15.;
         clr[3] = ((*src >>  0) & 15)/15.;

         util_pack_color(clr, PIPE_FORMAT_R32G32B32A32_FLOAT, &uc);
         rgba[i][0] = uc.f[0];
         rgba[i][1] = uc.f[1];
         rgba[i][2] = uc.f[2];
         rgba[i][3] = uc.f[3];
         ++src;
      }
   }
      return;
   case VG_sL_8: {
      VGubyte *src = (VGubyte *)data;
      src += offset;
      for (i = 0; i < n; ++i) {
         util_pack_color_ub(0xff, 0xff, 0xff, *src, PIPE_FORMAT_R32G32B32A32_FLOAT, &uc);
         rgba[i][0] = uc.f[0];
         rgba[i][1] = uc.f[1];
         rgba[i][2] = uc.f[2];
         rgba[i][3] = uc.f[3];
         ++src;
      }
   }
      return;
   case VG_lRGBX_8888: {
      VGuint *src = (VGuint *)data;
      src += offset;
      for (i = 0; i < n; ++i) {
         VGubyte r, g, b ,a;
         r = (*src >> 24) & 0xff;
         g = (*src >> 16) & 0xff;
         b = (*src >>  8) & 0xff;
         a = 0xff;

         util_pack_color_ub(r, g, b, a, PIPE_FORMAT_R32G32B32A32_FLOAT, &uc);
         rgba[i][0] = uc.f[0];
         rgba[i][1] = uc.f[1];
         rgba[i][2] = uc.f[2];
         rgba[i][3] = uc.f[3];
         ++src;
      }
   }
      return;
   case VG_lRGBA_8888: {
      VGint *src = (VGint *)data;
      src += offset;
      for (i = 0; i < n; ++i) {
         VGubyte r, g, b ,a;
         r = (*src >> 24) & 0xff;
         g = (*src >> 16) & 0xff;
         b = (*src >>  8) & 0xff;
         a = (*src >>  0) & 0xff;

         util_pack_color_ub(r, g, b, a, PIPE_FORMAT_R32G32B32A32_FLOAT, &uc);
         rgba[i][0] = uc.f[0];
         rgba[i][1] = uc.f[1];
         rgba[i][2] = uc.f[2];
         rgba[i][3] = uc.f[3];
         ++src;
      }
      return;
   }
      break;
   case VG_lRGBA_8888_PRE: {
      VGint *src = (VGint *)data;
      src += offset;
      for (i = 0; i < n; ++i) {
         VGubyte r, g, b ,a;
         r = (*src >> 24) & 0xff;
         g = (*src >> 16) & 0xff;
         b = (*src >>  8) & 0xff;
         a = (*src >>  0) & 0xff;

         util_pack_color_ub(r, g, b, a, PIPE_FORMAT_R32G32B32A32_FLOAT, &uc);
         rgba[i][0] = uc.f[0];
         rgba[i][1] = uc.f[1];
         rgba[i][2] = uc.f[2];
         rgba[i][3] = uc.f[3];
         ++src;
      }
      return;
   }
      break;
   case VG_lL_8: {
      VGubyte *src = (VGubyte *)data;
      src += offset;
      for (i = 0; i < n; ++i) {
         util_pack_color_ub(0xff, 0xff, 0xff, *src, PIPE_FORMAT_R32G32B32A32_FLOAT, &uc);
         rgba[i][0] = uc.f[0];
         rgba[i][1] = uc.f[1];
         rgba[i][2] = uc.f[2];
         rgba[i][3] = uc.f[3];
         ++src;
      }
   }
      return;
   case VG_A_8: {
      VGubyte *src = (VGubyte *)data;
      src += offset;
      for (i = 0; i < n; ++i) {
         util_pack_color_ub(0xff, 0xff, 0xff, *src, PIPE_FORMAT_R32G32B32A32_FLOAT, &uc);
         rgba[i][0] = uc.f[0];
         rgba[i][1] = uc.f[1];
         rgba[i][2] = uc.f[2];
         rgba[i][3] = uc.f[3];
         ++src;
      }
   }
      return;
   case VG_BW_1: {
      VGubyte *src = (VGubyte *)data;
      src += offset;
      for (i = 0; i < n; i += 8) {
         VGfloat clr[4];
         VGint j;
         for (j = 0; j < 8 && j < n ; ++j) {
            VGint shift = j;
            clr[0] = (((*src) & (1<<shift)) >> shift);
            clr[1] = clr[0];
            clr[2] = clr[0];
            clr[3] = 1.f;

            util_pack_color(clr, PIPE_FORMAT_R32G32B32A32_FLOAT, &uc);
            rgba[i+j][0] = uc.f[0];
            rgba[i+j][1] = uc.f[1];
            rgba[i+j][2] = uc.f[2];
            rgba[i+j][3] = uc.f[3];
         }
         ++src;
      }
   }
      return;
#ifdef OPENVG_VERSION_1_1
   case VG_A_1: {
      VGubyte *src = (VGubyte *)data;
      src += offset;
      for (i = 0; i < n; i += 8) {
         VGfloat clr[4];
         VGint j;
         for (j = 0; j < 8 && j < n ; ++j) {
            VGint shift = j;
            clr[0] = 0.f;
            clr[1] = 0.f;
            clr[2] = 0.f;
            clr[3] = (((*src) & (1<<shift)) >> shift);

            util_pack_color(clr, PIPE_FORMAT_R32G32B32A32_FLOAT, &uc);
            rgba[i+j][0] = uc.f[0];
            rgba[i+j][1] = uc.f[1];
            rgba[i+j][2] = uc.f[2];
            rgba[i+j][3] = uc.f[3];
         }
         ++src;
      }
   }
      return;
   case VG_A_4: {
      VGubyte *src = (VGubyte *)data;
      src += offset/2;
      for (i = 0; i < n; i += 2) {
         VGfloat clr[4];
         VGint j;
         for (j = 0; j < n && j < 2; ++j) {
            VGint bitter, shift;
            if (j == 0) {
               bitter = 0x0f;
               shift = 0;
            } else {
               bitter = 0xf0;
               shift = 4;
            }
            clr[0] = 0.f;
            clr[1] = 0.f;
            clr[2] = 0.f;
            clr[3] = ((*src) & (bitter)) >> shift;

            util_pack_color(clr, PIPE_FORMAT_R32G32B32A32_FLOAT, &uc);
            rgba[i+j][0] = uc.f[0];
            rgba[i+j][1] = uc.f[1];
            rgba[i+j][2] = uc.f[2];
            rgba[i+j][3] = uc.f[3];
         }
         ++src;
      }
   }
      return;
#endif
   case VG_sXRGB_8888:
      break;
   case VG_sARGB_8888: {
      VGuint *src = (VGuint *)data;
      src += offset;
      for (i = 0; i < n; ++i) {
         VGubyte r, g, b ,a;
         a = (*src >> 24) & 0xff;
         r = (*src >> 16) & 0xff;
         g = (*src >>  8) & 0xff;
         b = (*src >>  0) & 0xff;

         util_pack_color_ub(r, g, b, a, PIPE_FORMAT_R32G32B32A32_FLOAT, &uc);
         rgba[i][0] = uc.f[0];
         rgba[i][1] = uc.f[1];
         rgba[i][2] = uc.f[2];
         rgba[i][3] = uc.f[3];
         ++src;
      }
      return;
   }
      break;
   case VG_sARGB_8888_PRE: {
      VGuint *src = (VGuint *)data;
      src += offset;
      for (i = 0; i < n; ++i) {
         VGubyte r, g, b ,a;
         a = (*src >> 24) & 0xff;
         r = (*src >> 16) & 0xff;
         g = (*src >>  8) & 0xff;
         b = (*src >>  0) & 0xff;

         util_pack_color_ub(r, g, b, a, PIPE_FORMAT_R32G32B32A32_FLOAT, &uc);
         rgba[i][0] = uc.f[0];
         rgba[i][1] = uc.f[1];
         rgba[i][2] = uc.f[2];
         rgba[i][3] = uc.f[3];
         ++src;
      }
      return;
   }
      break;
   case VG_sARGB_1555:
      break;
   case VG_sARGB_4444:
      break;
   case VG_lXRGB_8888:
      break;
   case VG_lARGB_8888: {
      VGint *src = (VGint *)data;
      src += offset;
      for (i = 0; i < n; ++i) {
         VGubyte r, g, b ,a;
         a = (*src >> 24) & 0xff;
         r = (*src >> 16) & 0xff;
         g = (*src >>  8) & 0xff;
         b = (*src >>  0) & 0xff;

         util_pack_color_ub(r, g, b, a, PIPE_FORMAT_R32G32B32A32_FLOAT, &uc);
         rgba[i][0] = uc.f[0];
         rgba[i][1] = uc.f[1];
         rgba[i][2] = uc.f[2];
         rgba[i][3] = uc.f[3];
         ++src;
      }
      return;
   }
      break;
   case VG_lARGB_8888_PRE: {
      VGint *src = (VGint *)data;
      src += offset;
      for (i = 0; i < n; ++i) {
         VGubyte r, g, b ,a;
         a = (*src >> 24) & 0xff;
         r = (*src >> 16) & 0xff;
         g = (*src >>  8) & 0xff;
         b = (*src >>  0) & 0xff;

         util_pack_color_ub(r, g, b, a, PIPE_FORMAT_R32G32B32A32_FLOAT, &uc);
         rgba[i][0] = uc.f[0];
         rgba[i][1] = uc.f[1];
         rgba[i][2] = uc.f[2];
         rgba[i][3] = uc.f[3];
         ++src;
      }
      return;
   }
      break;
   case VG_sBGRX_8888:
      break;
   case VG_sBGRA_8888:  {
      VGuint *src = (VGuint *)data;
      src += offset;
      for (i = 0; i < n; ++i) {
         VGubyte r, g, b ,a;
         b = (*src >> 24) & 0xff;
         g = (*src >> 16) & 0xff;
         r = (*src >>  8) & 0xff;
         a = (*src >>  0) & 0xff;

         util_pack_color_ub(r, g, b, a, PIPE_FORMAT_R32G32B32A32_FLOAT, &uc);
         rgba[i][0] = uc.f[0];
         rgba[i][1] = uc.f[1];
         rgba[i][2] = uc.f[2];
         rgba[i][3] = uc.f[3];
         ++src;
      }
      return;
   }
      break;
   case VG_sBGRA_8888_PRE:  {
      VGuint *src = (VGuint *)data;
      src += offset;
      for (i = 0; i < n; ++i) {
         VGubyte r, g, b ,a;
         b = (*src >> 24) & 0xff;
         g = (*src >> 16) & 0xff;
         r = (*src >>  8) & 0xff;
         a = (*src >>  0) & 0xff;

         util_pack_color_ub(r, g, b, a, PIPE_FORMAT_R32G32B32A32_FLOAT, &uc);
         rgba[i][0] = uc.f[0];
         rgba[i][1] = uc.f[1];
         rgba[i][2] = uc.f[2];
         rgba[i][3] = uc.f[3];
         ++src;
      }
      return;
   }
      break;
   case VG_sBGR_565:
      break;
   case VG_sBGRA_5551:
      break;
   case VG_sBGRA_4444:
      break;
   case VG_lBGRX_8888:
      break;
   case VG_lBGRA_8888:  {
      VGuint *src = (VGuint *)data;
      src += offset;
      for (i = 0; i < n; ++i) {
         VGubyte r, g, b ,a;
         b = (*src >> 24) & 0xff;
         g = (*src >> 16) & 0xff;
         r = (*src >>  8) & 0xff;
         a = (*src >>  0) & 0xff;

         util_pack_color_ub(r, g, b, a, PIPE_FORMAT_R32G32B32A32_FLOAT, &uc);
         rgba[i][0] = uc.f[0];
         rgba[i][1] = uc.f[1];
         rgba[i][2] = uc.f[2];
         rgba[i][3] = uc.f[3];
         ++src;
      }
      return;
   }
      break;
   case VG_lBGRA_8888_PRE:  {
      VGuint *src = (VGuint *)data;
      src += offset;
      for (i = 0; i < n; ++i) {
         VGubyte r, g, b ,a;
         b = (*src >> 24) & 0xff;
         g = (*src >> 16) & 0xff;
         r = (*src >>  8) & 0xff;
         a = (*src >>  0) & 0xff;

         util_pack_color_ub(r, g, b, a, PIPE_FORMAT_R32G32B32A32_FLOAT, &uc);
         rgba[i][0] = uc.f[0];
         rgba[i][1] = uc.f[1];
         rgba[i][2] = uc.f[2];
         rgba[i][3] = uc.f[3];
         ++src;
      }
      return;
   }
      break;
   case VG_sXBGR_8888:
      break;
   case VG_sABGR_8888: {
      VGuint *src = (VGuint *)data;
      src += offset;
      for (i = 0; i < n; ++i) {
         VGubyte r, g, b ,a;
         a = (*src >> 24) & 0xff;
         b = (*src >> 16) & 0xff;
         g = (*src >>  8) & 0xff;
         r = (*src >>  0) & 0xff;

         util_pack_color_ub(r, g, b, a, PIPE_FORMAT_R32G32B32A32_FLOAT, &uc);
         rgba[i][0] = uc.f[0];
         rgba[i][1] = uc.f[1];
         rgba[i][2] = uc.f[2];
         rgba[i][3] = uc.f[3];
         ++src;
      }
      return;
   }
      break;
   case VG_sABGR_8888_PRE: {
      VGuint *src = (VGuint *)data;
      src += offset;
      for (i = 0; i < n; ++i) {
         VGubyte r, g, b ,a;
         a = (*src >> 24) & 0xff;
         b = (*src >> 16) & 0xff;
         g = (*src >>  8) & 0xff;
         r = (*src >>  0) & 0xff;

         util_pack_color_ub(r, g, b, a, PIPE_FORMAT_R32G32B32A32_FLOAT, &uc);
         rgba[i][0] = uc.f[0];
         rgba[i][1] = uc.f[1];
         rgba[i][2] = uc.f[2];
         rgba[i][3] = uc.f[3];
         ++src;
      }
      return;
   }
      break;
   case VG_sABGR_1555:
      break;
   case VG_sABGR_4444:
      break;
   case VG_lXBGR_8888:
      break;
   case VG_lABGR_8888: {
      VGuint *src = (VGuint *)data;
      src += offset;
      for (i = 0; i < n; ++i) {
         VGubyte r, g, b ,a;
         a = (*src >> 24) & 0xff;
         b = (*src >> 16) & 0xff;
         g = (*src >>  8) & 0xff;
         r = (*src >>  0) & 0xff;

         util_pack_color_ub(r, g, b, a, PIPE_FORMAT_R32G32B32A32_FLOAT, &uc);
         rgba[i][0] = uc.f[0];
         rgba[i][1] = uc.f[1];
         rgba[i][2] = uc.f[2];
         rgba[i][3] = uc.f[3];
         ++src;
      }
      return;
   }
      break;
   case VG_lABGR_8888_PRE: {
      VGuint *src = (VGuint *)data;
      src += offset;
      for (i = 0; i < n; ++i) {
         VGubyte r, g, b ,a;
         a = (*src >> 24) & 0xff;
         b = (*src >> 16) & 0xff;
         g = (*src >>  8) & 0xff;
         r = (*src >>  0) & 0xff;

         util_pack_color_ub(r, g, b, a, PIPE_FORMAT_R32G32B32A32_FLOAT, &uc);
         rgba[i][0] = uc.f[0];
         rgba[i][1] = uc.f[1];
         rgba[i][2] = uc.f[2];
         rgba[i][3] = uc.f[3];
         ++src;
      }
      return;
   }
      break;
   default:
      assert(!"Unknown ReadPixels format");
      break;
   }
   assert(!"Not implemented ReadPixels format");
}

VGint _vega_size_for_format(VGImageFormat dataFormat)
{
   switch (dataFormat) {
   case VG_sRGBX_8888:
   case VG_sRGBA_8888:
   case VG_sRGBA_8888_PRE:
      return 4;
   case VG_sRGB_565:
   case VG_sRGBA_5551:
   case VG_sRGBA_4444:
      return 2;
   case VG_sL_8:
      return 1;
   case VG_lRGBX_8888:
   case VG_lRGBA_8888:
   case VG_lRGBA_8888_PRE:
      return 4;
   case VG_lL_8:
      return 1;
   case VG_A_8:
      return 1;
   case VG_BW_1:
      return 1;
#ifdef OPENVG_VERSION_1_1
   case VG_A_1:
      break;
   case VG_A_4:
      break;
#endif
   case VG_sXRGB_8888:
   case VG_sARGB_8888:
   case VG_sARGB_8888_PRE:
      return 4;
   case VG_sARGB_1555:
   case VG_sARGB_4444:
      return 2;
   case VG_lXRGB_8888:
   case VG_lARGB_8888:
   case VG_lARGB_8888_PRE:
   case VG_sBGRX_8888:
   case VG_sBGRA_8888:
   case VG_sBGRA_8888_PRE:
      return 4;
   case VG_sBGR_565:
   case VG_sBGRA_5551:
   case VG_sBGRA_4444:
      return 2;
   case VG_lBGRX_8888:
   case VG_lBGRA_8888:
   case VG_lBGRA_8888_PRE:
   case VG_sXBGR_8888:
   case VG_sABGR_8888:
   case VG_sABGR_8888_PRE:
      return 4;
   case VG_sABGR_1555:
   case VG_sABGR_4444:
      return 2;
   case VG_lXBGR_8888:
   case VG_lABGR_8888:
   case VG_lABGR_8888_PRE:
      return 4;
   default:
      assert(!"Unknown ReadPixels format");
      break;
   }
   assert(!"Not implemented ReadPixels format");
   return 0;
}
