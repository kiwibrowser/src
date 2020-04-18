// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fpdfapi/cmaps/cmap_int.h"

#include <algorithm>

#include "core/fpdfapi/cpdf_modulemgr.h"
#include "core/fpdfapi/font/cpdf_fontglobals.h"
#include "core/fpdfapi/page/cpdf_pagemodule.h"

namespace {

const FXCMAP_CMap* FindNextCMap(const FXCMAP_CMap* pMap) {
  return pMap->m_UseOffset ? pMap + pMap->m_UseOffset : nullptr;
}

}  // namespace

const FXCMAP_CMap* FPDFAPI_FindEmbeddedCMap(const ByteString& bsName,
                                            int charset,
                                            int coding) {
  CPDF_FontGlobals* pFontGlobals =
      CPDF_ModuleMgr::Get()->GetPageModule()->GetFontGlobals();

  const FXCMAP_CMap* pCMaps;
  uint32_t count;
  std::tie(count, pCMaps) = pFontGlobals->GetEmbeddedCharset(charset);
  for (uint32_t i = 0; i < count; i++) {
    if (bsName == pCMaps[i].m_Name)
      return &pCMaps[i];
  }
  return nullptr;
}

uint16_t FPDFAPI_CIDFromCharCode(const FXCMAP_CMap* pMap, uint32_t charcode) {
  ASSERT(pMap);
  const uint16_t loword = static_cast<uint16_t>(charcode);
  if (charcode >> 16) {
    while (pMap) {
      if (pMap->m_pDWordMap) {
        const FXCMAP_DWordCIDMap* begin = pMap->m_pDWordMap;
        const auto* end = begin + pMap->m_DWordCount;
        const auto* found = std::lower_bound(
            begin, end, charcode,
            [](const FXCMAP_DWordCIDMap& element, uint32_t charcode) {
              uint16_t hiword = static_cast<uint16_t>(charcode >> 16);
              if (element.m_HiWord != hiword)
                return element.m_HiWord < hiword;
              return element.m_LoWordHigh < static_cast<uint16_t>(charcode);
            });
        if (found != end && loword >= found->m_LoWordLow &&
            loword <= found->m_LoWordHigh) {
          return found->m_CID + loword - found->m_LoWordLow;
        }
      }
      pMap = FindNextCMap(pMap);
    }
    return 0;
  }

  while (pMap) {
    if (!pMap->m_pWordMap)
      return 0;
    if (pMap->m_WordMapType == FXCMAP_CMap::Single) {
      struct SingleCmap {
        uint16_t code;
        uint16_t cid;
      };
      const auto* begin = reinterpret_cast<const SingleCmap*>(pMap->m_pWordMap);
      const auto* end = begin + pMap->m_WordCount;
      const auto* found = std::lower_bound(
          begin, end, loword, [](const SingleCmap& element, uint16_t code) {
            return element.code < code;
          });
      if (found != end && found->code == loword)
        return found->cid;
    } else {
      ASSERT(pMap->m_WordMapType == FXCMAP_CMap::Range);
      struct RangeCmap {
        uint16_t low;
        uint16_t high;
        uint16_t cid;
      };
      const auto* begin = reinterpret_cast<const RangeCmap*>(pMap->m_pWordMap);
      const auto* end = begin + pMap->m_WordCount;
      const auto* found = std::lower_bound(
          begin, end, loword, [](const RangeCmap& element, uint16_t code) {
            return element.high < code;
          });
      if (found != end && loword >= found->low && loword <= found->high)
        return found->cid + loword - found->low;
    }
    pMap = FindNextCMap(pMap);
  }
  return 0;
}

uint32_t FPDFAPI_CharCodeFromCID(const FXCMAP_CMap* pMap, uint16_t cid) {
  // TODO(dsinclair): This should be checking both pMap->m_WordMap and
  // pMap->m_DWordMap. There was a second while() but it was never reached as
  // the first always returns. Investigate and determine how this should
  // really be working. (https://codereview.chromium.org/2235743003 removed the
  // second while loop.)
  ASSERT(pMap);
  while (pMap) {
    if (pMap->m_WordMapType == FXCMAP_CMap::Single) {
      const uint16_t* pCur = pMap->m_pWordMap;
      const uint16_t* pEnd = pMap->m_pWordMap + pMap->m_WordCount * 2;
      while (pCur < pEnd) {
        if (pCur[1] == cid)
          return pCur[0];

        pCur += 2;
      }
    } else {
      ASSERT(pMap->m_WordMapType == FXCMAP_CMap::Range);
      const uint16_t* pCur = pMap->m_pWordMap;
      const uint16_t* pEnd = pMap->m_pWordMap + pMap->m_WordCount * 3;
      while (pCur < pEnd) {
        if (cid >= pCur[2] && cid <= pCur[2] + pCur[1] - pCur[0])
          return pCur[0] + cid - pCur[2];

        pCur += 3;
      }
    }
    pMap = FindNextCMap(pMap);
  }
  return 0;
}
