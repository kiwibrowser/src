// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FXCODEC_CODEC_CCODEC_PNGMODULE_H_
#define CORE_FXCODEC_CODEC_CCODEC_PNGMODULE_H_

#include <memory>

#include "core/fxcrt/fx_system.h"

class CFX_DIBAttribute;

class CCodec_PngModule {
 public:
  class Context {
   public:
    virtual ~Context() {}
  };

  class Delegate {
   public:
    virtual bool PngReadHeader(int width,
                               int height,
                               int bpc,
                               int pass,
                               int* color_type,
                               double* gamma) = 0;

    // Returns true on success. |pSrcBuf| will be set if this succeeds.
    // |pSrcBuf| does not take ownership of the buffer.
    virtual bool PngAskScanlineBuf(int line, uint8_t** pSrcBuf) = 0;

    virtual void PngFillScanlineBufCompleted(int pass, int line) = 0;
  };

  std::unique_ptr<Context> Start(Delegate* pDelegate);
  bool Input(Context* pContext,
             const uint8_t* src_buf,
             uint32_t src_size,
             CFX_DIBAttribute* pAttribute);
};

#endif  // CORE_FXCODEC_CODEC_CCODEC_PNGMODULE_H_
