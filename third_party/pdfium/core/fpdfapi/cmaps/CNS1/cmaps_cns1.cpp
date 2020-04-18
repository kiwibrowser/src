// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fpdfapi/cmaps/CNS1/cmaps_cns1.h"

#include "core/fpdfapi/cmaps/cmap_int.h"
#include "core/fpdfapi/cpdf_modulemgr.h"
#include "core/fpdfapi/font/cpdf_fontglobals.h"
#include "core/fpdfapi/page/cpdf_pagemodule.h"

static const FXCMAP_CMap g_FXCMAP_CNS1_cmaps[] = {
    {"B5pc-H", g_FXCMAP_B5pc_H_0, nullptr, 247, 0, FXCMAP_CMap::Range, 0},
    {"B5pc-V", g_FXCMAP_B5pc_V_0, nullptr, 12, 0, FXCMAP_CMap::Range, -1},
    {"HKscs-B5-H", g_FXCMAP_HKscs_B5_H_5, nullptr, 1210, 0, FXCMAP_CMap::Range,
     0},
    {"HKscs-B5-V", g_FXCMAP_HKscs_B5_V_5, nullptr, 13, 0, FXCMAP_CMap::Range,
     -1},
    {"ETen-B5-H", g_FXCMAP_ETen_B5_H_0, nullptr, 254, 0, FXCMAP_CMap::Range, 0},
    {"ETen-B5-V", g_FXCMAP_ETen_B5_V_0, nullptr, 13, 0, FXCMAP_CMap::Range, -1},
    {"ETenms-B5-H", g_FXCMAP_ETenms_B5_H_0, nullptr, 1, 0, FXCMAP_CMap::Range,
     -2},
    {"ETenms-B5-V", g_FXCMAP_ETenms_B5_V_0, nullptr, 18, 0, FXCMAP_CMap::Range,
     -1},
    {"CNS-EUC-H", g_FXCMAP_CNS_EUC_H_0, g_FXCMAP_CNS_EUC_H_0_DWord, 157, 238,
     FXCMAP_CMap::Range, 0},
    {"CNS-EUC-V", g_FXCMAP_CNS_EUC_V_0, g_FXCMAP_CNS_EUC_V_0_DWord, 180, 261,
     FXCMAP_CMap::Range, 0},
    {"UniCNS-UCS2-H", g_FXCMAP_UniCNS_UCS2_H_3, nullptr, 16418, 0,
     FXCMAP_CMap::Range, 0},
    {"UniCNS-UCS2-V", g_FXCMAP_UniCNS_UCS2_V_3, nullptr, 13, 0,
     FXCMAP_CMap::Range, -1},
    {"UniCNS-UTF16-H", g_FXCMAP_UniCNS_UTF16_H_0, nullptr, 14557, 0,
     FXCMAP_CMap::Single, 0},
    {"UniCNS-UTF16-V", g_FXCMAP_UniCNS_UCS2_V_3, nullptr, 13, 0,
     FXCMAP_CMap::Range, -1},
};

void CPDF_ModuleMgr::LoadEmbeddedCNS1CMaps() {
  CPDF_FontGlobals* pFontGlobals =
      CPDF_ModuleMgr::Get()->GetPageModule()->GetFontGlobals();
  pFontGlobals->SetEmbeddedCharset(CIDSET_CNS1, g_FXCMAP_CNS1_cmaps,
                                   FX_ArraySize(g_FXCMAP_CNS1_cmaps));
  pFontGlobals->SetEmbeddedToUnicode(CIDSET_CNS1, g_FXCMAP_CNS1CID2Unicode_5,
                                     19088);
}
