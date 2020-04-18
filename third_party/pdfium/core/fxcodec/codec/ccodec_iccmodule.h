// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FXCODEC_CODEC_CCODEC_ICCMODULE_H_
#define CORE_FXCODEC_CODEC_CCODEC_ICCMODULE_H_

#include <memory>

#include "core/fxcodec/fx_codec_def.h"
#include "core/fxcrt/fx_string.h"
#include "core/fxcrt/fx_system.h"
#include "third_party/base/ptr_util.h"

#if defined(USE_SYSTEM_LCMS2)
#include <lcms2.h>
#else
#include "third_party/lcms/include/lcms2.h"
#endif

class CLcmsCmm {
 public:
  CLcmsCmm(int srcComponents, cmsHTRANSFORM transform, bool isLab);
  ~CLcmsCmm();

  cmsHTRANSFORM m_hTransform;
  int m_nSrcComponents;
  bool m_bLab;
};

class CCodec_IccModule {
 public:
  CCodec_IccModule();
  ~CCodec_IccModule();

  std::unique_ptr<CLcmsCmm> CreateTransform_sRGB(const uint8_t* pProfileData,
                                                 uint32_t dwProfileSize,
                                                 uint32_t* nComponents);
  void Translate(CLcmsCmm* pTransform,
                 const float* pSrcValues,
                 float* pDestValues);
  void TranslateScanline(CLcmsCmm* pTransform,
                         uint8_t* pDest,
                         const uint8_t* pSrc,
                         int pixels);
  void SetComponents(uint32_t nComponents) { m_nComponents = nComponents; }

 protected:
  uint32_t m_nComponents;
};

#endif  // CORE_FXCODEC_CODEC_CCODEC_ICCMODULE_H_
