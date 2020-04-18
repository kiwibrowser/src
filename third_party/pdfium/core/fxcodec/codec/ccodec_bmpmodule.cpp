// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fxcodec/codec/ccodec_bmpmodule.h"

#include "core/fxcodec/bmp/cfx_bmpcontext.h"
#include "core/fxcodec/codec/codec_int.h"
#include "core/fxcodec/fx_codec.h"
#include "core/fxcrt/unowned_ptr.h"
#include "core/fxge/fx_dib.h"
#include "third_party/base/ptr_util.h"

CCodec_BmpModule::CCodec_BmpModule() {}

CCodec_BmpModule::~CCodec_BmpModule() {}

std::unique_ptr<CCodec_BmpModule::Context> CCodec_BmpModule::Start(
    Delegate* pDelegate) {
  auto p = pdfium::MakeUnique<CFX_BmpContext>(this, pDelegate);
  p->m_Bmp.context_ptr_ = p.get();
  return p;
}

int32_t CCodec_BmpModule::ReadHeader(Context* pContext,
                                     int32_t* width,
                                     int32_t* height,
                                     bool* tb_flag,
                                     int32_t* components,
                                     int32_t* pal_num,
                                     std::vector<uint32_t>* palette,
                                     CFX_DIBAttribute* pAttribute) {
  auto* ctx = static_cast<CFX_BmpContext*>(pContext);
  if (setjmp(ctx->m_Bmp.jmpbuf_))
    return 0;

  int32_t ret = ctx->m_Bmp.ReadHeader();
  if (ret != 1)
    return ret;

  *width = ctx->m_Bmp.width_;
  *height = ctx->m_Bmp.height_;
  *tb_flag = ctx->m_Bmp.imgTB_flag_;
  *components = ctx->m_Bmp.components_;
  *pal_num = ctx->m_Bmp.pal_num_;
  *palette = ctx->m_Bmp.palette_;
  if (pAttribute) {
    pAttribute->m_wDPIUnit = FXCODEC_RESUNIT_METER;
    pAttribute->m_nXDPI = ctx->m_Bmp.dpi_x_;
    pAttribute->m_nYDPI = ctx->m_Bmp.dpi_y_;
    pAttribute->m_nBmpCompressType = ctx->m_Bmp.compress_flag_;
  }
  return 1;
}

int32_t CCodec_BmpModule::LoadImage(Context* pContext) {
  auto* ctx = static_cast<CFX_BmpContext*>(pContext);
  if (setjmp(ctx->m_Bmp.jmpbuf_))
    return 0;

  return ctx->m_Bmp.DecodeImage();
}

FX_FILESIZE CCodec_BmpModule::GetAvailInput(Context* pContext,
                                            uint8_t** avail_buf_ptr) {
  auto* ctx = static_cast<CFX_BmpContext*>(pContext);
  return ctx->m_Bmp.GetAvailInput(avail_buf_ptr);
}

void CCodec_BmpModule::Input(Context* pContext,
                             const uint8_t* src_buf,
                             uint32_t src_size) {
  auto* ctx = static_cast<CFX_BmpContext*>(pContext);
  ctx->m_Bmp.SetInputBuffer(const_cast<uint8_t*>(src_buf), src_size);
}
