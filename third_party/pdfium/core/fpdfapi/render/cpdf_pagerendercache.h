// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FPDFAPI_RENDER_CPDF_PAGERENDERCACHE_H_
#define CORE_FPDFAPI_RENDER_CPDF_PAGERENDERCACHE_H_

#include <map>

#include "core/fxcrt/fx_system.h"
#include "core/fxcrt/retain_ptr.h"
#include "core/fxcrt/unowned_ptr.h"

class CFX_DIBitmap;
class CPDF_Image;
class CPDF_ImageCacheEntry;
class CPDF_Page;
class CPDF_RenderStatus;
class CPDF_Stream;
class PauseIndicatorIface;

class CPDF_PageRenderCache {
 public:
  explicit CPDF_PageRenderCache(CPDF_Page* pPage);
  ~CPDF_PageRenderCache();

  void CacheOptimization(int32_t dwLimitCacheSize);
  uint32_t GetTimeCount() const { return m_nTimeCount; }
  void ResetBitmap(const RetainPtr<CPDF_Image>& pImage,
                   const RetainPtr<CFX_DIBitmap>& pBitmap);
  CPDF_Page* GetPage() const { return m_pPage.Get(); }
  CPDF_ImageCacheEntry* GetCurImageCacheEntry() const {
    return m_pCurImageCacheEntry;
  }

  bool StartGetCachedBitmap(const RetainPtr<CPDF_Image>& pImage,
                            bool bStdCS,
                            uint32_t GroupFamily,
                            bool bLoadMask,
                            CPDF_RenderStatus* pRenderStatus);

  bool Continue(PauseIndicatorIface* pPause, CPDF_RenderStatus* pRenderStatus);

 private:
  void ClearImageCacheEntry(CPDF_Stream* pStream);

  UnownedPtr<CPDF_Page> const m_pPage;
  CPDF_ImageCacheEntry* m_pCurImageCacheEntry;
  std::map<CPDF_Stream*, CPDF_ImageCacheEntry*> m_ImageCache;
  uint32_t m_nTimeCount;
  uint32_t m_nCacheSize;
  bool m_bCurFindCache;
};

#endif  // CORE_FPDFAPI_RENDER_CPDF_PAGERENDERCACHE_H_
