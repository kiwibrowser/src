// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FXGE_FX_DIB_H_
#define CORE_FXGE_FX_DIB_H_

#include <tuple>
#include <utility>

#include "core/fxcrt/fx_coordinates.h"
#include "core/fxcrt/widestring.h"

enum FXDIB_Format {
  FXDIB_Invalid = 0,
  FXDIB_1bppRgb = 0x001,
  FXDIB_8bppRgb = 0x008,
  FXDIB_Rgb = 0x018,
  FXDIB_Rgb32 = 0x020,
  FXDIB_1bppMask = 0x101,
  FXDIB_8bppMask = 0x108,
  FXDIB_8bppRgba = 0x208,
  FXDIB_Rgba = 0x218,
  FXDIB_Argb = 0x220,
  FXDIB_1bppCmyk = 0x401,
  FXDIB_8bppCmyk = 0x408,
  FXDIB_Cmyk = 0x420,
  FXDIB_8bppCmyka = 0x608,
  FXDIB_Cmyka = 0x620,
};

struct PixelWeight {
  int m_SrcStart;
  int m_SrcEnd;
  int m_Weights[1];
};

using FX_ARGB = uint32_t;

// FX_COLORREF, like win32 COLORREF, is BGR.
using FX_COLORREF = uint32_t;

using FX_CMYK = uint32_t;

class CFX_ClipRgn;
class CFX_DIBSource;
class CStretchEngine;

extern const int16_t SDP_Table[513];

#define FXDIB_DOWNSAMPLE 0x04
#define FXDIB_INTERPOL 0x20
#define FXDIB_BICUBIC_INTERPOL 0x80
#define FXDIB_NOSMOOTH 0x100

#define FXDIB_BLEND_NORMAL 0
#define FXDIB_BLEND_MULTIPLY 1
#define FXDIB_BLEND_SCREEN 2
#define FXDIB_BLEND_OVERLAY 3
#define FXDIB_BLEND_DARKEN 4
#define FXDIB_BLEND_LIGHTEN 5
#define FXDIB_BLEND_COLORDODGE 6
#define FXDIB_BLEND_COLORBURN 7
#define FXDIB_BLEND_HARDLIGHT 8
#define FXDIB_BLEND_SOFTLIGHT 9
#define FXDIB_BLEND_DIFFERENCE 10
#define FXDIB_BLEND_EXCLUSION 11
#define FXDIB_BLEND_NONSEPARABLE 21
#define FXDIB_BLEND_HUE 21
#define FXDIB_BLEND_SATURATION 22
#define FXDIB_BLEND_COLOR 23
#define FXDIB_BLEND_LUMINOSITY 24
#define FXDIB_BLEND_UNSUPPORTED -1

constexpr uint32_t FXSYS_BGR(uint8_t b, uint8_t g, uint8_t r) {
  return (b << 16) | (g << 8) | r;
}

constexpr uint8_t FXSYS_GetRValue(uint32_t bgr) {
  return bgr & 0xff;
}

constexpr uint8_t FXSYS_GetGValue(uint32_t bgr) {
  return (bgr >> 8) & 0xff;
}

constexpr uint8_t FXSYS_GetBValue(uint32_t bgr) {
  return (bgr >> 16) & 0xff;
}

#define FXSYS_GetCValue(cmyk) ((uint8_t)((cmyk) >> 24) & 0xff)
#define FXSYS_GetMValue(cmyk) ((uint8_t)((cmyk) >> 16) & 0xff)
#define FXSYS_GetYValue(cmyk) ((uint8_t)((cmyk) >> 8) & 0xff)
#define FXSYS_GetKValue(cmyk) ((uint8_t)(cmyk)&0xff)

inline int GetBppFromFormat(FXDIB_Format format) {
  return format & 0xff;
}

inline int GetCompsFromFormat(FXDIB_Format format) {
  return (format & 0xff) / 8;
}

inline FX_CMYK CmykEncode(int c, int m, int y, int k) {
  return (c << 24) | (m << 16) | (y << 8) | k;
}

// Returns (a, r, g, b)
std::tuple<int, int, int, int> ArgbDecode(FX_ARGB argb);

// Returns (a, FX_COLORREF)
std::pair<int, FX_COLORREF> ArgbToAlphaAndColorRef(FX_ARGB argb);

// Returns FX_COLORREF.
FX_COLORREF ArgbToColorRef(FX_ARGB argb);

constexpr FX_ARGB ArgbEncode(int a, int r, int g, int b) {
  return (a << 24) | (r << 16) | (g << 8) | b;
}

FX_ARGB AlphaAndColorRefToArgb(int a, FX_COLORREF colorref);

FX_ARGB StringToFXARGB(const WideStringView& view);

#define FXARGB_A(argb) ((uint8_t)((argb) >> 24))
#define FXARGB_R(argb) ((uint8_t)((argb) >> 16))
#define FXARGB_G(argb) ((uint8_t)((argb) >> 8))
#define FXARGB_B(argb) ((uint8_t)(argb))
#define FXARGB_MAKE(a, r, g, b) \
  (((uint32_t)(a) << 24) | ((r) << 16) | ((g) << 8) | (b))
#define FXARGB_MUL_ALPHA(argb, alpha) \
  (((((argb) >> 24) * (alpha) / 255) << 24) | ((argb)&0xffffff))

#define FXRGB2GRAY(r, g, b) (((b)*11 + (g)*59 + (r)*30) / 100)
#define FXDIB_ALPHA_MERGE(backdrop, source, source_alpha) \
  (((backdrop) * (255 - (source_alpha)) + (source) * (source_alpha)) / 255)
#define FXARGB_GETDIB(p)                              \
  ((((uint8_t*)(p))[0]) | (((uint8_t*)(p))[1] << 8) | \
   (((uint8_t*)(p))[2] << 16) | (((uint8_t*)(p))[3] << 24))
#define FXARGB_SETDIB(p, argb)                  \
  ((uint8_t*)(p))[0] = (uint8_t)(argb),         \
  ((uint8_t*)(p))[1] = (uint8_t)((argb) >> 8),  \
  ((uint8_t*)(p))[2] = (uint8_t)((argb) >> 16), \
  ((uint8_t*)(p))[3] = (uint8_t)((argb) >> 24)
#define FXARGB_SETRGBORDERDIB(p, argb)          \
  ((uint8_t*)(p))[3] = (uint8_t)(argb >> 24),   \
  ((uint8_t*)(p))[0] = (uint8_t)((argb) >> 16), \
  ((uint8_t*)(p))[1] = (uint8_t)((argb) >> 8),  \
  ((uint8_t*)(p))[2] = (uint8_t)(argb)
#define FXARGB_TODIB(argb) (argb)
#define FXCMYK_TODIB(cmyk)                                    \
  ((uint8_t)((cmyk) >> 24) | ((uint8_t)((cmyk) >> 16)) << 8 | \
   ((uint8_t)((cmyk) >> 8)) << 16 | ((uint8_t)(cmyk) << 24))
#define FXARGB_TOBGRORDERDIB(argb)                       \
  ((uint8_t)(argb >> 16) | ((uint8_t)(argb >> 8)) << 8 | \
   ((uint8_t)(argb)) << 16 | ((uint8_t)(argb >> 24) << 24))

FX_RECT FXDIB_SwapClipBox(const FX_RECT& clip,
                          int width,
                          int height,
                          bool bFlipX,
                          bool bFlipY);

#endif  // CORE_FXGE_FX_DIB_H_
