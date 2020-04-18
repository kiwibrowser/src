// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fxcodec/codec/ccodec_pngmodule.h"

#include <algorithm>

#include "core/fxcodec/codec/codec_int.h"
#include "core/fxcodec/fx_codec.h"
#include "core/fxcrt/unowned_ptr.h"
#include "core/fxge/fx_dib.h"
#include "third_party/base/compiler_specific.h"
#include "third_party/base/ptr_util.h"

#ifdef USE_SYSTEM_LIBPNG
#include <png.h>
#else
#include "third_party/libpng16/png.h"
#endif

#define PNG_ERROR_SIZE 256

class CPngContext : public CCodec_PngModule::Context {
 public:
  CPngContext(CCodec_PngModule* pModule, CCodec_PngModule::Delegate* pDelegate);
  ~CPngContext() override;

  png_structp m_pPng;
  png_infop m_pInfo;
  UnownedPtr<CCodec_PngModule> m_pModule;
  UnownedPtr<CCodec_PngModule::Delegate> m_pDelegate;
  void* (*m_AllocFunc)(unsigned int);
  void (*m_FreeFunc)(void*);
  char m_szLastError[PNG_ERROR_SIZE];
};

extern "C" {

static void _png_error_data(png_structp png_ptr, png_const_charp error_msg) {
  if (png_get_error_ptr(png_ptr))
    strncpy((char*)png_get_error_ptr(png_ptr), error_msg, PNG_ERROR_SIZE - 1);

  longjmp(png_jmpbuf(png_ptr), 1);
}

static void _png_warning_data(png_structp png_ptr, png_const_charp error_msg) {}

static void _png_load_bmp_attribute(png_structp png_ptr,
                                    png_infop info_ptr,
                                    CFX_DIBAttribute* pAttribute) {
  if (pAttribute) {
#if defined(PNG_pHYs_SUPPORTED)
    pAttribute->m_nXDPI = png_get_x_pixels_per_meter(png_ptr, info_ptr);
    pAttribute->m_nYDPI = png_get_y_pixels_per_meter(png_ptr, info_ptr);
    png_uint_32 res_x, res_y;
    int unit_type;
    png_get_pHYs(png_ptr, info_ptr, &res_x, &res_y, &unit_type);
    switch (unit_type) {
      case PNG_RESOLUTION_METER:
        pAttribute->m_wDPIUnit = FXCODEC_RESUNIT_METER;
        break;
      default:
        pAttribute->m_wDPIUnit = FXCODEC_RESUNIT_NONE;
    }
#endif
#if defined(PNG_iCCP_SUPPORTED)
    png_charp icc_name;
    png_bytep icc_profile;
    png_uint_32 icc_proflen;
    int compress_type;
    png_get_iCCP(png_ptr, info_ptr, &icc_name, &compress_type, &icc_profile,
                 &icc_proflen);
#endif
#if defined(PNG_TEXT_SUPPORTED)
    int num_text;
    png_textp text = nullptr;
    png_get_text(png_ptr, info_ptr, &text, &num_text);
#endif
  }
}

static void* _png_alloc_func(unsigned int size) {
  return FX_Alloc(char, size);
}

static void _png_free_func(void* p) {
  FX_Free(p);
}

static void _png_get_header_func(png_structp png_ptr, png_infop info_ptr) {
  auto* pContext =
      reinterpret_cast<CPngContext*>(png_get_progressive_ptr(png_ptr));
  if (!pContext)
    return;

  png_uint_32 width = 0;
  png_uint_32 height = 0;
  int bpc = 0;
  int color_type = 0;
  png_get_IHDR(png_ptr, info_ptr, &width, &height, &bpc, &color_type, nullptr,
               nullptr, nullptr);
  int color_type1 = color_type;
  if (bpc > 8)
    png_set_strip_16(png_ptr);
  else if (bpc < 8)
    png_set_expand_gray_1_2_4_to_8(png_ptr);

  bpc = 8;
  if (color_type == PNG_COLOR_TYPE_PALETTE)
    png_set_palette_to_rgb(png_ptr);

  int pass = png_set_interlace_handling(png_ptr);
  double gamma = 1.0;
  if (!pContext->m_pDelegate->PngReadHeader(width, height, bpc, pass,
                                            &color_type, &gamma)) {
    png_error(pContext->m_pPng, "Read Header Callback Error");
  }
  int intent;
  if (png_get_sRGB(png_ptr, info_ptr, &intent)) {
    png_set_gamma(png_ptr, gamma, 0.45455);
  } else {
    double image_gamma;
    if (png_get_gAMA(png_ptr, info_ptr, &image_gamma))
      png_set_gamma(png_ptr, gamma, image_gamma);
    else
      png_set_gamma(png_ptr, gamma, 0.45455);
  }
  switch (color_type) {
    case PNG_COLOR_TYPE_GRAY:
    case PNG_COLOR_TYPE_GRAY_ALPHA: {
      if (color_type1 & PNG_COLOR_MASK_COLOR) {
        png_set_rgb_to_gray(png_ptr, 1, 0.299, 0.587);
      }
    } break;
    case PNG_COLOR_TYPE_PALETTE:
      if (color_type1 != PNG_COLOR_TYPE_PALETTE) {
        png_error(pContext->m_pPng, "Not Support Output Palette Now");
      }
      FALLTHROUGH;
    case PNG_COLOR_TYPE_RGB:
    case PNG_COLOR_TYPE_RGB_ALPHA:
      if (!(color_type1 & PNG_COLOR_MASK_COLOR)) {
        png_set_gray_to_rgb(png_ptr);
      }
      png_set_bgr(png_ptr);
      break;
  }
  if (!(color_type & PNG_COLOR_MASK_ALPHA))
    png_set_strip_alpha(png_ptr);

  if (color_type & PNG_COLOR_MASK_ALPHA &&
      !(color_type1 & PNG_COLOR_MASK_ALPHA)) {
    png_set_filler(png_ptr, 0xff, PNG_FILLER_AFTER);
  }
  png_read_update_info(png_ptr, info_ptr);
}

static void _png_get_end_func(png_structp png_ptr, png_infop info_ptr) {}

static void _png_get_row_func(png_structp png_ptr,
                              png_bytep new_row,
                              png_uint_32 row_num,
                              int pass) {
  auto* pContext =
      reinterpret_cast<CPngContext*>(png_get_progressive_ptr(png_ptr));
  if (!pContext)
    return;

  uint8_t* src_buf;
  if (!pContext->m_pDelegate->PngAskScanlineBuf(row_num, &src_buf))
    png_error(png_ptr, "Ask Scanline buffer Callback Error");

  if (src_buf)
    png_progressive_combine_row(png_ptr, src_buf, new_row);

  pContext->m_pDelegate->PngFillScanlineBufCompleted(pass, row_num);
}

}  // extern "C"

