// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FPDFAPI_RENDER_CPDF_IMAGELOADER_H_
#define CORE_FPDFAPI_RENDER_CPDF_IMAGELOADER_H_

#include <memory>

#include "core/fxcrt/retain_ptr.h"
#include "core/fxcrt/unowned_ptr.h"
#include "core/fxge/fx_dib.h"

class CPDF_ImageObject;
class CPDF_PageRenderCache;
class CPDF_RenderStatus;
class PauseIndicatorIface;

class CPDF_ImageLoader {
 public:
  CPDF_ImageLoader();
  ~CPDF_ImageLoader();

  bool Start(const CPDF_ImageObject* pImage,
             CPDF_PageRenderCache* pCache,
             bool bStdCS,
             uint32_t GroupFamily,
             bool bLoadMask,
             CPDF_RenderStatus* pRenderStatus);
  bool Continue(PauseIndicatorIface* pPause, CPDF_RenderStatus* pRenderStatus);

  RetainPtr<CFX_DIBSource> m_pBitmap;
  RetainPtr<CFX_DIBSource> m_pMask;
  uint32_t m_MatteColor;
  bool m_bCached;

 private:
  void HandleFailure();

  UnownedPtr<CPDF_PageRenderCache> m_pCache;
  UnownedPtr<CPDF_ImageObject> m_pImageObject;
};

#endif  // CORE_FPDFAPI_RENDER_CPDF_IMAGELOADER_H_
