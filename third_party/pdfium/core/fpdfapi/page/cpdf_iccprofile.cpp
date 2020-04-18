// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fpdfapi/page/cpdf_iccprofile.h"

#include "core/fpdfapi/cpdf_modulemgr.h"
#include "core/fpdfapi/parser/cpdf_stream.h"
#include "core/fxcodec/codec/ccodec_iccmodule.h"

namespace {

bool DetectSRGB(const uint8_t* pData, uint32_t dwSize) {
  return dwSize == 3144 && memcmp(pData + 0x190, "sRGB IEC61966-2.1", 17) == 0;
}

}  // namespace

CPDF_IccProfile::CPDF_IccProfile(const CPDF_Stream* pStream,
                                 const uint8_t* pData,
                                 uint32_t dwSize)
    : m_bsRGB(DetectSRGB(pData, dwSize)), m_pStream(pStream) {
  if (m_bsRGB) {
    m_nSrcComponents = 3;
    return;
  }

  uint32_t nSrcComps = 0;
  auto* pIccModule = CPDF_ModuleMgr::Get()->GetIccModule();
  m_Transform = pIccModule->CreateTransform_sRGB(pData, dwSize, &nSrcComps);
  if (m_Transform)
    m_nSrcComponents = nSrcComps;
}

CPDF_IccProfile::~CPDF_IccProfile() {}