CPngContext::CPngContext(CCodec_PngModule* pModule,
                         CCodec_PngModule::Delegate* pDelegate)
    : m_pPng(nullptr),
      m_pInfo(nullptr),
      m_pModule(pModule),
      m_pDelegate(pDelegate),
      m_AllocFunc(_png_alloc_func),
      m_FreeFunc(_png_free_func) {
  memset(m_szLastError, 0, sizeof(m_szLastError));
}

CPngContext::~CPngContext() {
  png_destroy_read_struct(m_pPng ? &m_pPng : nullptr,
                          m_pInfo ? &m_pInfo : nullptr, nullptr);
}

std::unique_ptr<CCodec_PngModule::Context> CCodec_PngModule::Start(
    Delegate* pDelegate) {
  auto p = pdfium::MakeUnique<CPngContext>(this, pDelegate);
  p->m_pPng =
      png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
  if (!p->m_pPng)
    return nullptr;

  p->m_pInfo = png_create_info_struct(p->m_pPng);
  if (!p->m_pInfo)
    return nullptr;

  if (setjmp(png_jmpbuf(p->m_pPng)))
    return nullptr;

  png_set_progressive_read_fn(p->m_pPng, p.get(), _png_get_header_func,
                              _png_get_row_func, _png_get_end_func);
  png_set_error_fn(p->m_pPng, p->m_szLastError, _png_error_data,
                   _png_warning_data);
  return p;
}

bool CCodec_PngModule::Input(Context* pContext,
                             const uint8_t* src_buf,
                             uint32_t src_size,
                             CFX_DIBAttribute* pAttribute) {
  auto* ctx = static_cast<CPngContext*>(pContext);
  if (setjmp(png_jmpbuf(ctx->m_pPng))) {
    if (pAttribute &&
        strcmp(ctx->m_szLastError, "Read Header Callback Error") == 0) {
      _png_load_bmp_attribute(ctx->m_pPng, ctx->m_pInfo, pAttribute);
    }
    return false;
  }
  png_process_data(ctx->m_pPng, ctx->m_pInfo, (uint8_t*)src_buf, src_size);
  return true;
}
