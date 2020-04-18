// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fpdfapi/font/cpdf_cid2unicodemap.h"

#include "core/fpdfapi/cpdf_modulemgr.h"
#include "core/fpdfapi/font/cpdf_cmapmanager.h"
#include "core/fpdfapi/page/cpdf_pagemodule.h"

CPDF_CID2UnicodeMap::CPDF_CID2UnicodeMap() {
  m_EmbeddedCount = 0;
}

CPDF_CID2UnicodeMap::~CPDF_CID2UnicodeMap() {}

bool CPDF_CID2UnicodeMap::IsLoaded() {
  return m_EmbeddedCount != 0;
}

wchar_t CPDF_CID2UnicodeMap::UnicodeFromCID(uint16_t CID) {
  if (m_Charset == CIDSET_UNICODE) {
    return CID;
  }
  if (CID < m_EmbeddedCount) {
    return m_pEmbeddedMap[CID];
  }
  return 0;
}

void CPDF_CID2UnicodeMap::Load(CPDF_CMapManager* pMgr,
                               CIDSet charset,
                               bool bPromptCJK) {
  m_Charset = charset;

  CPDF_FontGlobals* pFontGlobals =
      CPDF_ModuleMgr::Get()->GetPageModule()->GetFontGlobals();
  std::tie(m_EmbeddedCount, m_pEmbeddedMap) =
      pFontGlobals->GetEmbeddedToUnicode(charset);
}
