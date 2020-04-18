// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FXCODEC_CODEC_CCODEC_JPEGMODULE_H_
#define CORE_FXCODEC_CODEC_CCODEC_JPEGMODULE_H_

#include <csetjmp>
#include <memory>

#include "core/fxcrt/fx_system.h"
#include "core/fxcrt/retain_ptr.h"

class CCodec_ScanlineDecoder;
class CFX_DIBSource;

#ifdef PDF_ENABLE_XFA
class CFX_DIBAttribute;
#endif  // PDF_ENABLE_XFA

class CCodec_JpegModule {
 public:
  class Context {
   public:
    virtual ~Context() {}
    virtual jmp_buf* GetJumpMark() = 0;
  };

  std::unique_ptr<CCodec_ScanlineDecoder> CreateDecoder(const uint8_t* src_buf,
                                                        uint32_t src_size,
                                                        int width,
                                                        int height,
                                                        int nComps,
                                                        bool ColorTransform);
  bool LoadInfo(const uint8_t* src_buf,
                uint32_t src_size,
                int* width,
                int* height,
                int* num_components,
                int* bits_per_components,
                bool* color_transform);

  std::unique_ptr<Context> Start();
  void Input(Context* pContext, const uint8_t* src_buf, uint32_t src_size);

#ifndef PDF_ENABLE_XFA
  int ReadHeader(Context* pContext, int* width, int* height, int* nComps);
#else   // PDF_ENABLE_XFA
  int ReadHeader(Context* pContext,
                 int* width,
                 int* height,
                 int* nComps,
                 CFX_DIBAttribute* pAttribute);
#endif  // PDF_ENABLE_XFA

  bool StartScanline(Context* pContext, int down_scale);
  bool ReadScanline(Context* pContext, uint8_t* dest_buf);
  uint32_t GetAvailInput(Context* pContext, uint8_t** avail_buf_ptr);

#if _FX_PLATFORM_ == _FX_PLATFORM_WINDOWS_
  static bool JpegEncode(const RetainPtr<CFX_DIBSource>& pSource,
                         uint8_t** dest_buf,
                         size_t* dest_size);
#endif  // _FX_PLATFORM_ == _FX_PLATFORM_WINDOWS_
};

#endif  // CORE_FXCODEC_CODEC_CCODEC_JPEGMODULE_H_
