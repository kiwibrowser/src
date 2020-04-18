// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FPDFAPI_FONT_CPDF_FONTGLOBALS_H_
#define CORE_FPDFAPI_FONT_CPDF_FONTGLOBALS_H_

#include <map>
#include <memory>
#include <utility>

#include "core/fpdfapi/cmaps/cmap_int.h"
#include "core/fpdfapi/font/cfx_stockfontarray.h"
#include "core/fpdfapi/font/cpdf_cmapmanager.h"

class CPDF_FontGlobals {
 public:
  CPDF_FontGlobals();
  ~CPDF_FontGlobals();

  void Clear(CPDF_Document* pDoc);
  CPDF_Font* Find(CPDF_Document* pDoc, uint32_t index);

  // Takes ownership of |pFont|, returns unowned pointer to it.
  CPDF_Font* Set(CPDF_Document* key,
                 uint32_t index,
                 std::unique_ptr<CPDF_Font> pFont);

  void SetEmbeddedCharset(size_t idx, const FXCMAP_CMap* map, uint32_t count) {
    m_EmbeddedCharsets[idx].m_pMapList = map;
    m_EmbeddedCharsets[idx].m_Count = count;
  }
  std::pair<uint32_t, const FXCMAP_CMap*> GetEmbeddedCharset(size_t idx) const {
    return {m_EmbeddedCharsets[idx].m_Count,
            m_EmbeddedCharsets[idx].m_pMapList.Get()};
  }
  void SetEmbeddedToUnicode(size_t idx, const uint16_t* map, uint32_t count) {
    m_EmbeddedToUnicodes[idx].m_pMap = map;
    m_EmbeddedToUnicodes[idx].m_Count = count;
  }
  std::pair<uint32_t, const uint16_t*> GetEmbeddedToUnicode(size_t idx) {
    return {m_EmbeddedToUnicodes[idx].m_Count,
            m_EmbeddedToUnicodes[idx].m_pMap};
  }

  CPDF_CMapManager* GetCMapManager() { return &m_CMapManager; }

 private:
  CPDF_CMapManager m_CMapManager;
  struct {
    UnownedPtr<const FXCMAP_CMap> m_pMapList;
    uint32_t m_Count;
  } m_EmbeddedCharsets[CIDSET_NUM_SETS];
  struct {
    const uint16_t* m_pMap;
    uint32_t m_Count;
  } m_EmbeddedToUnicodes[CIDSET_NUM_SETS];

  std::map<CPDF_Document*, std::unique_ptr<CFX_StockFontArray>> m_StockMap;
};

#endif  // CORE_FPDFAPI_FONT_CPDF_FONTGLOBALS_H_
