// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FXCODEC_CODEC_CCODEC_BMPMODULE_H_
#define CORE_FXCODEC_CODEC_CCODEC_BMPMODULE_H_

#include <memory>
#include <vector>

#include "core/fxcrt/fx_system.h"
#include "core/fxcrt/unowned_ptr.h"

class CFX_DIBAttribute;

class CCodec_BmpModule {
 public:
  class Context {
   public:
    virtual ~Context() {}
  };

  class Delegate {
   public:
    virtual bool BmpInputImagePositionBuf(uint32_t rcd_pos) = 0;
    virtual void BmpReadScanline(uint32_t row_num,
                                 const std::vector<uint8_t>& row_buf) = 0;
  };

  CCodec_BmpModule();
  ~CCodec_BmpModule();

  std::unique_ptr<Context> Start(Delegate* pDelegate);
  FX_FILESIZE GetAvailInput(Context* pContext, uint8_t** avail_buf_ptr);
  void Input(Context* pContext, const uint8_t* src_buf, uint32_t src_size);
  int32_t ReadHeader(Context* pContext,
                     int32_t* width,
                     int32_t* height,
                     bool* tb_flag,
                     int32_t* components,
                     int32_t* pal_num,
                     std::vector<uint32_t>* palette,
                     CFX_DIBAttribute* pAttribute);
  int32_t LoadImage(Context* pContext);
};

#endif  // CORE_FXCODEC_CODEC_CCODEC_BMPMODULE_H_
