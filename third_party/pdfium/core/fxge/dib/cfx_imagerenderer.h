// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FXGE_DIB_CFX_IMAGERENDERER_H_
#define CORE_FXGE_DIB_CFX_IMAGERENDERER_H_

#include <memory>

#include "core/fxcrt/fx_coordinates.h"
#include "core/fxcrt/retain_ptr.h"
#include "core/fxcrt/unowned_ptr.h"
#include "core/fxge/dib/cfx_bitmapcomposer.h"
#include "core/fxge/dib/cfx_dibitmap.h"
#include "core/fxge/dib/cfx_dibsource.h"
#include "core/fxge/fx_dib.h"
#include "third_party/base/stl_util.h"

class CFX_ImageTransformer;
class CFX_ImageStretcher;

class CFX_ImageRenderer {
 public:
  CFX_ImageRenderer(const RetainPtr<CFX_DIBitmap>& pDevice,
                    const CFX_ClipRgn* pClipRgn,
                    const RetainPtr<CFX_DIBSource>& pSource,
                    int bitmap_alpha,
                    uint32_t mask_color,
                    const CFX_Matrix* pMatrix,
                    uint32_t dib_flags,
                    bool bRgbByteOrder);
  ~CFX_ImageRenderer();

  bool Continue(PauseIndicatorIface* pPause);

 private:
  const RetainPtr<CFX_DIBitmap> m_pDevice;
  const UnownedPtr<const CFX_ClipRgn> m_pClipRgn;
  const CFX_Matrix m_Matrix;
  const int m_BitmapAlpha;
  const int m_BlendType;
  const bool m_bRgbByteOrder;
  uint32_t m_MaskColor;
  std::unique_ptr<CFX_ImageTransformer> m_pTransformer;
  std::unique_ptr<CFX_ImageStretcher> m_Stretcher;
  CFX_BitmapComposer m_Composer;
  int m_Status;
  FX_RECT m_ClipBox;
  int m_AlphaFlag;
};

#endif  // CORE_FXGE_DIB_CFX_IMAGERENDERER_H_
