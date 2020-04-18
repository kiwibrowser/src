// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fpdfapi/render/cpdf_imageloader.h"

#include "core/fpdfapi/page/cpdf_image.h"
#include "core/fpdfapi/page/cpdf_imageobject.h"
#include "core/fpdfapi/render/cpdf_dibsource.h"
#include "core/fpdfapi/render/cpdf_imagecacheentry.h"
#include "core/fpdfapi/render/cpdf_pagerendercache.h"
#include "core/fpdfapi/render/cpdf_renderstatus.h"

CPDF_ImageLoader::CPDF_ImageLoader()
    : m_MatteColor(0),
      m_bCached(false),
      m_pCache(nullptr),
      m_pImageObject(nullptr) {}

CPDF_ImageLoader::~CPDF_ImageLoader() {}

bool CPDF_ImageLoader::Start(const CPDF_ImageObject* pImage,
                             CPDF_PageRenderCache* pCache,
                             bool bStdCS,
                             uint32_t GroupFamily,
                             bool bLoadMask,
                             CPDF_RenderStatus* pRenderStatus) {
  m_pCache = pCache;
  m_pImageObject = const_cast<CPDF_ImageObject*>(pImage);
  bool ret;
  if (pCache) {
    ret = pCache->StartGetCachedBitmap(m_pImageObject->GetImage(), bStdCS,
                                       GroupFamily, bLoadMask, pRenderStatus);
  } else {
    ret = m_pImageObject->GetImage()->StartLoadDIBSource(
        pRenderStatus->GetFormResource(), pRenderStatus->GetPageResource(),
        bStdCS, GroupFamily, bLoadMask);
  }
  if (!ret)
    HandleFailure();
  return ret;
}

bool CPDF_ImageLoader::Continue(PauseIndicatorIface* pPause,
                                CPDF_RenderStatus* pRenderStatus) {
  bool ret = m_pCache ? m_pCache->Continue(pPause, pRenderStatus)
                      : m_pImageObject->GetImage()->Continue(pPause);
  if (!ret)
    HandleFailure();
  return ret;
}

void CPDF_ImageLoader::HandleFailure() {
  if (m_pCache) {
    CPDF_ImageCacheEntry* entry = m_pCache->GetCurImageCacheEntry();
    m_bCached = true;
    m_pBitmap = entry->DetachBitmap();
    m_pMask = entry->DetachMask();
    m_MatteColor = entry->m_MatteColor;
    return;
  }
  RetainPtr<CPDF_Image> pImage = m_pImageObject->GetImage();
  m_bCached = false;
  m_pBitmap = pImage->DetachBitmap();
  m_pMask = pImage->DetachMask();
  m_MatteColor = pImage->m_MatteColor;
}
